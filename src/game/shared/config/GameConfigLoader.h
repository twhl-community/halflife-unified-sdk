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

#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/logger.h>

#include "utils/GameSystem.h"
#include "utils/JSONSystem.h"

class GameConfigDefinition;
class GameConfigIncludeStack;
class GameConfigLoader;
class GameConfigSection;

struct GameConfigLoadParameters final
{
	const char* PathID = nullptr;
	std::any UserData;
};

struct GameConfigContext final
{
	const std::string_view ConfigFileName;
	const json& Input;
	const GameConfigDefinition& Definition;

	GameConfigLoader& Loader;
	std::any UserData;
};

/**
*	@brief Loads JSON-based configuration files and processes them according to a provided definition.
*/
class GameConfigLoader final : public IGameSystem
{
private:
	struct LoadContext
	{
		const GameConfigDefinition& Definition;
		const GameConfigLoadParameters& Parameters;

		GameConfigIncludeStack& IncludeStack;

		int Depth = 1;
	};

public:
	GameConfigLoader();
	~GameConfigLoader();

	const char* GetName() const override { return "GameConfig"; }

	bool Initialize() override;
	void PostInitialize() override {}
	void Shutdown() override;

	std::shared_ptr<spdlog::logger> GetLogger() { return m_Logger; }

	std::shared_ptr<const GameConfigDefinition> CreateDefinition(std::string&& name, std::vector<std::unique_ptr<const GameConfigSection>>&& sections);

	bool TryLoad(const char* fileName, const GameConfigDefinition& definition, const GameConfigLoadParameters& parameters = {});

private:
	bool TryLoadCore(LoadContext& loadContext, const char* fileName);

	void ParseConfig(LoadContext& loadContext, std::string_view configFileName, const json& input);

	void ParseIncludedFiles(LoadContext& loadContext, const json& input);

	void ParseSections(LoadContext& loadContext, std::string_view configFileName, const json& input);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
};

inline GameConfigLoader g_GameConfigLoader;
