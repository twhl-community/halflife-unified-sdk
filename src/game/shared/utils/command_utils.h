/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <spdlog/logger.h>

/**
*	@brief Provides an interface to get command arguments without explicitly depending on library interfaces
*/
class CCommandArgs final
{
public:
	CCommandArgs() = default;
	~CCommandArgs() = default;
	CCommandArgs(const CCommandArgs&) = delete;
	CCommandArgs& operator=(const CCommandArgs&) = delete;
	CCommandArgs(CCommandArgs&&) = delete;
	CCommandArgs& operator=(CCommandArgs&&) = delete;

	int Count() const;

	const char* Argument(int index) const;
};

/**
*	@brief Manages console commands and cvars with client/server prefixes and stateful callbacks
*/
class CConCommandSystem final
{
private:
	struct CVarData
	{
		//Store the name in a way that's safe to move construct
		std::unique_ptr<char[]> Name;

		cvar_t* CVar = nullptr;

		//Non-null only on the server, do not use directly
		std::unique_ptr<cvar_t> CVarInstance;
	};

	struct CommandData
	{
		//Store the name in a way that's safe to move construct
		std::unique_ptr<char[]> Name;

		std::function<void(const CCommandArgs&)> Callback;
	};

public:
	CConCommandSystem();
	~CConCommandSystem();
	CConCommandSystem(const CConCommandSystem&) = delete;
	CConCommandSystem& operator=(const CConCommandSystem&) = delete;

	bool Initialize();
	bool PostInitialize();
	void Shutdown();

	/**
	*	@brief Gets a cvar by name
	*	Safe to call at any time the library itself is initialized
	*/
	cvar_t* GetCVar(const char* name) const;

	/**
	*	@brief Creates a cvar with a library-specific prefix
	*/
	cvar_t* CreateCVar(std::string_view name, const char* defaultValue, int flags = 0);

	/**
	*	@brief Creates a command with a library-specific prefix
	*/
	void CreateCommand(std::string_view name, std::function<void(const CCommandArgs&)>&& callback);

private:
	static void CommandCallbackWrapper();
	void CommandCallback();

	const char* TryGetCVarCommandLineValue(std::string_view name) const;

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	std::vector<CVarData> m_Cvars;

	std::unordered_map<std::string_view, CommandData> m_Commands;
};

inline CConCommandSystem g_ConCommands;
