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
#include <string>

#include <fmt/core.h>

#include <angelscript.h>

#include "cbase.h"
#include "CustomEntityConstants.h"
#include "CustomEntityScriptClasses.h"
#include "CustomEntityWrappers.h"

#include "scripting/AS/ASManager.h"
#include "scripting/AS/ModuleLoader.h"

namespace scripting
{
struct ScriptClassData
{
	const std::string_view ScriptClassName;
	const std::string_view CppClassName;
	const std::string_view BaseClassName;
	const std::function<std::unique_ptr<CBaseEntity>()> Factory;
	const std::string_view AdditionalMembers;
};

const ScriptClassData ScriptClassesData[] =
	{
		{"ScriptBaseEntity", "CBaseEntity", "BaseEntity", &CreateCustomEntityWrapper<CustomBaseEntity<CBaseEntity>>}};

CustomEntityScriptClasses::CustomEntityScriptClasses(std::shared_ptr<spdlog::logger> logger, asIScriptEngine* engine, ModuleLoader* moduleLoader)
	: m_Logger(logger)
{
	assert(m_Logger);
	assert(engine);
	assert(moduleLoader);

	RegisterCustomEntityInterface(*engine);

	auto result = GenerateContents();

	m_BaseClassesModule = moduleLoader->CreatePrecompiledModule(ScriptBaseClassSectionName, std::get<0>(result));
	m_ExternalDeclarations = std::move(std::get<1>(result));
}

const std::function<std::unique_ptr<CBaseEntity>()>* CustomEntityScriptClasses::FindWrapperFactory(std::string_view scriptBaseClassName) const
{
	for (const auto& data : ScriptClassesData)
	{
		if (data.ScriptClassName == scriptBaseClassName)
		{
			return &data.Factory;
		}
	}

	return nullptr;
}

void CustomEntityScriptClasses::WriteBaseClassesToFile(const char* fileName) const
{
	assert(fileName);

	if (FSFile file{fileName, "w", "GAMECONFIG"}; file)
	{
		m_Logger->info("Writing custom entity base classes to file \"{}\"", fileName);

		const auto [contents, externalDeclarations] = GenerateContents();

		file.Printf("// Script base class declarations\n");
		file.Printf("%s\n", contents.c_str());

		file.Printf("// External class declarations added to each plugin\n");
		file.Printf("%s\n", externalDeclarations.c_str());
	}
	else
	{
		m_Logger->error("Couldn't open file \"{}\" for writing", fileName);
	}
}

void CustomEntityScriptClasses::OnModuleInitializing(std::string& codeToAdd)
{
	codeToAdd += m_ExternalDeclarations;
}

std::string CustomEntityScriptClasses::CreateScriptClassBaseClass()
{
	return fmt::format(R"(
shared abstract class {} : {}
{{
	private CustomEntityCallbackHandler@ m_CallbackHandler;

	void SetThink(ThinkFunction@ function) final
	{{
		m_CallbackHandler.SetThinkFunction(function);
	}}

	void SetTouch(TouchFunction@ function) final
	{{
		m_CallbackHandler.SetTouchFunction(function);
	}}

	void SetBlocked(BlockedFunction@ function) final
	{{
		m_CallbackHandler.SetBlockedFunction(function);
	}}

	void SetUse(UseFunction@ function) final
	{{
		m_CallbackHandler.SetUseFunction(function);
	}}
}}
)",
		ScriptBaseClass, ScriptBaseClassInterface);
}

std::string CustomEntityScriptClasses::CreateScriptClassDeclaration(
	std::string_view scriptClassName, std::string_view cppClassName, std::string_view baseClassName, std::string_view additionalMembers)
{
	return fmt::format(R"(
shared abstract class {1} : {0}
{{
	private {2}@ m_Self = null;
	{2}@ self {{ get const final {{ return @m_Self; }} }}

	private {3}@ m_BaseClass;
	{3}@ BaseClass {{ get const final {{ return @m_BaseClass; }} }}

	{4}
}}
)",
		ScriptBaseClass, scriptClassName, cppClassName, baseClassName, additionalMembers);
}

std::tuple<std::string, std::string> CustomEntityScriptClasses::GenerateContents()
{
	std::string contents = CreateScriptClassBaseClass();

	std::string externalDeclarations = fmt::format("external shared class {};\n", ScriptBaseClass);

	for (const auto& data : ScriptClassesData)
	{
		contents += CreateScriptClassDeclaration(data.ScriptClassName, data.CppClassName, data.BaseClassName, data.AdditionalMembers);
		externalDeclarations += fmt::format("external shared class {};\n", data.ScriptClassName);
	}

	externalDeclarations.shrink_to_fit();

	return {std::move(contents), std::move(externalDeclarations)};
}

/**
*	@brief This is an interface exposed by custom entities only to change the various callback functions used by entities.
*/
static void RegisterCustomEntityCallbackHandler(asIScriptEngine& engine)
{
	engine.RegisterFuncdef("void ThinkFunction()");
	engine.RegisterFuncdef("void TouchFunction(CBaseEntity@ other)");
	engine.RegisterFuncdef("void BlockedFunction(CBaseEntity@ other)");
	engine.RegisterFuncdef("void UseFunction(CBaseEntity@ activator, CBaseEntity@ caller, USE_TYPE useType, float value)");

	const char* name = "CustomEntityCallbackHandler";

	engine.RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOCOUNT);

	engine.RegisterObjectMethod(name, "void SetThinkFunction(ThinkFunction@ function)",
		asMETHOD(ICustomEntity, SetThinkFunction), asCALL_THISCALL);

	engine.RegisterObjectMethod(name, "void SetTouchFunction(TouchFunction@ function)",
		asMETHOD(ICustomEntity, SetTouchFunction), asCALL_THISCALL);

	engine.RegisterObjectMethod(name, "void SetBlockedFunction(BlockedFunction@ function)",
		asMETHOD(ICustomEntity, SetBlockedFunction), asCALL_THISCALL);

	engine.RegisterObjectMethod(name, "void SetUseFunction(UseFunction@ function)",
		asMETHOD(ICustomEntity, SetUseFunction), asCALL_THISCALL);
}

void RegisterCustomEntityInterface(asIScriptEngine& engine)
{
	const std::string previousNamespace = engine.GetDefaultNamespace();
	engine.SetDefaultNamespace("");

	engine.RegisterInterface(ScriptBaseClassInterface);
	RegisterCustomEntityCallbackHandler(engine);

	engine.SetDefaultNamespace(previousNamespace.c_str());
}
}
