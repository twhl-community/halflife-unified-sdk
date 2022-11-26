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

#include "config/GameConfig.h"

#include "utils/JSONSystem.h"
#include "utils/ReplacementMaps.h"

/**
 *	@brief Allows a configuration file to specify a global sentence replacement file.
 */
class GlobalSentenceReplacementSection final : public GameConfigSection<MapState>
{
public:
	explicit GlobalSentenceReplacementSection() = default;

	std::string_view GetName() const override final { return "GlobalSentenceReplacement"; }

	std::tuple<std::string, std::string> GetSchema() const override final
	{
		return {
			fmt::format(R"(
"FileName": {{
	"type": "string"
}}
)"),
			{"\"FileName\""}};
	}

	bool TryParse(GameConfigContext<MapState>& context) const override final
	{
		const auto fileName = context.Input.value("FileName", std::string{});

		if (!fileName.empty())
		{
			context.Logger.debug("Adding global sentence replacement file \"{}\"", fileName);

			if (!context.Data.m_GlobalSentenceReplacement->empty())
			{
				context.Logger.error("Only one global sentence replacement file may be specified");
				return false;
			}

			context.Data.m_GlobalSentenceReplacement = g_ReplacementMaps.Load(fileName);
		}

		return true;
	}
};
