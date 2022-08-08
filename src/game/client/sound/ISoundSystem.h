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

struct IMusicSystem;

/**
*	@brief Provides access to high-level sound system interfaces.
*/
struct ISoundSystem
{
	virtual ~ISoundSystem() = default;

	virtual void Block() = 0;

	virtual void Unblock() = 0;

	virtual void Pause() = 0;

	virtual void Resume() = 0;

	virtual IMusicSystem* GetMusicSystem() = 0;
};

inline std::unique_ptr<ISoundSystem> g_SoundSystem;

/**
*	@brief Creates an instance of the sound system using an available implementation.
*/
void CreateSoundSystem();
