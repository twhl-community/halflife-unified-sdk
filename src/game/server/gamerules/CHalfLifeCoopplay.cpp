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
#include "CHalfLifeCoopplay.h"
#include "items.h"
#include "UserMessages.h"
#include "items/CBaseItem.h"

CHalfLifeCoopplay::CHalfLifeCoopplay()
{
	m_MenuSelectCommand = g_ClientCommands.CreateScoped("menuselect", [](CBasePlayer* player, const auto& args)
		{
			if (CMD_ARGC() < 2)
				return;

			int slot = atoi(CMD_ARGV(1));

			// select the item from the current menu
		});
}

void CHalfLifeCoopplay::UpdateGameMode(CBasePlayer* pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, nullptr, pPlayer);
	g_engfuncs.pfnWriteByte(1);
	g_engfuncs.pfnMessageEnd();
}

void CHalfLifeCoopplay::MonsterKilled(CBaseMonster* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor)
{
	auto killer = ToBasePlayer(pKiller);

	if (killer && !pVictim->IsPlayer())
	{
		const int points = IPointsForMonsterKill(killer, pVictim);

		killer->pev->frags += points;

		killer->SendScoreInfoAll();
	}
}

int CHalfLifeCoopplay::PlayerRelationship(CBasePlayer* pPlayer, CBaseEntity* pTarget)
{
	return GR_TEAMMATE;
}

bool CHalfLifeCoopplay::ShouldAutoAim(CBasePlayer* pPlayer, CBaseEntity* target)
{
	if (target && target->IsPlayer())
	{
		return PlayerRelationship(pPlayer, target) != GR_TEAMMATE;
	}

	return true;
}

int CHalfLifeCoopplay::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	if (!pKilled)
	{
		return 0;
	}

	if (pAttacker && pAttacker != pKilled && PlayerRelationship(pAttacker, pKilled) == GR_TEAMMATE)
	{
		return -1;
	}

	return 1;
}

int CHalfLifeCoopplay::IPointsForMonsterKill(CBasePlayer* pAttacker, CBaseMonster* pKilled)
{
	return pKilled != nullptr ? 1 : 0;
}

float CHalfLifeCoopplay::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return gpGlobals->time;
}

int CHalfLifeCoopplay::DeadPlayerWeapons(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_GUN_NO;
}

void CHalfLifeCoopplay::PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor)
{
	if (!m_DisableDeathPenalty)
		CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, inflictor);
}

void CHalfLifeCoopplay::Think()
{
	if (g_fGameOver)
		CHalfLifeMultiplay::Think();
}

bool CHalfLifeCoopplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	if (friendlyfire.value == 0 && pAttacker->IsPlayer() && pAttacker != pPlayer)
		return false;

	return CHalfLifeMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker);
}
