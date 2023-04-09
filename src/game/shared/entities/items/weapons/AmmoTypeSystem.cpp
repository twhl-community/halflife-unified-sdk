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

#include "AmmoTypeSystem.h"

bool AmmoTypeSystem::Initialize()
{
	g_NetworkData.RegisterHandler("AmmoTypes", this);
	return true;
}

void AmmoTypeSystem::HandleNetworkDataBlock(NetworkDataBlock& block)
{
	if (block.Mode == NetworkDataMode::Serialize)
	{
		block.Data = json::array();

		for (const auto& type : m_AmmoTypes)
		{
			json data = json::object();

			data.emplace("Name", type.Name.c_str());
			data.emplace("MaximumCapacity", type.MaximumCapacity);

			if (!type.WeaponName.empty())
			{
				data.emplace("WeaponName", type.WeaponName.c_str());
			}

			block.Data.push_back(std::move(data));
		}
	}
	else
	{
		Clear();

		for (const auto& data : block.Data)
		{
			const auto name = data.value("Name", "");
			const int maximumCapacity = data.value("MaximumCapacity", -2);
			const auto weaponName = data.value("WeaponName", "");

			if (Register(name, maximumCapacity, weaponName) == -1)
			{
				block.ErrorMessage = "Invalid ammo type received from server";
				return;
			}
		}
	}
}

int AmmoTypeSystem::IndexOf(std::string_view name) const
{
	for (std::size_t i = 0; i < m_AmmoTypes.size(); ++i)
	{
		const auto& type = m_AmmoTypes[i];

		if (eastl::string::comparei(type.Name.data(), type.Name.data() + type.Name.size(), name.data(), name.data() + name.size()) == 0)
		{
			return static_cast<int>(i + 1);
		}
	}

	return -1;
}

const AmmoType* AmmoTypeSystem::GetByIndex(int index) const
{
	--index;

	if (index >= 0 && static_cast<std::size_t>(index) < m_AmmoTypes.size())
	{
		return &m_AmmoTypes[index];
	}

	return nullptr;
}

const AmmoType* AmmoTypeSystem::GetByName(std::string_view name) const
{
	return GetByIndex(IndexOf(name));
}

int AmmoTypeSystem::GetMaxCapacity(std::string_view name) const
{
	if (auto type = GetByName(name); type)
	{
		return type->MaximumCapacity;
	}

	CBasePlayerWeapon::WeaponsLogger->error("GetMaxCapacity() doesn't recognize '{}'!", name);

	return -1;
}

void AmmoTypeSystem::Clear()
{
	m_AmmoTypes.clear();
}

int AmmoTypeSystem::Register(std::string_view name, int maximumCapacity, std::string_view weaponName)
{
	if (const int index = IndexOf(name); index != -1)
	{
		const auto type = GetByIndex(index);

		if (eastl::string::comparei(
				type->WeaponName.data(), type->WeaponName.data() + type->WeaponName.size(),
				weaponName.data(), weaponName.data() + weaponName.size()) != 0)
		{
			CBasePlayerWeapon::WeaponsLogger->warn(
				"Attempting to register duplicate ammo type \"{}\" with differing properties");
		}

		return index;
	}

	// You can raise the limit, but then you'll have to figure out
	// how to send all of the ammo values to the client for prediction.
	if (m_AmmoTypes.size() >= m_AmmoTypes.kMaxSize)
	{
		assert(!"Too many ammo types");
		CBasePlayerWeapon::WeaponsLogger->error("Attempting to register too many ammo types (max {})", MAX_AMMO_TYPES);
		return -1;
	}

	if (name != Trim(name) || std::find_if_not(name.begin(), name.end(), [](auto& c)
								  { return std::isalnum(c) || std::isspace(c) || c == '_'; }) != name.end())
	{
		assert(!"Invalid ammo type name");
		CBasePlayerWeapon::WeaponsLogger->error("Attempting to register ammo type with invalid name \"{}\"", name);
		return -1;
	}

	if (maximumCapacity < 0)
	{
		assert(!"Invalid ammo type capacity");
		CBasePlayerWeapon::WeaponsLogger->error("Attempting to register ammo type \"{}\" with invalid maximum capacity {}", name, maximumCapacity);
		return -1;
	}

	if (!weaponName.empty() && std::find_if_not(weaponName.begin(), weaponName.end(), [](auto& c)
								   { return std::isalnum(c) || c == '_'; }) != weaponName.end())
	{
		assert(!"Invalid ammo type weapon name");
		CBasePlayerWeapon::WeaponsLogger->error("Attempting to register ammo type \"{}\" with invalid weapon name \"{}\"", name, weaponName);
		return -1;
	}

	CBasePlayerWeapon::WeaponsLogger->trace("Registering ammo type \"{}\"", name);

	auto& type = m_AmmoTypes.push_back();
	type.Id = static_cast<int>(m_AmmoTypes.size());
	type.Name.assign(name.data(), name.size());
	type.MaximumCapacity = maximumCapacity;
	type.WeaponName.assign(weaponName.data(), weaponName.size());

	if (!weaponName.empty())
	{
		CBasePlayerWeapon::WeaponsLogger->trace("Associated with weapon \"{}\"", weaponName);
	}

	return type.Id;
}
