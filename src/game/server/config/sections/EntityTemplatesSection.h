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
 *	@brief Allows a configuration file to specify a set of templates to apply to entities of a specific class.
 */
class EntityTemplatesSection final : public GameConfigSection<ServerConfigContext>
{
public:
	explicit EntityTemplatesSection() = default;

	std::string_view GetName() const override final { return "EntityTemplates"; }

	json::value_t GetType() const override final { return json::value_t::object; }

	std::string GetSchema() const override final
	{
		return R"|(
"properties": {
	"^.+$": {
		"title": "Template FileName",
		"type": "string"
	}
})|";
	}

	bool TryParse(GameConfigContext<ServerConfigContext>& context) const override final
	{
		EntityTemplateMap templateMap;

		for (const auto& [key, value] : context.Input.items())
		{
			templateMap.insert_or_assign(key, value.get<std::string>());
		}

		templateMap.insert(context.Data.EntityTemplates.begin(), context.Data.EntityTemplates.end());

		context.Data.EntityTemplates = std::move(templateMap);

		return true;
	}
};
