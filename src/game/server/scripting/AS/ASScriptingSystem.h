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

#include <spdlog/logger.h>

#include <angelscript.h>

#include "GameSystem.h"

#include "scripting/AS/as_utils.h"

namespace scripting
{
class CustomEntityDictionary;
class CustomEntityScriptClasses;
class EventSystem;
class ModuleLoader;
class Scheduler;
struct ScriptPlugin;
class ScriptPluginList;

/**
 *	@brief Handles scripting functionality for maps and plugins.
 */
class ASScriptingSystem final : public IGameSystem
{
public:
	ASScriptingSystem();
	~ASScriptingSystem() override;

	ASScriptingSystem(const ASScriptingSystem&) = delete;
	ASScriptingSystem& operator=(const ASScriptingSystem&) = delete;

	const char* GetName() const override { return "Scripting"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override;

	void NewMapStarted();

	void PostNewMapStarted();

	void RunFrame();

	spdlog::logger* GetLogger() { return m_Logger.get(); }

	asIScriptEngine* GetEngine() { return m_ScriptEngine.get(); }

	asIScriptContext* GetContext() { return m_ScriptContext.get(); }

	EventSystem* GetEventSystem() { return m_EventSystem.get(); }

	CustomEntityDictionary* GetCustomEntityDictionary() { return m_CustomEntityDictionary.get(); }

	ScriptPluginList* GetScriptPlugins() { return m_ScriptPlugins.get(); }

	const ScriptPlugin* GetMapPlugin() const;

private:
	void LoadPlugins();
	void ReloadPlugins();

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	as::EnginePtr m_ScriptEngine;
	as::UniquePtr<asIScriptContext> m_ScriptContext;

	std::shared_ptr<EventSystem> m_EventSystem;
	std::shared_ptr<Scheduler> m_Scheduler;

	std::unique_ptr<ModuleLoader> m_ModuleLoader;
	std::unique_ptr<ScriptPluginList> m_ScriptPlugins;

	std::shared_ptr<CustomEntityScriptClasses> m_CustomEntityScriptClasses;
	std::shared_ptr<CustomEntityDictionary> m_CustomEntityDictionary;

	cvar_t* m_PluginListFileCVar{};

	float m_PreviousGameTime{1};
};

inline ASScriptingSystem g_Scripting;
}
