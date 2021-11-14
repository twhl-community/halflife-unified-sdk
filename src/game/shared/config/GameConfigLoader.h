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

#include "scripting/AS/as_utils.h"

#include "utils/json_fwd.h"

#include "GameConfig.h"
#include "GameConfigConditionals.h"

class asIScriptContext;

class GameConfigDefinition;
class GameConfigIncludeStack;
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
private:
	struct LoadContext
	{
		const GameConfigDefinition& Definition;
		const GameConfigLoadParameters& Parameters;

		GameConfigIncludeStack& IncludeStack;
		GameConfig& Config;

		int Depth = 1;
	};

public:
	GameConfigLoader();
	~GameConfigLoader();

	bool Initialize();
	void Shutdown();

	std::shared_ptr<spdlog::logger> GetLogger() { return m_Logger; }

	void SetConditionals(GameConfigConditionals&& conditionals)
	{
		m_Conditionals = std::move(conditionals);
	}

	std::shared_ptr<const GameConfigDefinition> CreateDefinition(std::string&& name, std::vector<std::unique_ptr<const GameConfigSection>>&& sections);

	std::optional<GameConfig> TryLoad(const char* fileName, const GameConfigDefinition& definition, const GameConfigLoadParameters& parameters = {});

	/**
	*	@brief Evaluate a conditional expression
	*	@return If the conditional was successfully evaluated, returns the result.
	*		Otherwise, returns an empty optional.
	*/
	std::optional<bool> EvaluateConditional(std::string_view conditional);

private:
	bool TryLoadCore(LoadContext& loadContext, const char* fileName);

	void ParseConfig(LoadContext& loadContext, std::string_view configFileName, const json& input);

	void ParseIncludedFiles(LoadContext& loadContext, const json& input);

	void ParseSections(LoadContext& loadContext, std::string_view configFileName, const json& input);

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	as::EnginePtr m_ScriptEngine;
	as::UniquePtr<asIScriptContext> m_ScriptContext;
	GameConfigConditionals m_Conditionals;
};

inline GameConfigLoader g_GameConfigLoader;
