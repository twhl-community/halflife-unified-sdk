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

#include <regex>

#include "cbase.h"
#include "CommandWhitelist.h"
#include "GameLibrary.h"
#include "utils/JSONSystem.h"

constexpr const char* const CommandWhitelistFileName = "cfg/CommandWhitelist.json";
const std::regex CommandWhitelistRegex{"^[\\w]+$"};
constexpr std::string_view CommandWhitelistSchemaName{"CommandWhitelist"sv};

static std::string GetCommandWhitelistSchema()
{
	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Command Whitelist",
	"description": "List of console commands that map configs and logic_setcvar can execute",
	"type": "array",
	"items": {{
		"title": "Command Name",
		"type": "string",
		"pattern": "^[\\w]+$"
	}}
}}
)");
}

void RegisterCommandWhitelistSchema()
{
	g_JSON.RegisterSchema(CommandWhitelistSchemaName, &GetCommandWhitelistSchema);
}

void LoadCommandWhitelist()
{
	// Load the whitelist from a file
	auto whitelist = g_JSON.ParseJSONFile(
		CommandWhitelistFileName,
		{.SchemaName = CommandWhitelistSchemaName, .PathID = "GAMECONFIG"},
		[](const json& input)
		{
			CommandWhitelist list;

			if (input.is_array())
			{
				list.reserve(input.size());

				for (const auto& element : input)
				{
					auto command = element.get<std::string>();

					if (std::regex_match(command, CommandWhitelistRegex))
					{
						if (!list.insert(std::move(command)).second)
						{
							g_GameLogger->debug("Whitelist command \"{}\" encountered more than once", command);
						}
					}
					else
					{
						g_GameLogger->warn("Whitelist command \"{}\" has invalid syntax, ignoring", command);
					}
				}
			}

			return list;
		});

	g_CommandWhitelist = whitelist.value_or(CommandWhitelist{});
}
