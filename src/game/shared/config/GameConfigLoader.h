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
#include <tuple>
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
 *	@brief Defines a section of a game configuration, and the logic to parse it.
 */
class GameConfigSection
{
protected:
	GameConfigSection() = default;

public:
	GameConfigSection(const GameConfigSection&) = delete;
	GameConfigSection& operator=(const GameConfigSection&) = delete;
	virtual ~GameConfigSection() = default;

	/**
	 *	@brief Gets the name of this section
	 */
	virtual std::string_view GetName() const = 0;

	/**
	 *	@brief Gets the partial schema for this section.
	 *	@details The schema is the part of an object's "properties" definition.
	 *	Define properties used by this section in it.
	 *	@return A tuple containing the object definition
	 *		and the contents of the \c required array applicable to the keys specified in the first element.
	 */
	virtual std::tuple<std::string, std::string> GetSchema() const = 0;

	/**
	 *	@brief Tries to parse the given configuration.
	 */
	virtual bool TryParse(GameConfigContext& context) const = 0;
};

/**
 *	@brief Immutable definition of a game configuration.
 *	Contains a set of sections that configuration files can use.
 *	Instances should be created with GameConfigLoader::CreateDefinition.
 *	@details Each section specifies what kind of data it supports, and provides a means of parsing it.
 */
class GameConfigDefinition
{
protected:
	GameConfigDefinition(std::string&& name, std::vector<std::unique_ptr<const GameConfigSection>>&& sections);

public:
	GameConfigDefinition(const GameConfigDefinition&) = delete;
	GameConfigDefinition& operator=(const GameConfigDefinition&) = delete;

	~GameConfigDefinition();

	std::string_view GetName() const { return m_Name; }

	const GameConfigSection* FindSection(std::string_view name) const;

	/**
	 *	@brief Gets the complete JSON Schema for this definition.
	 *	This includes all of the sections.
	 */
	std::string GetSchema() const;

private:
	std::string m_Name;
	std::vector<std::unique_ptr<const GameConfigSection>> m_Sections;
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
