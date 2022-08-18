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
	m_Logger = g_Logging.CreateLogger("replacementmap");

	g_JSON.RegisterSchema(ModelReplacementSchemaName, &GetModelReplacementSchema);

	return true;
}

void ModelReplacementSystem::Shutdown()
{
	m_Logger.reset();
}

ModelReplacementMap ModelReplacementSystem::ParseModelReplacement(const json& input) const
{
	ModelReplacementMap map;

	if (input.is_object())
	{
		map.reserve(input.size());

		for (const auto& kv : input.items())
		{
			std::string key = kv.key();
			auto value = kv.value().get<std::string>();

			ToLower(key);
			ToLower(value);

			if (const auto result = map.emplace(std::move(key), value); !result.second)
			{
				if (value != result.first->second)
				{
					m_Logger->warn("Ignoring duplicate replacement \"{}\" => \"{}\" (existing replacement is \"{}\")",
						kv.key(), value, result.first->second);
				}
				else
				{
					m_Logger->debug("Ignoring duplicate replacement \"{}\"", kv.key());
				}
			}
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
