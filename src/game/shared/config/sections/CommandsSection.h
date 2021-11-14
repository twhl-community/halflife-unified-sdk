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

#include <spdlog/logger.h>
#include <spdlog/fmt/fmt.h>

#include "config/GameConfig.h"
#include "config/GameConfigSection.h"

/**
*	@brief A list of command strings that will be executed when applied.
*/
class CommandsData final : public GameConfigData
{
public:
	CommandsData(std::shared_ptr<spdlog::logger>&& logger)
		: m_Logger(std::move(logger))
	{
	}

	void Apply(const std::any& userData) const override final
	{
		std::string buffer;

		for (const auto& command : m_Commands)
		{
			m_Logger->trace("Executing command \"{}\"", command);

			buffer.clear();
			fmt::format_to(std::back_inserter(buffer), "{}\n", command);

			g_engfuncs.pfnServerCommand(buffer.c_str());
		}
	}

	void AddCommands(std::vector<std::string>&& commands)
	{
		//Prevent accidental self-append
		if (&m_Commands == &commands)
		{
			return;
		}

		//Append commands from other into my list.
		m_Commands.insert(
			m_Commands.end(),
			std::make_move_iterator(commands.begin()),
			std::make_move_iterator(commands.end()));

		commands.clear();
	}

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::vector<std::string> m_Commands;
};

/**
*	@brief Defines a section containing an array of console command strings.
*/
class CommandsSection final : public GameConfigSection
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

	std::tuple<std::string, std::string> GetSchema() const override final
	{
		return
		{
			fmt::format(R"(
"Commands": {{
	"type": "array",
	"items": {{
		"title": "Command",
		"type": "string",
		"pattern": ".+"
	}}
}}
)"),
			{"\"Commands\""}
		};
	}

	bool TryParse(GameConfigContext& context) const override final
	{
		using namespace std::literals;

		const auto commandsIt = context.Input.find("Commands");

		if (commandsIt == context.Input.end())
		{
			return false;
		}

		const auto& commandsInput = *commandsIt;

		if (!commandsInput.is_array())
		{
			return false;
		}

		std::vector<std::string> commands;

		commands.reserve(commandsInput.size());

		for (const auto& command : commandsInput)
		{
			if (!command.is_string())
			{
				continue;
			}

			auto value = command.get<std::string>();

			//First token is the command name
			COM_Parse(value.c_str());

			if (!com_token[0])
			{
				continue;
			}

			//Prevent anything screwy from going into names
			//Prevent commands from being snuck in by appending it to the end of another command
			if (!std::regex_match(com_token, CommandNameRegex)
				|| !ValidateCommand(value))
			{
				context.Loader.GetLogger()->warn(
					"Command \"{:.10}{}\" contains illegal characters",
					value, value.length() > 10 ? "..."sv : ""sv);
				continue;
			}

			if (!m_CommandWhitelist || m_CommandWhitelist->contains(com_token))
			{
				commands.emplace_back(std::move(value));
			}
			else
			{
				context.Loader.GetLogger()->warn("Command \"{}\" is not whitelisted, ignoring", com_token);
			}
		}

		auto data = context.Configuration.GetOrCreate<CommandsData>(context.Loader.GetLogger());

		data->AddCommands(std::move(commands));

		return true;
	}

private:
	static bool ValidateCommand(std::string_view command)
	{
		bool inQuotes = false;

		for (auto c : command)
		{
			//This logic matches that of the engine's command parser
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
