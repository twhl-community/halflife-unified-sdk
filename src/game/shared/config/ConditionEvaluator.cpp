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

#include <chrono>

#include <angelscript.h>
#include <angelscript/scriptstdstring/scriptstdstring.h>

#include "cbase.h"
#include "ConditionEvaluator.h"

#include "scripting/AS/ASManager.h"

/**
 *	@brief Amount of time that a conditional evaluation can take before timing out.
 */
constexpr std::chrono::seconds ConditionalEvaluationTimeout{1};

struct TimeoutHandler
{
	using Clock = std::chrono::high_resolution_clock;

	void OnLineCallback(asIScriptContext* context)
	{
		if (Timeout <= Clock::now())
		{
			context->Abort();
		}
	}

	const Clock::time_point Timeout;
};

#ifndef CLIENT_DLL
namespace
{
// TODO: need to get this from gamerules
static bool GetSingleplayer()
{
	return gpGlobals->deathmatch == 0;
}

static bool GetMultiplayer()
{
	return gpGlobals->deathmatch != 0;
}

static bool GetListenServer()
{
	return IS_DEDICATED_SERVER() != 0;
}

static bool GetDedicatedServer()
{
	return !GetListenServer();
}
}
#endif

static void RegisterGameConfigConditionalsScriptAPI(asIScriptEngine& engine)
{
	// So we can use strings in conditionals
	RegisterStdString(&engine);

	// Register everything as globals to keep conditional strings short and easy to read
	// TODO: figure out what to do about the client side.
#ifndef CLIENT_DLL
	engine.RegisterGlobalFunction("bool get_Singleplayer() property", asFUNCTION(GetSingleplayer), asCALL_CDECL);
	engine.RegisterGlobalFunction("bool get_Multiplayer() property", asFUNCTION(GetMultiplayer), asCALL_CDECL);
	engine.RegisterGlobalFunction("bool get_ListenServer() property", asFUNCTION(GetListenServer), asCALL_CDECL);
	engine.RegisterGlobalFunction("bool get_DedicatedServer() property", asFUNCTION(GetDedicatedServer), asCALL_CDECL);
#endif
}

bool ConditionEvaluator::Initialize()
{
	m_Logger = g_Logging.CreateLogger("conditional_evaluation");

	m_ScriptEngine = g_ASManager.CreateEngine();

	if (!m_ScriptEngine)
	{
		return false;
	}

	m_ScriptContext = g_ASManager.CreateContext(*m_ScriptEngine);

	if (!m_ScriptContext)
	{
		return false;
	}

	RegisterGameConfigConditionalsScriptAPI(*m_ScriptEngine);

	return true;
}

void ConditionEvaluator::Shutdown()
{
	m_ScriptContext.reset();
	m_ScriptEngine.reset();
	m_Logger.reset();
}

std::optional<bool> ConditionEvaluator::Evaluate(std::string_view conditional)
{
	// Create a temporary module to evaluate the conditional with
	const as::ModulePtr module{g_ASManager.CreateModule(*m_ScriptEngine, "gamecfg_conditional")};

	if (!module)
		return {};

	// Wrap the conditional in a function we can call
	{
		const auto script{fmt::format("bool Evaluate() {{ return {}; }}", conditional)};

		// Engine message callback reports any errors
		const int addResult = module->AddScriptSection("conditional", script.c_str(), script.size());

		if (!g_ASManager.HandleAddScriptSectionResult(addResult, module->GetName(), "conditional"))
			return {};
	}

	if (!g_ASManager.HandleBuildResult(module->Build(), module->GetName()))
		return {};

	const auto function = module->GetFunctionByName("Evaluate");

	if (!g_ASManager.PrepareContext(*m_ScriptContext, function))
		return {};

	// Since this is expected to run very quickly, use a line callback to handle timeout to prevent infinite loops from locking up the game
	TimeoutHandler timeoutHandler{TimeoutHandler::Clock::now() + ConditionalEvaluationTimeout};

	m_ScriptContext->SetLineCallback(asMETHOD(TimeoutHandler, OnLineCallback), &timeoutHandler, asCALL_THISCALL);

	// Clear line callback so there's no dangling reference left in the context
	struct Cleanup
	{
		~Cleanup()
		{
			Context.ClearLineCallback();
		}

		asIScriptContext& Context;
	} cleanup{*m_ScriptContext};

	if (!g_ASManager.ExecuteContext(*m_ScriptContext))
		return {};

	const auto result = m_ScriptContext->GetReturnByte();

	// Free up resources used by the context and engine
	g_ASManager.UnprepareContext(*m_ScriptContext);
	m_ScriptEngine->GarbageCollect();

	return result != 0;
}
