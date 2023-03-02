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

#include "MapState.h"
#include "palette.h"

#include "config/GameConfig.h"

#include "utils/JSONSystem.h"
#include "utils/shared_utils.h"

/**
 *	@brief Allows a configuration file to specify the player's hud color.
 */
class HudColorSection final : public GameConfigSection<MapState>
{
public:
	explicit HudColorSection() = default;

	std::string_view GetName() const override final { return "HudColor"; }

	json::value_t GetType() const override final { return json::value_t::string; }

	std::string GetSchema() const override final
	{
		return fmt::format(R"("pattern": "^\\d\\d?\\d? \\d\\d?\\d? \\d\\d?\\d?$")");
	}

	bool TryParse(GameConfigContext<MapState>& context) const override final
	{
		const auto color = context.Input.get<std::string>();

		if (!color.empty())
		{
			Vector colorValue{255, 255, 255};

			UTIL_StringToVector(colorValue, color);

			context.Data.m_HudColor = {
				static_cast<std::uint8_t>(colorValue.x),
				static_cast<std::uint8_t>(colorValue.y),
				static_cast<std::uint8_t>(colorValue.z)};
		}

		return true;
	}
};
