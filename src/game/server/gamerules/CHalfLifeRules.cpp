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
#include "CHalfLifeRules.h"
#include "items.h"
#include "UserMessages.h"
#include "items/CBaseItem.h"

CHalfLifeRules::CHalfLifeRules()
{
	// Define this as a dummy command to silence console errors.
	m_VModEnableCommand = g_ClientCommands.CreateScoped("vmodenable", [](auto, const auto&) {});
}

void CHalfLifeRules::Think()
{
}

bool CHalfLifeRules::FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
	if (!pPlayer->m_pActiveWeapon)
	{
		// player doesn't have an active item!
		return true;
	}

	if (!pPlayer->m_pActiveWeapon->CanHolster())
	{
		return false;
	}

	return true;
}

bool CHalfLifeRules::GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon, bool alwaysSearch)
{
	// If this is an exhaustible weapon and it's out of ammo, always try to switch even in singleplayer.
	if (alwaysSearch || ((pCurrentWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE) != 0 && pCurrentWeapon->PrimaryAmmoIndex() != -1 && pPlayer->m_rgAmmo[pCurrentWeapon->PrimaryAmmoIndex()] == 0))
	{
		return CGameRules::GetNextBestWeapon(pPlayer, pCurrentWeapon);
	}

	return false;
}

bool CHalfLifeRules::ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	return true;
}

void CHalfLifeRules::InitHUD(CBasePlayer* pl)
{
}

void CHalfLifeRules::ClientDisconnected(edict_t* pClient)
{
}

float CHalfLifeRules::FlPlayerFallDamage(CBasePlayer* pPlayer)
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

void CHalfLifeRules::PlayerSpawn(CBasePlayer* pPlayer)
{
}

bool CHalfLifeRules::AllowAutoTargetCrosshair()
{
	return (g_Skill.GetSkillLevel() == SkillLevel::Easy);
}

void CHalfLifeRules::PlayerThink(CBasePlayer* pPlayer)
{
}

bool CHalfLifeRules::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return true;
}

float CHalfLifeRules::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return gpGlobals->time; // now!
}

int CHalfLifeRules::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	return 1;
}

void CHalfLifeRules::PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor)
{
}

void CHalfLifeRules::DeathNotice(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor)
{
}

bool CHalfLifeRules::CanHaveItem(CBasePlayer* player, CBaseItem* item)
{
	return true;
}

void CHalfLifeRules::PlayerGotItem(CBasePlayer* player, CBaseItem* item)
{
}

bool CHalfLifeRules::ItemShouldRespawn(CBaseItem* item)
{
	// Items don't respawn in singleplayer by default
	return item->m_RespawnDelay >= 0;
}

float CHalfLifeRules::ItemRespawnTime(CBaseItem* item)
{
	if (item->m_RespawnDelay == ITEM_NEVER_RESPAWN_DELAY)
	{
		return -1;
	}

	// Allow respawn if it has a custom delay
	if (item->m_RespawnDelay >= ITEM_DEFAULT_RESPAWN_DELAY)
	{
		return gpGlobals->time + item->m_RespawnDelay;
	}

	// No respawn
	return -1;
}

Vector CHalfLifeRules::ItemRespawnSpot(CBaseItem* item)
{
	return item->pev->origin;
}

float CHalfLifeRules::ItemTryRespawn(CBaseItem* item)
{
	// If it has a custom delay then it can spawn when it first checks, otherwise never respawn
	if (item->m_RespawnDelay >= ITEM_DEFAULT_RESPAWN_DELAY)
	{
		return 0;
	}

	return -1;
}

bool CHalfLifeRules::IsAllowedToSpawn(CBaseEntity* pEntity)
{
	return true;
}

int CHalfLifeRules::DeadPlayerWeapons(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_GUN_NO;
}

int CHalfLifeRules::DeadPlayerAmmo(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_AMMO_NO;
}

int CHalfLifeRules::PlayerRelationship(CBasePlayer* pPlayer, CBaseEntity* pTarget)
{
	// why would a single player in half life need this?
	return GR_NOTTEAMMATE;
}

bool CHalfLifeRules::FAllowMonsters()
{
	return true;
}
