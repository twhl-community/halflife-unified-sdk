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

#include <string_view>

#include <angelscript.h>

namespace scripting
{
enum class UserDataId : asPWORD
{
	// See Angelscript documentation for why these are reserved.
	AddonReservedStart = 1000,
	AddonReservedEnd = 1999,
};

template <typename T>
inline T GetModuleUserData(const asIScriptModule& module, UserDataId id)
{
	return static_cast<T>(reinterpret_cast<std::uintptr_t>(module.GetUserData(static_cast<asPWORD>(id))));
}

enum class ModuleType : std::uintptr_t
{
	Map = 0,
	Plugin
};

std::string_view ModuleTypeToString(ModuleType type);

constexpr std::string_view PluginListFileSchemaName{"ScriptingPluginListFile"};

/**
 *	@brief The directory that all scripts are located in.
 *	They can be in a subdirectory of this directory.
 */
constexpr std::string_view BaseScriptsDirectory{"scripts/"};

constexpr char ModuleInitializationFunctionName[]{"ScriptInit"};
constexpr char ModuleShutdownFunctionName[]{"ScriptShutdown"};

constexpr char PrecompiledModuleSectionName[]{"PrecompiledSection"};

constexpr char MapPluginName[]{"MapPlugin"};

constexpr char CustomEntityBaseClassesFileName[]{"script_custom_baseclasses.txt"};
}
