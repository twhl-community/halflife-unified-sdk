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
#include "spawnpoints.h"

LINK_ENTITY_TO_CLASS(info_player_start, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_deathmatch, CBaseDMStart);

bool CBaseDMStart::IsTriggered(CBaseEntity* pEntity)
{
	return UTIL_IsMasterTriggered(m_sMaster, pEntity, m_UseLocked);
}

/**
 *	@brief checks if the spot is clear of players
 */
bool IsSpawnPointValid(CBaseEntity* pPlayer, CBaseEntity* pSpot)
{
	CBaseEntity* ent = nullptr;

	if (!pSpot->IsTriggered(pPlayer))
	{
		return false;
	}

	while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != nullptr)
	{
		// if ent is a client, don't spawn on 'em
		if (ent->IsPlayer() && ent != pPlayer)
			return false;
	}

	return true;
}

static CBaseEntity* EntTrySelectSpawnPoint(CBasePlayer* pPlayer)
{
	CBaseEntity* pSpot;

	if (g_pGameRules->IsCTF() && pPlayer->m_iTeamNum != CTFTeam::None)
	{
		const auto pszTeamSpotName = pPlayer->m_iTeamNum == CTFTeam::BlackMesa ? "ctfs1" : "ctfs2";

		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for (int i = RANDOM_LONG(1, 5); i > 0; i--)
			pSpot = UTIL_FindEntityByClassname(pSpot, pszTeamSpotName);
		if (FNullEnt(pSpot)) // skip over the null point
			pSpot = UTIL_FindEntityByClassname(pSpot, pszTeamSpotName);

		CBaseEntity* pFirstSpot = pSpot;

		do
		{
			if (pSpot)
			{
				// check if pSpot is valid
				if (IsSpawnPointValid(pPlayer, pSpot) && pSpot->pev->origin != g_vecZero)
				{
					// if so, go to pSpot
					return pSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname(pSpot, pszTeamSpotName);
		} while (pSpot != pFirstSpot); // loop if we're not back to the start

		// Try a shared spawn spot
		//  Randomize the start spot
		for (int i = RANDOM_LONG(1, 5); i > 0; i--)
			pSpot = UTIL_FindEntityByClassname(pSpot, "ctfs0");
		if (FNullEnt(pSpot)) // skip over the null point
			pSpot = UTIL_FindEntityByClassname(pSpot, "ctfs0");

		pFirstSpot = pSpot;

		do
		{
			if (pSpot)
			{
				// check if pSpot is valid
				if (IsSpawnPointValid(pPlayer, pSpot) && pSpot->pev->origin != g_vecZero)
				{
					// if so, go to pSpot
					return pSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname(pSpot, "ctfs0");
		} while (pSpot != pFirstSpot); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if (!FNullEnt(pSpot))
		{
			CBaseEntity* ent = nullptr;
			while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != nullptr)
			{
				// if ent is a client, kill em (unless they are ourselves)
				if (ent->IsPlayer() && ent != pPlayer)
					ent->TakeDamage(CBaseEntity::World, CBaseEntity::World, 300, DMG_GENERIC);
			}
			return pSpot;
		}
	}

	// choose a info_player_deathmatch point
	if (g_pGameRules->IsCoOp())
	{
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_coop");
		if (!FNullEnt(pSpot))
			return pSpot;
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_start");
		if (!FNullEnt(pSpot))
			return pSpot;
	}
	else if (g_pGameRules->IsMultiplayer())
	{
		pSpot = g_pLastSpawn;
		// Randomize the start spot
		for (int i = RANDOM_LONG(1, 5); i > 0; i--)
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
		if (FNullEnt(pSpot)) // skip over the null point
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");

		CBaseEntity* pFirstSpot = pSpot;

		do
		{
			if (pSpot)
			{
				// check if pSpot is valid
				if (IsSpawnPointValid(pPlayer, pSpot))
				{
					if (pSpot->pev->origin == Vector(0, 0, 0))
					{
						pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
						continue;
					}

					// if so, go to pSpot
					return pSpot;
				}
			}
			// increment pSpot
			pSpot = UTIL_FindEntityByClassname(pSpot, "info_player_deathmatch");
		} while (pSpot != pFirstSpot); // loop if we're not back to the start

		// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
		if (!FNullEnt(pSpot))
		{
			CBaseEntity* ent = nullptr;
			while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != nullptr)
			{
				// if ent is a client, kill em (unless they are ourselves)
				if (ent->IsPlayer() && ent != pPlayer)
					ent->TakeDamage(CBaseEntity::World, CBaseEntity::World, 300, DMG_GENERIC);
			}
			return pSpot;
		}
	}

	// If startspot is set, (re)spawn there.
	if (FStringNull(gpGlobals->startspot) || 0 == strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = UTIL_FindEntityByClassname(nullptr, "info_player_start");
		if (!FNullEnt(pSpot))
			return pSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname(nullptr, STRING(gpGlobals->startspot));
		if (!FNullEnt(pSpot))
			return pSpot;
	}

	return nullptr;
}

CBaseEntity* EntSelectSpawnPoint(CBasePlayer* pPlayer)
{
	auto pSpot = EntTrySelectSpawnPoint(pPlayer);

	if (FNullEnt(pSpot))
	{
		CBaseEntity::Logger->error("PutClientInServer: no info_player_start on level");
		return CBaseEntity::World;
	}

	g_pLastSpawn = pSpot;
	return pSpot;
}
