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

#include "CMapState.h"
#include "palette.h"

#include "config/GameConfigLoader.h"
#include "config/GameConfigSection.h"

#include "utils/json_utils.h"
#include "utils/shared_utils.h"

/**
*	@brief Global model replacement map.
*/
class HudColorData final : public GameConfigData
{
public:
	explicit HudColorData() = default;

	void Apply(const std::any& userData) const override final
	{
		auto mapState = std::any_cast<CMapState*>(userData);

		mapState->m_HudColor = m_HudColor;
	}

	RGB24 m_HudColor{255, 255, 255};
};

/**
*	@brief Allows a configuration file to specify the player's hud color.
*/
class HudColorSection final : public GameConfigSection
{
public:
	explicit HudColorSection() = default;

	std::string_view GetName() const override final { return "HudColor"; }

	std::tuple<std::string, std::string> GetSchema() const override final
	{
		return {
			fmt::format(R"(
"Color": {{
	"type": "string",
	"pattern": "^\d\d?\d? \d\d?\d? \d\d?\d?$"
}}
)"),
			{"\"Color\""}};
	}

	bool TryParse(GameConfigContext& context) const override final
	{
		using namespace std::literals;
		const auto color = context.Input.value("Color", std::string{});

		if (!color.empty())
		{
			auto data = context.Configuration.GetOrCreate<HudColorData>();

			Vector colorValue{255, 255, 255};

			UTIL_StringToVector(colorValue, color);

			data->m_HudColor =
			{
					static_cast<std::uint8_t>(colorValue.x),
					static_cast<std::uint8_t>(colorValue.y),
					static_cast<std::uint8_t>(colorValue.z)
			};
		}

		return true;
	}
};
