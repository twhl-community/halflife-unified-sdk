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

#include <fmt/format.h>

#include <angelscript.h>

namespace as
{
struct ScriptEngineDeleter
{
	void operator()(asIScriptEngine* engine) const noexcept
	{
		if (engine)
		{
			engine->ShutDownAndRelease();
		}
	}
};

using EnginePtr = std::unique_ptr<asIScriptEngine, ScriptEngineDeleter>;

struct ScriptModuleDeleter
{
	void operator()(asIScriptModule* module) const noexcept
	{
		if (module)
		{
			module->Discard();
		}
	}
};

using ModulePtr = std::unique_ptr<asIScriptModule, ScriptModuleDeleter>;

/**
 *	@brief Custom deleter for Angelscript objects stored in \c std::unique_ptr
 */
template <typename T>
struct ObjectDeleter
{
	void operator()(T* object) const noexcept
	{
		if (object)
		{
			object->Release();
		}
	}
};

template <typename T>
using UniquePtr = std::unique_ptr<T, ObjectDeleter<T>>;

inline std::string FormatFunctionName(const asIScriptFunction& function)
{
	std::string buffer;

	if (const auto ns = function.GetNamespace(); ns && ns[0])
	{
		buffer += ns;
		buffer += "::";
	}

	if (const auto clazz = function.GetObjectName(); clazz)
	{
		buffer += clazz;
		buffer += "::";
	}

	buffer += function.GetName();

	return buffer;
}

std::string_view ReturnCodeToString(int code);
std::string_view ContextStateToString(int code);

/**
 *	@brief Gets a printable string for the function's module
 */
inline const char* GetModuleName(const asIScriptFunction& function)
{
	if (const auto moduleName = function.GetModuleName(); moduleName)
	{
		return moduleName;
	}

	switch (function.GetFuncType())
	{
	case asFUNC_SYSTEM:
		return "System";
	default:
		return "Unknown";
	}
}

struct SectionInfo
{
	std::string SectionName;
	int Line = 0;
	int Column = 0;
};

inline std::string GetSectionName(const asIScriptFunction& function)
{
	if (const auto sectionName = function.GetScriptSectionName(); sectionName && sectionName[0])
	{
		return sectionName;
	}

	if (const auto module = function.GetModule(); module)
	{
		return fmt::format("Unknown section in module \"{}\"", module->GetName());
	}

	switch (function.GetFuncType())
	{
	case asFUNC_SYSTEM:
		return "System function";
	default:
		return "Unknown section";
	}
}

inline std::string GetSectionName(const asIScriptFunction* function)
{
	if (!function)
	{
		// Should never happen
		return "No function to get section";
	}

	return GetSectionName(*function);
}

inline SectionInfo GetExecutingSectionInfo(asIScriptContext& context, asUINT stackLevel = 0)
{
	const char* sectionName;
	int column;
	const int lineNumber = context.GetLineNumber(stackLevel, &column, &sectionName);

	std::string sectionNameString;

	if (sectionName)
	{
		sectionNameString = sectionName;
	}
	else
	{
		sectionNameString = GetSectionName(context.GetFunction(stackLevel));
	}

	return {
		.SectionName = std::move(sectionNameString),
		.Line = lineNumber,
		.Column = column};
}

inline SectionInfo GetExceptionSectionInfo(asIScriptContext& context)
{
	const char* sectionName;
	int column;
	const int lineNumber = context.GetExceptionLineNumber(&column, &sectionName);

	std::string sectionNameString;

	if (sectionName)
	{
		sectionNameString = sectionName;
	}
	else
	{
		sectionNameString = GetSectionName(context.GetExceptionFunction());
	}

	return {
		.SectionName = std::move(sectionNameString),
		.Line = lineNumber,
		.Column = column};
}
}
