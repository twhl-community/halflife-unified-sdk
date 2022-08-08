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
#include <limits>

#include <AL/al.h>
#include <AL/alc.h>

#define AL_ALEXT_PROTOTYPES
#include <AL/alext.h>

#include "hud.h"

#include "OpenALMusicSystem.h"
#include "OpenALSoundSystem.h"

#include "utils/command_utils.h"

OpenALMusicSystem::~OpenALMusicSystem()
{
	//Stop worker thread and wait until it finishes.
	m_Quit = true;

	m_Thread.join();

	//Since we switch to this context on-demand we can't rely on the default destructor to clean up.
	if (const ContextSwitcher switcher{m_Context.get(), *m_SoundSystem->GetLogger()}; switcher)
	{
		m_Source.Delete();
		m_Buffers.clear();
	}
}

bool OpenALMusicSystem::Create(OpenALSoundSystem* soundSystem)
{
	m_SoundSystem = soundSystem;

	auto& logger = *m_SoundSystem->GetLogger();

	m_VolumeCvar = gEngfuncs.pfnGetCvarPointer("MP3Volume");
	m_FadeTimeCvar = gEngfuncs.pfnGetCvarPointer("MP3FadeTime");

	if (!m_VolumeCvar)
	{
		logger.error("Couldn't get bgmvolume cvar");
		return false;
	}

	auto device = m_SoundSystem->GetDevice();

	if (!CheckOpenALContextExtension(device, "ALC_EXT_thread_local_context", logger))
	{
		return false;
	}

	m_Context.reset(alcCreateContext(device, nullptr));

	if (!m_Context)
	{
		logger.error("Couldn't create OpenAL context");
		return false;
	}

	const ContextSwitcher switcher{m_Context.get(), logger};

	if (!switcher)
	{
		return false;
	}

	if (!CheckOpenALExtension("AL_EXT_FLOAT32", logger))
	{
		return false;
	}

	m_Source = OpenALSource::Create();

	if (!m_Source.IsValid())
	{
		logger.error("Couldn't create OpenAL source");
		return false;
	}

	m_Loader = std::make_unique<nqr::NyquistIO>();

	//Start the worker thread.
	m_Thread = std::thread{&OpenALMusicSystem::Run, this};

	g_ConCommands.CreateCommand(
		"music", [this](const auto& args)
		{ Music_Command(args); },
		CommandLibraryPrefix::No);

	return true;
}

void OpenALMusicSystem::Play(std::string&& fileName, bool looping)
{
	if (m_Playing || m_Paused)
	{
		if (m_FileName == fileName)
			return;
	}

	char absolutePath[MAX_PATH_LENGTH];

	if (!g_pFileSystem->GetLocalPath(fileName.c_str(), absolutePath, std::size(absolutePath)))
	{
		Con_Printf("OpenALMusicSystem: File \"%s\" does not exist.\n", fileName.c_str());
		return;
	}

	FILE* file = fopen(absolutePath, "rb");

	if (!file)
	{
		Con_Printf("OpenALMusicSystem: Could not open file \"%s\".\n", fileName.c_str());
		return;
	}

	m_FileName = std::move(fileName);

	FileWrapper deleter{file};

	RunOnWorkerThread(&OpenALMusicSystem::StartPlaying, looping, std::move(deleter));
}

void OpenALMusicSystem::StartPlaying(bool looping, FileWrapper file)
{
	if (!m_Enabled)
		return;

	if (m_Playing || m_Paused)
	{
		Stop();
	}

	m_FadeStartTime = std::chrono::system_clock::time_point{};
	m_Volume = -1; //Force recalculation so volume resets.

	m_Info = {};

	m_Info.Data = std::make_unique<nqr::AudioData>();

	try
	{
		std::vector<std::uint8_t> contents;

		fseek(file.get(), 0, SEEK_END);

		contents.resize(ftell(file.get()));

		fseek(file.get(), 0, SEEK_SET);

		if (fread(contents.data(), 1, contents.size(), file.get()) != contents.size())
		{
			return;
		}

		m_Loader->Load(m_Info.Data.get(), contents);
	}
	catch (const std::exception&)
	{
		//Can happen if the file was deleted or otherwise rendered inaccessible since queueing this.
		//Can't log the error since we're on the worker thread.
		return;
	}

	//No longer need the file.
	file.reset();

	m_Info.Format = m_Info.Data->channelCount == 1 ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32;

	//Allocate enough buffers to hold around a second's worth of data.
	const std::size_t bytesInASecond = m_Info.Data->sampleRate * m_Info.Data->channelCount * sizeof(short);

	const auto numberOfBuffers = static_cast<std::size_t>(std::ceil(static_cast<float>(bytesInASecond) / BufferSize));

	m_Buffers.reserve(numberOfBuffers);

	for (std::size_t i = m_Buffers.size(); i < numberOfBuffers; ++i)
	{
		m_Buffers.push_back(OpenALBuffer::Create());
	}

	//Fill all buffers with data.
	std::byte dataBuffer[BufferSize];

	ALsizei buffersFilled = 0;

	for (auto& buffer : m_Buffers)
	{
		const std::size_t bytesRead = Read(dataBuffer, sizeof(dataBuffer));

		if (bytesRead == 0)
			break;

		alBufferData(buffer.Id, m_Info.Format, dataBuffer, bytesRead, m_Info.Data->sampleRate);

		++buffersFilled;
	}

	//Attach all buffers and play.
	static_assert(sizeof(ALuint) == sizeof(OpenALBuffer), "Must create an array to hold buffer ids");
	alSourceQueueBuffers(m_Source.Id, buffersFilled, reinterpret_cast<const ALuint*>(m_Buffers.data()));

	alSourcePlay(m_Source.Id);

	m_Looping = looping;
	m_Playing = true;

	if (m_Volume == 0.0)
		Pause();
}

void OpenALMusicSystem::Stop()
{
	if (RunOnWorkerThread(&OpenALMusicSystem::Stop))
	{
		return;
	}

	if (!m_Enabled)
		return;

	if (!m_Playing && !m_Paused)
		return;

	alSourceStop(m_Source.Id);

	//Unqueue all buffers as well to free them up.
	alSourcei(m_Source.Id, AL_BUFFER, NullBuffer);

	m_Paused = false;
	m_Playing = false;
}

void OpenALMusicSystem::Pause()
{
	if (RunOnWorkerThread(&OpenALMusicSystem::Pause))
	{
		return;
	}

	if (!m_Enabled)
		return;

	if (!m_Playing)
		return;

	alSourcePause(m_Source.Id);

	m_Paused = true;
	m_Playing = false;
}

void OpenALMusicSystem::Resume()
{
	if (RunOnWorkerThread(&OpenALMusicSystem::Resume))
	{
		return;
	}

	if (!m_Enabled)
		return;

	if (!m_Paused)
		return;

	alSourcePlay(m_Source.Id);

	m_Paused = false;
	m_Playing = true;
}

void OpenALMusicSystem::Block()
{
	if (RunOnWorkerThread(&OpenALMusicSystem::Block))
	{
		return;
	}

	m_Blocked = true;

	alListenerf(AL_GAIN, 0);
}

void OpenALMusicSystem::Unblock()
{
	if (RunOnWorkerThread(&OpenALMusicSystem::Unblock))
	{
		return;
	}

	m_Blocked = false;

	// Unblock to user-set volume immediately.
	UpdateVolume(true);
}

template <typename Func, typename... Args>
bool OpenALMusicSystem::RunOnWorkerThread(Func func, Args&&... args)
{
	if (std::this_thread::get_id() == m_Thread.get_id())
	{
		return false;
	}

	//Queue up for execution on the worker thread.
	const std::lock_guard guard{m_JobMutex};

	m_Jobs.emplace_back([=, this]()
		{ (this->*func)(args...); });

	return true;
}

std::size_t OpenALMusicSystem::Read(std::byte* dest, std::size_t bufferSize)
{
	const std::size_t startIndex = m_Info.NextSampleIndex;
	const std::size_t endIndex = std::min(m_Info.Data->samples.size(), m_Info.NextSampleIndex + (bufferSize / sizeof(float)));

	const std::size_t samplesToCopy = endIndex - startIndex;

	std::memcpy(dest, m_Info.Data->samples.data() + startIndex, sizeof(float) * samplesToCopy);

	m_Info.NextSampleIndex += samplesToCopy;

	return samplesToCopy * sizeof(float);
}

void OpenALMusicSystem::Run()
{
	//Use our context on our own thread only.
	alcSetThreadContext(m_Context.get());

	while (!m_Quit)
	{
		//Run pending jobs, if any.
		if (const std::unique_lock guard{m_JobMutex, std::try_to_lock}; guard)
		{
			m_Jobs.swap(m_JobsToExecute);
		}

		for (auto& job : m_JobsToExecute)
		{
			job();
		}

		m_JobsToExecute.clear();

		Update();

		//Let other threads do work.
		std::this_thread::sleep_for(std::chrono::milliseconds{1});
	}

	alcSetThreadContext(nullptr);
}

void OpenALMusicSystem::Update()
{
	if (!m_Enabled)
		return;

	if (m_Playing)
	{
		//Fill processed buffers with new data if needed.
		ALint processed;
		alGetSourcei(m_Source.Id, AL_BUFFERS_PROCESSED, &processed);

		ALuint bufferId;

		std::byte dataBuffer[BufferSize];

		while (processed-- > 0)
		{
			alSourceUnqueueBuffers(m_Source.Id, 1, &bufferId);

			std::size_t bytesRead = Read(dataBuffer, sizeof(dataBuffer));

			if (bytesRead == 0)
			{
				if (!m_Looping)
				{
					Stop();
					return;
				}

				m_Info.NextSampleIndex = 0;

				bytesRead = Read(dataBuffer, sizeof(dataBuffer));
			}

			if (bytesRead == 0)
			{
				Stop();
				return;
			}

			alBufferData(bufferId, m_Info.Format, dataBuffer, bytesRead, m_Info.Data->sampleRate);

			alSourceQueueBuffers(m_Source.Id, 1, &bufferId);

			ALint state;
			alGetSourcei(m_Source.Id, AL_SOURCE_STATE, &state);

			if (state != AL_PLAYING)
			{
				//If we're really slow to load samples then playback might have ended already.
				//Restart it so we at least play it chunk by chunk.
				alSourcePlay(m_Source.Id);
			}
		}
	}

	if (!m_Blocked)
	{
		UpdateVolume();
	}
}

void OpenALMusicSystem::UpdateVolume(bool force)
{
	float fadeMultiplier = 1;

	bool needsVolumeUpdate = force;

	// NOTE: cvar values can potentially change while being read.

	if (const auto fadeTime = m_FadeStartTime.load(); fadeTime != std::chrono::system_clock::time_point{})
	{
		const auto now = std::chrono::system_clock::now();

		if (now >= fadeTime)
		{
			// Finished fading out.
			// Volume doesn't need updating now.
			Stop();
			return;
		}

		const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(fadeTime - now).count();
		fadeMultiplier = std::max(0.f, millis / 1000.f / m_FadeDuration);

		needsVolumeUpdate = true;
	}

	if (float setting = m_VolumeCvar->value; setting != m_Volume)
	{
		setting = std::clamp(setting, 0.f, 1.f);

		m_Volume = setting;

		needsVolumeUpdate = true;
	}

	if (needsVolumeUpdate)
	{
		const float finalVolume = m_Volume * fadeMultiplier;

		alListenerf(AL_GAIN, finalVolume);
	}
}

void OpenALMusicSystem::Music_Command(const CCommandArgs& args)
{
	auto command = args.Count() >= 2 ? args.Argument(1) : "";

	if (stricmp(command, "on") == 0)
	{
		m_Enabled = true;
	}
	else if (stricmp(command, "off") == 0)
	{
		Stop();
		m_Enabled = false;
	}
	else if (stricmp(command, "reset") == 0)
	{
		m_Enabled = true;
		Stop();
	}
	else if (stricmp(command, "play") == 0)
	{
		Play(args.Argument(2), false);
	}
	else if (stricmp(command, "loop") == 0)
	{
		Play(args.Argument(2), true);
	}
	else if (stricmp(command, "stop") == 0)
	{
		Stop();
	}
	else if (stricmp(command, "fadeout") == 0)
	{
		m_FadeDuration = m_FadeTimeCvar->value;
		m_FadeStartTime = std::chrono::system_clock::now() + std::chrono::milliseconds{static_cast<long long>(m_FadeTimeCvar->value * 1000.f)};
	}
	else if (stricmp(command, "pause") == 0)
	{
		Pause();
	}
	else if (stricmp(command, "resume") == 0)
	{
		Resume();
	}
	else if (stricmp(command, "info") == 0)
	{
		if (m_Playing)
			Con_Printf("Currently %s track %s\n", m_Looping ? "looping" : "playing", m_FileName.c_str());
		else if (m_Paused)
			Con_Printf("Paused %s track %s\n", m_Looping ? "looping" : "playing", m_FileName.c_str());
		Con_Printf("Volume is %f\n", m_Volume.load());
	}
	else
	{
		Con_Printf("Usage: music <on|off|reset|play <filename>|loop <filename>|stop|fadeout|pause|resume|info>\n");
	}
}
