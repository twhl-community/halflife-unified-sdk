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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "CItemCTF.h"
#include "CItemSpawnCTF.h"
#include "gamerules.h"
#include "ctfplay_gamerules.h"

const auto SF_ITEMCTF_RANDOM_SPAWN = 1 << 2;
const auto SF_ITEMCTF_IGNORE_TEAM = 1 << 3;

CItemSpawnCTF* CItemCTF::m_pLastSpawn = nullptr;

bool CItemCTF::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("team_no", pkvd->szKeyName))
	{
		team_no = static_cast<CTFTeam>(atoi(pkvd->szValue));
		return true;
	}

	//TODO: should invoke base class KeyValue here
	return false;
}

void CItemCTF::Precache()
{
	if (!FStringNull(pev->model))
		PRECACHE_MODEL(const_cast<char*>(STRING(pev->model)));

	PRECACHE_SOUND("ctf/itemthrow.wav");
	PRECACHE_SOUND("items/ammopickup1.wav");
}

void CItemCTF::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin(pev, pev->origin);

	UTIL_SetSize(pev, {-16, -16, 0}, {16, 16, 48});

	SetTouch(&CItemCTF::ItemTouch);

	if (DROP_TO_FLOOR(edict()) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove(this);
		return;
	}

	if (!FStringNull(pev->model))
	{
		SET_MODEL(edict(), STRING(pev->model));

		pev->sequence = LookupSequence("idle");

		if (pev->sequence != -1)
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}

	if ((pev->spawnflags & SF_ITEMCTF_RANDOM_SPAWN) != 0)
	{
		SetThink(&CItemCTF::DropThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}

	m_iLastTouched = 0;
	m_flPickupTime = 0;

	//TODO: already done above
	SetTouch(&CItemCTF::ItemTouch);

	m_flNextTouchTime = 0;
	m_pszItemName = "";
}

void CItemCTF::DropPreThink()
{
	const auto contents = UTIL_PointContents(pev->origin);

	if (contents == CONTENTS_SLIME || contents == CONTENTS_LAVA || contents == CONTENTS_SOLID || contents == CONTENTS_SKY)
	{
		DropThink();
	}
	else
	{
		SetThink(&CItemCTF::DropThink);

		pev->nextthink = V_max(0, g_flPowerupRespawnTime - 5) + gpGlobals->time;
	}
}

void CItemCTF::DropThink()
{
	SetThink(nullptr);

	pev->origin = pev->oldorigin;

	int nTested = 0;
	CItemSpawnCTF* spawn = nullptr;

	if ((pev->spawnflags & SF_ITEMCTF_IGNORE_TEAM) == 0)
	{
		int iLosingTeam, iScoreDiff;
		GetLosingTeam(iLosingTeam, iScoreDiff);

		for (int i = 0; i < 2; ++i)
		{
			if (i == 0)
				spawn = m_pLastSpawn;
			else
				spawn = nullptr;

			while ((spawn = (CItemSpawnCTF*)UTIL_FindEntityByClassname(spawn, "info_ctfspawn_powerup")) != nullptr)
			{
				// Assume occupied until proven otherwise.
				bool nOccupied = true;

				if (iScoreDiff == 0 ||
					(iScoreDiff == 1 && (g_engfuncs.pfnRandomLong(0, 1) != 0 && spawn->team_no == CTFTeam::None ||
											g_engfuncs.pfnRandomLong(0, 1) != 0 && spawn->team_no == CTFTeam(iLosingTeam + 1))) ||
					(iScoreDiff > 1 && spawn->team_no == CTFTeam(iLosingTeam + 1)))
				{
					nOccupied = false;
					++nTested;

					for (CBaseEntity* test = nullptr; (test = UTIL_FindEntityInSphere(test, spawn->pev->origin, 128)) != nullptr;)
					{
						if (test->Classify() == CLASS_CTFITEM && this != test)
						{
							nOccupied = true;
							break;
						}
					}
				}

				if (!nOccupied)
				{
					pev->origin = spawn->pev->origin;
					if (g_engfuncs.pfnRandomLong(0, 1) != 0)
					{
						break;
					}
				}
			}

			if (spawn == m_pLastSpawn)
				break;

			if (spawn)
				break;
		}
	}

	if (!spawn && pev->origin == pev->oldorigin)
	{
		for (spawn = nullptr; (spawn = (CItemSpawnCTF*)UTIL_FindEntityByClassname(spawn, "info_ctfspawn_powerup")) != nullptr;)
		{
			bool nOccupied = false;

			for (CBaseEntity* test = nullptr; (test = UTIL_FindEntityInSphere(test, spawn->pev->origin, 128)) != nullptr;)
			{
				if (test->Classify() == CLASS_CTFITEM && this != test)
				{
					nOccupied = true;
					break;
				}
			}

			if (!nOccupied)
			{
				pev->origin = spawn->pev->origin;
				if (g_engfuncs.pfnRandomLong(0, 1) != 0)
				{
					break;
				}
			}
		}
	}

	m_pLastSpawn = spawn;

	if (!spawn && nTested > 0)
		ALERT(at_console, "Warning: No available spawn points found.  Powerup returned to original coordinates.");

	UTIL_SetOrigin(pev, pev->origin);

	if (g_engfuncs.pfnDropToFloor(edict()) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f",
			STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove(this);
	}
}

void CItemCTF::CarryThink()
{
	auto pOwner = Instance<CBasePlayer>(pev->owner);

	if (pOwner && pOwner->IsPlayer())
	{
		if ((m_iItemFlag & pOwner->m_iItems) != 0)
		{
			pev->nextthink = gpGlobals->time + 20;
		}
		else
		{
			pOwner->m_iClientItems = m_iItemFlag;
			DropItem(pOwner, true);
		}
	}
	else
	{
		DropItem(nullptr, true);
	}
}

void CItemCTF::ItemTouch(CBaseEntity* pOther)
{
	//TODO: really shouldn't be using the index here tbh
	if (pOther->IsPlayer() && pOther->IsAlive() && (m_iLastTouched != pOther->entindex() || m_flNextTouchTime <= gpGlobals->time))
	{
		m_iLastTouched = 0;

		if (MyTouch(static_cast<CBasePlayer*>(pOther)))
		{
			SUB_UseTargets(pOther, USE_TOGGLE, 0);
			SetTouch(nullptr);

			pev->movetype = MOVETYPE_NONE;
			pev->solid = SOLID_NOT;
			pev->effects |= EF_NODRAW;

			UTIL_SetOrigin(pev, pev->origin);

			pev->owner = pOther->edict();

			SetThink(&CItemCTF::CarryThink);
			pev->nextthink = gpGlobals->time + 20.0;

			m_flPickupTime = gpGlobals->time;

			UTIL_LogPrintf(
				"\"%s<%i><%u><%s>\" triggered \"take_%s_Powerup\"\n",
				STRING(pOther->pev->netname),
				GETPLAYERUSERID(pOther->edict()),
				g_engfuncs.pfnGetPlayerWONId(pOther->edict()),
				GetTeamName(pOther->edict()),
				m_pszItemName);
		}
	}
}

void CItemCTF::DropItem(CBasePlayer* pPlayer, bool bForceRespawn)
{
	if (pPlayer)
	{
		RemoveEffect(pPlayer);

		UTIL_LogPrintf(
			"\"%s<%i><%u><%s>\" triggered \"drop_%s_Powerup\"\n",
			STRING(pPlayer->pev->netname),
			GETPLAYERUSERID(pPlayer->edict()),
			g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
			GetTeamName(pPlayer->edict()),
			m_pszItemName);

		pev->origin = pPlayer->pev->origin + Vector(0, 0, (pPlayer->pev->flags & FL_DUCKING) != 0 ? 34 : 16);
	}

	UTIL_SetOrigin(pev, pev->origin);

	pev->flags &= ~FL_ONGROUND;
	pev->movetype = MOVETYPE_TOSS;
	pev->angles = g_vecZero;
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	pev->velocity = g_vecZero;

	pev->velocity.z = -100;

	if (pev->owner)
	{
		auto pOwner = GET_PRIVATE<CBasePlayer>(pev->owner);

		if (pOwner)
		{
			if (pOwner->IsPlayer())
				pOwner->m_iItems = static_cast<CTFItem::CTFItem>(pOwner->m_iItems & ~m_iItemFlag);
		}

		pev->owner = nullptr;
	}

	SetTouch(&CItemCTF::ItemTouch);

	if (bForceRespawn)
	{
		DropThink();
	}
	else
	{
		SetThink(&CItemCTF::DropPreThink);
		pev->nextthink = gpGlobals->time + 5.0;
	}
}

void CItemCTF::ScatterItem(CBasePlayer* pPlayer)
{
	if (pPlayer)
	{
		RemoveEffect(pPlayer);

		pev->origin = pPlayer->pev->origin + Vector(0, 0, (pPlayer->pev->flags & FL_DUCKING) != 0 ? 34 : 16);
	}

	UTIL_SetOrigin(pev, pev->origin);

	pev->flags &= ~FL_ONGROUND;
	pev->movetype = MOVETYPE_TOSS;
	pev->angles = g_vecZero;
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	pev->velocity = g_vecZero;

	pev->velocity = pPlayer->pev->velocity + Vector(RANDOM_FLOAT(0, 1) * 274, RANDOM_FLOAT(0, 1) * 274, 0);

	pev->avelocity.y = 400;

	if (pev->owner)
	{
		auto pOwner = GET_PRIVATE<CBasePlayer>(pev->owner);

		if (pOwner)
		{
			if (pOwner->IsPlayer())
				pOwner->m_iItems = static_cast<CTFItem::CTFItem>(pOwner->m_iItems & ~m_iItemFlag);
		}

		pev->owner = nullptr;
	}

	SetTouch(&CItemCTF::ItemTouch);
	SetThink(&CItemCTF::DropPreThink);
	pev->nextthink = gpGlobals->time + 5.0;

	m_iLastTouched = pPlayer->entindex();
	m_flNextTouchTime = 5.0 + gpGlobals->time;

	UTIL_LogPrintf(
		"\"%s<%i><%u><%s>\" triggered \"drop_%s_Powerup\"\n",
		STRING(pPlayer->pev->netname),
		GETPLAYERUSERID(pPlayer->edict()),
		g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
		GetTeamName(pPlayer->edict()),
		m_pszItemName);
}

void CItemCTF::ThrowItem(CBasePlayer* pPlayer)
{
	if (pPlayer)
	{
		RemoveEffect(pPlayer);

		pev->origin = pPlayer->pev->origin + Vector(0, 0, (pPlayer->pev->flags & FL_DUCKING) != 0 ? 34 : 16);
	}

	UTIL_SetOrigin(pev, pev->origin);

	EMIT_SOUND(edict(), CHAN_VOICE, "ctf/itemthrow.wav", VOL_NORM, ATTN_NORM);

	pev->flags &= ~FL_ONGROUND;
	pev->movetype = MOVETYPE_TOSS;
	pev->angles = g_vecZero;
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	pev->velocity = pPlayer->pev->velocity + gpGlobals->v_forward * 274;

	pev->avelocity.y = 400;

	if (pev->owner)
	{
		auto pOwner = GET_PRIVATE<CBasePlayer>(pev->owner);

		if (pOwner)
		{
			if (pOwner->IsPlayer())
				pOwner->m_iItems = static_cast<CTFItem::CTFItem>(pOwner->m_iItems & ~m_iItemFlag);
		}

		pev->owner = nullptr;
	}

	SetTouch(&CItemCTF::ItemTouch);
	SetThink(&CItemCTF::DropPreThink);
	pev->nextthink = gpGlobals->time + 5.0;

	m_iLastTouched = pPlayer->entindex();
	m_flNextTouchTime = 5.0 + gpGlobals->time;

	UTIL_LogPrintf(
		"\"%s<%i><%u><%s>\" triggered \"drop_%s_Powerup\"\n",
		STRING(pPlayer->pev->netname),
		GETPLAYERUSERID(pPlayer->edict()),
		g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
		GetTeamName(pPlayer->edict()),
		m_pszItemName);
}
