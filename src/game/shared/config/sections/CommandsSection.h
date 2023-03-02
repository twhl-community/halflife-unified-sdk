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

#include <iterator>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_set>

#include <fmt/format.h>

#include <spdlog/logger.h>

#include "config/GameConfig.h"

#include "utils/JSONSystem.h"

/**
 *	@brief Defines a section containing an array of console command strings.
 */
template <typename DataContext>
class CommandsSection final : public GameConfigSection<DataContext>
{
private:
	static const inline std::regex CommandNameRegex{"^[\\w]+$"};

public:
	/**
	 *	@param commandWhitelist If provided, only commands listed in this whitelist are allowed to be executed.
	 */
	explicit CommandsSection(std::optional<std::unordered_set<std::string>>&& commandWhitelist = {})
		: m_CommandWhitelist(std::move(commandWhitelist))
	{
	}

	std::string_view GetName() const override final { return "Commands"; }

	json::value_t GetType() const override final { return json::value_t::array; }

	std::string GetSchema() const override final
	{
		return fmt::format(R"(
"items": {{
	"title": "Command",
	"type": "string",
	"pattern": ".+"
}})");
	}

	bool TryParse(GameConfigContext<DataContext>& context) const override final
	{
		std::string buffer;

		auto& logger = context.Logger;

		auto executor = [&](const std::string& command)
		{
			logger.trace("Executing command \"{}\"", command);

			buffer.clear();
			fmt::format_to(std::back_inserter(buffer), "{}\n", command);

			g_engfuncs.pfnServerCommand(buffer.c_str());
		};

		std::vector<std::string> commands;

		commands.reserve(context.Input.size());

		for (const auto& command : context.Input)
		{
			if (!command.is_string())
			{
				continue;
			}

			auto value = command.template get<std::string>();

			// First token is the command name
			COM_Parse(value.c_str());

			if (!com_token[0])
			{
				continue;
			}

			// Prevent anything screwy from going into names
			// Prevent commands from being snuck in by appending it to the end of another command
			if (!std::regex_match(com_token, CommandNameRegex) || !ValidateCommand(value))
			{
				logger.warn(
					"Command \"{:.10}{}\" contains illegal characters",
					value, value.length() > 10 ? "..."sv : ""sv);
				continue;
			}

			if (!m_CommandWhitelist || m_CommandWhitelist->contains(com_token))
			{
				executor(value);
			}
			else
			{
				logger.warn("Command \"{}\" is not whitelisted, ignoring", com_token);
			}
		}

		return true;
	}

private:
	static bool ValidateCommand(std::string_view command)
	{
		bool inQuotes = false;

		for (auto c : command)
		{
			// This logic matches that of the engine's command parser
			if (c == '\"')
			{
				inQuotes = !inQuotes;
			}
			else if ((!inQuotes && c == ';') || c == '\n')
			{
				return false;
			}
		}

		return true;
	}

private:
	const std::optional<std::unordered_set<std::string>> m_CommandWhitelist;
};
