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
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <spdlog/logger.h>

#include <angelscript.h>

#include "ModuleLoader.h"
#include "ScriptingConstants.h"

namespace scripting
{
std::string GetPluginListFileScriptsSchema();
std::string GetPluginListFileSchema();

struct IPluginListener
{
	virtual ~IPluginListener() = default;

	virtual void OnModuleInitializing(std::string& codeToAdd) {}

	virtual void OnModuleDestroying(asIScriptModule& module) {}
};

struct ScriptPlugin
{
	const ModuleType Type;
	std::vector<as::ModulePtr> Modules;
};

/**
 *	@brief Manages the list of script plugins, loads them from a config file, handles reloading.
 */
class ScriptPluginList final
{
public:
	ScriptPluginList(std::shared_ptr<spdlog::logger> logger, asIScriptContext* context, ModuleLoader* moduleLoader);
	~ScriptPluginList();

	ScriptPluginList(const ScriptPluginList&) = delete;
	ScriptPluginList& operator=(const ScriptPluginList&) = delete;

	const ScriptPlugin* FindPluginByName(ModuleType type, std::string_view pluginName) const;

	void LoadConfigFile(const char* configFileName);

	bool CreatePluginFromConfig(ModuleType type, std::string_view pluginName, const json& input);
	
	void AddScriptToPlugin(ModuleType type, std::string_view pluginName, std::string_view scriptFileName);

	void RemoveAllOfType(ModuleType type);

	void RemoveAll();

	void AddListener(std::shared_ptr<IPluginListener> listener);

	void RemoveListener(std::shared_ptr<IPluginListener> listener);

	void ListPlugins();

private:
	void Create(ModuleType type, std::string_view pluginName, std::span<std::string_view> scriptFileNames);

	as::ModulePtr CreateModuleFromScript(ModuleType type, std::string_view pluginName, std::string_view scriptFileName);

	void RemoveModules(std::vector<as::ModulePtr>& modules);

	bool ParseConfigFile(const json& input);

	bool CallVoidFunction(asIScriptModule& module, const char* functionName, bool isOptional);

	template<typename Function, typename... Args>
	void NotifyListeners(Function&& function, Args&&... args);

private:
	const std::shared_ptr<spdlog::logger> m_Logger;
	asIScriptContext* const m_Context;
	ModuleLoader* const m_ModuleLoader;

	std::unordered_map<std::string, std::unique_ptr<ScriptPlugin>> m_Plugins;

	std::vector<std::weak_ptr<IPluginListener>> m_Listeners;
};
}
