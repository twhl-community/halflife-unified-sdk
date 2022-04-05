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
//=========================================================
// skill.cpp - code for skill level concerns
//=========================================================

#include <string_view>

#include "cbase.h"
#include "skill.h"

#include "command_utils.h"
#include "json_utils.h"

using namespace std::literals;

constexpr std::string_view SkillConfigSchemaName{"SkillConfig"sv};
constexpr const char* const SkillConfigName = "cfg/skill.json";

//=========================================================
// take the name of a cvar, tack a digit for the skill level
// on, and return the value.of that Cvar
//=========================================================
float GetSkillCvar(const char* pName)
{
	int iCount;
	float flValue;
	char szBuffer[64];

	iCount = sprintf(szBuffer, "%s%d", pName, gSkillData.iSkillLevel);

	flValue = CVAR_GET_FLOAT(szBuffer);

	if (flValue <= 0)
	{
		ALERT(at_console, "\n\n** GetSkillCVar Got a zero for %s **\n\n", szBuffer);
	}

	return flValue;
}

static json GetSkillConfigSchema()
{
	auto schema = fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Skill System Configuration",
	"type": "object",
	"patternProperties": {{
		".*": {{
			"type": "string"
		}}
	}},
	"additionalProperties": false
}}
)");

	return g_JSON.ParseJSONSchema(schema).value_or(json{});
}

bool SkillSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("skill");

	g_JSON.RegisterSchema(SkillConfigSchemaName, &GetSkillConfigSchema);

	g_ConCommands.CreateCommand("reload_skill", [this](const CCommandArgs& args) { LoadSkillConfigFile(); });

	return true;
}

void SkillSystem::Shutdown()
{
	m_Logger.reset();
}

void SkillSystem::LoadSkillConfigFile()
{
	if (const auto result = g_JSON.ParseJSONFile(SkillConfigName,
		{.SchemaName = SkillConfigSchemaName, .PathID = "GAMECONFIG"},
		[this](const json& input){ return ParseConfiguration(input); }); !result.value_or(false))
	{
		m_Logger->error("Error loading skill configuration file \"{}\"", SkillConfigName);
	}
}

bool SkillSystem::ParseConfiguration(const json& input)
{
	if (!input.is_object())
	{
		return false;
	}

	for (const auto& item : input.items())
	{
		const std::string key = item.key();

		const json value = item.value();

		if (value.is_string())
		{
			const std::string valueString = value.get<std::string>();

			m_Logger->debug("Setting skill value for \"{}\" to \"{}\"", key, valueString);

			if (const auto cvar = CVAR_GET_POINTER(key.c_str()); cvar != nullptr)
			{
				g_engfuncs.pfnCvar_DirectSet(cvar, valueString.c_str());
			}
			else
			{
				m_Logger->error("No skill cvar for \"{}\"", key);
			}
		}
	}

	return true;
}
