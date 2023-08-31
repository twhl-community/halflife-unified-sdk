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

#include "GameSystem.h"

struct cvar_t;

/**
 *	@brief Provides an interface to get command arguments without explicitly depending on library interfaces
 */
class CommandArgs final
{
public:
	CommandArgs() = default;
	~CommandArgs() = default;
	CommandArgs(const CommandArgs&) = delete;
	CommandArgs& operator=(const CommandArgs&) = delete;
	CommandArgs(CommandArgs&&) = delete;
	CommandArgs& operator=(CommandArgs&&) = delete;

	int Count() const;

	const char* Argument(int index) const;
};

struct ChangeCallback
{
	cvar_t* Cvar;

	std::string OldString;
	float OldValue;
};

enum class CommandLibraryPrefix
{
	No = 0,
	Yes
};

/**
 *	@brief Manages console commands and cvars with client/server prefixes and stateful callbacks
 */
class ConCommandSystem final : public IGameSystem
{
private:
	struct CVarData
	{
		// Store the name in a way that's safe to move construct
		std::unique_ptr<char[]> Name;

		cvar_t* CVar = nullptr;

		// Non-null only on the server, do not use directly
		std::unique_ptr<cvar_t> CVarInstance;
	};

	struct CommandData
	{
		// Store the name in a way that's safe to move construct
		std::unique_ptr<char[]> Name;

		std::function<void(const CommandArgs&)> Callback;
	};

	struct ChangeCallbackData
	{
		ChangeCallback State;

		std::function<void(ChangeCallback&)> Callback;
	};

public:
	ConCommandSystem();
	~ConCommandSystem();
	ConCommandSystem(const ConCommandSystem&) = delete;
	ConCommandSystem& operator=(const ConCommandSystem&) = delete;

	const char* GetName() const override { return "ConCommands"; }

	bool Initialize() override;
	void PostInitialize() override;
	void Shutdown() override;

	void RunFrame();

	/**
	 *	@brief Gets a cvar by name
	 *	Safe to call at any time the library itself is initialized
	 */
	cvar_t* GetCVar(const char* name) const;

	/**
	 *	@brief Creates a cvar, with a library-specific prefix if @c useLibraryPrefix is CommandLibraryPrefix::Yes
	 */
	cvar_t* CreateCVar(std::string_view name, const char* defaultValue, int flags = 0,
		CommandLibraryPrefix useLibraryPrefix = CommandLibraryPrefix::Yes);

	/**
	 *	@brief Creates a command, with a library-specific prefix if @c useLibraryPrefix is CommandLibraryPrefix::Yes
	 */
	void CreateCommand(std::string_view name, std::function<void(const CommandArgs&)>&& callback,
		CommandLibraryPrefix useLibraryPrefix = CommandLibraryPrefix::Yes);

	/**
	 *	@brief Registers a callback to be executed whenever the value of the given cvar changes.
	 */
	void RegisterChangeCallback(const char* name, std::function<void(const ChangeCallback&)>&& callback);

	/**
	 *	@copydoc RegisterChangeCallback(const char*, std::function<void(const ChangeCallback&)>&&)
	 */
	void RegisterChangeCallback(cvar_t* cvar, std::function<void(const ChangeCallback&)>&& callback);

private:
	static void CommandCallbackWrapper();
	void CommandCallback();

	const char* TryGetCVarCommandLineValue(std::string_view name) const;

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	std::vector<CVarData> m_Cvars;

	std::unordered_map<std::string_view, CommandData> m_Commands;

	std::vector<ChangeCallbackData> m_ChangeCallbacks;
};

inline ConCommandSystem g_ConCommands;
