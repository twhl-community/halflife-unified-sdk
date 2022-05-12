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

#include "cbase.h"
#include "json_utils.h"
#include "ModelReplacement.h"

using namespace std::literals;

constexpr std::string_view ModelReplacementSchemaName{"ModelReplacement"sv};

static std::string GetModelReplacementSchema()
{
	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Model Replacement File",
	"type": "object",
	"patternProperties": {{
		".*": {{
			"type": "string"
		}}
	}}
}}
)");
}

bool ModelReplacementSystem::Initialize()
{
	g_JSON.RegisterSchema(ModelReplacementSchemaName, &GetModelReplacementSchema);

	return true;
}

static ModelReplacementMap ParseModelReplacement(const json& input)
{
	ModelReplacementMap map;

	if (input.is_object())
	{
		map.reserve(input.size());

		for (const auto& kv : input.items())
		{
			map.emplace(kv.key(), kv.value().get<std::string>());
		}
	}

	return map;
}

ModelReplacementMap ModelReplacementSystem::LoadReplacementMap(const std::string& fileName) const
{
	return g_JSON.ParseJSONFile(
					 fileName.c_str(),
					 {.SchemaName = ModelReplacementSchemaName, .PathID = "GAMECONFIG"},
					 [this](const json& input)
					 { return ParseModelReplacement(input); })
		.value_or(ModelReplacementMap{});
}
