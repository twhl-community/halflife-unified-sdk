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

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <regex>
#include <tuple>

#include "cbase.h"
#include "skill.h"

#include "command_utils.h"
#include "json_utils.h"
#include "config/GameConfigLoader.h"

using namespace std::literals;

constexpr std::string_view SkillConfigSchemaName{"SkillConfig"sv};
constexpr const char* const SkillConfigName = "cfg/skill.json";

constexpr std::string_view SkillVariableNameRegexPattern{"^([a-zA-Z_](?:[a-zA-Z_0-9]*[a-zA-Z_]))([123]?)$"};
const std::regex SkillVariableNameRegex{SkillVariableNameRegexPattern.data(), SkillVariableNameRegexPattern.length()};

static std::string GetSkillConfigSchema()
{
	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Skill System Configuration",
	"type": "array",
	"items": {{
		"type": "object",
		"properties": {{
			"Description": {{
				"type": "string"
			}},
			"Condition": {{
				"type": "string"
			}},
			"Variables": {{
				"type": "object",
				"patternProperties": {{
					"{}": {{
						"type": "number"
					}}
				}},
				"additionalProperties": false
			}}
		}},
		"required": ["Variables"]
	}}
}}
)",
		SkillVariableNameRegexPattern);
}

static std::optional<std::tuple<std::string_view, std::optional<int>>> TryParseSkillVariableName(std::string_view key, spdlog::logger& logger)
{
	std::match_results<std::string_view::const_iterator> matches;

	if (!std::regex_match(key.begin(), key.end(), matches, SkillVariableNameRegex))
	{
		return {};
	}

	const std::string_view baseName{matches[1].first, matches[1].second};
	const std::string_view skillLevelString{matches[2].first, matches[2].second};

	if (skillLevelString.empty())
	{
		//Only a name, no skill level.
		return {{baseName, {}}};
	}

	int skillLevel = 0;
	if (const auto result = std::from_chars(skillLevelString.data(), skillLevelString.data() + skillLevelString.length(), skillLevel);
		result.ec != std::errc())
	{
		if (result.ec != std::errc::result_out_of_range)
		{
			//In case something else goes wrong.
			logger.error("Invalid skill variable name \"{}\": {}", key, std::make_error_code(result.ec).message());
		}

		return {};
	}

	return {{baseName, skillLevel}};
}

bool SkillSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("skill");

	g_JSON.RegisterSchema(SkillConfigSchemaName, &GetSkillConfigSchema);

	g_ConCommands.CreateCommand("sk_find", [this](const CCommandArgs& args)
		{
			if (args.Count() != 2)
			{
				Con_Printf("Usage: %s <search_term>\nUse * to list all keys\n", args.Argument(0));
				return;
			}

			const std::string_view searchTerm{args.Argument(1)};

			const auto printer = [](const SkillVariable& variable)
			{
				Con_Printf("%s = %.2f\n", variable.Name.c_str(), variable.Value);
			};

			//TODO: maybe replace this with proper wildcard searching at some point.
			if (searchTerm == "*")
			{
				//List all keys.
				for (const auto& variable : m_SkillVariables)
				{
					printer(variable);
				}
			}
			else
			{
				//List all keys that are a full or partial match.
				for (const auto& variable : m_SkillVariables)
				{
					if (variable.Name.find(searchTerm) != std::string::npos)
					{
						printer(variable);
					}
				}
			}
		},
		CommandLibraryPrefix::No);

	g_ConCommands.CreateCommand("sk_set", [this](const CCommandArgs& args)
		{
			if (args.Count() != 3)
			{
				Con_Printf("Usage: %s <name> <value>\n", args.Argument(0));
				return;
			}

			const std::string_view name{args.Argument(1)};
			const std::string_view valueString{args.Argument(2)};

			float value = 0;
			if (const auto result = std::from_chars(valueString.data(), valueString.data() + valueString.length(), value);
				result.ec != std::errc())
			{
				Con_Printf("Invalid value\n");
				return;
			}

			SetValue(name, value);
		},
		CommandLibraryPrefix::No);

	g_ConCommands.CreateCommand("sk_remove", [this](const CCommandArgs& args)
		{
			if (args.Count() != 2)
			{
				Con_Printf("Usage: %s <name>\n", args.Argument(0));
				return;
			}

			RemoveValue(args.Argument(1));
		},
		CommandLibraryPrefix::No);

	//Don't name this sk_remove_all because the console will always autocomplete sk_remove to that.
	g_ConCommands.CreateCommand("sk_reset", [this](const CCommandArgs& args)
		{
			if (args.Count() != 1)
			{
				Con_Printf("Usage: %s\n", args.Argument(0));
				return;
			}

			m_SkillVariables.clear();
		},
		CommandLibraryPrefix::No);

	g_ConCommands.CreateCommand("sk_reload", [this](const CCommandArgs& args)
		{ LoadSkillConfigFile(); },
		CommandLibraryPrefix::No);

	return true;
}

void SkillSystem::Shutdown()
{
	m_Logger.reset();
}

void SkillSystem::NewMapStarted()
{
	LoadSkillConfigFile();
}

void SkillSystem::LoadSkillConfigFile()
{
	//Refresh skill level setting first.
	int iSkill = (int)CVAR_GET_FLOAT("skill");

	iSkill = std::clamp(iSkill, static_cast<int>(SkillLevel::Easy), static_cast<int>(SkillLevel::Hard));

	SetSkillLevel(iSkill);

	//Erase all previous data.
	m_SkillVariables.clear();

	if (const auto result = g_JSON.ParseJSONFile(SkillConfigName,
		{.SchemaName = SkillConfigSchemaName, .PathID = "GAMECONFIG"},
		[this](const json& input){ return ParseConfiguration(input); }); !result.value_or(false))
	{
		m_Logger->error("Error loading skill configuration file \"{}\"", SkillConfigName);
	}
}

float SkillSystem::GetValue(std::string_view name) const
{
	float value = 0;

	if (const auto it = std::find_if(m_SkillVariables.begin(), m_SkillVariables.end(), [&](const auto& variable)
			{ return variable.Name == name; }); it != m_SkillVariables.end())
	{
		value = it->Value;

		if ( value > 0)
		{
			return value;
		}
	}

	m_Logger->debug("Got a zero for {}{}", name, m_SkillLevel);

	return value;
}

void SkillSystem::SetValue(std::string_view name,  float value)
{
	auto it = std::find_if(m_SkillVariables.begin(), m_SkillVariables.end(), [&](const auto& variable)
		{ return variable.Name == name; });

	if (it == m_SkillVariables.end())
	{
		m_SkillVariables.emplace_back(std::string{name}, float{});

		it = m_SkillVariables.end() - 1;
	}

	if (it->Value != value)
	{
		m_Logger->debug("Skill value \"{}\" changed to \"{}\"", name, value);
		it->Value = value;
	}
}

void SkillSystem::RemoveValue(std::string_view name)
{
	if (const auto it = std::find_if(m_SkillVariables.begin(), m_SkillVariables.end(), [&](const auto& variable)
		{ return variable.Name == name; });
		it != m_SkillVariables.end())
	{
		m_SkillVariables.erase(it);
	}
}

bool SkillSystem::ParseConfiguration(const json& input)
{
	if (!input.is_array())
	{
		return false;
	}

	//Apply each section in order of occurrence.
	for (const auto& section : input)
	{
		if (!section.is_object())
		{
			continue;
		}

		if (m_Logger->should_log(spdlog::level::trace))
		{
			const auto description = section.value<std::string>("Description", "Skill configuration section");

			m_Logger->trace("Processing section \"{}\"", description);
		}

		if (const auto it = section.find("Condition"); it != section.end())
		{
			//Evaluate condition to determine whether to apply this section.

			if (!it->is_string())
			{
				continue;
			}

			const auto result = g_GameConfigLoader.EvaluateConditional(it->get<std::string>());

			if (!result.has_value())
			{
				m_Logger->error("Error evaluating condition");
				continue;
			}

			if (!result.value())
			{
				m_Logger->debug("Skipping section because condition evaluated false");
				continue;
			}

			m_Logger->debug("Applying section because condition evaluated true");
		}
		else
		{
			m_Logger->debug("Applying section unconditionally");
		}

		const auto it = section.find("Variables");

		if (it == section.end())
		{
			continue;
		}

		const auto& variables = *it;

		if (!variables.is_object())
		{
			continue;
		}

		for (const auto& item : variables.items())
		{
			[&]()
			{
				const json value = item.value();

				if (!value.is_number())
				{
					//Already validated by schema.
					return;
				}

				//Get the skill variable base name and skill level.
				const auto maybeVariableName = TryParseSkillVariableName(item.key(), *m_Logger);

				if (!maybeVariableName.has_value())
				{
					return;
				}

				const auto& variableName = maybeVariableName.value();

				const auto valueFloat = value.get<float>();

				const auto& skillLevel = std::get<1>(variableName);

				if (!skillLevel.has_value() || skillLevel.value() == GetSkillLevel())
				{
					SetValue(std::get<0>(variableName), valueFloat);
				}
			}();
		}
	}

	return true;
}
