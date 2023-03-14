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

struct TEMPENTITY;

namespace sound
{
struct IGameSoundSystem;
struct IMusicSystem;

/**
 *	@brief Provides access to high-level sound system interfaces.
 */
struct ISoundSystem
{
	virtual ~ISoundSystem() = default;

	virtual void Update() = 0;

	virtual void Block() = 0;

	virtual void Unblock() = 0;

	virtual void Pause() = 0;

	virtual void Resume() = 0;

	virtual IGameSoundSystem* GetGameSoundSystem() = 0;

	virtual IMusicSystem* GetMusicSystem() = 0;
};

inline std::unique_ptr<ISoundSystem> g_SoundSystem;

inline cvar_t* g_cl_snd_openal = nullptr;
inline cvar_t* g_cl_snd_room_off = nullptr;

/**
 *	@brief Creates an instance of the sound system using an available implementation.
 */
void CreateSoundSystem();
}

void CL_StartSound(int ent, int channel, const char* sample, const Vector& origin, float volume, float attenuation, int pitch, int fFlags);

// Wrappers for engine functions used by client code.
void EV_PlaySound(int ent, const Vector& origin, int channel, const char* sample, float volume, float attenuation, int fFlags, int pitch);

void EV_StopSound(int ent, int channel, const char* sample);

void PlaySound(const char* szSound, float vol);
void PlaySoundByNameAtLocation(const char* szSound, float volume, const Vector& origin);

void CL_TempEntPlaySound(TEMPENTITY* pTemp, float damp);

void R_RicochetSound(const Vector& pos, int index);
void R_RicochetSound(const Vector& pos);

void R_Explosion(const Vector& pos, int model, float scale, float framerate, int flags);
