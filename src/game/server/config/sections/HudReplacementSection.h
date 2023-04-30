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

#include "ServerConfigContext.h"

#include "config/GameConfig.h"

#include "utils/JSONSystem.h"
#include "utils/shared_utils.h"

/**
 *	@brief Allows a configuration file to specify the <tt>hud.json</tt> file to use.
 */
class HudReplacementSection final : public GameConfigSection<ServerConfigContext>
{
public:
	explicit HudReplacementSection() = default;

	std::string_view GetName() const override final { return "HudReplacement"; }

	json::value_t GetType() const override final { return json::value_t::object; }

	std::string GetSchema() const override final
	{
		return R"(
"properties": {
	"HudReplacementFile": {
		"title": "Hud Replacement File",
		"type": "string",
		"pattern": "^.+$"
	},
	"Weapons": {
		"type": "object",
		"properties": {
			"^.+$": {
				"title": "Weapon Hud Config Filename",
				"type": "string",
				"pattern": "^.+$"
			}
		}
	}
})";
	}

	bool TryParse(GameConfigContext<ServerConfigContext>& context) const override final
	{
		if (auto it = context.Input.find("HudReplacementFile"); it != context.Input.end())
		{
			context.Data.HudReplacementFile = it->get<std::string>();
		}

		if (auto it = context.Input.find("Weapons"); it != context.Input.end())
		{
			for (const auto& [key, value] : it->items())
			{
				context.Data.WeaponHudReplacementFiles.insert_or_assign(key, value.get<std::string>());
			}
		}

		return true;
	}
};
