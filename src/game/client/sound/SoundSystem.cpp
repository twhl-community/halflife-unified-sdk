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

#include <limits>

#include "hud.h"

#include "DummySoundSystem.h"
#include "GameSoundSystem.h"
#include "MusicSystem.h"
#include "SoundSystem.h"

#include "utils/ConCommandSystem.h"

namespace sound
{
SoundSystem::~SoundSystem()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

bool SoundSystem::Create()
{
	m_Logger = g_Logging.CreateLogger("sound");

	m_Device.reset(alcOpenDevice(nullptr));

	if (!m_Device)
	{
		m_Logger->error("Couldn't open OpenAL device");
		return false;
	}

	m_Logger->trace("OpenAL device opened");

	if (auto system = std::make_unique<GameSoundSystem>(); system->Create(m_Logger, m_Device.get()))
	{
		m_GameSoundSystem = std::move(system);
	}
	else
	{
		return false;
	}

	if (auto system = std::make_unique<MusicSystem>(); system->Create(m_Logger, m_Device.get()))
	{
		m_MusicSystem = std::move(system);
	}
	else
	{
		return false;
	}

	return true;
}

void SoundSystem::Update()
{
	m_GameSoundSystem->Update();
}

void SoundSystem::Block()
{
	if (m_Blocked)
	{
		return;
	}

	m_Blocked = true;

	m_GameSoundSystem->Block();
	m_MusicSystem->Block();
}

void SoundSystem::Unblock()
{
	if (!m_Blocked)
	{
		return;
	}

	m_Blocked = false;

	m_GameSoundSystem->Unblock();
	m_MusicSystem->Unblock();
}

void SoundSystem::Pause()
{
	if (m_Paused)
	{
		return;
	}

	m_Paused = true;

	m_GameSoundSystem->Pause();
	m_MusicSystem->Pause();
}

void SoundSystem::Resume()
{
	if (!m_Paused)
	{
		return;
	}

	m_Paused = false;

	m_GameSoundSystem->Resume();
	m_MusicSystem->Resume();
}

IGameSoundSystem* SoundSystem::GetGameSoundSystem()
{
	return m_GameSoundSystem.get();
}

IMusicSystem* SoundSystem::GetMusicSystem()
{
	return m_MusicSystem.get();
}

static int UserMsg_EmitSound(const char* pszName, int iSize, void* pbuf)
{
	if (g_SoundSystem)
	{
		g_SoundSystem->GetGameSoundSystem()->UserMsg_EmitSound(pszName, iSize, pbuf);
	}

	return 1;
}

static void S_PlaySound(const CommandArgs& args, int channelIndex)
{
	if (args.Count() < 2)
	{
		Con_Printf("Usage: %s <sound name> [volume]\n", args.Argument(0));
		return;
	}

	const char* name = args.Argument(1);

	const float volume = args.Count() >= 3 ? std::clamp(atof(args.Argument(2)), 0., 1.) : VOL_NORM;
	const float attenuation = args.Count() >= 4 ? atof(args.Argument(3)) : ATTN_NORM;
	const int pitch = args.Count() >= 5
						  ? std::clamp(
								atoi(args.Argument(4)),
								static_cast<int>(std::numeric_limits<std::uint8_t>::min()),
								static_cast<int>(std::numeric_limits<std::uint8_t>::max()))
						  : PITCH_NORM;

	int entityIndex = 0;
	Vector origin = vec3_origin;

	if (auto levelName = gEngfuncs.pfnGetLevelName(); levelName && *levelName)
	{
		auto localPlayer = gEngfuncs.GetLocalPlayer();

		entityIndex = localPlayer->index;
		origin = localPlayer->curstate.origin;
	}

	g_SoundSystem->GetGameSoundSystem()->StartSound(
		entityIndex, channelIndex, name, origin, volume, attenuation, pitch, 0);
}

static void S_PlayStaticSound(const CommandArgs& args)
{
	S_PlaySound(args, CHAN_STATIC);
}

static void S_PlayDynamicSound(const CommandArgs& args)
{
	S_PlaySound(args, CHAN_VOICE);
}

void CreateSoundSystem()
{
	g_cl_snd_openal = g_ConCommands.CreateCVar("snd_openal", "1", FCVAR_ARCHIVE);
	g_cl_snd_room_off = g_ConCommands.CreateCVar("snd_room_off", "0", FCVAR_ARCHIVE);

	g_SoundSystem = []() -> std::unique_ptr<ISoundSystem>
	{
		if (auto system = std::make_unique<SoundSystem>(); system->Create())
		{
			return system;
		}

		return std::make_unique<DummySoundSystem>();
	}();

	gEngfuncs.pfnHookUserMsg("EmitSound", &UserMsg_EmitSound);

	g_ConCommands.CreateCommand("snd_playstatic", &S_PlayStaticSound);
	g_ConCommands.CreateCommand("snd_playdynamic", &S_PlayDynamicSound);
	g_ConCommands.CreateCommand("snd_stopsound", [](const auto&)
		{ g_SoundSystem->GetGameSoundSystem()->StopAllSounds(); });
}
}
