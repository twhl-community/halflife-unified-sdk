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

#include "config/GameConfigLoader.h"
#include "config/GameConfigSection.h"

#include "utils/JSONSystem.h"
#include "utils/ReplacementMaps.h"

/**
*	@brief Allows a configuration file to specify a global sound replacement file.
*/
class GlobalSoundReplacementSection final : public GameConfigSection
{
public:
	explicit GlobalSoundReplacementSection() = default;

	std::string_view GetName() const override final { return "GlobalSoundReplacement"; }

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

	bool TryParse(GameConfigContext& context) const override final
	{
		const auto fileName = context.Input.value("FileName", std::string{});

		if (!fileName.empty())
		{
			context.Loader.GetLogger()->debug("Adding global sound replacement file \"{}\"", fileName);

			auto mapState = std::any_cast<MapState*>(context.UserData);

			if (!mapState->m_GlobalSoundReplacement->empty())
			{
				context.Loader.GetLogger()->error("Only one global sound replacement file may be specified");
				return false;
			}

			mapState->m_GlobalSoundReplacementFileName.assign(fileName.data(), fileName.size());

			if (fileName.size() > (MaxUserMessageLength - 1))
			{
				context.Loader.GetLogger()->error(
					"Global sound replacement file name must be no longer than {} bytes (prefer ASCII characters)",
					MaxUserMessageLength - 1);
				return false;
			}

			mapState->m_GlobalSoundReplacement = g_ReplacementMaps.Load(fileName, {.ConvertToLowercase = true});
		}

		return true;
	}
};
