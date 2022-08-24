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
//
// coopplay_gamerules.cpp
//
#include "cbase.h"
#include "coopplay_gamerules.h"
#include "items.h"
#include "UserMessages.h"

CHalfLifeCoopplay::CHalfLifeCoopplay()
	: CHalfLifeMultiplay()
{
	m_DisableDeathMessages = false;
	m_DisableDeathPenalty = false;
	g_engfuncs.pfnCVarSetFloat("mp_allowmonsters", 1);

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
	g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgGameMode, nullptr, pPlayer->edict());
	g_engfuncs.pfnWriteByte(1);
	g_engfuncs.pfnMessageEnd();
}

void CHalfLifeCoopplay::MonsterKilled(CBaseMonster* pVictim, entvars_t* pKiller, entvars_t* pInflictor)
{
	auto killer = CBaseEntity::Instance(pKiller);

	if (killer->IsPlayer() && !pVictim->IsPlayer())
	{
		auto killerPlayer = static_cast<CBasePlayer*>(killer);

		const int points = IPointsForMonsterKill(killerPlayer, pVictim);

		killerPlayer->pev->frags += points;

		killerPlayer->SendScoreInfoAll();
	}
}

int CHalfLifeCoopplay::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	return GR_TEAMMATE;
}

bool CHalfLifeCoopplay::ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target)
{
	auto targetEntity = CBaseEntity::Instance(target);

	if (targetEntity)
	{
		if (targetEntity->IsPlayer())
		{
			return PlayerRelationship(pPlayer, targetEntity) != GR_TEAMMATE;
		}
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

float CHalfLifeCoopplay::FlPlayerFallDamage(CBasePlayer* pPlayer)
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

float CHalfLifeCoopplay::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return gpGlobals->time;
}

float CHalfLifeCoopplay::FlHealthChargerRechargeTime()
{
	return coopweprespawn.value > 1 ? 60 : 0;
}

int CHalfLifeCoopplay::DeadPlayerWeapons(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_GUN_NO;
}

void CHalfLifeCoopplay::PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor)
{
	if (!m_DisableDeathPenalty)
		CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);
}

void CHalfLifeCoopplay::Think()
{
	if (g_fGameOver)
		CHalfLifeMultiplay::Think();
}

int CHalfLifeCoopplay::WeaponShouldRespawn(CBasePlayerItem* pWeapon)
{
	if (coopweprespawn.value > 0 && (pWeapon->pev->spawnflags & SF_NORESPAWN) == 0)
		return GR_WEAPON_RESPAWN_YES;

	return GR_WEAPON_RESPAWN_NO;
}

int CHalfLifeCoopplay::ItemShouldRespawn(CItem* pItem)
{
	if (coopweprespawn.value > 0 && (pItem->pev->spawnflags & SF_NORESPAWN) == 0)
		return GR_ITEM_RESPAWN_YES;

	return GR_ITEM_RESPAWN_NO;
}

float CHalfLifeCoopplay::FlItemRespawnTime(CItem* pItem)
{
	if (coopweprespawn.value > 0)
	{
		return gpGlobals->time + ITEM_RESPAWN_TIME;
	}

	return -1;
}

int CHalfLifeCoopplay::AmmoShouldRespawn(CBasePlayerAmmo* pAmmo)
{
	if (coopweprespawn.value > 0 && (pAmmo->pev->spawnflags & SF_NORESPAWN) == 0)
		return GR_AMMO_RESPAWN_YES;

	return GR_AMMO_RESPAWN_NO;
}

float CHalfLifeCoopplay::FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo)
{
	if (coopweprespawn.value > 0)
	{
		return gpGlobals->time + AMMO_RESPAWN_TIME;
	}

	return -1;
}

bool CHalfLifeCoopplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	if (friendlyfire.value == 0 && pAttacker->IsPlayer() && pAttacker != pPlayer)
		return false;

	return CHalfLifeMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker);
}

float CHalfLifeCoopplay::FlWeaponTryRespawn(CBasePlayerItem* pWeapon)
{
	if (coopweprespawn.value > 0 && pWeapon && WEAPON_NONE != pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) != 0)
	{
		if (NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE))
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime(pWeapon);
	}

	return 0;
}

float CHalfLifeCoopplay::FlWeaponRespawnTime(CBasePlayerItem* pWeapon)
{
	if (coopweprespawn.value <= 0)
	{
		return -1;
	}
	else if (weaponstay.value <= 0 || (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) != 0)
	{
		return gpGlobals->time + WEAPON_RESPAWN_TIME;
	}

	return gpGlobals->time;
}
