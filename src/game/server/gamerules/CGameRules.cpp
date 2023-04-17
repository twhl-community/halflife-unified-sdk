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

#include <algorithm>
#include <string>
#include <vector>

#include "cbase.h"
#include "GameLibrary.h"
#include "CGameRules.h"
#include "CHalfLifeCoopplay.h"
#include "CHalfLifeCTFplay.h"
#include "CHalfLifeMultiplay.h"
#include "CHalfLifeRules.h"
#include "CHalfLifeTeamplay.h"
#include "PersistentInventorySystem.h"
#include "SpawnInventorySystem.h"
#include "spawnpoints.h"
#include "UserMessages.h"
#include "items/weapons/AmmoTypeSystem.h"

void GameRulesCanHaveItemVisitor::Visit(CBasePlayerWeapon* weapon)
{
	// only living players can have items
	if (Player->pev->deadflag != DEAD_NO)
	{
		CanHaveItem = false;
		return;
	}

	if (weapon->pszAmmo1())
	{
		if (!GameRules->CanHaveAmmo(Player, weapon->pszAmmo1()))
		{
			// we can't carry anymore ammo for this gun. We can only
			// have the gun if we aren't already carrying one of this type
			if (Player->HasPlayerWeapon(weapon))
			{
				CanHaveItem = false;
				return;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if (Player->HasPlayerWeapon(weapon))
		{
			CanHaveItem = false;
			return;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
}

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

void CGameRules::SetupPlayerInventory(CBasePlayer* player)
{
	// Originally game_player_equip entities were triggered in PlayerSpawn to set up the player's inventory.
	// This is now handled by naming them game_playerspawn (see CBasePlayer::UpdateClientData).
	// Handling it there avoids edge cases where this function is called during ClientPutInServer.
	// It is not possible to send messages to clients during that function so ammo change messages are ignored.

	if (!g_PersistentInventory.TryApplyToPlayer(player))
	{
		g_SpawnInventory.GetInventory()->ApplyToPlayer(player);
	}
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

void CGameRules::PlayerSpawn(CBasePlayer* pPlayer)
{
	SetupPlayerInventory(pPlayer);
	pPlayer->m_FireSpawnTarget = true;
}

CBaseEntity* CGameRules::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	CBaseEntity* pSpawnSpot = EntSelectSpawnPoint(pPlayer);

	pPlayer->pev->origin = pSpawnSpot->pev->origin + Vector(0, 0, 1);
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = pSpawnSpot->pev->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = FIXANGLE_ABSOLUTE;

	return pSpawnSpot;
}

bool CGameRules::CanHaveItem(CBasePlayer* player, CBaseItem* item)
{
	GameRulesCanHaveItemVisitor visitor{this, player};
	item->Accept(visitor);
	return visitor.CanHaveItem;
}

void CGameRules::BecomeSpectator(CBasePlayer* player, const CommandArgs& args)
{
	// Default implementation: applies to all game modes, even singleplayer.

	// always allow proxies to become a spectator
	if ((player->pev->flags & FL_PROXY) != 0 || allow_spectators.value != 0)
	{
		CBaseEntity* pSpawnSpot = GetPlayerSpawnSpot(player);
		player->StartObserver(player->pev->origin, pSpawnSpot->pev->angles);

		// notify other clients of player switching to spectator mode
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s switched to spectator mode\n",
												 (!FStringNull(player->pev->netname) && STRING(player->pev->netname)[0] != 0) ? STRING(player->pev->netname) : "unconnected"));
	}
	else
		ClientPrint(player, HUD_PRINTCONSOLE, "Spectator mode is disabled.\n");
}

template <typename TGameRules>
CGameRules* CreateGameRules()
{
	return new TGameRules();
}

using GameRulesFactory = CGameRules* (*)();

using GameRulesEntry = std::pair<const char*, GameRulesFactory>;

template <typename TGameRules>
GameRulesEntry CreateGameRulesEntry()
{
	return {TGameRules::GameModeName, &CreateGameRules<TGameRules>};
}

// Map of all multiplayer gamerules.
static const std::vector<GameRulesEntry> GameRulesList{
	CreateGameRulesEntry<CHalfLifeMultiplay>(),
	CreateGameRulesEntry<CHalfLifeTeamplay>(),
	CreateGameRulesEntry<CHalfLifeCTFplay>(),
	CreateGameRulesEntry<CHalfLifeCoopplay>()};

CGameRules* InstallGameRules(std::string_view gameModeName)
{
	CGameRules::Logger->trace("Creating gamerules");

	if (gpGlobals->maxClients == 1)
	{
		if (!gameModeName.empty())
		{
			CGameRules::Logger->info("Ignoring gamemode setting {} in singleplayer", gameModeName);
		}

		CGameRules::Logger->trace("Creating singleplayer gamerules");
		// generic half-life
		return CreateGameRules<CHalfLifeRules>();
	}

	if (!gameModeName.empty())
	{
		if (auto it = std::find_if(GameRulesList.begin(), GameRulesList.end(), [&](const auto& candidate)
				{ return candidate.first == gameModeName; });
			it != GameRulesList.end())
		{
			CGameRules::Logger->trace("Creating {} gamerules", gameModeName);
			return it->second();
		}

		CGameRules::Logger->error("Couldn't create {} gamerules, falling back to deathmatch game mode", gameModeName);
	}
	else
	{
		CGameRules::Logger->trace("Autodetected deathmatch game mode");
	}

	CGameRules::Logger->trace("Creating deathmatch gamerules");

	return CreateGameRules<CHalfLifeMultiplay>();
}

const char* GameModeIndexToString(int index)
{
	// Autodetect game mode.
	if (index == 0)
	{
		return "";
	}

	--index;

	if (index < 0 || static_cast<std::size_t>(index) >= GameRulesList.size())
	{
		CGameRules::Logger->error("Invalid mp_createserver_gamemode value");
		return "";
	}

	return GameRulesList[index].first;
}

void PrintMultiplayerGameModes()
{
	Con_Printf("Singleplayer always uses singleplayer gamerules\n");
	Con_Printf("Set mp_gamemode to \"autodetect\" to autodetect the game mode\n");

	Con_Printf("%s (not an option for mp_gamemode)\n", CHalfLifeRules::GameModeName);

	for (const auto& gameMode : GameRulesList)
	{
		Con_Printf("%s\n", gameMode.first);
	}
}
