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

#pragma once

#include <array>
#include <string>

#include "cbase.h"
#include "PlayerInventory.h"

class CBasePlayer;

class PersistentInventorySystem final
{
private:
	struct PersistentPlayerInventory
	{
		std::string PlayerId;
		PlayerInventory Inventory;
	};

public:
	void NewMapStarted();

	void InitializeFromPlayers();

	bool TryApplyToPlayer(CBasePlayer* player);

private:
	static std::string GetPlayerId(CBasePlayer* player);

private:
	std::array<PersistentPlayerInventory, MAX_PLAYERS> m_Inventories;

	bool m_InitializedInventories = false;
	int m_ExpectedSpawnCount = 0;

	float m_GracePeriodEndTime = 0;
};

inline PersistentInventorySystem g_PersistentInventory;
