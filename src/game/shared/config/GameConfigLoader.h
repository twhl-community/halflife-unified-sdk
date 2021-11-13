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

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/logger.h>

#include "utils/json_fwd.h"

#include "GameConfig.h"

class GameConfigDefinition;
class GameConfigLoader;
class GameConfigSection;

struct GameConfigLoadParameters final
{
	const char* PathID = nullptr;
};

struct GameConfigContext final
{
	const std::string_view ConfigFileName;
	const json& Input;
	const GameConfigDefinition& Definition;

	GameConfigLoader& Loader;
	GameConfig& Configuration;
};

/**
*	@brief Loads JSON-based configuration files and processes them according to a provided definition.
*/
class GameConfigLoader final
{
public:
	GameConfigLoader();
	~GameConfigLoader();

	bool Initialize();
	void Shutdown();

	std::shared_ptr<spdlog::logger> GetLogger() { return m_Logger; }

	std::shared_ptr<const GameConfigDefinition> CreateDefinition(std::string&& name, std::vector<std::unique_ptr<const GameConfigSection>>&& sections);

	std::optional<GameConfig> TryLoad(const char* fileName, const GameConfigDefinition& definition, const GameConfigLoadParameters& parameters = {});

private:
	GameConfig ParseConfig(std::string_view configFileName, const GameConfigDefinition& definition, const json& input);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
};

inline GameConfigLoader g_GameConfigLoader;
