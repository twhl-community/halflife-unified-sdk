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

#include <bit>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/logger.h>

#include <angelscript.h>

#include "cbase.h"
#include "ScriptingConstants.h"
#include "scripting/AS/as_utils.h"

class CScriptBuilder;

namespace scripting
{
/**
 *	@brief Loads Angelscript modules.
 */
class ModuleLoader final
{
public:
	ModuleLoader(std::shared_ptr<spdlog::logger> logger, asIScriptEngine* engine);

	ModuleLoader(const ModuleLoader&) = delete;
	ModuleLoader& operator=(const ModuleLoader&) = delete;

	/**
	 *	@brief Creates a module of the given type with the name @c "type:scope:scriptFileName" and adds the given script to it,
	 *	including any scripts referenced by @c #include statements.
	 *	@return If the module was successfully created, returns the module. Otherwise, returns @c nullptr.
	 */
	as::ModulePtr Load(
		ModuleType type, std::string_view scope, std::string_view scriptFileName, std::string_view generatedCode = {});

	/**
	 *	@brief Creates a precompiled module that can be used to add shared script types to all modules.
	 *	@return If the module was successfully created, returns the module. Otherwise, returns @c nullptr.
	 */
	as::ModulePtr CreatePrecompiledModule(std::string_view scope, std::string_view contents);

	Filename GetAbsoluteScriptFileName(std::string_view scriptFileName);

private:
	std::string_view ValidateInput(std::string_view value, std::string_view parameterName, bool allowAnyCharacter);

	as::ModulePtr CreateCore(
		const std::string& moduleName, const Filename& absoluteFilename, std::string_view generatedCode);

private:
	const std::shared_ptr<spdlog::logger> m_Logger;
	asIScriptEngine* const m_Engine;
};
}
