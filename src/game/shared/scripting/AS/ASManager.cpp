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

#include <exception>
#include <string>

#include <fmt/format.h>

#include "cbase.h"

#include "as_utils.h"
#include "ASManager.h"

using namespace as;

ASManager::ASManager() = default;
ASManager::~ASManager() = default;

bool ASManager::Initialize()
{
	m_Logger = g_Logging.CreateLogger("angelscript");
	return true;
}

void ASManager::Shutdown()
{
	m_Logger.reset();
}

EnginePtr ASManager::CreateEngine()
{
	if (!m_Logger)
	{
		// Not initialized yet
		return {};
	}

	EnginePtr engine{asCreateScriptEngine()};

	if (!engine)
	{
		m_Logger->error("Could not create Angelscript engine instance");
		return {};
	}

	// Configure engine
	engine->SetMessageCallback(asMETHOD(ASManager, OnMessageCallback), this, asCALL_THISCALL);
	engine->SetTranslateAppExceptionCallback(asMETHOD(ASManager, OnTranslateAppExceptionCallback), this, asCALL_THISCALL);

	return engine;
}

UniquePtr<asIScriptContext> ASManager::CreateContext(asIScriptEngine& engine)
{
	if (!m_Logger)
	{
		// Not initialized yet
		return {};
	}

	UniquePtr<asIScriptContext> context{engine.CreateContext()};

	if (!context)
	{
		m_Logger->error("Could not create Angelscript context");
		return {};
	}

	// Configure context
	context->SetExceptionCallback(asMETHOD(ASManager, OnThrownExceptionCallback), this, asCALL_THISCALL);

	return context;
}

as::ModulePtr ASManager::CreateModule(asIScriptEngine& engine, const char* moduleName)
{
	as::ModulePtr module{engine.GetModule(moduleName, asGM_ALWAYS_CREATE)};

	if (!module)
	{
		m_Logger->error("Could not create Angelscript script module \"{}\"", moduleName);
		return {};
	}

	return module;
}

bool ASManager::HandleAddScriptSectionResult(int returnCode, std::string_view moduleName, std::string_view sectionName)
{
	if (asSUCCESS < returnCode)
	{
		m_Logger->error("Error \"{}\" adding script section \"{}\" to module \"{}\"", ReturnCodeToString(returnCode), moduleName, sectionName);
		return false;
	}

	return true;
}

bool ASManager::HandleBuildResult(int returnCode, std::string_view moduleName)
{
	if (asSUCCESS > returnCode)
	{
		m_Logger->error("Error \"{}\" building module \"{}\"", ReturnCodeToString(returnCode), moduleName);
		return false;
	}

	return true;
}

bool ASManager::PrepareContext(asIScriptContext& context, asIScriptFunction* function)
{
	if (!function)
	{
		m_Logger->error("No function to prepare context");
		return false;
	}

	const int result = context.Prepare(function);

	if (asSUCCESS > result)
	{
		m_Logger->error("Error \"{}\" preparing context", ReturnCodeToString(result));
		return false;
	}

	return true;
}

void ASManager::UnprepareContext(asIScriptContext& context)
{
	const int result = context.Unprepare();

	if (asSUCCESS > result)
	{
		m_Logger->error("Error \"{}\" unpreparing context", ReturnCodeToString(result));
	}
}

bool ASManager::ExecuteContext(asIScriptContext& context)
{
	if (m_Logger->level() <= spdlog::level::trace)
	{
		if (const auto function = context.GetFunction(); function)
		{
			auto moduleName = function->GetModuleName();

			if (!moduleName)
			{
				moduleName = "System function";
			}

			m_Logger->trace("Executing function \"{}\" ({})", FormatFunctionName(*function), moduleName);
		}
	}

	const int result = context.Execute();

	if (asEXECUTION_FINISHED < result)
	{
		m_Logger->error("Error \"{}\" executing function", ContextStateToString(result));
		return false;
	}

	return true;
}

void ASManager::OnMessageCallback(const asSMessageInfo* msg)
{
	const auto level = [&]()
	{
		switch (msg->type)
		{
		case asEMsgType::asMSGTYPE_ERROR:
			return spdlog::level::err;
		case asEMsgType::asMSGTYPE_WARNING:
			return spdlog::level::warn;
		default:
		case asEMsgType::asMSGTYPE_INFORMATION:
			return spdlog::level::info;
		}
	}();

	// The engine will often log information not related to a script by passing an empty section string and 0, 0 for the location.
	// Only prepend this information if it's relevant.
	if (msg->section && '\0' != msg->section[0])
	{
		m_Logger->log(level, "In section \"{}\" at line {}, column {}: {}", msg->section, msg->row, msg->col, msg->message);
	}
	else
	{
		m_Logger->log(level, "{}", msg->message);
	}
}

void ASManager::OnTranslateAppExceptionCallback(asIScriptContext* context)
{
	// See https://www.angelcode.com/angelscript/sdk/docs/manual/doc_cpp_exceptions.html
	try
	{
		throw;
	}
	catch (const std::exception& e)
	{
		// Catch exceptions for logging, but don't allow scripts to catch them.
		context->SetException(e.what(), false);
	}
	catch (...)
	{
		// Rethrow so longjmp reaches its destination.
		throw;
	}
}

void ASManager::OnThrownExceptionCallback(asIScriptContext* context)
{
	// Log exception info for debugging purposes.
	assert(m_Logger);

	const auto exceptionFunction = context->GetExceptionFunction();
	const auto executingFunction = context->GetFunction();

	// Should never happen
	if (!exceptionFunction || !executingFunction)
		return;

	// A caught exception is useful to know when debugging, an uncaught exception is an error.
	const auto level = context->WillExceptionBeCaught() ? spdlog::level::debug : spdlog::level::err;

	{
		const auto executingFunctionName = FormatFunctionName(*executingFunction);
		const auto executingModuleName = GetModuleName(*executingFunction);
		const auto sectionInfo{GetExecutingSectionInfo(*context)};

		m_Logger->log(level, "Exception thrown while executing function \"{}\" (module \"{}\", section \"{}\" at line {}, column {})",
			executingFunctionName, executingModuleName, sectionInfo.SectionName, sectionInfo.Line, sectionInfo.Column);
	}

	{
		const auto exceptionFunctionName = FormatFunctionName(*exceptionFunction);
		const auto exceptionModuleName = GetModuleName(*exceptionFunction);
		const auto sectionInfo{GetExceptionSectionInfo(*context)};

		m_Logger->log(level, "Thrown in function \"{}\" (module \"{}\", section \"{}\" at line {}, column {})",
			exceptionFunctionName, exceptionModuleName, sectionInfo.SectionName, sectionInfo.Line, sectionInfo.Column);
	}

	m_Logger->log(level, "Message: \"{}\"", context->GetExceptionString());
}
