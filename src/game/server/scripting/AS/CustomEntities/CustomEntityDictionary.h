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
#include <string>
#include <string_view>
#include <unordered_map>

#include <spdlog/logger.h>

#include <angelscript.h>

#include "heterogeneous_lookup.h"

#include "scripting/AS/as_utils.h"

#include "scripting/AS/ScriptPluginList.h"

class CBaseEntity;

namespace scripting
{
class CustomEntityScriptClasses;

/**
*	@brief Dictionary of custom entity classnames to type info and constructor functions.
*/
class CustomEntityDictionary final : public IPluginListener
{
private:
	struct CustomTypeInfo
	{
		const std::function<std::unique_ptr<CBaseEntity>()>* WrapperFactory{};
		as::UniquePtr<asITypeInfo> Type;
		as::UniquePtr<asIScriptFunction> Constructor;
	};

public:
	explicit CustomEntityDictionary(std::shared_ptr<spdlog::logger> logger,
		asIScriptEngine* engine, asIScriptContext* context, CustomEntityScriptClasses* customEntityScriptClasses);

	CustomEntityDictionary(const CustomEntityDictionary&) = delete;
	CustomEntityDictionary& operator=(const CustomEntityDictionary&) = delete;

	bool RegisterType(const std::string& className, const std::string& typeName);

	std::unique_ptr<CBaseEntity> TryCreateCustomEntity(const char* className, entvars_t* pev);

	void RemoveAllFromModule(asIScriptModule& module);

	void RemoveAll();

	void OnModuleDestroying(asIScriptModule& module) override;

private:
	bool RegisterTypeCore(const std::string& className, const std::string& typeName, asIScriptContext& context, asIScriptModule& module);

private:
	const std::shared_ptr<spdlog::logger> m_Logger;
	asIScriptContext* const m_Context;
	CustomEntityScriptClasses* const m_CustomEntityScriptClasses;
	as::UniquePtr<asITypeInfo> m_BaseClassType;
	std::unordered_map<std::string, CustomTypeInfo, TransparentStringHash, TransparentEqual> m_Dictionary;
};

void RegisterCustomEntityDictionary(asIScriptEngine& engine, CustomEntityDictionary* customEntityDictionary);
}
