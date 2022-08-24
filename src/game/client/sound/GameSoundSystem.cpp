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
}

bool GameSoundSystem::Create(std::shared_ptr<spdlog::logger> logger, ALCdevice* device)
{
	m_Logger = logger;

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

	if (!CheckOpenALContextExtension(device, "ALC_EXT_EFX", *m_Logger))
	{
		return false;
	}

	ALint attribs[4] = {0};

	// We only need 1.
	attribs[0] = ALC_MAX_AUXILIARY_SENDS;
	attribs[1] = 1;

	m_Context.reset(alcCreateContext(device, attribs));

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

	ALCint auxSendsCount = 0;
	alcGetIntegerv(device, ALC_MAX_AUXILIARY_SENDS, 1, &auxSendsCount);

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

	m_SoundCache = std::make_unique<SoundCache>(m_Logger);
	m_Sentences = std::make_unique<SentencesSystem>(m_Logger, m_SoundCache.get());

	return true;
}

void GameSoundSystem::LoadSentences()
{
	m_Sentences->LoadSentences();
}

void GameSoundSystem::Update()
{
	if (!MakeCurrent())
	{
		return;
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

	for (auto& channel : m_Channels)
	{
		alSourcePlay(channel.Source.Id);
	}
}

void GameSoundSystem::StartSound(
	int entityIndex, int channelIndex,
	const char* soundOrSentence, const Vector& origin, float volume, float attenuation, int pitch, int flags)
{
	if (g_cl_snd_openal->value == 0)
	{
		// Convert sentences back into an index.
		Filename sentenceIndexString;

		if (soundOrSentence[0] == '!')
		{
			const auto sentenceIndex = m_Sentences->FindSentence(soundOrSentence + 1);

			if (sentenceIndex == SentencesSystem::InvalidIndex)
			{
				return;
			}

			fmt::format_to(std::back_inserter(sentenceIndexString), "!{}", sentenceIndex);

			soundOrSentence = sentenceIndexString.c_str();
		}

		gEngfuncs.pEventAPI->EV_PlaySound(entityIndex, origin, channelIndex, soundOrSentence, volume, attenuation, flags, pitch);
		return;
	}

	if (!MakeCurrent())
	{
		return;
	}

	SoundData sound;

	if (soundOrSentence[0] != '!')
	{
		// Skip stream prefix.
		if (soundOrSentence[0] == '*')
		{
			++soundOrSentence;
		}

		const auto index = m_SoundCache->FindName(soundOrSentence);

		if (!index.IsValid())
		{
			return;
		}

		if (auto soundData = m_SoundCache->GetSound(index); !m_SoundCache->LoadSound(*soundData))
		{
			return;
		}

		sound = index;
	}
	else
	{
		const auto sentenceIndex = m_Sentences->FindSentence(soundOrSentence + 1);

		if (sentenceIndex == SentencesSystem::InvalidIndex)
		{
			return;
		}

		sound = SentenceChannel{sentenceIndex, 0};
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

	if (SetupChannel(*newChannel, entityIndex, channelIndex, std::move(sound), origin, volume, pitch, attenuation, false))
	{
		alSourcePlay(newChannel->Source.Id);
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

void GameSoundSystem::ClearCaches()
{
	// Need to stop sounds to clear OpenAL buffers.
	StopAllSounds();

	// Clear all cached data. The sounds themselves need to stay because the sentences reference them.
	m_SoundCache->ClearBuffers();
}

void GameSoundSystem::UserMsg_EmitSound(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const int flags = READ_BYTE();

	const float volume = (flags & SND_VOLUME) != 0 ? READ_BYTE() / 255.f : 1;
	const float attenuation = (flags & SND_ATTENUATION) != 0 ? READ_BYTE() / 64.f : 1;
	const int channel = READ_BYTE();
	const int entityIndex = READ_SHORT();

	int soundIndex;

	if ((flags & SND_LARGE_INDEX) != 0)
	{
		// Cast the signed short back to an unsigned short so the index is correct.
		// See ServerSoundSystem::EmitSound for why this is needed.
		soundIndex = static_cast<std::uint16_t>(READ_SHORT());
	}
	else
	{
		soundIndex = READ_BYTE();
	}

	Vector origin;
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();

	const int pitch = (flags & SND_PITCH) != 0 ? READ_BYTE() : 100;

	if (entityIndex < 0 || entityIndex >= g_MaxEntities)
	{
		m_Logger->error("UserMsg_EmitSound: Entity index {} is invalid, max edicts is {}", entityIndex, g_MaxEntities);
		return;
	}

	if ((flags & SND_SENTENCE) == 0)
	{
		m_Logger->error("UserMsg_EmitSound: Cannot play regular sounds");
		return;
	}

	const std::size_t absoluteSoundIndex = soundIndex >= 0 ? static_cast<std::size_t>(soundIndex) : SentencesSystem::InvalidIndex;

	const auto sentence = m_Sentences->GetSentence(absoluteSoundIndex);

	if (!sentence)
	{
		m_Logger->error("UserMsg_EmitSound: Invalid sentence index {}", soundIndex);
		return;
	}

	Filename sample;

	fmt::format_to(std::back_inserter(sample), "!{}", sentence->Name.c_str());

	StartSound(entityIndex, channel, sample.c_str(), origin, volume, attenuation, pitch, flags);
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

void GameSoundSystem::SetVolume()
{
	alListenerf(AL_GAIN, m_Volume->value);
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

Channel* GameSoundSystem::FindOrCreateChannel(int entityIndex, int channelIndex)
{
	if (channelIndex != CHAN_STATIC)
	{
		// Dynamic channels only allow a single sound to play on them for a given entity.
		for (auto& channel : m_Channels)
		{
			if (channel.ChannelIndex != CHAN_STATIC && channel.EntityIndex == entityIndex && channel.ChannelIndex == channelIndex)
			{
				ClearChannel(channel);
				return &channel;
			}
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

	alSourcefv(channel.Source.Id, AL_POSITION, origin);
	alSourcef(channel.Source.Id, AL_GAIN, volume);
	alSourcef(channel.Source.Id, AL_PITCH, pitch / 100.f);
	alSourcef(channel.Source.Id, AL_ROLLOFF_FACTOR, attenuation);
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

				ALint offset = -1;
				alGetSourcei(otherChannel.Source.Id, AL_BYTE_OFFSET, &offset);

				if (offset == 0)
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
		const auto isUnderwater = UTIL_IsMapLoaded() ? g_WaterLevel >= 3 : false;

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

	return state == AL_STOPPED;
}

void GameSoundSystem::Spatialize(Channel& channel, int messagenum)
{
	// Update sound channel position.
	if (channel.EntityIndex > 0 && channel.EntityIndex < g_MaxEntities)
	{
		auto entity = gEngfuncs.GetEntityByIndex(channel.EntityIndex);

		// Entities without a model are not sent to the client so there is no point in updating their position.
		if (entity && entity->model && entity->curstate.messagenum == messagenum)
		{
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
