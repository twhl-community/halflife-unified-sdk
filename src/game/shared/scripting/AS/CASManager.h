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
#include <string_view>
#include <vector>

#include <spdlog/logger.h>

#include <angelscript.h>

#include "as_utils.h"

/**
*	@brief Creates Angelscript objects used by the game.
*	The use of a factory ensures that things like engine settings, error handling,
*	etc are consistent between all instances.
*/
class CASManager final
{
public:
	CASManager();
	~CASManager();

	bool Initialize();
	void Shutdown();

	std::shared_ptr<spdlog::logger> GetLogger() { return m_Logger; }

	as::EnginePtr CreateEngine();

	as::UniquePtr<asIScriptContext> CreateContext(asIScriptEngine& engine);

	as::ModulePtr CreateModule(asIScriptEngine& engine, const char* moduleName);

	//Helpers to handle module creation
	bool HandleAddScriptSectionResult(int returnCode, std::string_view moduleName, std::string_view sectionName);
	bool HandleBuildResult(int returnCode, std::string_view moduleName);

	//Helpers to handle contexts
	bool PrepareContext(asIScriptContext& context, asIScriptFunction* function);
	void UnprepareContext(asIScriptContext& context);
	bool ExecuteContext(asIScriptContext& context);

private:
	void OnMessageCallback(const asSMessageInfo* msg);

	void OnTranslateAppExceptionCallback(asIScriptContext* context);

	void OnThrownExceptionCallback(asIScriptContext* context);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
};

inline CASManager g_ASManager;
