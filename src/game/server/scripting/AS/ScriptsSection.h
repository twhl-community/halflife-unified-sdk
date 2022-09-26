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

#include "ASScriptingSystem.h"
#include "ScriptingConstants.h"
#include "ScriptPluginList.h"

#include "config/GameConfig.h"

#include "utils/JSONSystem.h"

namespace scripting
{
/**
 *	@brief Allows a configuration file to specify a list of scripts to load.
 */
template <typename Context>
class ScriptsSection final : public GameConfigSection<Context>
{
public:
	explicit ScriptsSection() = default;

	std::string_view GetName() const override final { return "Scripts"; }

	json::value_t GetType() const override final { return json::value_t::object; }

	std::string GetSchema() const override final
	{
		return fmt::format(R"(
"properties": {{
	{}
}}
)",
			GetPluginListFileScriptsSchema());
	}

	bool TryParse(GameConfigContext<Context>& context) const override final
	{
		// This should be the only place where the map config is created.
		return g_Scripting.GetScriptPlugins()->CreatePluginFromConfig(ModuleType::Map, MapPluginName, context.Input);
	}
};
}
