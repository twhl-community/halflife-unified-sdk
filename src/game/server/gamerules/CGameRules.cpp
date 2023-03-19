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
#include "GameLibrary.h"
#include "CGameRules.h"
#include "CHalfLifeCoopplay.h"
#include "CHalfLifeCTFplay.h"
#include "CHalfLifeMultiplay.h"
#include "CHalfLifeRules.h"
#include "CHalfLifeTeamplay.h"
#include "spawnpoints.h"
#include "world.h"
#include "UserMessages.h"
#include "items/weapons/AmmoTypeSystem.h"

CGameRules::CGameRules()
{
	m_SpectateCommand = g_ClientCommands.CreateScoped("spectate", [this](CBasePlayer* player, const auto& args)
		{
			// clients wants to become a spectator
			BecomeSpectator(player, args); });

	m_SpecModeCommand = g_ClientCommands.CreateScoped("specmode", [this](CBasePlayer* player, const auto& args)
		{
			// new spectator mode
			if (player->IsObserver())
				player->Observer_SetMode(atoi(CMD_ARGV(1))); });
}

CBasePlayerWeapon* CGameRules::FindNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon)
{
	if (pCurrentWeapon != nullptr && !pCurrentWeapon->CanHolster())
	{
		// can't put this gun away right now, so can't switch.
		return nullptr;
	}

	const int currentWeight = pCurrentWeapon != nullptr ? pCurrentWeapon->iWeight() : -1;

	CBasePlayerWeapon* pBest = nullptr; // this will be used in the event that we don't find a weapon in the same category.

	int iBestWeight = -1; // no weapon lower than -1 can be autoswitched to

	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		for (auto pCheck = pPlayer->m_rgpPlayerWeapons[i]; pCheck; pCheck = pCheck->m_pNext)
		{
			// don't reselect the weapon we're trying to get rid of
			if (pCheck == pCurrentWeapon)
			{
				continue;
			}

			if (pCheck->iWeight() > -1 && pCheck->iWeight() == currentWeight)
			{
				// this weapon is from the same category.
				if (pCheck->CanDeploy())
				{
					if (pPlayer->SwitchWeapon(pCheck))
					{
						return pCheck;
					}
				}
			}
			else if (pCheck->iWeight() > iBestWeight)
			{
				// Logger->debug("Considering {}", STRING(pCheck->pev->classname));
				//  we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				//  that the player was using. This will end up leaving the player with his heaviest-weighted
				//  weapon.
				if (pCheck->CanDeploy())
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}
		}
	}

	// if we make it here, we've checked all the weapons and found no useable
	// weapon in the same catagory as the current weapon.

	// if pBest is nullptr, we didn't find ANYTHING. Shouldn't be possible- should always
	// at least get the crowbar, but ya never know.

	return pBest;
}

bool CGameRules::GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon, bool alwaysSearch)
{
	if (auto pBest = FindNextBestWeapon(pPlayer, pCurrentWeapon); pBest != nullptr)
	{
		pPlayer->SwitchWeapon(pBest);
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
bool CGameRules::CanHaveAmmo(CBasePlayer* pPlayer, const char* pszAmmoName)
{
	if (pszAmmoName)
	{
		if (const auto type = g_AmmoTypes.GetByName(pszAmmoName); type)
		{
			if (pPlayer->AmmoInventory(type->Id) < type->MaximumCapacity)
			{
				// player has room for more of this type of ammo
				return true;
			}
		}
	}

	return false;
}

//=========================================================
//=========================================================
CBaseEntity* CGameRules::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	CBaseEntity* pSpawnSpot = EntSelectSpawnPoint(pPlayer);

	pPlayer->pev->origin = pSpawnSpot->pev->origin + Vector(0, 0, 1);
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = pSpawnSpot->pev->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = 1;

	return pSpawnSpot;
}

//=========================================================
//=========================================================
bool CGameRules::CanHavePlayerWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
	// only living players can have items
	if (pPlayer->pev->deadflag != DEAD_NO)
		return false;

	if (pWeapon->pszAmmo1())
	{
		if (!CanHaveAmmo(pPlayer, pWeapon->pszAmmo1()))
		{
			// we can't carry anymore ammo for this gun. We can only
			// have the gun if we aren't already carrying one of this type
			if (pPlayer->HasPlayerWeapon(pWeapon))
			{
				return false;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if (pPlayer->HasPlayerWeapon(pWeapon))
		{
			return false;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return true;
}

void CGameRules::BecomeSpectator(CBasePlayer* player, const CommandArgs& args)
{
	// Default implementation: applies to all game modes, even singleplayer.

	// always allow proxies to become a spectator
	if ((player->pev->flags & FL_PROXY) != 0 || allow_spectators.value != 0)
	{
		CBaseEntity* pSpawnSpot = g_pGameRules->GetPlayerSpawnSpot(player);
		player->StartObserver(player->pev->origin, pSpawnSpot->pev->angles);

		// notify other clients of player switching to spectator mode
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s switched to spectator mode\n",
												 (!FStringNull(player->pev->netname) && STRING(player->pev->netname)[0] != 0) ? STRING(player->pev->netname) : "unconnected"));
	}
	else
		ClientPrint(player->pev, HUD_PRINTCONSOLE, "Spectator mode is disabled.\n");
}

//=========================================================
// instantiate the proper game rules object
//=========================================================

CGameRules* InstallGameRules(CBaseEntity* pWorld)
{
	g_GameLogger->trace("Creating gamerules");

	SERVER_COMMAND("exec game.cfg\n");
	SERVER_EXECUTE();

	// TODO: need to be able to identify gamerules by name.
	if (0 == gpGlobals->deathmatch)
	{
		g_GameLogger->trace("Creating singleplayer gamerules");
		// generic half-life
		g_teamplay = false;
		return new CHalfLifeRules;
	}

	if (coopplay.value > 0)
	{
		g_GameLogger->trace("Creating coop gamerules");
		return new CHalfLifeCoopplay();
	}
	else
	{
		if ((pWorld->pev->spawnflags & SF_WORLD_CTF) != 0)
		{
			g_GameLogger->trace("Creating ctf gamerules");
			return new CHalfLifeCTFplay();
		}

		if (teamplay.value > 0)
		{
			g_GameLogger->trace("Creating teamplay gamerules");
			// teamplay

			g_teamplay = true;
			return new CHalfLifeTeamplay;
		}
		if ((int)gpGlobals->deathmatch == 1)
		{
			g_GameLogger->trace("Creating deathmatch gamerules");
			// vanilla deathmatch
			g_teamplay = false;
			return new CHalfLifeMultiplay;
		}
		else
		{
			g_GameLogger->trace("Creating deathmatch gamerules");
			// vanilla deathmatch??
			g_teamplay = false;
			return new CHalfLifeMultiplay;
		}
	}
}
