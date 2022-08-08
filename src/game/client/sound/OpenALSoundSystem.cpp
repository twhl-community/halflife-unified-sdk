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

#include "hud.h"

#include "DummySoundSystem.h"
#include "OpenALMusicSystem.h"
#include "OpenALSoundSystem.h"

OpenALSoundSystem::~OpenALSoundSystem()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

bool OpenALSoundSystem::Create()
{
	m_Logger = g_Logging.CreateLogger("sound");

	m_Device.reset(alcOpenDevice(nullptr));

	if (!m_Device)
	{
		m_Logger->error("Couldn't open OpenAL device");
		return false;
	}

	m_Logger->trace("OpenAL device created");

	if (auto system = std::make_unique<OpenALMusicSystem>(); system->Create(this))
	{
		m_MusicSystem = std::move(system);
	}
	else
	{
		return false;
	}

	return true;
}

void OpenALSoundSystem::Block()
{
	if (m_Blocked)
	{
		return;
	}

	m_Blocked = true;

	m_MusicSystem->Block();
}

void OpenALSoundSystem::Unblock()
{
	if (!m_Blocked)
	{
		return;
	}

	m_Blocked = false;

	m_MusicSystem->Unblock();
}

void OpenALSoundSystem::Pause()
{
	if (m_Paused)
	{
		return;
	}

	m_Paused = true;

	m_MusicSystem->Pause();
}

void OpenALSoundSystem::Resume()
{
	if (!m_Paused)
	{
		return;
	}

	m_Paused = false;

	m_MusicSystem->Resume();
}

IMusicSystem* OpenALSoundSystem::GetMusicSystem()
{
	return m_MusicSystem.get();
}

void CreateSoundSystem()
{
	g_SoundSystem = []() -> std::unique_ptr<ISoundSystem>
	{
		if (auto system = std::make_unique<OpenALSoundSystem>(); system->Create())
		{
			return system;
		}

		return std::make_unique<DummySoundSystem>();
	}();
}
