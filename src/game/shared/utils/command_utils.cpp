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

#include <algorithm>
#include <string>
#include <string_view>

#include <spdlog/fmt/fmt.h>

#include "extdll.h"
#include "util.h"
#include "command_utils.h"

#ifdef CLIENT_DLL
#include "hud.h"
#endif

/**
*	@brief Prefix used for cvars registered by this system that can be set on the command line.
*	@details Typical use on the command line is :cvar_name_with_or_without_library_prefix value
*/
constexpr char CommandLineCVarPrefix = ':';

int CCommandArgs::Count() const
{
	return CMD_ARGC();
}

const char* CCommandArgs::Argument(int index) const
{
	return CMD_ARGV(index);
}

CConCommandSystem::CConCommandSystem() = default;
CConCommandSystem::~CConCommandSystem() = default;

bool CConCommandSystem::Initialize()
{
	//Use global logger during startup
	m_Logger = g_Logging->GetGlobalLogger();

	return true;
}

bool CConCommandSystem::PostInitialize()
{
	//Create the logger using user-provided configuration
	m_Logger = g_Logging->CreateLogger("cvar");

	return true;
}

void CConCommandSystem::Shutdown()
{
	m_Commands.clear();
	m_Cvars.clear();
	g_Logging->RemoveLogger(m_Logger);
	m_Logger.reset();
}

cvar_t* CConCommandSystem::GetCVar(const char* name) const
{
	return g_engfuncs.pfnCVarGetPointer(name);
}

cvar_t* CConCommandSystem::CreateCVar(std::string_view name, const char* defaultValue, int flags)
{
	if (name.empty() || !defaultValue)
	{
		m_Logger->error("CConCommandSystem::CreateCVar: Invalid cvar data");
		return nullptr;
	}

	const std::string completeName{fmt::format("{}_{}", GetShortLibraryPrefix(), name)};

	if (std::find_if(m_Cvars.begin(), m_Cvars.end(), [&](const auto& data)
		{
			return data.Name.get() == completeName;
		}) != m_Cvars.end())
	{
		m_Logger->warn("CConCommandSystem::CreateCVar: CVar \"{}\" already registered", completeName);

		//Can't guarantee that the cvar is the same so return null
		return nullptr;
	}

	CVarData data;

	data.Name = std::make_unique<char[]>(completeName.size() + 1);
	strncpy(data.Name.get(), completeName.c_str(), completeName.size() + 1);
	data.Name[completeName.size()] = '\0';

	//The client creates cvars in the engine, the server has to create them itself
#ifdef CLIENT_DLL
	data.CVar = gEngfuncs.pfnRegisterVariable(data.Name.get(), defaultValue, flags);
#else
	data.CVarInstance = std::make_unique<cvar_t>();
	data.CVar = data.CVarInstance.get();

	data.CVar->name = data.Name.get();
	data.CVar->string = defaultValue;
	data.CVar->flags = flags;
	data.CVar->value = 0;
	data.CVar->next = nullptr;

	g_engfuncs.pfnCVarRegister(data.CVar);
#endif

	m_Cvars.push_back(std::move(data));

	auto& cvar = m_Cvars.back();

	//Check if an initial value was passed on the command line.
	//Two cases to consider: is the complete name being used or is the non-prefixed name being used?
	//Prefer complete name to allow for more granular control.
	//This is required because the server is loaded after stuffcmds has run on a listen server, so the cvars aren't set.
	auto value = TryGetCVarCommandLineValue(cvar.Name.get());

	if (!value)
	{
		value = TryGetCVarCommandLineValue(name);
	}

	if (value)
	{
		g_engfuncs.pfnCvar_DirectSet(cvar.CVar, value);
	}

	return cvar.CVar;
}

void CConCommandSystem::CreateCommand(std::string_view name, std::function<void(const CCommandArgs&)>&& callback)
{
	if (name.empty() || !callback)
	{
		m_Logger->error("CConCommandSystem::CreateCommand: Invalid command data");
		return;
	}

	const std::string completeName{fmt::format("{}_{}", GetShortLibraryPrefix(), name)};

	if (m_Commands.contains(completeName))
	{
		m_Logger->warn("CConCommandSystem::CreateCommand: Command \"{}\" already registered", completeName);
		return;
	}

	CommandData data;

	data.Name = std::make_unique<char[]>(completeName.size() + 1);
	strncpy(data.Name.get(), completeName.c_str(), completeName.size() + 1);
	data.Name[completeName.size()] = '\0';

	data.Callback = std::move(callback);

	const std::string_view key{data.Name.get(), completeName.size()};

	m_Commands.emplace(key, std::move(data));

	g_engfuncs.pfnAddServerCommand(key.data(), &CConCommandSystem::CommandCallbackWrapper);
}

void CConCommandSystem::CommandCallbackWrapper()
{
	g_ConCommands.CommandCallback();
}

void CConCommandSystem::CommandCallback()
{
	const CCommandArgs args{};

	if (auto it = m_Commands.find(args.Argument(0)); it != m_Commands.end())
	{
		it->second.Callback(args);
	}
	else
	{
		//Should never happen
		m_Logger->error("CConCommandSystem::CommandCallback: Couldn't find command \"{}\"", args.Argument(0));
	}
}

const char* CConCommandSystem::TryGetCVarCommandLineValue(std::string_view name) const
{
	const auto parameter = fmt::format("{}{}", CommandLineCVarPrefix, name);

	const char* next = nullptr;

	if (!COM_GetParam(parameter.c_str(), &next))
	{
		return nullptr;
	}

	//Check for common mistakes, like not specifying a value (if it's the last argument or followed by another parameter or engine cvar set).
	//Note that this can happen if a cvar contains a map name that happens to start with these characters.
	//Filenames shouldn't include these characters anyway because it confuses command line parsing code.
	if (!next || next[0] == '-' || next[0] == '+')
	{
		m_Logger->error("CVar specified on the command line with no value");
		return nullptr;
	}

	return next;
}
