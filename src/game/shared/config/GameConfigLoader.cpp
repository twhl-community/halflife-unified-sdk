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

#include <sstream>

#include <spdlog/fmt/fmt.h>

#include "extdll.h"
#include "util.h"

#include "GameConfigDefinition.h"
#include "GameConfigLoader.h"
#include "GameConfigSection.h"
#include "utils/json_utils.h"

using namespace std::literals;

//Provides access to the constructor.
struct GameConfigDefinitionImplementation : public GameConfigDefinition
{
	GameConfigDefinitionImplementation(std::string&& name, std::vector<std::unique_ptr<const GameConfigSection>>&& sections)
		: GameConfigDefinition(std::move(name), std::move(sections))
	{
	}
};

GameConfigLoader::GameConfigLoader() = default;
GameConfigLoader::~GameConfigLoader() = default;

bool GameConfigLoader::Initialize()
{
	m_Logger = g_Logging->CreateLogger("gamecfg");

	return true;
}

void GameConfigLoader::Shutdown()
{
	m_Logger.reset();
}

std::shared_ptr<const GameConfigDefinition> GameConfigLoader::CreateDefinition(
	std::string&& name, std::vector<std::unique_ptr<const GameConfigSection>>&& sections)
{
	auto definition = std::make_shared<const GameConfigDefinitionImplementation>(std::move(name), std::move(sections));

	g_JSON.RegisterSchema(std::string{definition->GetName()}, [=]() { return definition->GetSchema(); });

	return definition;
}

std::optional<GameConfig> GameConfigLoader::TryLoad(
	const char* fileName, const GameConfigDefinition& definition, const GameConfigLoadParameters& parameters)
{
	m_Logger->trace("Loading \"{}\" configuration file \"{}\"", definition.GetName(), fileName);

	auto config = g_JSON.ParseJSONFile(
		fileName,
		definition.GetValidator(),
		[&, this](const json& input) { return ParseConfig(fileName, definition, input); },
		parameters.PathID);

	if (config.has_value())
	{
		m_Logger->trace("Successfully loaded configuration file");
	}
	else
	{
		m_Logger->trace("Failed to load configuration file");
	}

	return config;
}

GameConfig GameConfigLoader::ParseConfig(std::string_view configFileName, const GameConfigDefinition& definition, const json& input)
{
	GameConfig config;

	if (!input.is_object())
	{
		return config;
	}

	if (auto sections = input.find("Sections"); sections != input.end())
	{
		if (!sections->is_array())
		{
			return config;
		}

		for (const auto& section : *sections)
		{
			if (!section.is_object())
			{
				continue;
			}

			if (const auto name = section.value("Name", ""sv); !name.empty())
			{
				if (const auto sectionObj = definition.FindSection(name); sectionObj)
				{
					GameConfigContext context
					{
						.ConfigFileName = configFileName,
						.Input = section,
						.Definition = definition,
						.Loader = *this,
						.Configuration = config
					};

					if (!sectionObj->TryParse(context))
					{
						m_Logger->error("Error parsing section \"{}\"", sectionObj->GetName());
					}
				}
				else
				{
					m_Logger->warn("Unknown section \"{}\"", name);
				}
			}
		}
	}

	return config;
}
