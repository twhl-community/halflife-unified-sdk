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

#include <EASTL/fixed_string.h>
#include <EASTL/fixed_vector.h>

#include "cbase.h"
#include "networking/NetworkDataSystem.h"
#include "utils/GameSystem.h"

struct AmmoType
{
	int Id{0};
	eastl::fixed_string<char, 32> Name;

	int MaximumCapacity{0};

	/**
	 *	@brief For exhaustible weapons. If provided, and the player does not have this weapon in their inventory yet it will be given to them.
	 */
	eastl::fixed_string<char, 32> WeaponName;
};

/**
 *	@brief Manages the ammo type definitions.
 *	@details Ammo type id 0 is the invalid/none index.
 */
class AmmoTypeSystem final : public IGameSystem, public INetworkDataBlockHandler
{
public:
	const char* GetName() const override { return "AmmoTypes"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override {}

	void HandleNetworkDataBlock(NetworkDataBlock& block) override;

	int GetCount() const { return static_cast<std::size_t>(m_AmmoTypes.size()); }

	int IndexOf(std::string_view name) const;

	const AmmoType* GetByIndex(int index) const;

	const AmmoType* GetByName(std::string_view name) const;

	int GetMaxCapacity(std::string_view name) const;

	void Clear();

	/**
	 *	@brief Registers a new ammo type.
	 *	@param name Name of the ammo type.
	 *	@param maximumCapacity The maximum carrying capacity for this ammo type. Must <tt>>= 0</tt>.
	 *	@param weaponName If specified, this is the name of the weapon to give the player when they get ammo of this type.
	 *		Used for exhaustible weapons like hand grenades to ensure the player always has the associated weapon.
	 */
	int Register(std::string_view name, int maximumCapacity, std::string_view weaponName = {});

private:
	// Public indices are 1-based, private ones are 0-based.
	// Store one less than max since index 0 is not used.
	eastl::fixed_vector<AmmoType, MAX_AMMO_TYPES - 1> m_AmmoTypes;
};

inline AmmoTypeSystem g_AmmoTypes;
