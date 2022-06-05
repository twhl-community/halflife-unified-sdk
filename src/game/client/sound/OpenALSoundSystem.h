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

	ContextSwitcher(ALCcontext* context)
		: Previous(alcGetCurrentContext()), Success(ALC_FALSE != alcMakeContextCurrent(context))
	{
		if (!Success)
		{
			Con_Printf("Couldn't make OpenAL context current\n");
		}
	}

	~ContextSwitcher()
	{
		alcMakeContextCurrent(Previous);
	}

	operator bool() const { return Success; }
};

inline bool CheckOpenALContextExtension(ALCdevice* device, const char* name)
{
	if (ALC_FALSE == alcIsExtensionPresent(device, name))
	{
		Con_Printf("OpenAL does not provide extension \"%s\", required for music playback\n", name);
		return false;
	}

	return true;
}

inline bool CheckOpenALExtension(const char* name)
{
	if (ALC_FALSE == alIsExtensionPresent(name))
	{
		Con_Printf("OpenAL does not provide extension \"%s\", required for music playback\n", name);
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

	constexpr operator bool() const { return Id != NullBuffer; }

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

	constexpr operator bool() const { return Id != NullSource; }

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
	bool Create();

	IMusicSystem* GetMusicSystem() override { return m_MusicSystem.get(); }

	ALCdevice* GetDevice() { return m_Device.get(); }

private:
	std::unique_ptr<ALCdevice, DeleterWrapper<alcCloseDevice>> m_Device;
	std::unique_ptr<IMusicSystem> m_MusicSystem;
};
