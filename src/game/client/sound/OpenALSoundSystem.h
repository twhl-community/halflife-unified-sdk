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

#pragma once

#include <memory>

#include <spdlog/logger.h>

#include <AL/al.h>
#include <AL/alc.h>

#include <libnyquist/Decoders.h>

#include "ISoundSystem.h"

template <auto Function>
struct DeleterWrapper final
{
	template <typename T>
	constexpr void operator()(T* object) const
	{
		Function(object);
	}
};

struct ContextSwitcher
{
	ALCcontext* const Previous;

	const bool Success;

	ContextSwitcher(ALCcontext* context, spdlog::logger& logger)
		: Previous(alcGetCurrentContext()), Success(ALC_FALSE != alcMakeContextCurrent(context))
	{
		if (!Success)
		{
			logger.error("Couldn't make OpenAL context current");
		}
	}

	~ContextSwitcher()
	{
		alcMakeContextCurrent(Previous);
	}

	operator bool() const { return Success; }
};

inline bool CheckOpenALContextExtension(ALCdevice* device, const char* name, spdlog::logger& logger)
{
	if (ALC_FALSE == alcIsExtensionPresent(device, name))
	{
		logger.error("OpenAL does not provide extension \"{}\", required for music playback", name);
		return false;
	}

	return true;
}

inline bool CheckOpenALExtension(const char* name, spdlog::logger& logger)
{
	if (ALC_FALSE == alIsExtensionPresent(name))
	{
		logger.error("OpenAL does not provide extension \"{}\", required for music playback", name);
		return false;
	}

	return true;
}

constexpr ALuint NullBuffer = 0;
constexpr ALuint NullSource = 0;

struct OpenALBuffer final
{
	constexpr OpenALBuffer() noexcept = default;

	OpenALBuffer(const OpenALBuffer&) = delete;
	OpenALBuffer& operator=(const OpenALBuffer&) = delete;

	constexpr OpenALBuffer(OpenALBuffer&& other) noexcept
		: Id(other.Id)
	{
		other.Id = NullBuffer;
	}

	constexpr OpenALBuffer& operator=(OpenALBuffer&& other) noexcept
	{
		if (this != &other)
		{
			Delete();
			Id = other.Id;
			other.Id = NullBuffer;
		}

		return *this;
	}

	~OpenALBuffer()
	{
		Delete();
	}

	static OpenALBuffer Create()
	{
		OpenALBuffer buffer;
		alGenBuffers(1, &buffer.Id);
		return buffer;
	}

	constexpr bool IsValid() const { return Id != NullBuffer; }

	void Delete()
	{
		if (Id != NullBuffer)
		{
			alDeleteBuffers(1, &Id);
			Id = NullBuffer;
		}
	}

	ALuint Id = NullBuffer;
};

struct OpenALSource final
{
	constexpr OpenALSource() noexcept = default;

	OpenALSource(const OpenALSource&) = delete;
	OpenALSource& operator=(const OpenALSource&) = delete;

	constexpr OpenALSource(OpenALSource&& other) noexcept
		: Id(other.Id)
	{
		other.Id = NullSource;
	}

	constexpr OpenALSource& operator=(OpenALSource&& other) noexcept
	{
		if (this != &other)
		{
			Delete();
			Id = other.Id;
			other.Id = NullSource;
		}

		return *this;
	}

	~OpenALSource()
	{
		Delete();
	}

	static OpenALSource Create()
	{
		OpenALSource source;
		alGenSources(1, &source.Id);
		return source;
	}

	constexpr bool IsValid() const { return Id != NullSource; }

	void Delete()
	{
		if (Id != NullSource)
		{
			alDeleteSources(1, &Id);
			Id = NullSource;
		}
	}

	ALuint Id = NullSource;
};

class OpenALSoundSystem final : public ISoundSystem
{
public:
	~OpenALSoundSystem() override;

	bool Create();

	IMusicSystem* GetMusicSystem() override { return m_MusicSystem.get(); }

	spdlog::logger* GetLogger() { return m_Logger.get(); }

	ALCdevice* GetDevice() { return m_Device.get(); }

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	std::unique_ptr<ALCdevice, DeleterWrapper<alcCloseDevice>> m_Device;
	std::unique_ptr<IMusicSystem> m_MusicSystem;
};
