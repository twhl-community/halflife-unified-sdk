/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   Use, distribution, and modification of this source code and/or resulting
 *   object code is restricted to non-commercial enhancements to products from
 *   Valve LLC.  All other use, distribution, or modification is prohibited
 *   without written permission from Valve LLC.
 *
 ****/

#include <exception>
#include <type_traits>

#include <fmt/format.h>

#define AL_ALEXT_PROTOTYPES
#include <AL/alext.h>

#include "cbase.h"
#include "com_model.h"
#include "event_api.h"
#include "ISoundSystem.h"
#include "GameSoundSystem.h"
#include "view.h"

namespace sound
{
GameSoundSystem::~GameSoundSystem()
{
	if (MakeCurrent())
	{
		// Destroy all sources first so none of the effects are still referenced.
		m_Channels.clear();
		m_Sentences.reset();
		m_SoundCache.reset();

		alDeleteFilters(1, &m_Filter);
		m_Filter = 0;

		alDeleteAuxiliaryEffectSlots(1, &m_AuxiliaryEffectSlot);
		m_AuxiliaryEffectSlot = 0;

		alDeleteEffects(m_RoomEffects.size(), m_RoomEffects.data());
		std::memset(m_RoomEffects.data(), 0, m_RoomEffects.size() * sizeof(ALuint));
	}

	g_Logging.RemoveLogger(m_SentencesLogger);
	m_SentencesLogger.reset();

	g_Logging.RemoveLogger(m_CacheLogger);
	m_CacheLogger.reset();
}

bool GameSoundSystem::Create(std::shared_ptr<spdlog::logger> logger)
{
	m_Logger = logger;
	m_CacheLogger = g_Logging.CreateLogger("sound.cache");
	m_SentencesLogger = g_Logging.CreateLogger("sound.sentences");

	m_Volume = gEngfuncs.pfnGetCvarPointer("volume");
	m_RoomOff = gEngfuncs.pfnGetCvarPointer("room_off");
	m_RoomType = gEngfuncs.pfnGetCvarPointer("room_type");
	m_WaterRoomType = gEngfuncs.pfnGetCvarPointer("waterroom_type");

	if (!m_Volume)
	{
		m_Logger->error("Couldn't get volume cvar");
		return false;
	}

	if (!m_RoomOff)
	{
		m_Logger->error("Couldn't get room_off cvar");
		return false;
	}

	if (!m_RoomType)
	{
		m_Logger->error("Couldn't get room_type cvar");
		return false;
	}

	if (!m_WaterRoomType)
	{
		m_Logger->error("Couldn't get waterroom_type cvar");
		return false;
	}

	m_Device.reset(alcOpenDevice(nullptr));

	if (!m_Device)
	{
		m_Logger->error("Couldn't open OpenAL device for GameSoundSystem");
		return false;
	}

	m_Logger->trace("OpenAL device opened for GameSoundSystem");

	if (!CheckOpenALContextExtension(m_Device.get(), "ALC_EXT_EFX", *m_Logger))
	{
		return false;
	}

	ALCint attribs[3] = {0};

	// We only need 1.
	attribs[0] = ALC_MAX_AUXILIARY_SENDS;
	attribs[1] = 1;

	m_Context.reset(alcCreateContext(m_Device.get(), attribs));

	if (!m_Context)
	{
		m_Logger->error("Couldn't create OpenAL context");
		return false;
	}

	if (ALC_FALSE == alcMakeContextCurrent(m_Context.get()))
	{
		m_Logger->error("Couldn't make OpenAL context current");
		return false;
	}

	// An active context is required to query these.
	{
		const auto getOpenALString = [](ALenum param)
		{
			if (const auto result = alGetString(param); result)
			{
				return result;
			}

			return "Unknown";
		};

		m_Logger->trace("OpenAL vendor: {}", getOpenALString(AL_VENDOR));
		m_Logger->trace("OpenAL version: {}", getOpenALString(AL_VERSION));
		m_Logger->trace("OpenAL renderer: {}", getOpenALString(AL_RENDERER));
	}

	ALCint auxSendsCount = 0;
	alcGetIntegerv(m_Device.get(), ALC_MAX_AUXILIARY_SENDS, 1, &auxSendsCount);

	if (auxSendsCount < 1)
	{
		m_Logger->error("OpenAL does not provide enough auxiliary send slots for EFX use (requested {}, got {})", attribs[1], auxSendsCount);
		return false;
	}

	if (!CheckOpenALExtension("AL_SOFT_loop_points", *m_Logger))
	{
		return false;
	}

	alGenEffects(m_RoomEffects.size(), m_RoomEffects.data());

	if (alGetError() != AL_NO_ERROR)
	{
		m_Logger->error("Error generating effects");
		return false;
	}

	alGenAuxiliaryEffectSlots(1, &m_AuxiliaryEffectSlot);

	if (alGetError() != AL_NO_ERROR)
	{
		m_Logger->error("Error generating auxiliary effect slot");
		return false;
	}

	alGenFilters(1, &m_Filter);

	if (alGetError() != AL_NO_ERROR)
	{
		m_Logger->error("Error generating filter");
		return false;
	}

	alFilteri(m_Filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);

	for (unsigned int roomType = 0; auto& effect : m_RoomEffects)
	{
		SetupEffect(effect, roomType++);
	}

	if (alGetError() != AL_NO_ERROR)
	{
		m_Logger->error("Error setting up room effects");
		return false;
	}

	// Immediately initialize the slot to a valid effect.
	UpdateRoomEffect();

	alDistanceModel(AL_LINEAR_DISTANCE);

	// GoldSource uses units, where 16 units == 1 foot.
	// See https://developer.valvesoftware.com/wiki/Dimensions
	constexpr double feetPerMeter = 3.281;
	constexpr double footInMeters = 1 / feetPerMeter;
	constexpr double metersPerUnit = footInMeters / 16;
	alListenerf(AL_METERS_PER_UNIT, static_cast<float>(metersPerUnit));

	alDopplerFactor(0.f);

	m_SoundCache = std::make_unique<SoundCache>(m_CacheLogger);
	m_Sentences = std::make_unique<SentencesSystem>(m_SentencesLogger, m_SoundCache.get());

	m_HRTFEnabled = g_ConCommands.CreateCVar("snd_hrtf_enabled", "0", FCVAR_ARCHIVE);
	m_HRTFImplementation = g_ConCommands.CreateCVar("snd_hrtf_implementation", "", FCVAR_ARCHIVE);
	g_ConCommands.CreateCommand("snd_hrtf_list_implementations", [this](const auto&)
		{ PrintHRTFImplementations(); });

	m_SupportsHRTF = alcIsExtensionPresent(m_Device.get(), "ALC_SOFT_HRTF") != ALC_FALSE;

	if (m_SupportsHRTF)
	{
		m_Logger->trace("HRTF is supported");
	}
	else
	{
		m_Logger->trace("HRTF is not supported");
	}

	return true;
}

void GameSoundSystem::OnBeginNetworkDataProcessing()
{
	// Clear all references to sounds and sentences.
	StopAllSounds();

	// Remove all sentences so we can load the incoming list.
	m_Sentences->Clear();

	// Clear the entire cache so we start fresh.
	m_SoundCache->Clear();

	m_PrecacheMap.clear();
}

void GameSoundSystem::HandleNetworkDataBlock(NetworkDataBlock& block)
{
	if (block.Name == "SoundList")
	{
		// The empty name, shouldn't be used.
		m_PrecacheMap.push_back(SoundIndex{});

		for (const auto& fileName : block.Data)
		{
			// TODO: avoid constructing multiple strings here
			m_PrecacheMap.push_back(m_SoundCache->FindName(fileName.get<std::string>().c_str()));
		}
	}
	else if (block.Name == "Sentences")
	{
		m_Sentences->HandleNetworkDataBlock(block);
	}
}

void GameSoundSystem::Update()
{
	if (!MakeCurrent())
	{
		return;
	}

	++m_CurrentGameFrame;

	if (m_SupportsHRTF)
	{
		if (const bool hrtfEnabled = m_HRTFEnabled->value != 0; m_CachedHRTFEnabled != hrtfEnabled)
		{
			m_CachedHRTFEnabled = hrtfEnabled;
			ConfigureHRTF(hrtfEnabled);
		}
	}

	Vector origin, forward, right, up;

	if (UTIL_IsMapLoaded())
	{
		// Use view entity values here.
		origin = v_origin;
		AngleVectors(v_client_aimangles, forward, right, up);
	}
	else
	{
		origin = vec3_origin;
		forward = vec3_forward;
		right = vec3_right;
		up = vec3_up;
	}

	const ALfloat orientation[6] =
		{
			forward[0], forward[1], forward[2],
			up[0], up[1], up[2]};

	alListenerfv(AL_POSITION, origin);
	alListenerfv(AL_ORIENTATION, orientation);

	// Update volume if changed. If we're currently blocked then volume is 0, so don't update it.
	if (!m_Blocked && m_LastKnownVolume != m_Volume->value)
	{
		m_LastKnownVolume = m_Volume->value;
		SetVolume();
	}

	UpdateSounds();

	UpdateRoomEffect();
}

void GameSoundSystem::Block()
{
	if (!MakeCurrent())
	{
		return;
	}

	m_Blocked = true;

	alListenerf(AL_GAIN, 0);
}

void GameSoundSystem::Unblock()
{
	if (!MakeCurrent())
	{
		return;
	}

	m_Blocked = false;

	// Unblock to user-set volume immediately.
	SetVolume();
}

void GameSoundSystem::Pause()
{
	if (!MakeCurrent())
	{
		return;
	}

	if (m_Paused)
	{
		return;
	}

	m_Paused = true;

	for (auto& channel : m_Channels)
	{
		alSourcePause(channel.Source.Id);
	}
}

void GameSoundSystem::Resume()
{
	if (!MakeCurrent())
	{
		return;
	}

	if (!m_Paused)
	{
		return;
	}

	m_Paused = false;

	for (auto& channel : m_Channels)
	{
		alSourcePlay(channel.Source.Id);
	}
}

void GameSoundSystem::StartSound(
	int entityIndex, int channelIndex,
	const char* soundOrSentence, const Vector& origin, float volume, float attenuation, int pitch, int flags)
{
	SoundData sound;

	if (soundOrSentence[0] != '!')
	{
		// Skip stream prefix.
		if (soundOrSentence[0] == '*')
		{
			++soundOrSentence;
		}

		sound = m_SoundCache->FindName(soundOrSentence);
	}
	else
	{
		sound = SentenceChannel{m_Sentences->FindSentence(soundOrSentence + 1), 0};
	}

	StartSound(entityIndex, channelIndex, std::move(sound), origin, volume, attenuation, pitch, flags);
}

void GameSoundSystem::StartSound(int entityIndex, int channelIndex, SoundData&& sound,
	const Vector& origin, float volume, float attenuation, int pitch, int flags)
{
	if (!MakeCurrent())
	{
		return;
	}

	if (!std::visit([&, this](auto&& sound)
			{
			using T = std::decay_t<decltype(sound)>;

			if constexpr (std::is_same_v<T, SoundIndex>)
			{
				if (!sound.IsValid())
				{
					return false;
				}

				auto& soundData = *m_SoundCache->GetSound(sound);

				return m_SoundCache->LoadSound(soundData);
			}
			else if constexpr (std::is_same_v<T, SentenceChannel>)
			{
				return m_Sentences->GetSentence(sound.Sentence) != nullptr;
			}
			else
			{
				static_assert(always_false_v<T>, "StartSound does not handle all sound types");
			}

			return false; },
			sound))
	{
		return;
	}

	if ((flags & (SND_STOP | SND_CHANGE_VOL | SND_CHANGE_PITCH)) != 0)
	{
		if (AlterChannel(entityIndex, channelIndex, sound, volume, pitch, flags))
		{
			return;
		}

		if ((flags & SND_STOP) != 0)
		{
			return;
		}
	}

	if (pitch == 0)
	{
		m_Logger->warn("StartSound ignored, called with pitch 0");
		return;
	}

	Channel* newChannel = FindOrCreateChannel(entityIndex, channelIndex);

	const std::string_view soundOrSentence = GetSoundName(sound);

	if (SetupChannel(*newChannel, entityIndex, channelIndex, std::move(sound), origin, volume, pitch, attenuation, false))
	{
		m_Logger->trace("Playing \"{}\": Entity {}, channel {}", soundOrSentence, entityIndex, channelIndex);

		if (!m_Paused || (flags & SND_PLAY_WHEN_PAUSED) != 0)
		{
			alSourcePlay(newChannel->Source.Id);
		}
	}
	else
	{
		RemoveChannel(*newChannel);
	}
}

void GameSoundSystem::StopAllSounds()
{
	m_Channels.clear();
}

void GameSoundSystem::MsgFunc_EmitSound(const char* pszName, BufferReader& reader)
{
	const int flags = reader.ReadByte();

	const float volume = (flags & SND_VOLUME) != 0 ? reader.ReadByte() / 255.f : 1;
	const float attenuation = (flags & SND_ATTENUATION) != 0 ? reader.ReadByte() / 64.f : 1;
	const int channel = reader.ReadByte();
	const int entityIndex = reader.ReadShort();

	int soundIndex;

	if ((flags & SND_LARGE_INDEX) != 0)
	{
		// Cast the signed short back to an unsigned short so the index is correct.
		// See ServerSoundSystem::EmitSound for why this is needed.
		soundIndex = static_cast<std::uint16_t>(reader.ReadShort());
	}
	else
	{
		soundIndex = reader.ReadByte();
	}

	Vector origin;
	origin.x = reader.ReadCoord();
	origin.y = reader.ReadCoord();
	origin.z = reader.ReadCoord();

	const int pitch = (flags & SND_PITCH) != 0 ? reader.ReadByte() : 100;

	const auto entity = gEngfuncs.GetEntityByIndex(entityIndex);

	if (!entity)
	{
		m_Logger->error("MsgFunc_EmitSound: Entity index {} is invalid", entityIndex);
		return;
	}

	const std::size_t absoluteSoundIndex = soundIndex >= 0 ? static_cast<std::size_t>(soundIndex) : SentencesSystem::InvalidIndex;

	if ((flags & SND_SENTENCE) == 0)
	{
		if (absoluteSoundIndex >= m_PrecacheMap.size())
		{
			m_Logger->error("MsgFunc_EmitSound: Invalid sound index {}", soundIndex);
			return;
		}

		const SoundIndex actualSoundIndex = m_PrecacheMap[absoluteSoundIndex];

		StartSound(entityIndex, channel, actualSoundIndex, origin, volume, attenuation, pitch, flags);
	}
	else
	{
		StartSound(entityIndex, channel, SentenceChannel{absoluteSoundIndex, 0}, origin, volume, attenuation, pitch, flags);
	}
}

bool GameSoundSystem::MakeCurrent()
{
	if (ALC_FALSE == alcMakeContextCurrent(m_Context.get()))
	{
		m_Logger->error("Couldn't make OpenAL context current");
		return false;
	}

	return true;
}

void GameSoundSystem::PrintHRTFImplementations()
{
	ALCint numHRTF;
	alcGetIntegerv(m_Device.get(), ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &numHRTF);

	if (numHRTF == 0)
	{
		Con_Printf("No HRTF implementations found\n");
		return;
	}

	Con_Printf("%d HRTF implementations found\n", numHRTF);

	for (ALCint j = 0; j < numHRTF; ++j)
	{
		const ALCchar* name = alcGetStringiSOFT(m_Device.get(), ALC_HRTF_SPECIFIER_SOFT, j);

		Con_Printf("%d: %s\n", j, name);
	}
}

void GameSoundSystem::ConfigureHRTF(bool enabled)
{
	ALCint numHRTF;
	alcGetIntegerv(m_Device.get(), ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &numHRTF);

	if (numHRTF == 0)
	{
		m_Logger->debug("No HRTF implementations found");
		return;
	}

	ALCint attribs[5]{0};

	int i = 0;
	attribs[i++] = ALC_HRTF_SOFT;
	attribs[i++] = enabled ? ALC_TRUE : ALC_FALSE;

	if (enabled)
	{
		ALCint implementationIndex = -1;

		if (m_HRTFImplementation->string[0] != '\0')
		{
			for (ALCint j = 0; j < numHRTF; ++j)
			{
				const ALCchar* name = alcGetStringiSOFT(m_Device.get(), ALC_HRTF_SPECIFIER_SOFT, j);

				if (strcmp(m_HRTFImplementation->string, name) == 0)
				{
					implementationIndex = j;
					break;
				}
			}

			if (implementationIndex == -1)
			{
				m_Logger->error("HRTF implementation \"{}\" not found", m_HRTFImplementation->string);
			}
		}

		if (implementationIndex != -1)
		{
			m_Logger->debug("Using HRTF implementation \"{}\" (index {})", m_HRTFImplementation->string, implementationIndex);
			attribs[i++] = ALC_HRTF_ID_SOFT;
			attribs[i++] = implementationIndex;
		}
		else
		{
			m_Logger->debug("Using default HRTF implementation");
		}
	}

	attribs[i++] = 0;

	if (ALC_FALSE == alcResetDeviceSOFT(m_Device.get(), attribs))
	{
		m_Logger->error("Failed to reset device: {}", alcGetString(m_Device.get(), alcGetError(m_Device.get())));
	}
}

void GameSoundSystem::SetVolume()
{
	alListenerf(AL_GAIN, m_Volume->value);
}

std::string_view GameSoundSystem::GetSoundName(const SoundData& sound) const
{
	return std::visit([&, this](auto&& sound)
		{
			using T = std::decay_t<decltype(sound)>;

			if constexpr (std::is_same_v<T, SoundIndex>)
			{
				const auto& soundData = *m_SoundCache->GetSound(sound);
				return std::string_view{soundData.Name.data(), soundData.Name.size()};
			}
			else if constexpr (std::is_same_v<T, SentenceChannel>)
			{
				const auto& sentence = *m_Sentences->GetSentence(sound.Sentence);
				return std::string_view{sentence.Name.data(), sentence.Name.size()};
			}
			else
			{
				static_assert(always_false_v<T>, "GetSoundName does not handle all sound types");
			}

			return std::string_view{}; },
		sound);
}

Channel* GameSoundSystem::CreateChannel()
{
	Channel channel;

	channel.Source = OpenALSource::Create();

	UpdateSourceEffect(channel.Source);

	m_Channels.push_back(std::move(channel));

	return &m_Channels.back();
}

void GameSoundSystem::ClearChannel(Channel& channel)
{
	if (std::holds_alternative<SentenceChannel>(channel.Sound))
	{
		m_Sentences->CloseMouth(channel.EntityIndex, channel.ChannelIndex);
	}

	alSourceStop(channel.Source.Id);

	// Detach buffer in case this is a time compressed buffer.
	alSourcei(channel.Source.Id, AL_BUFFER, NullBuffer);
}

void GameSoundSystem::RemoveChannel(Channel& channel)
{
	ClearChannel(channel);

	m_Channels.erase(m_Channels.begin() + (&channel - m_Channels.data()));
}

template <typename FilterFunction>
Channel* GameSoundSystem::FindDynamicChannel(int entityIndex, FilterFunction&& filterFunction)
{
	for (auto& channel : m_Channels)
	{
		if (channel.ChannelIndex == CHAN_STATIC)
			continue;

		if (channel.EntityIndex != entityIndex)
			continue;

		if (filterFunction(channel))
			return &channel;
	}

	return nullptr;
}

Channel* GameSoundSystem::FindOrCreateChannel(int entityIndex, int channelIndex)
{
	// The engine's original behavior works like this:
	// There are 128 channels. 4 ambient, 8 dynamic, 116 static.
	// Ambient channels are left over from Quake 1 and are never used.
	// For dynamic channels it depends on the channel index.
	// CHAN_AUTO picks the first unused channel or the channel closest to finishing its sound.
	// CHAN_VOICE prefers channels assigned to the current entity's voice channel,
	// otherwise same as CHAN_AUTO.
	// All other channels will reuse channels assigned to the current entity if
	// they are the same channel or if the requested channel is -1, otherwise same as CHAN_AUTO.
	// For static channels it always tries to find an unused static channel.

	// This sound system allocates channels on demand so it tries to follow the old rules first,
	// falling back to allocating a channel if no in-use channels match.
	if (channelIndex != CHAN_STATIC && channelIndex != CHAN_AUTO)
	{
		Channel* channel = nullptr;

		if (channelIndex == CHAN_VOICE)
		{
			// Always override existing voice lines.
			channel = FindDynamicChannel(entityIndex, [&](Channel& channel)
				{ return channel.ChannelIndex == CHAN_VOICE; });
		}
		else if (channelIndex != -1)
		{
			channel = FindDynamicChannel(entityIndex, [&](Channel& channel)
				{ return channel.ChannelIndex == channelIndex; });
		}
		else
		{
			channel = FindDynamicChannel(entityIndex, [&](Channel& channel)
				{ return true; });
		}

		if (channel)
		{
			m_Logger->trace("Clearing \"{}\": entity {}, channel {}", GetSoundName(channel->Sound), entityIndex, channelIndex);
			ClearChannel(*channel);
			return channel;
		}
	}

	return CreateChannel();
}

bool GameSoundSystem::SetupChannel(Channel& channel, int entityIndex, int channelIndex,
	SoundData&& sound, const Vector& origin, float volume, int pitch, float attenuation, bool isRelative)
{
	channel.Sound = std::move(sound);
	channel.EntityIndex = entityIndex;
	channel.ChannelIndex = channelIndex;
	channel.Pitch = pitch;
	channel.CreatedOnFrame = m_CurrentGameFrame;

	// If attenuation is 0 then the sound will play everywhere.
	// If the entity is the current view entity then it should always sound like it's playing "here".
	// Make it relative to the listener to stop it from having 3D spatialization.
	if (!isRelative && (attenuation == 0 || entityIndex == g_ViewEntity))
	{
		isRelative = true;
		alSourcefv(channel.Source.Id, AL_POSITION, vec3_origin);
	}
	else
	{
		alSourcefv(channel.Source.Id, AL_POSITION, origin);
	}

	alSourcef(channel.Source.Id, AL_GAIN, volume);
	alSourcef(channel.Source.Id, AL_PITCH, pitch / 100.f);
	alSourcef(channel.Source.Id, AL_ROLLOFF_FACTOR, attenuation);
	alSourcef(channel.Source.Id, AL_REFERENCE_DISTANCE, 0);
	alSourcef(channel.Source.Id, AL_MAX_DISTANCE, NominalClippingDistance);
	alSourcei(channel.Source.Id, AL_SOURCE_RELATIVE, isRelative ? AL_TRUE : AL_FALSE);
	alSourcei(channel.Source.Id, AL_LOOPING, AL_FALSE);

	alSourcei(channel.Source.Id, AL_BUFFER, NullBuffer);

	const bool success = std::visit([&, this](auto&& sound)
		{
			using T = std::decay_t<decltype(sound)>;

			if constexpr (std::is_same_v<T, SoundIndex>)
			{
				const auto& soundData = *m_SoundCache->GetSound(sound);

				alSourcei(channel.Source.Id, AL_BUFFER, soundData.Buffer.Id);
				alSourcei(channel.Source.Id, AL_LOOPING, soundData.IsLooping ? AL_TRUE : AL_FALSE);
			}
			else if constexpr (std::is_same_v<T, SentenceChannel>)
			{
				m_Sentences->InitMouth(channel.EntityIndex, channel.ChannelIndex);

				// Can't just queue all of them because they don't always have the same format, which isn't supported by OpenAL.
				return m_Sentences->SetSentenceWord(channel, sound);
			}
			else
			{
				static_assert(always_false_v<T>, "SetupChannel does not handle all sound types");
			}

			return true; },
		channel.Sound);

	if (!success)
	{
		return false;
	}

	if (channelIndex != CHAN_STATIC)
	{
		// Offset play start position if there are other sounds with the same sound starting this frame.
		// This avoids the sounds stacking to essentially create a higher volume sound.
		// Does not apply to sentences since they are rarely playing at the same time and need to play as intended.
		if (std::holds_alternative<SoundIndex>(channel.Sound))
		{
			const auto& soundData = *m_SoundCache->GetSound(std::get<SoundIndex>(channel.Sound));

			for (auto& otherChannel : m_Channels)
			{
				if (&otherChannel == &channel)
				{
					continue;
				}

				if (otherChannel.Sound != channel.Sound)
				{
					continue;
				}

				if (otherChannel.CreatedOnFrame != channel.CreatedOnFrame)
				{
					continue;
				}

				// Don't query this; it causes performance issues with HRTF enabled.
				// ALint offset = -1;
				// alGetSourcei(otherChannel.Source.Id, AL_BYTE_OFFSET, &offset);

				// if (offset == 0)
				{
					ALint frequency = 0;
					alGetBufferi(soundData.Buffer.Id, AL_FREQUENCY, &frequency);

					ALint channels = 0;
					alGetBufferi(soundData.Buffer.Id, AL_CHANNELS, &channels);

					ALint size = 0;
					alGetBufferi(soundData.Buffer.Id, AL_SIZE, &size);

					// Need to convert the frequency to bytes for this to work.
					const ALint sampleSizeInBytes = channels * sizeof(float);
					const ALint sampleRateInBytes = frequency * sampleSizeInBytes;

					ALint skip = static_cast<ALint>(gEngfuncs.pfnRandomLong(0, (int)(0.1 * sampleRateInBytes)));

					// If the sound is empty or really short it could be out of range.
					skip = std::clamp(skip, 0, std::max(0, size - sampleSizeInBytes));

					alSourcei(otherChannel.Source.Id, AL_BYTE_OFFSET, skip);
					break;
				}
			}
		}
	}

	return true;
}

bool GameSoundSystem::AlterChannel(int entityIndex, int channelIndex, const SoundData& sound, float volume, int pitch, int flags)
{
	decltype(m_Channels.begin()) existingChannel;

	if (std::holds_alternative<SentenceChannel>(sound))
	{
		existingChannel = std::find_if(m_Channels.begin(), m_Channels.end(), [&](const auto& candidate)
			{ return candidate.EntityIndex == entityIndex && candidate.ChannelIndex == channelIndex && std::holds_alternative<SentenceChannel>(candidate.Sound); });
	}
	else
	{
		existingChannel = std::find_if(m_Channels.begin(), m_Channels.end(), [&](const auto& candidate)
			{ return candidate.EntityIndex == entityIndex && candidate.ChannelIndex == channelIndex && candidate.Sound == sound; });
	}

	if (existingChannel == m_Channels.end())
	{
		return false;
	}

	auto& channelToAlter = *existingChannel;

	if ((flags & SND_CHANGE_VOL) != 0)
	{
		alSourcef(channelToAlter.Source.Id, AL_GAIN, volume);
	}

	if ((flags & SND_CHANGE_PITCH) != 0)
	{
		channelToAlter.Pitch = pitch;
		alSourcef(channelToAlter.Source.Id, AL_PITCH, pitch / 100.f);
	}

	if ((flags & SND_STOP) != 0)
	{
		RemoveChannel(channelToAlter);
	}

	return true;
}

void GameSoundSystem::UpdateRoomEffect()
{
	// Allow users to disable room effects for the OpenAL version separately.
	const bool roomOn = g_cl_snd_room_off->value == 0 && m_RoomOff->value == 0;

	if (roomOn)
	{
		const auto isUnderwater = UTIL_IsMapLoaded() ? g_WaterLevel >= WaterLevel::Head : false;

		const auto cvar = isUnderwater ? m_WaterRoomType : m_RoomType;

		const float hfGain = isUnderwater ? UnderwaterLowPassHFGain : NormalLowPassHFGain;

		int roomType = static_cast<int>(cvar->value);

		if (roomType < 0 || roomType >= RoomEffectCount)
		{
			roomType = 0;
		}

		if (m_PreviousRoomOn != roomOn)
		{
			m_PreviousRoomOn = roomOn;

			// Force a reset if we're turning effects back on.
			m_PreviousRoomType = -1;
			m_PreviousHFGain = -1;
		}

		if (m_PreviousRoomType != roomType)
		{
			m_PreviousRoomType = roomType;
			alAuxiliaryEffectSloti(m_AuxiliaryEffectSlot, AL_EFFECTSLOT_EFFECT, static_cast<ALint>(m_RoomEffects[roomType]));
		}

		if (m_PreviousHFGain != hfGain)
		{
			m_PreviousHFGain = hfGain;
			alFilterf(m_Filter, AL_LOWPASS_GAINHF, hfGain);

			// Re-attach the filter to all active sources.
			for (auto& channel : m_Channels)
			{
				UpdateSourceEffect(channel.Source);
			}
		}
	}
	else if (m_PreviousRoomOn != roomOn)
	{
		m_PreviousRoomOn = roomOn;
		alAuxiliaryEffectSloti(m_AuxiliaryEffectSlot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
	}
}

void GameSoundSystem::UpdateSourceEffect(OpenALSource& source)
{
	alSource3i(source.Id, AL_AUXILIARY_SEND_FILTER, static_cast<ALint>(m_AuxiliaryEffectSlot), 0, m_Filter);
}

void GameSoundSystem::UpdateSounds()
{
	const auto localPlayer = UTIL_IsMapLoaded() ? gEngfuncs.GetLocalPlayer() : nullptr;

	const int messagenum = localPlayer ? localPlayer->curstate.messagenum : 0;

	// Update all sounds that are looping, clear finished sounds.
	for (std::size_t index = 0; index < m_Channels.size();)
	{
		auto& channel = m_Channels[index];

		const bool remove = std::visit([&, this](auto&& sound)
			{
				using T = std::decay_t<decltype(sound)>;

				if constexpr (std::is_same_v<T, SoundIndex>)
				{
					return UpdateSound(channel);
				}
				else if constexpr (std::is_same_v<T, SentenceChannel>)
				{
					return m_Sentences->UpdateSentence(channel);
				}
				else
				{
					static_assert(always_false_v<T>, "UpdateSounds does not handle all sound types");
				} },
			channel.Sound);

		if (remove)
		{
			RemoveChannel(channel);
		}
		else
		{
			Spatialize(channel, messagenum);

			++index;
		}
	}
}

bool GameSoundSystem::UpdateSound(Channel& channel)
{
	ALint state = AL_STOPPED;
	alGetSourcei(channel.Source.Id, AL_SOURCE_STATE, &state);

	if (state == AL_STOPPED)
	{
		return true;
	}

	m_Sentences->MoveMouth(channel, *m_SoundCache->GetSound(std::get<SoundIndex>(channel.Sound)));

	return false;
}

void GameSoundSystem::Spatialize(Channel& channel, int messagenum)
{
	// Update sound channel position.
	if (channel.EntityIndex > 0)
	{
		auto entity = gEngfuncs.GetEntityByIndex(channel.EntityIndex);

		// Entities without a model are not sent to the client so there is no point in updating their position.
		if (entity && entity->model && entity->curstate.messagenum == messagenum)
		{
			ALint isRelative = AL_FALSE;
			alGetSourcei(channel.Source.Id, AL_SOURCE_RELATIVE, &isRelative);

			// Don't update relative sources (always vec3_origin).
			if (isRelative != AL_FALSE)
			{
				return;
			}

			if (entity->model->type == mod_brush)
			{
				alSourcefv(channel.Source.Id, AL_POSITION, entity->origin + (entity->model->mins + entity->model->maxs) * 0.5f);
			}
			else
			{
				alSourcefv(channel.Source.Id, AL_POSITION, entity->origin);
			}
		}
	}
}
}
