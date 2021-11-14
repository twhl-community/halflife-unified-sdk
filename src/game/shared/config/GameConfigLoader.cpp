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

#include "extdll.h"
#include "util.h"

#include "GameConfigDefinition.h"
#include "GameConfigLoader.h"
#include "GameConfigSection.h"

#include "scripting/AS/CASManager.h"

#include "utils/json_utils.h"

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
	m_Logger = g_Logging->CreateLogger("gamecfg");

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

	//Free up resources used by the context and engine
	g_ASManager.UnprepareContext(*m_ScriptContext);
	m_ScriptEngine->GarbageCollect();

	return config;
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
