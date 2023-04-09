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
#include <EASTL/fixed_string.h>

#include "Platform.h"
#include "cdll_dll.h"
#include "networking/NetworkDataSystem.h"
#include "utils/GameSystem.h"

struct WeaponAttackMode
{
	eastl::fixed_string<char, 32> AmmoType;
	int MagazineSize{WEAPON_NOCLIP};
};

struct WeaponInfo
{
	int Id{WEAPON_NONE};
	eastl::fixed_string<char, 32> Name;

	int Slot{0};
	int Position{0};
	int Weight{0}; //!< this value used to determine this weapon's importance in autoselection.
	int Flags{0};

	std::array<WeaponAttackMode, MAX_WEAPON_ATTACK_MODES> AttackModeInfo;
};

/**
 *	@brief Handles the networking of weapon metadata from server to client.
 */
class WeaponDataSystem final : public IGameSystem, public INetworkDataBlockHandler
{
public:
	static inline const WeaponInfo DummyInfo;

	const char* GetName() const override { return "WeaponData"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override {}

	void HandleNetworkDataBlock(NetworkDataBlock& block) override;

	int GetCount() const { return static_cast<std::size_t>(m_WeaponInfos.size()); }

	int IndexOf(std::string_view name) const;

	const WeaponInfo* GetByIndex(int index) const;

	const WeaponInfo* GetByName(std::string_view name) const;

	void Clear();

	/**
	 *	@brief Registers a new weapon.
	 */
	int Register(WeaponInfo&& info);

private:
	// Public indices are 1-based, private ones are 0-based.
	// Store one less than max since index 0 is not used.
	std::array<WeaponInfo, MAX_WEAPONS - 1> m_WeaponInfos;
};

inline WeaponDataSystem g_WeaponData;
