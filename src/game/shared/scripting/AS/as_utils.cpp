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

#include "cbase.h"

#include "as_utils.h"

using namespace std::literals;

namespace as
{
void RegisterRefCountedClass(asIScriptEngine& engine, const char* name)
{
	engine.RegisterObjectBehaviour(name, asBEHAVE_ADDREF, "void AddRef()", asMETHOD(RefCountedClass, AddRef), asCALL_THISCALL);
	engine.RegisterObjectBehaviour(name, asBEHAVE_RELEASE, "void Release()", asMETHOD(RefCountedClass, Release), asCALL_THISCALL);
}

asITypeInfo* GetFuncDefByName(asIScriptEngine& engine, const char* name)
{
	const auto count = engine.GetFuncdefCount();

	for (asUINT i = 0; i < count; ++i)
	{
		auto funcdef = engine.GetFuncdefByIndex(i);

		// TODO: namespaces
		if (0 == strcmp(name, funcdef->GetName()))
		{
			return funcdef;
		}
	}

	return nullptr;
}

const asIScriptFunction* GetUnderlyingFunction(const asIScriptFunction* function)
{
	assert(function);

	if (!function)
	{
		return nullptr;
	}

	if (auto delegate = function->GetDelegateFunction(); delegate)
	{
		return delegate;
	}

	return function;
}

asIScriptFunction* GetUnderlyingFunction(asIScriptFunction* function)
{
	return const_cast<asIScriptFunction*>(GetUnderlyingFunction(const_cast<const asIScriptFunction*>(function)));
}

std::string FormatFunctionName(const asIScriptFunction& function)
{
	const auto actualFunction = GetUnderlyingFunction(&function);
	return actualFunction->GetDeclaration(true, true, false);
}

std::string FormatTypeName(const asITypeInfo& typeInfo)
{
	std::string result;

	if (const auto ns = typeInfo.GetNamespace(); ns && ns[0])
	{
		result += ns;
		result += "::";
	}

	result += typeInfo.GetName();

	return result;
}

std::string_view ReturnCodeToString(int code)
{
	switch (code)
	{
	case asSUCCESS:
		return "Success"sv;
	case asERROR:
		return "General error (see log)"sv;
	case asCONTEXT_ACTIVE:
		return "Context active"sv;
	case asCONTEXT_NOT_FINISHED:
		return "Context not finished"sv;
	case asCONTEXT_NOT_PREPARED:
		return "Context not prepared"sv;
	case asINVALID_ARG:
		return "Invalid argument"sv;
	case asNO_FUNCTION:
		return "No function"sv;
	case asNOT_SUPPORTED:
		return "Not supported"sv;
	case asINVALID_NAME:
		return "Invalid name"sv;
	case asNAME_TAKEN:
		return "Name taken"sv;
	case asINVALID_DECLARATION:
		return "Invalid declaration"sv;
	case asINVALID_OBJECT:
		return "Invalid object"sv;
	case asINVALID_TYPE:
		return "Invalid type"sv;
	case asALREADY_REGISTERED:
		return "Already registered"sv;
	case asMULTIPLE_FUNCTIONS:
		return "Multiple functions"sv;
	case asNO_MODULE:
		return "No module"sv;
	case asNO_GLOBAL_VAR:
		return "No global variable"sv;
	case asINVALID_CONFIGURATION:
		return "Invalid configuration"sv;
	case asINVALID_INTERFACE:
		return "Invalid interface"sv;
	case asCANT_BIND_ALL_FUNCTIONS:
		return "Can't bind all functions"sv;
	case asLOWER_ARRAY_DIMENSION_NOT_REGISTERED:
		return "Lower array dimension not registered"sv;
	case asWRONG_CONFIG_GROUP:
		return "Wrong configuration group"sv;
	case asCONFIG_GROUP_IS_IN_USE:
		return "Configuration group is in use"sv;
	case asILLEGAL_BEHAVIOUR_FOR_TYPE:
		return "Illegal behavior for type"sv;
	case asWRONG_CALLING_CONV:
		return "Wrong calling convention"sv;
	case asBUILD_IN_PROGRESS:
		return "Build in progress"sv;
	case asINIT_GLOBAL_VARS_FAILED:
		return "Initialization of global variables failed"sv;
	case asOUT_OF_MEMORY:
		return "Out of memory"sv;
	case asMODULE_IS_IN_USE:
		return "Module is in use"sv;

		// No default case so new constants added in the future will show more easily
	}

	return "Unknown return code"sv;
}

std::string_view ContextStateToString(int code)
{
	switch (code)
	{
	case asEXECUTION_FINISHED:
		return "Finished"sv;
	case asEXECUTION_SUSPENDED:
		return "Suspended"sv;
	case asEXECUTION_ABORTED:
		return "Aborted"sv;
	case asEXECUTION_EXCEPTION:
		return "Exception"sv;
	case asEXECUTION_PREPARED:
		return "Prepared"sv;
	case asEXECUTION_UNINITIALIZED:
		return "Uninitialized"sv;
	case asEXECUTION_ACTIVE:
		return "Active"sv;
	case asEXECUTION_ERROR:
		return "Error"sv;
	}

	return "Unknown context state"sv;
}

const char* GetModuleName(const asIScriptFunction& function)
{
	const auto actualFunction = GetUnderlyingFunction(&function);

	if (const auto moduleName = actualFunction->GetModuleName(); moduleName)
	{
		return moduleName;
	}

	switch (actualFunction->GetFuncType())
	{
	case asFUNC_SYSTEM:
		return "System";
	default:
		return "Unknown";
	}
}

asIScriptModule* GetCallingModule()
{
	const auto context = asGetActiveContext();
	
	assert(context);

	if (!context)
	{
		return nullptr;
	}

	// TODO: if for some reason a system function (native function) is calling us then this won't work.
	// Maybe walk the stack to find the first actual script function?
	const auto function = context->GetFunction();

	assert(function);

	if (!function)
	{
		return nullptr;
	}

	const auto module = function->GetModule();

	assert(module);

	return module;
}

std::string GetSectionName(const asIScriptFunction& function)
{
	if (const auto sectionName = function.GetScriptSectionName(); sectionName && sectionName[0])
	{
		return sectionName;
	}

	if (const auto module = function.GetModule(); module)
	{
		return fmt::format("Unknown section in module \"{}\"", *module);
	}

	switch (function.GetFuncType())
	{
	case asFUNC_SYSTEM:
		return "System function";
	default:
		return "Unknown section";
	}
}

std::string GetSectionName(const asIScriptFunction* function)
{
	if (!function)
	{
		// Should never happen
		return "No function to get section";
	}

	return GetSectionName(*function);
}

SectionInfo GetExecutingSectionInfo(asIScriptContext& context, asUINT stackLevel)
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

SectionInfo GetExceptionSectionInfo(asIScriptContext& context)
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

void PrintStackTrace(spdlog::logger& logger, spdlog::level::level_enum level, asIScriptContext& context, asUINT maxDepth)
{
	const auto stackLevels = std::min(maxDepth, context.GetCallstackSize());

	if (stackLevels == 0)
	{
		logger.error("No function to print");
		return;
	}

	logger.log(level, "Stack trace ({} out of {} levels):", stackLevels, context.GetCallstackSize());

	for (asUINT stackLevel = 0; stackLevel < stackLevels; ++stackLevel)
	{
		const auto function = context.GetFunction(stackLevel);

		const auto sectionInfo{GetExecutingSectionInfo(context, stackLevel)};

		logger.log(level, "{}: {} at {}:{},{}", stackLevel, *function, sectionInfo.SectionName, sectionInfo.Line, sectionInfo.Column);
	}
}
}
