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
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <fmt/format.h>

#include <spdlog/logger.h>

#include "ConditionEvaluator.h"
#include "GameConfigIncludeStack.h"

#include "utils/GameSystem.h"
#include "utils/JSONSystem.h"
#include "utils/string_utils.h"

template<typename DataContext>
struct GameConfigDefinition;

template<typename DataContext>
struct GameConfigLoadParameters final
{
	DataContext& Data;
	const char* PathID = nullptr;
};

template<typename DataContext>
struct GameConfigContext final
{
	const std::string_view ConfigFileName;
	const json& Input;
	const GameConfigDefinition<DataContext>& Definition;

	spdlog::logger& Logger;
	DataContext& Data;
};

/**
 *	@brief Defines a section of a game configuration, and the logic to parse it.
 *	@tparam DataContext The type of the data context this section operates on.
 */
template<typename DataContext>
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
	virtual bool TryParse(GameConfigContext<DataContext>& context) const = 0;
};

/**
 *	@brief Immutable definition of a game configuration.
 *	Contains a set of sections that configuration files can use.
 *	Instances should be created with GameConfigLoader::CreateDefinition.
 *	@details Each section specifies what kind of data it supports, and provides a means of parsing it.
 *	@tparam DataContext Type of the data context this definition operates on.
 */
template<typename DataContext>
struct GameConfigDefinition
{
	GameConfigDefinition(
		std::shared_ptr<spdlog::logger> logger,
		std::string&& name,
		std::vector<std::unique_ptr<const GameConfigSection<DataContext>>>&& sections)
		: m_Logger(logger), m_Name(std::move(name)), m_Sections(std::move(sections))
	{
	}

	GameConfigDefinition(const GameConfigDefinition&) = delete;
	GameConfigDefinition& operator=(const GameConfigDefinition&) = delete;

	~GameConfigDefinition() = default;

	std::string_view GetName() const { return m_Name; }

	const GameConfigSection<DataContext>* FindSection(std::string_view name) const;

	/**
	 *	@brief Gets the complete JSON Schema for this definition.
	 *	This includes all of the sections.
	 */
	std::string GetSchema() const;

	bool TryLoad(const char* fileName, const GameConfigLoadParameters<DataContext>& parameters = {}) const;

private:
	struct LoadContext
	{
		const GameConfigLoadParameters<DataContext>& Parameters;
		GameConfigIncludeStack& IncludeStack;
		int Depth = 1;
	};

	bool TryLoadCore(LoadContext& loadContext, const char* fileName) const;

	void ParseConfig(LoadContext& loadContext, std::string_view configFileName, const json& input) const;

	void ParseIncludedFiles(LoadContext& loadContext, const json& input) const;

	void ParseSections(LoadContext& loadContext, std::string_view configFileName, const json& input) const;

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::string m_Name;
	std::vector<std::unique_ptr<const GameConfigSection<DataContext>>> m_Sections;
};

/**
*	@brief Loads JSON-based configuration files and processes them according to a provided definition.
*/
class GameConfigSystem final : public IGameSystem
{
public:
	GameConfigSystem() = default;
	~GameConfigSystem() = default;

	const char* GetName() const override { return "GameConfig"; }

	bool Initialize() override;
	void PostInitialize() override {}
	void Shutdown() override;

	std::shared_ptr<spdlog::logger> GetLogger() { return m_Logger; }

	template<typename DataContext>
	std::shared_ptr<const GameConfigDefinition<DataContext>> CreateDefinition(
		std::string&& name, std::vector<std::unique_ptr<const GameConfigSection<DataContext>>>&& sections);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
};

inline GameConfigSystem g_GameConfigSystem;

template <typename DataContext>
inline const GameConfigSection<DataContext>* GameConfigDefinition<DataContext>::FindSection(std::string_view name) const
{
	if (auto it = std::find_if(m_Sections.begin(), m_Sections.end(), [&](const auto& section)
			{ return section->GetName() == name; });
		it != m_Sections.end())
	{
		return it->get();
	}

	return nullptr;
}

template<typename DataContext>
inline std::string GameConfigDefinition<DataContext>::GetSchema() const
{
	using namespace std::literals;

	// A configuration is structured like this:
	/*
	{
		"Includes": [
			"some_filename.json"
		],
		"Sections": [
			{
				"Name": "SectionName",
				Section-specific properties here
			}
		]
	}
	*/

	// This structure allows for future expansion without making breaking changes

	const auto sections = [this]()
	{
		std::string buffer;

		bool first = true;

		// Each section is comprised of a reserved key "Name" whose value is required to be the name of the section,
		// and keys specified by each section
		for (const auto& section : m_Sections)
		{
			if (!first)
			{
				buffer += ',';
			}

			first = false;

			const auto [definition, required] = section->GetSchema();

			buffer += fmt::format(R"(
{{
	"title": "{0} Section",
	"type": "object",
	"properties": {{
		"Name": {{
			"type": "string",
			"const": "{0}",
			"default": "{0}",
			"readOnly": true
		}},
		"Condition": {{
			"description": "If specified, this section will only be used if the condition evaluates true",
			"type": "string"
		}},
		{1}
	}},
	"required": [
		"Name"{2}
		{3}
	]
}}
)",
				section->GetName(), definition, required.empty() ? ""sv : ","sv, required);
		}

		return buffer;
	}();

	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "{} Game Configuration",
	"type": "object",
	"properties": {{
		"Includes": {{
			"title": "Included Files",
			"description": "List of JSON files to include before this file.",
			"type": "array",
			"items": {{
				"title": "Include",
				"type": "string",
				"pattern": "^[\\w/]+.json$"
			}}
		}},
		"Sections": {{
			"title": "Sections",
			"type": "array",
			"items": {{
				"anyOf": [
					{}
				]
			}}
		}}
	}}
}}
)",
		this->GetName(), sections);
}

template<typename DataContext>
bool GameConfigDefinition<DataContext>::TryLoad(const char* fileName, const GameConfigLoadParameters<DataContext>& parameters) const
{
	GameConfigIncludeStack includeStack;

	// So you can't get a stack overflow including yourself over and over.
	includeStack.Add(fileName);

	LoadContext loadContext{
		.Parameters = parameters,
		.IncludeStack = includeStack};

	return TryLoadCore(loadContext, fileName);
}

template <typename DataContext>
bool GameConfigDefinition<DataContext>::TryLoadCore(LoadContext& loadContext, const char* fileName) const
{
	m_Logger->trace("Loading \"{}\" configuration file \"{}\" (depth {})", GetName(), fileName, loadContext.Depth);

	auto result = g_JSON.ParseJSONFile(
		fileName,
		{.SchemaName = GetName(), .PathID = loadContext.Parameters.PathID},
		[&, this](const json& input)
		{
			ParseConfig(loadContext, fileName, input);
			return true; // Just return something so optional doesn't complain.
		});

	if (result.has_value())
	{
		m_Logger->trace("Successfully loaded configuration file");
	}
	else
	{
		m_Logger->trace("Failed to load configuration file");
	}

	return result.has_value();
}

template <typename DataContext>
void GameConfigDefinition<DataContext>::ParseConfig(LoadContext& loadContext, std::string_view configFileName, const json& input) const
{
	if (input.is_object())
	{
		ParseIncludedFiles(loadContext, input);
		ParseSections(loadContext, configFileName, input);
	}
}

template <typename DataContext>
void GameConfigDefinition<DataContext>::ParseIncludedFiles(LoadContext& loadContext, const json& input) const
{
	if (auto includes = input.find("Includes"); includes != input.end())
	{
		if (!includes->is_array())
		{
			return;
		}

		for (const auto& include : *includes)
		{
			if (!include.is_string())
			{
				continue;
			}

			auto includeFileName = include.get<std::string>();

			includeFileName = Trim(includeFileName);

			switch (loadContext.IncludeStack.Add(includeFileName.c_str()))
			{
			case IncludeAddResult::Success:
			{
				m_Logger->trace("Including file \"{}\"", includeFileName);

				++loadContext.Depth;

				TryLoadCore(loadContext, includeFileName.c_str());

				--loadContext.Depth;
				break;
			}

			case IncludeAddResult::AlreadyIncluded:
			{
				m_Logger->debug("Included file \"{}\" already included before", includeFileName);
				break;
			}


			case IncludeAddResult::CouldNotResolvePath:
			{
				m_Logger->error("Couldn't resolve path for included configuration file \"{}\"", includeFileName);
				break;
			}
			}
		}
	}
}

template <typename DataContext>
void GameConfigDefinition<DataContext>::ParseSections(LoadContext& loadContext, std::string_view configFileName, const json& input) const
{
	using namespace std::literals;

	if (auto sections = input.find("Sections"); sections != input.end())
	{
		if (!sections->is_array())
		{
			return;
		}

		for (const auto& section : *sections)
		{
			if (!section.is_object())
			{
				continue;
			}

			if (const auto name = section.value("Name", ""sv); !name.empty())
			{
				if (const auto sectionObj = FindSection(name); sectionObj)
				{
					if (const auto condition = section.value("Condition", ""sv); !condition.empty())
					{
						m_Logger->trace("Testing section \"{}\" condition \"{}\"", sectionObj->GetName(), condition);

						const auto shouldUseSection = g_ConditionEvaluator.Evaluate(condition);

						if (!shouldUseSection.has_value())
						{
							// Error already reported
							m_Logger->trace("Skipping section because an error occurred while evaluating condition");
							continue;
						}

						if (!shouldUseSection.value())
						{
							m_Logger->trace("Skipping section because condition evaluated false");
							continue;
						}
						else
						{
							m_Logger->trace("Using section because condition evaluated true");
						}
					}

					GameConfigContext<DataContext> context{
						.ConfigFileName = configFileName,
						.Input = section,
						.Definition = *this,
						.Logger = *m_Logger,
						.Data = loadContext.Parameters.Data};

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
}

template <typename DataContext>
inline std::shared_ptr<const GameConfigDefinition<DataContext>> GameConfigSystem::CreateDefinition(
	std::string&& name, std::vector<std::unique_ptr<const GameConfigSection<DataContext>>>&& sections)
{
	auto definition = std::make_shared<const GameConfigDefinition<DataContext>>(m_Logger, std::move(name), std::move(sections));

	g_JSON.RegisterSchema(std::string{definition->GetName()}, [=]()
		{ return definition->GetSchema(); });

	return definition;
}
