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

#include <algorithm>
#include <cassert>
#include <cctype>

#include <fmt/core.h>

#include <angelscript/scriptbuilder/scriptbuilder.h>

#include "cbase.h"
#include "ModuleLoader.h"

#include "scripting/AS/as_utils.h"
#include "scripting/AS/ASManager.h"

namespace scripting
{
struct IncludeResolver
{
	IncludeResolver(spdlog::logger* logger, ModuleLoader* moduleLoader)
		: m_Logger(logger), m_ModuleLoader(moduleLoader)
	{
		assert(m_Logger);
		assert(m_ModuleLoader);
	}

	int IncludeCallback(const char* include, const char* from, CScriptBuilder* builder);

	static int IncludeCallbackWrapper(const char* include, const char* from, CScriptBuilder* builder, void* userParam)
	{
		return static_cast<IncludeResolver*>(userParam)->IncludeCallback(include, from, builder);
	}

private:
	spdlog::logger* const m_Logger;
	ModuleLoader* const m_ModuleLoader;
	int m_Depth{0};
};

int IncludeResolver::IncludeCallback(const char* include, const char* from, CScriptBuilder* builder)
{
	++m_Depth;

	const CallOnDestroy cleanup{[this]()
		{
			--m_Depth;
		}};

	m_Logger->trace("Encountered #include \"{}\" in \"{}\"", include, from);

	const Filename absoluteFilename = m_ModuleLoader->GetAbsoluteScriptFileName(include);

	if (absoluteFilename.empty())
	{
		return -1;
	}

	m_Logger->debug("Adding script \"{}\" (depth {})", absoluteFilename.c_str(), m_Depth);

	return builder->AddSectionFromFile(absoluteFilename.c_str());
}

ModuleLoader::ModuleLoader(std::shared_ptr<spdlog::logger> logger, asIScriptEngine* engine)
	: m_Logger(logger), m_Engine(engine)
{
	assert(m_Logger);
	assert(m_Engine);
}

as::ModulePtr ModuleLoader::Load(ModuleType type, std::string_view scope, std::string_view scriptFileName, std::string_view generatedCode)
{
	m_Logger->trace("Trying to create module");

	scope = ValidateInput(scope, "scope", false);
	scriptFileName = ValidateInput(scriptFileName, "scriptName", true);

	if (scope.empty() || scriptFileName.empty())
	{
		return {};
	}

	const auto moduleName = fmt::format("{}:{}:{}", ModuleTypeToString(type), scope, scriptFileName);

	m_Logger->debug("Creating module \"{}\"", moduleName);

	if (m_Engine->GetModule(moduleName.c_str()))
	{
		m_Logger->error("A module with name \"{}\" already exists", moduleName);
		return {};
	}

	const Filename absoluteFilename = GetAbsoluteScriptFileName(scriptFileName);

	if (absoluteFilename.empty())
	{
		return {};
	}

	return CreateCore(moduleName, absoluteFilename, generatedCode);
}

as::ModulePtr ModuleLoader::CreatePrecompiledModule(std::string_view scope, std::string_view contents)
{
	m_Logger->trace("Trying to create precompiled module");

	scope = ValidateInput(scope, "scope", false);

	if (scope.empty())
	{
		return {};
	}

	const auto moduleName = fmt::format("Precompiled:{}", scope);

	m_Logger->debug("Creating precompiled module \"{}\"", moduleName);

	if (m_Engine->GetModule(moduleName.c_str()))
	{
		m_Logger->error("A precompiled module with name \"{}\" already exists", moduleName);
		return {};
	}

	contents = Trim(contents);

	if (contents.empty())
	{
		m_Logger->error("Precompiled module \"{}\" is empty", moduleName);
		return {};
	}

	as::ModulePtr module(m_Engine->GetModule(moduleName.c_str(), asGM_ALWAYS_CREATE));

	int result = module->AddScriptSection(PrecompiledModuleSectionName, contents.data(), contents.size());

	if (!g_ASManager.HandleAddScriptSectionResult(result, moduleName, PrecompiledModuleSectionName))
	{
		return {};
	}

	result = module->Build();

	if (!g_ASManager.HandleBuildResult(result, moduleName))
	{
		return {};
	}

	m_Logger->debug("Successfully built precompiled module \"{}\"", moduleName);

	return module;
}

Filename ModuleLoader::GetAbsoluteScriptFileName(std::string_view scriptFileName)
{
	Filename relativeFilename;

	fmt::format_to(std::back_inserter(relativeFilename), "{}{}", BaseScriptsDirectory, scriptFileName);

	// TODO: need to make a wrapper function to do this.
	Filename absoluteFilename;

	absoluteFilename.resize(absoluteFilename.kMaxSize);

	if (g_pFileSystem->GetLocalPath(relativeFilename.c_str(), absoluteFilename.data(), absoluteFilename.size()))
	{
		absoluteFilename.resize(strlen(absoluteFilename.c_str()));
	}
	else
	{
		m_Logger->error("Could not find script \"{}\"", relativeFilename.c_str());
		absoluteFilename.resize(0);
	}

	return absoluteFilename;
}

std::string_view ModuleLoader::ValidateInput(std::string_view value, std::string_view parameterName, bool allowAnyCharacter)
{
	value = Trim(value);

	if (value.empty())
	{
		m_Logger->error("Cannot create module with empty {}", parameterName);
		return {};
	}

	if (!allowAnyCharacter)
	{
		if (std::find_if(value.begin(), value.end(), [](auto c)
				{ return 0 == std::isalnum(c) && c != '_'; }) != value.end())
		{
			m_Logger->error("Cannot create module with invalid {} value", parameterName);
			return {};
		}
	}

	return value;
}

as::ModulePtr ModuleLoader::CreateCore(const std::string& moduleName, const Filename& absoluteFilename, std::string_view generatedCode)
{
	CScriptBuilder builder;

	IncludeResolver includeResolver{m_Logger.get(), this};

	builder.SetIncludeCallback(&IncludeResolver::IncludeCallbackWrapper, &includeResolver);

	int result = builder.StartNewModule(m_Engine, moduleName.c_str());

	if (result < 0)
	{
		m_Logger->error("Error {} creating new module", result);
		return {};
	}

	as::ModulePtr module{builder.GetModule()};

	generatedCode = Trim(generatedCode);

	if (!generatedCode.empty())
	{
		m_Logger->debug("Adding generated code");

		int result = builder.AddSectionFromMemory(PrecompiledModuleSectionName, generatedCode.data(), generatedCode.size());

		if (!g_ASManager.HandleAddScriptSectionResult(result, module->GetName(), PrecompiledModuleSectionName))
		{
			m_Logger->error("Error adding generated code to module");
			return {};
		}
	}

	m_Logger->debug("Adding script \"{}\" (root)", absoluteFilename.c_str());

	result = builder.AddSectionFromFile(absoluteFilename.c_str());

	if (!g_ASManager.HandleAddScriptSectionResult(result, moduleName, absoluteFilename.c_str()))
	{
		return {};
	}

	result = builder.BuildModule();

	if (!g_ASManager.HandleBuildResult(result, moduleName))
	{
		return {};
	}

	m_Logger->debug("Successfully built module \"{}\"", moduleName);

	return module;
}
}
