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

#include "cbase.h"
#include "PersistentInventorySystem.h"
#include "ServerLibrary.h"

void PersistentInventorySystem::NewMapStarted()
{
	// Make sure we forget the previous map's inventories if this level change was not initiated by the map itself.
	const bool wasInitialized = m_InitializedInventories;
	m_InitializedInventories = false;

	if (!wasInitialized || !g_pGameRules->IsCoOp() || m_ExpectedSpawnCount != g_Server.GetSpawnCount())
	{
		// Persistent inventory is only used by co-op.
		std::fill(m_Inventories.begin(), m_Inventories.end(), PersistentPlayerInventory{});
		return;
	}
}

void PersistentInventorySystem::InitializeFromPlayers()
{
	if (!g_pGameRules->IsCoOp())
	{
		return;
	}

	m_InitializedInventories = true;

	// Catch the case where a map fails to load and then gets loaded manually so we don't restore inventories.
	m_ExpectedSpawnCount = g_Server.GetSpawnCount() + 1;

	std::fill(m_Inventories.begin(), m_Inventories.end(), PersistentPlayerInventory{});

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto player = UTIL_PlayerByIndex(i);

		if (!player || !player->IsConnected())
		{
			continue;
		}

		auto& inventory = m_Inventories[i - 1];

		inventory.PlayerId = GetPlayerId(player);
		inventory.Inventory = PlayerInventory::CreateFromPlayer(player);
	}
}

bool PersistentInventorySystem::TryApplyToPlayer(CBasePlayer* player)
{
	if (!g_pGameRules->IsCoOp())
	{
		return false;
	}

	const float gracePeriod = g_Skill.GetValue("coop_persistent_inventory_grace_period", 0);

	// Grace period ended, can't restore.
	if (gracePeriod != -1 && gpGlobals->time >= (player->m_ConnectTime + std::max(0.f, gracePeriod)))
	{
		return false;
	}

	const std::string playerId = GetPlayerId(player);

	for (const auto& inventory : m_Inventories)
	{
		if (inventory.PlayerId == playerId)
		{
			inventory.Inventory.ApplyToPlayer(player);
			return true;
		}
	}

	return false;
}

std::string PersistentInventorySystem::GetPlayerId(CBasePlayer* player)
{
	// This gets the Steam Id even if sv_lan is 1.
	// Format is signed 64 bit integer in string form.
	return g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(player->edict()), "*sid");
}
