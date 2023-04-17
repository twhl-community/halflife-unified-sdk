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

#include <any>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <EASTL/fixed_vector.h>

#include "cbase.h"
#include "heterogeneous_lookup.h"

class CBasePlayer;

struct PersistentWeaponState
{
	std::unordered_map<std::string, std::any, TransparentStringHash, TransparentEqual> Properties;
};

/**
 *	@brief Represents a player's inventory.
 *	Can be copied to and from players.
 */
class PlayerInventory
{
private:
	struct WeaponData
	{
		std::string ClassName;

		std::optional<int> DefaultAmmo;
		std::optional<PersistentWeaponState> PersistentState;
	};

	struct AmmoValue
	{
		// Stored as string to avoid tying to map-specific indices.
		std::string Name;
		int Count{};
	};

public:
	void AddWeapon(std::string className, std::optional<int> defaultAmmo = std::nullopt);

	int GetAmmoCount(std::string_view name, int defaultValue = 0) const;

	void SetAmmoCount(std::string_view name, int count);

	static PlayerInventory CreateFromPlayer(CBasePlayer* player);

	void ApplyToPlayer(CBasePlayer* player) const;

private:
	const AmmoValue* FindAmmoValue(std::string_view name) const;

	AmmoValue* FindOrCreateAmmoValue(std::string_view name);

public:
	std::optional<bool> HasSuit;
	std::optional<bool> HasLongJump;

	std::optional<int> Health;
	std::optional<int> Armor;

	std::string WeaponToSelect;

private:
	eastl::fixed_vector<WeaponData, 16> m_WeaponData;
	eastl::fixed_vector<AmmoValue, MAX_AMMO_TYPES> m_AmmoValues;
};
