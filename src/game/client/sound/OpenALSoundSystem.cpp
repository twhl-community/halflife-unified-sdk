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

bool OpenALSoundSystem::Create()
{
	m_Device.reset(alcOpenDevice(nullptr));

	if (!m_Device)
	{
		Con_Printf("Couldn't open OpenAL device\n");
		return false;
	}

	m_MusicSystem = [this]() -> std::unique_ptr<IMusicSystem>
	{
		if (auto system = std::make_unique<OpenALMusicSystem>(); system->Create(*this))
		{
			return system;
		}

		return std::make_unique<DummyMusicSystem>();
	}();

	return true;
}

std::unique_ptr<ISoundSystem> CreateSoundSystem()
{
	if (auto system = std::make_unique<OpenALSoundSystem>(); system->Create())
	{
		return system;
	}

	return std::make_unique<DummySoundSystem>();
}
