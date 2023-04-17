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
#include "AmmoTypeSystem.h"
#include "PlayerInventory.h"

void PlayerInventory::AddWeapon(std::string className, std::optional<int> defaultAmmo)
{
	m_WeaponData.emplace_back(std::move(className), defaultAmmo);
}

int PlayerInventory::GetAmmoCount(std::string_view name, int defaultValue) const
{
	if (auto value = FindAmmoValue(name); value)
	{
		return value->Count;
	}

	return defaultValue;
}

void PlayerInventory::SetAmmoCount(std::string_view name, int count)
{
	auto value = FindOrCreateAmmoValue(name);
	value->Count = count;
}

const PlayerInventory::AmmoValue* PlayerInventory::FindAmmoValue(std::string_view name) const
{
	if (auto it = std::find_if(m_AmmoValues.begin(), m_AmmoValues.end(), [&](const auto& value)
			{ return value.Name == name; });
		it != m_AmmoValues.end())
	{
		return &(*it);
	}

	return nullptr;
}

PlayerInventory::AmmoValue* PlayerInventory::FindOrCreateAmmoValue(std::string_view name)
{
	if (auto value = FindAmmoValue(name); value)
	{
		return const_cast<AmmoValue*>(value);
	}

	return &m_AmmoValues.emplace_back(std::string{name});
}

PlayerInventory PlayerInventory::CreateFromPlayer(CBasePlayer* player)
{
	assert(player);

	PlayerInventory inventory;

	inventory.HasSuit = player->HasSuit();
	inventory.HasLongJump = player->HasLongJump();
	inventory.Health = player->pev->health;
	inventory.Armor = player->pev->armorvalue;

	for (int i = 0; i < g_AmmoTypes.GetCount(); ++i)
	{
		auto ammoType = g_AmmoTypes.GetByIndex(i + 1);

		inventory.SetAmmoCount(ammoType->Name.c_str(), player->AmmoInventory(ammoType->Id));
	}

	for (int slot = 0; slot < MAX_WEAPON_SLOTS; ++slot)
	{
		for (auto weapon = player->m_rgpPlayerWeapons[slot]; weapon; weapon = weapon->m_pNext)
		{
			auto& data = inventory.m_WeaponData.emplace_back(weapon->GetClassname(), 0);

			PersistentWeaponState persistentState;
			weapon->SavePersistentState(persistentState);
			data.PersistentState = std::move(persistentState);
		}
	}

	if (player->m_pActiveWeapon)
	{
		inventory.WeaponToSelect = player->m_pActiveWeapon->GetClassname();
	}

	return inventory;
}

void PlayerInventory::ApplyToPlayer(CBasePlayer* player) const
{
	assert(player);

	if (HasSuit)
	{
		player->SetHasSuit(*HasSuit);
	}

	if (HasLongJump)
	{
		player->SetHasLongJump(*HasLongJump);
	}

	if (Health)
	{
		player->pev->health = std::clamp(float(*Health), 1.f, player->pev->max_health);
	}

	if (Armor)
	{
		player->pev->armorvalue = std::clamp(*Armor, 0, MAX_NORMAL_BATTERY);
	}

	const bool hasActiveWeapon = player->m_pActiveWeapon != nullptr;

	// Set ammo first so weapons can reload themselves if needed.
	for (const auto& ammoValue : m_AmmoValues)
	{
		player->SetAmmoCount(ammoValue.Name.c_str(), ammoValue.Count);
	}

	for (const auto& itemData : m_WeaponData)
	{
		auto item = player->GiveNamedItem(itemData.ClassName, itemData.DefaultAmmo);

		if (!itemData.PersistentState)
		{
			continue;
		}

		auto weapon = dynamic_cast<CBasePlayerWeapon*>(item);

		if (!weapon)
		{
			continue;
		}

		weapon->LoadPersistentState(*itemData.PersistentState);
	}

	if (!hasActiveWeapon && !WeaponToSelect.empty())
	{
		player->SelectItem(WeaponToSelect.c_str());
	}
}
