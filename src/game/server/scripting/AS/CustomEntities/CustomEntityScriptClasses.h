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

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>

#include <spdlog/logger.h>

#include <angelscript.h>

#include "scripting/AS/as_utils.h"
#include "scripting/AS/ScriptPluginList.h"

class CBaseEntity;

namespace scripting
{
class ModuleLoader;

/**
 *	@brief Creates the custom entity script base classes module, handles external declarations and provides the list of wrapper classes.
 */
class CustomEntityScriptClasses final : public IPluginListener
{
public:
	CustomEntityScriptClasses(std::shared_ptr<spdlog::logger> logger, asIScriptEngine* engine, ModuleLoader* moduleLoader);

	CustomEntityScriptClasses(const CustomEntityScriptClasses&) = delete;
	CustomEntityScriptClasses& operator=(const CustomEntityScriptClasses&) = delete;

	asIScriptModule* GetModule() { return m_BaseClassesModule.get(); }

	const std::function<std::unique_ptr<CBaseEntity>()>* FindWrapperFactory(std::string_view scriptBaseClassName) const;

	void WriteBaseClassesToFile(const char* fileName) const;

	void OnModuleInitializing(std::string& codeToAdd) override;

private:
	static std::string CreateScriptClassBaseClass();

	static std::string CreateScriptClassDeclaration(
		std::string_view scriptClassName, std::string_view cppClassName, std::string_view baseClassName, std::string_view additionalMembers);

	static std::tuple<std::string, std::string> GenerateContents();

private:
	const std::shared_ptr<spdlog::logger> m_Logger;

	as::ModulePtr m_BaseClassesModule;

	std::string m_ExternalDeclarations;
};

void RegisterCustomEntityInterface(asIScriptEngine& engine);
}
