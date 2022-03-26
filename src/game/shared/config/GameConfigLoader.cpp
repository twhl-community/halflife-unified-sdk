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

#include <chrono>
#include <sstream>

#include <spdlog/fmt/fmt.h>

#include <angelscript.h>

#include "cbase.h"

#include "GameConfigDefinition.h"
#include "GameConfigIncludeStack.h"
#include "GameConfigLoader.h"
#include "GameConfigSection.h"

#include "scripting/AS/CASManager.h"

#include "utils/json_utils.h"
#include "utils/string_utils.h"

using namespace std::literals;

/**
*	@brief Amount of time that a conditional evaluation can take before timing out.
*/
constexpr std::chrono::seconds ConditionalEvaluationTimeout{1};

//Provides access to the constructor.
struct GameConfigDefinitionImplementation : public GameConfigDefinition
{
	GameConfigDefinitionImplementation(std::string&& name, std::vector<std::unique_ptr<const GameConfigSection>>&& sections)
		: GameConfigDefinition(std::move(name), std::move(sections))
	{
	}
};

struct TimeoutHandler
{
	using Clock = std::chrono::high_resolution_clock;

	void OnLineCallback(asIScriptContext* context)
	{
		if (Timeout <= Clock::now())
		{
			context->Abort();
		}
	}

	const Clock::time_point Timeout;
};

GameConfigLoader::GameConfigLoader() = default;
GameConfigLoader::~GameConfigLoader() = default;

bool GameConfigLoader::Initialize()
{
	m_Logger = g_Logging.CreateLogger("gamecfg");

	m_ScriptEngine = g_ASManager.CreateEngine();

	if (!m_ScriptEngine)
	{
		return false;
	}

	m_ScriptContext = g_ASManager.CreateContext(*m_ScriptEngine);

	if (!m_ScriptContext)
	{
		return false;
	}

	RegisterGameConfigConditionalsScriptAPI(*m_ScriptEngine, m_Conditionals);

	return true;
}

void GameConfigLoader::Shutdown()
{
	m_ScriptContext.reset();
	m_ScriptEngine.reset();
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
	GameConfigIncludeStack includeStack;
	GameConfig config;

	//So you can't get a stack overflow including yourself over and over.
	includeStack.Add(fileName);

	LoadContext loadContext{
		.Definition = definition,
		.Parameters = parameters,
		.IncludeStack = includeStack,
		.Config = config};

	if (TryLoadCore(loadContext, fileName))
	{
		return config;
	}

	return {};
}

std::optional<bool> GameConfigLoader::EvaluateConditional(std::string_view conditional)
{
	//Create a temporary module to evaluate the conditional with
	const as::ModulePtr module{g_ASManager.CreateModule(*m_ScriptEngine, "gamecfg_conditional")};

	if (!module)
		return {};

	//Wrap the conditional in a function we can call
	{
		const auto script{fmt::format("bool Evaluate() {{ return {}; }}", conditional)};

		//Engine message callback reports any errors
		const int addResult = module->AddScriptSection("conditional", script.c_str(), script.size());

		if (!g_ASManager.HandleAddScriptSectionResult(addResult, module->GetName(), "conditional"))
			return {};
	}

	if (!g_ASManager.HandleBuildResult(module->Build(), module->GetName()))
		return {};

	const auto function = module->GetFunctionByName("Evaluate");

	if (!g_ASManager.PrepareContext(*m_ScriptContext, function))
		return {};

	//Since this is expected to run very quickly, use a line callback to handle timeout to prevent infinite loops from locking up the game
	TimeoutHandler timeoutHandler{TimeoutHandler::Clock::now() + ConditionalEvaluationTimeout};

	m_ScriptContext->SetLineCallback(asMETHOD(TimeoutHandler, OnLineCallback), &timeoutHandler, asCALL_THISCALL);

	//Clear line callback so there's no dangling reference left in the context
	struct Cleanup
	{
		~Cleanup()
		{
			Context.ClearLineCallback();
		}

		asIScriptContext& Context;
	} cleanup{*m_ScriptContext};

	if (!g_ASManager.ExecuteContext(*m_ScriptContext))
		return {};

	const auto result = m_ScriptContext->GetReturnByte();

	return result != 0;
}

bool GameConfigLoader::TryLoadCore(LoadContext& loadContext, const char* fileName)
{
	m_Logger->trace("Loading \"{}\" configuration file \"{}\" (depth {})", loadContext.Definition.GetName(), fileName, loadContext.Depth);

	auto result = g_JSON.ParseJSONFile(
		fileName,
		{.SchemaName = loadContext.Definition.GetName(), .PathID = loadContext.Parameters.PathID},
		[&, this](const json& input)
		{
			ParseConfig(loadContext, fileName, input);
			return true; //Just return something so optional doesn't complain.
		});

	if (result.has_value())
	{
		m_Logger->trace("Successfully loaded configuration file");
	}
	else
	{
		m_Logger->trace("Failed to load configuration file");
	}

	//Free up resources used by the context and engine
	g_ASManager.UnprepareContext(*m_ScriptContext);
	m_ScriptEngine->GarbageCollect();

	return result.has_value();
}

void GameConfigLoader::ParseConfig(LoadContext& loadContext, std::string_view configFileName, const json& input)
{
	if (input.is_object())
	{
		ParseIncludedFiles(loadContext, input);
		ParseSections(loadContext, configFileName, input);
	}
}

void GameConfigLoader::ParseIncludedFiles(LoadContext& loadContext, const json& input)
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

			if (loadContext.IncludeStack.Add(includeFileName))
			{
				m_Logger->trace("Including file \"{}\"", includeFileName);

				++loadContext.Depth;

				TryLoadCore(loadContext, includeFileName.c_str());

				--loadContext.Depth;
			}
			else
			{
				m_Logger->debug("Included file \"{}\" already included before", includeFileName);
			}
		}
	}
}

void GameConfigLoader::ParseSections(LoadContext& loadContext, std::string_view configFileName, const json& input)
{
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
				if (const auto sectionObj = loadContext.Definition.FindSection(name); sectionObj)
				{
					if (const auto condition = section.value("Condition", ""sv); !condition.empty())
					{
						m_Logger->trace("Testing section \"{}\" condition \"{}\"", sectionObj->GetName(), condition);

						const auto shouldUseSection = EvaluateConditional(condition);

						if (!shouldUseSection.has_value())
						{
							//Error already reported
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

					GameConfigContext context{
						.ConfigFileName = configFileName,
						.Input = section,
						.Definition = loadContext.Definition,
						.Loader = *this,
						.Configuration = loadContext.Config};

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
