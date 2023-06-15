/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#include "cbase.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "MapState.h"
#include "ServerLibrary.h"
#include "ServerSoundSystem.h"
#include "UserMessages.h"
#include "utils/ReplacementMaps.h"

namespace sound
{
bool ServerSoundSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("sound");
	g_NetworkData.RegisterHandler("SoundList", this);
	g_NetworkData.RegisterHandler("GlobalSoundReplacement", this);
	return true;
}

void ServerSoundSystem::PostInitialize()
{
}

void ServerSoundSystem::Shutdown()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

void ServerSoundSystem::HandleNetworkDataBlock(NetworkDataBlock& block)
{
	if (block.Name == "SoundList")
	{
		block.Data = json::array();

		for (std::size_t i = 1; i < g_SoundPrecache->GetCount(); ++i)
		{
			block.Data.push_back(g_SoundPrecache->GetString(i));
		}
	}
	else if (block.Name == "GlobalSoundReplacement")
	{
		block.Data = g_ReplacementMaps.Serialize(*g_Server.GetMapState()->m_GlobalSoundReplacement);
	}
}

void ServerSoundSystem::EmitSound(CBaseEntity* entity, int channel, const char* sample, float volume, float attenuation, int flags, int pitch)
{
	if (sample && *sample == '!')
	{
		sentences::SentenceIndexName name;
		if (sentences::g_Sentences.LookupSentence(entity, sample, &name) >= 0)
			EmitSoundSentence(entity, channel, name.c_str(), volume, attenuation, flags, pitch);
		else
			m_Logger->debug("Unable to find {} in sentences", sample);
	}
	else
	{
		sample = CheckForSoundReplacement(entity, sample);

		const Vector origin = entity->pev->origin + (entity->pev->mins + entity->pev->maxs) * 0.5f;
		EmitSoundCore(entity, channel, sample, volume, attenuation, flags, pitch, origin, false);
	}
}

void ServerSoundSystem::EmitAmbientSound(CBaseEntity* entity, const Vector& vecOrigin, const char* samp, float vol, float attenuation, int fFlags, int pitch)
{
	if (samp && *samp == '!')
	{
		sentences::SentenceIndexName name;
		if (sentences::g_Sentences.LookupSentence(entity, samp, &name) >= 0)
			EmitSoundCore(entity, CHAN_STATIC, name.c_str(), vol, attenuation, fFlags, pitch, vecOrigin, true);
	}
	else
	{
		samp = CheckForSoundReplacement(entity, samp);
		EmitSoundCore(entity, CHAN_STATIC, samp, vol, attenuation, fFlags, pitch, vecOrigin, true);
	}
}

const char* ServerSoundSystem::CheckForSoundReplacement(const char* soundName) const
{
	// Skip stream prefix. This effectively disables all sound streaming.
	// That's not a problem since streaming has no advantages on modern systems.
	if (soundName[0] == '*')
	{
		++soundName;
	}

	return g_Server.GetMapState()->m_GlobalSoundReplacement->Lookup(soundName);
}

const char* ServerSoundSystem::CheckForSoundReplacement(CBaseEntity* entity, const char* soundName) const
{
	if (soundName[0] == '*')
	{
		++soundName;
	}

	if (entity->m_SoundReplacement)
	{
		soundName = entity->m_SoundReplacement->Lookup(soundName);
	}

	return CheckForSoundReplacement(soundName);
}

static void BuildSoundMessage(int entityIndex, int channel, int soundIndex, int volumeInt, float attenuation,
	int flags, int pitch, const Vector& origin)
{
	// TODO: can compress this down by writing bits instead of bytes.
	WRITE_BYTE(flags);

	if ((flags & SND_VOLUME) != 0)
	{
		WRITE_BYTE(volumeInt);
	}

	if ((flags & SND_ATTENUATION) != 0)
	{
		WRITE_BYTE(static_cast<int>(attenuation * 64));
	}

	WRITE_BYTE(channel);
	WRITE_SHORT(entityIndex);

	if (soundIndex > std::numeric_limits<std::uint8_t>::max())
	{
		// This controls the maximum number of sounds and sentences.
		// The cast to signed short is needed to ensure the engine writes values > 32767 properly.
		WRITE_SHORT(static_cast<std::int16_t>(soundIndex));
	}
	else
	{
		WRITE_BYTE(soundIndex);
	}

	WRITE_COORD_VECTOR(origin);

	if ((flags & SND_PITCH) != 0)
	{
		WRITE_BYTE(pitch);
	}
}

void ServerSoundSystem::EmitSoundCore(CBaseEntity* entity, int channel, const char* sample, float volume, float attenuation,
	int flags, int pitch, const Vector& origin, bool alwaysBroadcast)
{
	if (volume < 0 || volume > 1)
	{
		m_Logger->error("EmitSound: volume = {}", volume);
		return;
	}

	if (attenuation < 0 || attenuation > 4)
	{
		m_Logger->error("EmitSound: attenuation = {}", attenuation);
		return;
	}

	if (channel > 7)
	{
		m_Logger->error("EmitSound: channel = {}", channel);
		return;
	}

	if (pitch > 255)
	{
		m_Logger->error("EmitSound: pitch = {}", pitch);
		return;
	}

	const int volumeInt = static_cast<int>(volume * 255);

	int soundIndex = 0;

	if (sample[0] == '!')
	{
		flags |= SND_SENTENCE;
		soundIndex = atoi(sample + 1);
	}
	else
	{
		soundIndex = g_SoundPrecache->IndexOf(sample);

		if (soundIndex <= 0)
		{
			m_Logger->error("EmitSound: {} not precached ({})", sample, soundIndex);
			return;
		}
	}

	const int entityIndex = entity->entindex();

	if (volumeInt != 255)
	{
		flags |= SND_VOLUME;
	}

	if (attenuation != 1)
	{
		flags |= SND_ATTENUATION;
	}

	if (pitch != PITCH_NORM)
	{
		flags |= SND_PITCH;
	}

	if (soundIndex > std::numeric_limits<std::uint8_t>::max())
	{
		flags |= SND_LARGE_INDEX;
	}

	if ((flags & SND_NOTHOST) == 0)
	{
		if ((flags & SND_SPAWNING) != 0)
		{
			MESSAGE_BEGIN(MSG_INIT, gmsgEmitSound);
		}
		else if (alwaysBroadcast)
		{
			MESSAGE_BEGIN(MSG_BROADCAST, gmsgEmitSound);
		}
		else if (channel != CHAN_STATIC && (flags & SND_STOP) == 0)
		{
			if (entity->m_SoundOffset != vec3_origin)
			{
				Vector adjustedOrigin = origin;

				Vector angles = entity->pev->angles;
				angles.x = -angles.x;

				float rmatrix[3][4];

				AngleMatrix(angles, rmatrix);

				Vector adjustedOffset;
				VectorTransform(entity->m_SoundOffset, rmatrix, adjustedOffset);

				adjustedOrigin = adjustedOrigin + adjustedOffset;

				MESSAGE_BEGIN(MSG_PAS, gmsgEmitSound, adjustedOrigin);
			}
			else
			{
				MESSAGE_BEGIN(MSG_PAS, gmsgEmitSound, origin);
			}
		}
		else if (!g_pGameRules->IsMultiplayer())
		{
			MESSAGE_BEGIN(MSG_BROADCAST, gmsgEmitSound);
		}
		else
		{
			MESSAGE_BEGIN(MSG_ALL, gmsgEmitSound);
		}

		BuildSoundMessage(entityIndex, channel, soundIndex, volumeInt, attenuation, flags, pitch, origin);

		MESSAGE_END();
		return;
	}

	if (entityIndex <= 0 || entityIndex > gpGlobals->maxClients)
	{
		m_Logger->error("EmitSound: Entity is not a player, cannot use SND_NOTHOST");
		return;
	}

	// These messages are used by player physics code only.
	for (auto player : UTIL_FindPlayers())
	{
		if (player == entity || !player->IsConnected() || !player->IsNetClient())
		{
			continue;
		}

		MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, gmsgEmitSound, nullptr, player);
		BuildSoundMessage(entityIndex, channel, soundIndex, volumeInt, attenuation, flags, pitch, origin);
		MESSAGE_END();
	}
}

void ServerSoundSystem::EmitSoundSentence(CBaseEntity* entity, int channel, const char* sample, float volume, float attenuation,
	int flags, int pitch)
{
	const Vector origin = entity->pev->origin + (entity->pev->mins + entity->pev->maxs) * 0.5f;

	EmitSoundCore(entity, channel, sample, volume, attenuation, flags, pitch, origin, false);
}
}

void EMIT_SOUND_PREDICTED(CBaseEntity* entity, int channel, const char* sample, float volume, float attenuation,
	int flags, int pitch)
{
	if (!entity)
	{
		return;
	}

	// If entity is not a player this will return false.
	if (0 != g_engfuncs.pfnCanSkipPlayer(entity->edict()))
	{
		flags |= SND_NOTHOST;
	}

	sound::g_ServerSound.EmitSound(entity, channel, sample, volume, attenuation, flags, pitch);
}

void EMIT_SOUND_SUIT(CBaseEntity* entity, const char* sample)
{
	float fvol;
	int pitch = PITCH_NORM;

	fvol = CVAR_GET_FLOAT("suitvolume");
	if (RANDOM_LONG(0, 1))
		pitch = RANDOM_LONG(0, 6) + 98;

	if (fvol > 0.05)
		sound::g_ServerSound.EmitSound(entity, CHAN_STATIC, sample, fvol, ATTN_NORM, 0, pitch);
}

void EMIT_GROUPNAME_SUIT(CBaseEntity* entity, const char* groupname)
{
	float fvol;
	int pitch = PITCH_NORM;

	fvol = CVAR_GET_FLOAT("suitvolume");
	if (RANDOM_LONG(0, 1))
		pitch = RANDOM_LONG(0, 6) + 98;

	if (fvol > 0.05)
		sentences::g_Sentences.PlayRndSz(entity, groupname, fvol, ATTN_NORM, 0, pitch);
}
