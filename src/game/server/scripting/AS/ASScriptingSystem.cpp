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

#include "cbase.h"
#include "ASScriptingSystem.h"
#include "ConCommandSystem.h"
#include "EventSystem.h"
#include "ModuleLoader.h"
#include "Scheduler.h"
#include "ScriptConsoleCommands.h"
#include "ScriptingAPI.h"
#include "ScriptingConstants.h"
#include "ScriptPluginList.h"
#include "JSONSystem.h"

#include "CustomEntities/CustomEntityDictionary.h"
#include "CustomEntities/CustomEntityScriptClasses.h"

#include "ScriptAPI/Events.h"

#include "scripting/AS/ASManager.h"

namespace scripting
{
ASScriptingSystem::ASScriptingSystem() = default;
ASScriptingSystem::~ASScriptingSystem() = default;

bool ASScriptingSystem::Initialize()
{
	g_JSON.RegisterSchema(PluginListFileSchemaName, &GetPluginListFileSchema);

	m_Logger = g_Logging.CreateLogger("scripting");

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

	RegisterSDKScriptingAPI(*m_ScriptEngine);

	m_EventSystem = std::make_shared<EventSystem>(m_Logger, m_ScriptEngine.get(), m_ScriptContext.get());

	RegisterEventSystem(*m_ScriptEngine, *m_EventSystem);
	RegisterEventTypes(*m_ScriptEngine, *m_EventSystem);

	m_Scheduler = std::make_shared<Scheduler>(m_Logger);

	RegisterScheduler(*m_ScriptEngine, m_Scheduler.get());

	m_ModuleLoader = std::make_unique<ModuleLoader>(m_Logger, m_ScriptEngine.get());
	m_ScriptPlugins = std::make_unique<ScriptPluginList>(m_Logger, m_ScriptContext.get(), m_ModuleLoader.get());

	m_CustomEntityScriptClasses = std::make_shared<CustomEntityScriptClasses>(m_Logger, m_ScriptEngine.get(), m_ModuleLoader.get());
	m_CustomEntityDictionary = std::make_shared<CustomEntityDictionary>(m_Logger,
		m_ScriptEngine.get(), m_ScriptContext.get(), m_CustomEntityScriptClasses.get());

	m_ScriptPlugins->AddListener(m_EventSystem);
	m_ScriptPlugins->AddListener(m_Scheduler);
	m_ScriptPlugins->AddListener(m_CustomEntityScriptClasses);
	m_ScriptPlugins->AddListener(m_CustomEntityDictionary);
	m_ScriptPlugins->AddListener(g_ScriptConCommands);

	m_PluginListFileCVar = g_ConCommands.CreateCVar("script_plugin_list_file", "default_plugins.json");

	g_ConCommands.CreateCommand("script_reload_plugins", [this](const auto&)
		{ ReloadPlugins(); });

	g_ConCommands.CreateCommand("script_list_plugins", [this](const auto&)
		{ m_ScriptPlugins->ListPlugins(); });

	g_ConCommands.CreateCommand("script_write_custom_entity_baseclasses", [this](const auto&)
		{
			if (m_CustomEntityScriptClasses)
			{
				m_CustomEntityScriptClasses->WriteBaseClassesToFile(CustomEntityBaseClassesFileName);
			}
		});

	LoadPlugins();

	return true;
}

void ASScriptingSystem::Shutdown()
{
	if (m_ScriptPlugins)
	{
		m_ScriptPlugins->RemoveListener(m_EventSystem);
	}

	m_ScriptPlugins.reset();
	m_ModuleLoader.reset();

	m_CustomEntityDictionary.reset();
	m_CustomEntityScriptClasses.reset();

	m_Scheduler.reset();
	m_EventSystem.reset();

	m_ScriptContext.reset();
	m_ScriptEngine.reset();
	
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

void ASScriptingSystem::NewMapStarted()
{
	m_Logger->debug("Removing all map plugins");
	m_ScriptPlugins->RemoveAllOfType(ModuleType::Map);
}

void ASScriptingSystem::PostNewMapStarted()
{
	// TODO: is time correct when loading save games?
	m_Scheduler->SetCurrentTime(gpGlobals->time);

	// Adjust callback times to use the new time base.
	m_Scheduler->AdjustExecutionTimes(gpGlobals->time - m_PreviousGameTime);

	// Let scripts do map-specific stuff.
	m_EventSystem->Publish<MapInitEventArgs>();
}

void ASScriptingSystem::RunFrame()
{
	m_PreviousGameTime = gpGlobals->time;
	m_Scheduler->SetCurrentTime(gpGlobals->time);
	m_Scheduler->Think(gpGlobals->time, *m_ScriptContext);
}

const ScriptPlugin* ASScriptingSystem::GetMapPlugin() const
{
	return m_ScriptPlugins->FindPluginByName(ModuleType::Map, MapPluginName);
}

void ASScriptingSystem::LoadPlugins()
{
	if (m_PluginListFileCVar->string[0] != '\0')
	{
		m_ScriptPlugins->LoadConfigFile(m_PluginListFileCVar->string);
	}
}

void ASScriptingSystem::ReloadPlugins()
{
	// TODO: if we're in the middle of a map then this will probably fail due to references from native code to script objects.
	// Either a way to clear all of those is needed or this needs to be delayed until a map change.
	m_ScriptPlugins->RemoveAll();
	LoadPlugins();
}
}
