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
#include "const.h"

#include "DummySoundSystem.h"
#include "GameSoundSystem.h"
#include "MusicSystem.h"
#include "r_efx.h"
#include "SoundSystem.h"
#include "view.h"

#include "networking/NetworkDataSystem.h"
#include "sound/ClientSoundReplacementSystem.h"
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

	if (auto system = std::make_unique<GameSoundSystem>(); system->Create(m_Logger))
	{
		m_GameSoundSystem = std::move(system);
	}
	else
	{
		return false;
	}

	if (auto system = std::make_unique<MusicSystem>(); system->Create(m_Logger))
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

static void S_PlaySound(const CommandArgs& args, int channelIndex)
{
	if (args.Count() < 2)
	{
		Con_Printf("Usage: %s <sound name> [volume] [attenuation] [pitch]\n", args.Argument(0));
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

	if (auto levelName = gEngfuncs.pfnGetLevelName(); levelName != nullptr && *levelName != '\0')
	{
		auto localPlayer = gEngfuncs.GetLocalPlayer();

		entityIndex = localPlayer->index;
		origin = localPlayer->curstate.origin;
	}

	g_SoundSystem->GetGameSoundSystem()->StartSound(
		entityIndex, channelIndex, name, origin, volume, attenuation, pitch, SND_PLAY_WHEN_PAUSED);
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

	g_ClientUserMessages.RegisterHandler("EmitSound", &IGameSoundSystem::MsgFunc_EmitSound, g_SoundSystem->GetGameSoundSystem());

	g_ConCommands.CreateCommand("snd_playstatic", &S_PlayStaticSound);
	g_ConCommands.CreateCommand("snd_playdynamic", &S_PlayDynamicSound);
	g_ConCommands.CreateCommand("snd_stopsound", [](const auto&)
		{ g_SoundSystem->GetGameSoundSystem()->StopAllSounds(); });

	g_NetworkData.RegisterHandler("SoundList", g_SoundSystem->GetGameSoundSystem());
	g_NetworkData.RegisterHandler("Sentences", g_SoundSystem->GetGameSoundSystem());
}
}

void CL_StartSound(int ent, int channel, const char* sample, const Vector& origin, float volume, float attenuation, int pitch, int fFlags)
{
	sample = sound::g_ClientSoundReplacement.Lookup(sample);
	sound::g_SoundSystem->GetGameSoundSystem()->StartSound(ent, channel, sample, origin, volume, attenuation, pitch, fFlags);
}

void EV_PlaySound(int ent, const Vector& origin, int channel, const char* sample, float volume, float attenuation, int fFlags, int pitch)
{
	CL_StartSound(ent, channel, sample, origin, volume, attenuation, pitch, fFlags);

	// gEngfuncs.pEventAPI->EV_PlaySound(ent, origin, channel, sample, volume, attenuation, fFlags, pitch);
}

void EV_StopSound(int ent, int channel, const char* sample)
{
	CL_StartSound(ent, channel, sample, vec3_origin, VOL_NORM, 1, PITCH_NORM, SND_STOP);

	// gEngfuncs.pEventAPI->EV_StopSound(ent, channel, sample);
}

void PlaySound(int channel, const char* szSound, float vol)
{
	vol = std::clamp(vol, 0.f, 1.f);

	CL_StartSound(g_ViewEntity, channel, szSound, v_origin, vol, 1.0, PITCH_NORM, 0);

	// gEngfuncs.pfnPlaySoundByName(szSound, vol);
}

void PlaySoundByNameAtLocation(const char* szSound, float volume, const Vector& origin)
{
	volume = std::clamp(volume, 0.f, 1.f);

	CL_StartSound(g_ViewEntity, CHAN_AUTO, szSound, origin, volume, 1.0, PITCH_NORM, 0);

	// gEngfuncs.pfnPlaySoundByNameAtLocation(szSound, 1.0, entity->attachment[0]);
}

void CL_TempEntPlaySound(TEMPENTITY* pTemp, float damp)
{
	bool isShell = false;
	float fvol = 0.8f;

	// Note: RandomLong is inclusive so this value should be one less than the number of elements!
	int count;
	const char* sounds[6]{};

	switch (pTemp->hitSound)
	{
	default: return;

	case BOUNCE_GLASS:
	{
		count = 2;
		sounds[0] = "debris/glass1.wav";
		sounds[1] = "debris/glass2.wav";
		sounds[2] = "debris/glass3.wav";
		break;
	}

	case BOUNCE_METAL:
	{
		count = 2;
		sounds[0] = "debris/metal1.wav";
		sounds[1] = "debris/metal2.wav";
		sounds[2] = "debris/metal3.wav";
		break;
	}

	case BOUNCE_FLESH:
	{
		count = 5;
		sounds[0] = "debris/flesh1.wav";
		sounds[1] = "debris/flesh2.wav";
		sounds[2] = "debris/flesh3.wav";
		sounds[3] = "debris/flesh5.wav";
		sounds[4] = "debris/flesh6.wav";
		sounds[5] = "debris/flesh7.wav";
		break;
	}

	case BOUNCE_WOOD:
	{
		count = 2;
		sounds[0] = "debris/wood1.wav";
		sounds[1] = "debris/wood2.wav";
		sounds[2] = "debris/wood3.wav";
		break;
	}

	case BOUNCE_SHRAP:
	{
		count = 4;
		sounds[0] = "weapons/ric1.wav";
		sounds[1] = "weapons/ric2.wav";
		sounds[2] = "weapons/ric3.wav";
		sounds[3] = "weapons/ric4.wav";
		sounds[4] = "weapons/ric5.wav";
		break;
	}

	case BOUNCE_SHELL:
	{
		isShell = true;
		count = 2;
		sounds[0] = "player/pl_shell1.wav";
		sounds[1] = "player/pl_shell2.wav";
		sounds[2] = "player/pl_shell3.wav";
		break;
	}

	case BOUNCE_CONCRETE:
	{
		count = 2;
		sounds[0] = "debris/concrete1.wav";
		sounds[1] = "debris/concrete2.wav";
		sounds[2] = "debris/concrete3.wav";
		break;
	}

	case BOUNCE_SHOTSHELL:
	{
		fvol = 0.5;
		isShell = true;
		count = 2;
		sounds[0] = "weapons/sshell1.wav";
		sounds[1] = "weapons/sshell2.wav";
		sounds[2] = "weapons/sshell3.wav";
		break;
	}
	}

	// Note: temp entities use baseline.origin to store off velocity. This is vertical movement speed.
	const int bounceSpeed = std::abs(static_cast<int>(pTemp->entity.baseline.origin[2]));

	if (isShell && bounceSpeed < 200)
	{
		if (gEngfuncs.pfnRandomLong(0, 3) != 0)
			return;
	}

	if (damp <= 0)
	{
		return;
	}

	if (isShell)
	{
		fvol = fvol * std::min(1.f, bounceSpeed / 350.f);
	}
	else
	{
		fvol = fvol * std::min(1.0f, bounceSpeed / 450.f);
	}

	int pitch = PITCH_NORM;

	if (gEngfuncs.pfnRandomLong(0, 3) == 0 && !isShell)
	{
		pitch = gEngfuncs.pfnRandomLong(90, 124);
	}

	CL_StartSound(-1, CHAN_AUTO, sounds[gEngfuncs.pfnRandomLong(0, count)], pTemp->entity.origin, fvol, 1.0, pitch, 0);

	// S_StartDynamicSound(-1, CHAN_AUTO, sounds[gEngfuncs.pfnRandomLong(0, count)], pTemp->entity.origin, fvol, 1.0, 0, pitch);
}

constexpr const char* RicochetSounds[] =
	{
		"weapons/ric1.wav",
		"weapons/ric2.wav",
		"weapons/ric3.wav",
		"weapons/ric4.wav",
		"weapons/ric5.wav"};

void R_RicochetSound(const Vector& pos, int index)
{
	CL_StartSound(-1, CHAN_AUTO, RicochetSounds[index % std::size(RicochetSounds)], pos, VOL_NORM, 1.0, PITCH_NORM, 0);
}

void R_RicochetSound(const Vector& pos)
{
	R_RicochetSound(pos, gEngfuncs.pfnRandomLong(0, 4));

	// gEngfuncs.pEfxAPI->R_RicochetSound(pos);
}

constexpr const char* ExplosionSounds[] =
	{
		"weapons/explode3.wav",
		"weapons/explode4.wav",
		"weapons/explode5.wav"};

void R_Explosion(const Vector& pos, int model, float scale, float framerate, int flags)
{
	if (scale != 0)
	{
		auto sprite = gEngfuncs.pEfxAPI->R_DefaultSprite(pos, model, framerate);
		gEngfuncs.pEfxAPI->R_Sprite_Explode(sprite, scale, flags);

		if ((flags & TE_EXPLFLAG_NOPARTICLES) == 0)
		{
			gEngfuncs.pEfxAPI->R_FlickerParticles(pos);
		}

		if ((flags & TE_EXPLFLAG_NODLIGHTS) == 0)
		{
			auto yellowLight = gEngfuncs.pEfxAPI->CL_AllocDlight(0);

			yellowLight->origin = pos;

			yellowLight->radius = 200;

			yellowLight->color.r = 250;
			yellowLight->color.g = 250;
			yellowLight->color.b = 150;

			yellowLight->die = gEngfuncs.GetClientTime() + 0.01;

			yellowLight->decay = 800;

			auto orangeLight = gEngfuncs.pEfxAPI->CL_AllocDlight(0);

			orangeLight->origin = pos;

			orangeLight->radius = 150;

			orangeLight->color.r = 255;
			orangeLight->color.g = 190;
			orangeLight->color.b = 40;

			orangeLight->die = gEngfuncs.GetClientTime() + 1;

			orangeLight->decay = 200;
		}
	}

	if ((flags & TE_EXPLFLAG_NOSOUND) == 0)
	{
		CL_StartSound(-1, CHAN_AUTO, ExplosionSounds[gEngfuncs.pfnRandomLong(0, 2)], pos, VOL_NORM, 0.3f, PITCH_NORM, 0);
	}
}
