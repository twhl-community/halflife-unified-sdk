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

#include <string>

#include <angelscript.h>

#include <angelscript/scriptarray/scriptarray.h>
#include <angelscript/scriptdictionary/scriptdictionary.h>
#include <angelscript/scriptstdstring/scriptstdstring.h>

#include "cbase.h"
#include "ASScriptingSystem.h"
#include "ScriptConsoleCommands.h"
#include "ScriptingAPI.h"

#include "ScriptAPI/EntityAPIs.h"
#include "ScriptAPI/MathAPIs.h"

#include "scripting/AS/ASManager.h"

namespace scripting
{
static void LogInfo(const std::string& msg)
{
	auto context = asGetActiveContext();
	g_Scripting.GetLogger()->info("[{}] {}", context->GetFunction()->GetModuleName(), msg);
}

static void RegisterLoggingAPI(asIScriptEngine& engine)
{
	const std::string previousNamespace = engine.GetDefaultNamespace();
	engine.SetDefaultNamespace("log");

	engine.RegisterGlobalFunction("void info(const string& in msg)", asFUNCTION(&LogInfo), asCALL_CDECL);

	engine.SetDefaultNamespace(previousNamespace.c_str());
}

void RegisterSDKScriptingAPI(asIScriptEngine& engine)
{
	RegisterScriptArray(&engine, true);
	RegisterStdString(&engine);
	RegisterStdStringUtils(&engine);
	RegisterScriptDictionary(&engine);

	RegisterLoggingAPI(engine);

	RegisterMathAPI(engine);

	RegisterScriptConsoleCommands(engine);

	RegisterEntityAPIs(engine);
}
}
