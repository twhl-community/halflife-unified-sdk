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

#include <cassert>

#include "cbase.h"
#include "ModuleLoader.h"
#include "ScriptPluginList.h"

#include "JSONSystem.h"

#include "scripting/AS/ASGenericCall.h"
#include "scripting/AS/ASManager.h"

namespace scripting
{
std::string GetPluginListFileScriptsSchema()
{
	return fmt::format(R"(
"Scripts": {{
	"title": "Scripts",
	"type": "array",
	"items": {{
		"title": "Script",
		"type": "string",
		"pattern": "^[\\w\\d/\\._\\-]+$"
	}}
}}
)");
}

std::string GetPluginListFileSchema()
{
	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Scripting Plugin List File Configuration",
	"type": "array",
	"items": {{
		"title": "Plugins",
		"type": "object",
		"properties": {{
			"Name": {{
				"title": "Plugin Name",
				"type": "string",
				"pattern": "^[\\w]+$"
			}},
			{}
		}},
		"required": [
			"Name",
			"Scripts"
		]
	}}
}}
)",
		GetPluginListFileScriptsSchema());
}

ScriptPluginList::ScriptPluginList(std::shared_ptr<spdlog::logger> logger, asIScriptContext* context, ModuleLoader* moduleLoader)
	: m_Logger(logger), m_Context(context), m_ModuleLoader(moduleLoader)
{
	assert(m_Logger);
	assert(m_Context);
	assert(m_ModuleLoader);
}

ScriptPluginList::~ScriptPluginList()
{
	RemoveAll();
}

static std::string CreatePluginName(ModuleType type, const std::string& pluginName)
{
	return ToLower(std::string{ModuleTypeToString(type)} + ':' + pluginName);
}

const ScriptPlugin* ScriptPluginList::FindPluginByName(ModuleType type, std::string_view pluginName) const
{
	const auto loweredPluginName{CreatePluginName(type, std::string{pluginName})};

	if (auto it = m_Plugins.find(loweredPluginName); it != m_Plugins.end())
	{
		return it->second.get();
	}

	return nullptr;
}

void ScriptPluginList::LoadConfigFile(const char* configFileName)
{
	g_JSON.ParseJSONFile(configFileName, {.SchemaName = PluginListFileSchemaName, .PathID = "GAMECONFIG"}, [this](const json& input)
		{ return ParseConfigFile(input); });
}

bool ScriptPluginList::CreatePluginFromConfig(ModuleType type, std::string_view pluginName, const json& input)
{
	if (auto scripts = input.find("Scripts"); scripts != input.end() && scripts->is_array())
	{
		std::vector<std::string> scriptFileNames;
		scriptFileNames.reserve(scripts->size());

		for (const auto& scriptFileName : *scripts)
		{
			if (!scriptFileName.is_string())
			{
				continue;
			}

			scriptFileNames.push_back(scriptFileName.get<std::string>());
		}

		std::vector<std::string_view> scriptFileNameViews;
		scriptFileNameViews.reserve(scriptFileNames.size());

		std::transform(scriptFileNames.begin(), scriptFileNames.end(), std::back_inserter(scriptFileNameViews),
			[](const auto& str)
			{ return std::string_view{str}; });

		Create(type, pluginName, scriptFileNameViews);

		return true;
	}

	return false;
}

void ScriptPluginList::AddScriptToPlugin(ModuleType type, std::string_view pluginName, std::string_view scriptFileName)
{
	m_Logger->trace("Adding script \"{}\" to plugin \"{}\"", scriptFileName, pluginName);

	const auto loweredPluginName{CreatePluginName(type, std::string{pluginName})};

	auto it = m_Plugins.find(loweredPluginName);

	if (it == m_Plugins.end())
	{
		m_Logger->error("Cannot add script to non-existent plugin \"{}\"", scriptFileName);
		return;
	}

	auto& plugin = *it->second;

	auto module = CreateModuleFromScript(plugin.Type, pluginName, scriptFileName);

	if (!module)
	{
		return;
	}

	if (!CallVoidFunction(*module, ModuleInitializationFunctionName, true))
	{
		m_Logger->error("Removing script \"{}\" from plugin \"{}\" due to error during module initialization", *module, pluginName);

		CallVoidFunction(*module, ModuleShutdownFunctionName, true);
		return;
	}

	plugin.Modules.push_back(std::move(module));

	m_Logger->debug("Added script \"{}\" to plugin \"{}\" ({} scripts total)", scriptFileName, pluginName, plugin.Modules.size());
}

void ScriptPluginList::RemoveAllOfType(ModuleType type)
{
	for (auto it = m_Plugins.begin(); it != m_Plugins.end();)
	{
		if (it->second->Type == type)
		{
			RemoveModules(it->second->Modules);
			it = m_Plugins.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void ScriptPluginList::RemoveAll()
{
	for (auto& plugin : m_Plugins)
	{
		RemoveModules(plugin.second->Modules);
	}

	m_Plugins.clear();
}

void ScriptPluginList::AddListener(std::shared_ptr<IPluginListener> listener)
{
	assert(listener);

	if (auto it = std::find_if(m_Listeners.begin(), m_Listeners.end(), [&](const auto& candidate)
			{ return candidate.lock() == listener; });
		it != m_Listeners.end())
	{
		return;
	}

	m_Listeners.push_back(listener);
}

void ScriptPluginList::RemoveListener(std::shared_ptr<IPluginListener> listener)
{
	assert(listener);

	if (auto it = std::find_if(m_Listeners.begin(), m_Listeners.end(), [&](const auto& candidate)
			{ return candidate.lock() == listener; });
		it != m_Listeners.end())
	{
		m_Listeners.erase(it);
	}
}

void ScriptPluginList::ListPlugins()
{
	Con_Printf("%u plugins (including map plugins)\n", m_Plugins.size());

	for (const auto& [name, plugin] : m_Plugins)
	{
		Con_Printf("%s: %u scripts:\n", name.c_str(), plugin->Modules.size());

		for (const auto& module : plugin->Modules)
		{
			Con_Printf("\t%s\n", module->GetName());
		}
	}
}

void ScriptPluginList::Create(ModuleType type, std::string_view pluginName, std::span<std::string_view> scriptFileNames)
{
	m_Logger->trace("Creating plugin \"{}\" with {} scripts", pluginName, scriptFileNames.size());

	auto loweredPluginName{CreatePluginName(type, std::string{pluginName})};

	if (m_Plugins.contains(loweredPluginName))
	{
		m_Logger->error("A plugin with name \"{}\" already exists", pluginName);
		return;
	}

	std::vector<as::ModulePtr> modules;

	modules.reserve(scriptFileNames.size());

	const CallOnDestroy cleanup{[&, this]()
		{
			// Remove the modules that were loaded.
			// This list will be empty if the plugin was successfully created.
			RemoveModules(modules);
		}};

	bool hasErrors = false;

	for (const auto& scriptFileName : scriptFileNames)
	{
		auto module = CreateModuleFromScript(type, pluginName, scriptFileName);

		if (!module)
		{
			hasErrors = true;
			continue;
		}

		modules.push_back(std::move(module));
	}

	if (hasErrors)
	{
		m_Logger->error("Could not create plugin \"{}\" due to errors creating the modules", pluginName);
		return;
	}

	// Initialize all modules now that they've all been made.
	// They may access each-other indirectly during this process.
	for (auto& module : modules)
	{
		if (!CallVoidFunction(*module, ModuleInitializationFunctionName, true))
		{
			m_Logger->error("Removing plugin \"{}\" due to error during module \"{}\" initialization", pluginName, *module);
			return;
		}
	}

	const auto result = m_Plugins.insert_or_assign(std::move(loweredPluginName), std::make_unique<ScriptPlugin>(type, std::move(modules)));

	auto plugin = result.first->second.get();

	m_Logger->debug("Created plugin \"{}\" with {} scripts", pluginName, plugin->Modules.size());
}

as::ModulePtr ScriptPluginList::CreateModuleFromScript(ModuleType type, std::string_view pluginName, std::string_view scriptFileName)
{
	m_Logger->trace("Creating plugin script module with script \"{}\"", scriptFileName);

	std::string codeToAdd;
	NotifyListeners(&IPluginListener::OnModuleInitializing, codeToAdd);

	auto module = m_ModuleLoader->Load(type, pluginName, scriptFileName, codeToAdd);

	if (!module)
	{
		m_Logger->error("Skipping plugin script \"{}\" due to errors creating the module", scriptFileName);
		return {};
	}

	bool isValid = true;

	// Check to make sure these functions are not marked shared, since it breaks all modules otherwise.
	for (auto functionName : {ModuleInitializationFunctionName, ModuleShutdownFunctionName})
	{
		const auto completeFunctionName{fmt::format("void {}()", functionName)};

		auto function = module->GetFunctionByDecl(completeFunctionName.c_str());

		if (!function)
		{
			continue;
		}

		if (function->IsShared())
		{
			isValid = false;
			m_Logger->error("Skipping plugin script \"{}\" because it marked the function \"{}\" as shared, which is not allowed",
				scriptFileName, completeFunctionName);
		}
	}

	if (!isValid)
	{
		return {};
	}

	return module;
}

void ScriptPluginList::RemoveModules(std::vector<as::ModulePtr>& modules)
{
	// Remove modules in reverse order so the last script to initialize is the first to be destroyed.
	for (auto it = modules.rbegin(), end = modules.rend(); it != end; ++it)
	{
		auto& module = **it;

		m_Logger->trace("Removing module \"{}\"", module);

		CallVoidFunction(module, ModuleShutdownFunctionName, true);

		// Unprepare the context to release references to script functions.
		g_ASManager.UnprepareContext(*m_Context);

		NotifyListeners(&IPluginListener::OnModuleDestroying, module);

		it->reset();
	}

	modules.clear();
}

bool ScriptPluginList::ParseConfigFile(const json& input)
{
	if (!input.is_array())
	{
		return false;
	}

	for (const auto& plugin : input)
	{
		if (!plugin.is_object())
		{
			continue;
		}

		const auto pluginName = plugin.value("Name", ""sv);

		if (pluginName.empty())
		{
			continue;
		}

		CreatePluginFromConfig(ModuleType::Plugin, pluginName, plugin);
	}

	return true;
}

bool ScriptPluginList::CallVoidFunction(asIScriptModule& module, const char* functionName, bool isOptional)
{
	assert(functionName);

	const auto completeFunctionName{fmt::format("void {}()", functionName)};

	auto function = module.GetFunctionByDecl(completeFunctionName.c_str());

	if (!function)
	{
		if (isOptional)
		{
			return true;
		}

		m_Logger->error("Could not find function \"{}\" in module \"{}\"", completeFunctionName, module);
		return false;
	}

	return call::ExecuteFunction<call::ReturnVoid>(*m_Context, function).has_value();
}

template <typename Function, typename... Args>
void ScriptPluginList::NotifyListeners(Function&& function, Args&&... args)
{
	for (auto it = m_Listeners.begin(); it != m_Listeners.end();)
	{
		auto listener = it->lock();

		if (!listener)
		{
			m_Logger->warn("Plugin listener was destroyed without being removed first");
			it = m_Listeners.erase(it);
		}
		else
		{
			(listener.get()->*function)(std::forward<Args>(args)...);
			++it;
		}
	}
}
}
