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

#include <string>

#include "cdll_dll.h"
#include "ServerConfigContext.h"

#include "config/GameConfig.h"

#include "utils/JSONSystem.h"
#include "utils/shared_utils.h"

/**
 *	@brief Allows a configuration file to specify the player's inventory on spawn.
 */
class SpawnInventorySection final : public GameConfigSection<ServerConfigContext>
{
public:
	explicit SpawnInventorySection() = default;

	std::string_view GetName() const override final { return "SpawnInventory"; }

	json::value_t GetType() const override final { return json::value_t::object; }

	std::string GetSchema() const override final
	{
		return R"|(
"properties": {
	"Reset": {
		"title": "Reset Spawn Inventory",
		"description": "Reset Spawn Inventory configuration to its default empty state",
		"type": "boolean"
	},
	"HasSuit": {
		"title": "Player has HEV Suit",
		"type": "boolean"
	},
	"HasLongJump": {
		"title": "Player has Long Jump Module",
		"type": "boolean"
	},
	"Health": {
		"title": "Player Health",
		"type": "integer",
		"minimum": 1
	},
	"Armor": {
		"title": "Player Armor",
		"type": "integer",
		"minimum": 0
	},
	"Weapons": {
		"title": "Weapons to give to the player",
		"type": "object",
		"patternProperties": {
			"^.*$": {
				"title": "Weapon",
				"type": "object",
				"properties": {
					"DefaultAmmo": {
						"title": "Default Ammo In Weapon",
						"description": "-1 == Maximum",
						"type": "integer",
						"minimum": -1
					}
				}
			}
		}
	},
	"Ammo": {
		"title": "Ammo to give to the player",
		"type": "object",
		"patternProperties": {
			"^.*$": {
				"title": "Ammo",
				"description": "Amount of ammo to give (-1 == Maximum)",
				"type": "integer",
				"minimum": -1
			}
		}
	},
	"WeaponToSelect": {
		"title": "Weapon to select if player has no active weapon",
		"type": "string"
	}
})|";
	}

	bool TryParse(GameConfigContext<ServerConfigContext>& context) const override final
	{
		PlayerInventory& inventory = context.Data.SpawnInventory;

		if (context.Input.value("Reset", false))
		{
			inventory = {};
		}

		if (auto it = context.Input.find("HasSuit"); it != context.Input.end())
		{
			inventory.HasSuit = it->get<bool>();
		}

		if (auto it = context.Input.find("HasLongJump"); it != context.Input.end())
		{
			inventory.HasLongJump = it->get<bool>();
		}

		if (auto it = context.Input.find("Health"); it != context.Input.end())
		{
			inventory.Health = it->get<int>();
		}

		if (auto it = context.Input.find("Armor"); it != context.Input.end())
		{
			inventory.Armor = it->get<int>();
		}

		if (auto weapons = context.Input.find("Weapons"); weapons != context.Input.end())
		{
			for (const auto& [className, props] : weapons->items())
			{
				std::optional<int> defaultAmmo;

				// TODO: need to clamp this value at some point
				if (auto it = props.find("DefaultAmmo"); it != props.end())
				{
					defaultAmmo = it->get<int>();
				}

				inventory.AddWeapon(className, defaultAmmo);
			}
		}

		if (auto ammo = context.Input.find("Ammo"); ammo != context.Input.end())
		{
			for (const auto& [name, count] : ammo->items())
			{
				inventory.SetAmmoCount(name, count.get<int>());
			}
		}

		if (auto it = context.Input.find("WeaponToSelect"); it != context.Input.end())
		{
			inventory.WeaponToSelect = it->get<std::string>();
		}

		return true;
	}
};
