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

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <regex>
#include <tuple>

#include <EASTL/fixed_vector.h>

#include "cbase.h"
#include "skill.h"

#include "ConCommandSystem.h"
#include "GameLibrary.h"
#include "JSONSystem.h"

#ifndef CLIENT_DLL
#include "UserMessages.h"
#else
#include "networking/ClientUserMessages.h"
#endif

using namespace std::literals;

constexpr std::string_view SkillConfigSchemaName{"SkillConfig"sv};

constexpr std::string_view SkillVariableNameRegexPattern{"^([a-zA-Z_](?:[a-zA-Z_0-9]*[a-zA-Z_]))([123]?)$"};
const std::regex SkillVariableNameRegex{SkillVariableNameRegexPattern.data(), SkillVariableNameRegexPattern.length()};

static std::string GetSkillConfigSchema()
{
	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Skill System Configuration",
	"type": "object",
	"patternProperties": {{
		"{}": {{
			"type": "number"
		}}
	}},
	"additionalProperties": false
}}
)",
		SkillVariableNameRegexPattern);
}

static std::optional<std::tuple<std::string_view, std::optional<SkillLevel>>> TryParseSkillVariableName(std::string_view key, spdlog::logger& logger)
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
		// Only a name, no skill level.
		return {{baseName, {}}};
	}

	int skillLevel = 0;
	if (const auto result = std::from_chars(skillLevelString.data(), skillLevelString.data() + skillLevelString.length(), skillLevel);
		result.ec != std::errc())
	{
		if (result.ec != std::errc::result_out_of_range)
		{
			// In case something else goes wrong.
			logger.error("Invalid skill variable name \"{}\": {}", key, std::make_error_code(result.ec).message());
		}

		return {};
	}

	return {{baseName, static_cast<SkillLevel>(skillLevel)}};
}

bool SkillSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("skill");

	g_JSON.RegisterSchema(SkillConfigSchemaName, &GetSkillConfigSchema);

	g_NetworkData.RegisterHandler("Skill", this);

#ifndef CLIENT_DLL
	g_ConCommands.CreateCommand(
		"sk_find", [this](const auto& args)
		{
			if (args.Count() < 2)
			{
				Con_Printf("Usage: %s <search_term> [filter]\nUse * to list all keys\nFilters: networkedonly, all\n", args.Argument(0));
				return;
			}

			const std::string_view searchTerm{args.Argument(1)};

			bool networkedOnly = false;

			if (args.Count() >= 3)
			{
				if (FStrEq("networkedonly", args.Argument(2)))
				{
					networkedOnly = true;
				}
				else if (!FStrEq("all", args.Argument(2)))
				{
					Con_Printf("Unknown filter option\n");
					return;
				}
			}

			const auto printer = [=](const SkillVariable& variable)
			{
				if (networkedOnly && variable.NetworkIndex == NotNetworkedIndex)
				{
					return;
				}

				Con_Printf("%s = %.2f%s\n",
					variable.Name.c_str(), variable.CurrentValue,
					variable.NetworkIndex != NotNetworkedIndex ? " (Networked)"  : "");
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
			} },
		CommandLibraryPrefix::No);

	g_ConCommands.CreateCommand(
		"sk_set", [this](const auto& args)
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

			SetValue(name, value); },
		CommandLibraryPrefix::No);
#else
	g_ClientUserMessages.RegisterHandler("SkillVars", &SkillSystem::MsgFunc_SkillVars, this);
#endif

	return true;
}

void SkillSystem::Shutdown()
{
	m_Logger.reset();
}

void SkillSystem::HandleNetworkDataBlock(NetworkDataBlock& block)
{
	if (block.Mode == NetworkDataMode::Serialize)
	{
		eastl::fixed_string<SkillVariable*, MaxNetworkedVariables> variables;

		for (auto& variable : m_SkillVariables)
		{
			if (variable.NetworkIndex != NotNetworkedIndex)
			{
				variables.push_back(&variable);
			}
		}

		std::sort(variables.begin(), variables.end(), [](const auto lhs, const auto rhs)
			{ return lhs->NetworkIndex < rhs->NetworkIndex; });

		block.Data = json::array();

		for (auto variable : variables)
		{
			json varData = json::object();

			varData.emplace("Name", variable->Name);
			varData.emplace("Value", variable->CurrentValue);

			// So SendAllNetworkedSkillVars only sends variables that have changed compared to the configured value.
			variable->NetworkedValue = variable->CurrentValue;

			block.Data.push_back(std::move(varData));
		}
	}
	else
	{
		m_SkillVariables.clear();
		m_NextNetworkedIndex = 0;

		for (const auto& varData : block.Data)
		{
			auto name = varData.value<std::string>("Name", "");
			const float value = varData.value<float>("Value", 0.f);

			if (name.empty())
			{
				block.ErrorMessage = "Invalid skill variable name";
				return;
			}

			DefineVariable(std::move(name), value, {.Networked = true});
		}
	}
}

void SkillSystem::LoadSkillConfigFiles(std::span<const std::string> fileNames)
{
	// Refresh skill level setting first.
	int iSkill = (int)CVAR_GET_FLOAT("skill");

	iSkill = std::clamp(iSkill, static_cast<int>(SkillLevel::Easy), static_cast<int>(SkillLevel::Hard));

	SetSkillLevel(static_cast<SkillLevel>(iSkill));

	// Erase all previous data.
	for (auto it = m_SkillVariables.begin(); it != m_SkillVariables.end();)
	{
		if ((it->Flags & VarFlag_IsExplicitlyDefined) != 0)
		{
			it->CurrentValue = it->InitialValue;
			++it;
		}
		else
		{
			it = m_SkillVariables.erase(it);
		}
	}

	m_LoadingSkillFiles = true;

	for (const auto& fileName : fileNames)
	{
		m_Logger->trace("Loading {}", fileName);

		if (const auto result = g_JSON.ParseJSONFile(fileName.c_str(),
				{.SchemaName = SkillConfigSchemaName, .PathID = "GAMECONFIG"},
				[this](const json& input)
				{ return ParseConfiguration(input); });
			!result.value_or(false))
		{
			m_Logger->error("Error loading skill configuration file \"{}\"", fileName);
		}
	}

	m_LoadingSkillFiles = false;
}

void SkillSystem::DefineVariable(std::string name, float initialValue, const SkillVarConstraints& constraints)
{
	auto it = std::find_if(m_SkillVariables.begin(), m_SkillVariables.end(), [&](const auto& variable)
		{ return variable.Name == name; });

	if (it != m_SkillVariables.end())
	{
		m_Logger->error("Cannot define variable \"{}\": already defined", name);
		assert(!"Variable already defined");
		return;
	}

	SkillVarConstraints updatedConstraints = constraints;

	int networkIndex = NotNetworkedIndex;

	if (updatedConstraints.Networked)
	{
		if (m_NextNetworkedIndex < LargestNetworkedIndex)
		{
			networkIndex = m_NextNetworkedIndex++;
		}
		else
		{
			m_Logger->error("Cannot define variable \"{}\": Too many networked variables", name);
			assert(!"Too many networked variables");
		}
	}

	if (updatedConstraints.Minimum && updatedConstraints.Maximum)
	{
		if (updatedConstraints.Minimum > updatedConstraints.Maximum)
		{
			m_Logger->warn("Variable \"{}\" has inverted bounds", name);
			std::swap(updatedConstraints.Minimum, updatedConstraints.Maximum);
		}
	}

	initialValue = ClampValue(initialValue, updatedConstraints);

	SkillVariable variable{
		.Name = std::move(name),
		.CurrentValue = initialValue,
		.InitialValue = initialValue,
		.Constraints = updatedConstraints,
		.NetworkIndex = networkIndex,
		.Flags = VarFlag_IsExplicitlyDefined
	};

	m_SkillVariables.emplace_back(std::move(variable));
}

float SkillSystem::GetValue(std::string_view name, float defaultValue) const
{
	if (const auto it = std::find_if(m_SkillVariables.begin(), m_SkillVariables.end(), [&](const auto& variable)
			{ return variable.Name == name; });
		it != m_SkillVariables.end())
	{
		return it->CurrentValue;
	}

	m_Logger->debug("Undefined variable {}{}", name, m_SkillLevel);

	return defaultValue;
}

void SkillSystem::SetValue(std::string_view name, float value)
{
	auto it = std::find_if(m_SkillVariables.begin(), m_SkillVariables.end(), [&](const auto& variable)
		{ return variable.Name == name; });

	if (it == m_SkillVariables.end())
	{
		SkillVariable variable{
			.Name = std::string{name},
			.CurrentValue = 0,
			.InitialValue = 0};

		m_SkillVariables.emplace_back(std::move(variable));

		it = m_SkillVariables.end() - 1;
	}

	value = ClampValue(value, it->Constraints);

	if (it->CurrentValue != value)
	{
		m_Logger->debug("Skill value \"{}\" changed to \"{}\"{}",
			name, value, it->NetworkIndex != NotNetworkedIndex ? " (Networked)" : "");

		it->CurrentValue = value;

#ifndef CLIENT_DLL
		if (!m_LoadingSkillFiles)
		{
			if (it->NetworkIndex != NotNetworkedIndex)
			{
				MESSAGE_BEGIN(MSG_ALL, gmsgSkillVars);
				WRITE_BYTE(it->NetworkIndex);
				WRITE_FLOAT(it->CurrentValue);
				MESSAGE_END();
			}
		}
#endif
	}
}

#ifndef CLIENT_DLL
void SkillSystem::SendAllNetworkedSkillVars(CBasePlayer* player)
{
	// Send skill vars in bursts.
	const int maxMessageSize = int(MaxUserMessageLength) / SingleMessageSize;

	int totalMessageSize = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgSkillVars, nullptr, player->edict());

	for (const auto& variable : m_SkillVariables)
	{
		if (variable.NetworkIndex == NotNetworkedIndex)
		{
			continue;
		}

		// Player already has these from networked data.
		if (variable.CurrentValue == variable.NetworkedValue)
		{
			continue;
		}

		if (totalMessageSize >= maxMessageSize)
		{
			MESSAGE_END();
			MESSAGE_BEGIN(MSG_ONE, gmsgSkillVars, nullptr, player->edict());
			totalMessageSize = 0;
		}

		WRITE_BYTE(variable.NetworkIndex);
		WRITE_FLOAT(variable.CurrentValue);

		totalMessageSize += SingleMessageSize;
	}

	MESSAGE_END();
}
#endif

float SkillSystem::ClampValue(float value, const SkillVarConstraints& constraints)
{
	if (constraints.Type == SkillVarType::Integer)
	{
		// Round value to integer.
		value = int(value);
	}

	if (constraints.Minimum)
	{
		value = std::max(*constraints.Minimum, value);
	}

	if (constraints.Maximum)
	{
		value = std::min(*constraints.Maximum, value);
	}

	return value;
}

bool SkillSystem::ParseConfiguration(const json& input)
{
	if (!input.is_object())
	{
		return false;
	}

	for (const auto& item : input.items())
	{
		const json value = item.value();

		if (!value.is_number())
		{
			// Already validated by schema.
			continue;
		}

		// Get the skill variable base name and skill level.
		const auto maybeVariableName = TryParseSkillVariableName(item.key(), *m_Logger);

		if (!maybeVariableName.has_value())
		{
			continue;
		}

		const auto& variableName = maybeVariableName.value();

		const auto valueFloat = value.get<float>();

		const auto& skillLevel = std::get<1>(variableName);

		if (!skillLevel.has_value() || skillLevel.value() == GetSkillLevel())
		{
			SetValue(std::get<0>(variableName), valueFloat);
		}
	}

	return true;
}

#ifdef CLIENT_DLL
void SkillSystem::MsgFunc_SkillVars(BufferReader& reader)
{
	const int messageCount = reader.GetSize() / SingleMessageSize;

	for (int i = 0; i < messageCount; ++i)
	{
		[&]()
		{
			const int networkIndex = reader.ReadByte();
			const float value = reader.ReadFloat();

			if (networkIndex < 0 || networkIndex >= m_NextNetworkedIndex)
			{
				m_Logger->error("Invalid network index {} received for networked skill variable", networkIndex);
				return;
			}

			for (auto& variable : m_SkillVariables)
			{
				if (variable.NetworkIndex == networkIndex)
				{
					// Don't need to log since the server logs the change. The client only follows its lead.
					variable.CurrentValue = value;
					return;
				}
			}

			m_Logger->error("Could not find networked skill variable with index {}", networkIndex);
		}();
	}
}
#endif
