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

#include "AmmoTypeSystem.h"
#include "GameLibrary.h"
#include "ProjectInfoSystem.h"
#include "skill.h"
#include "WeaponDataSystem.h"

#include "networking/NetworkDataSystem.h"

#include "scripting/AS/ASManager.h"

#include "sound/MaterialSystem.h"

#include "utils/ConCommandSystem.h"
#include "utils/GameSystem.h"
#include "utils/JSONSystem.h"
#include "utils/ReplacementMaps.h"

bool GameLibrary::Initialize()
{
	if (!FileSystem_LoadFileSystem())
	{
		Con_Printf("Could not load filesystem library\n");
		return false;
	}

	g_pDeveloper = g_ConCommands.GetCVar("developer");

	AddGameSystems();

	if (!g_GameSystems.Initialize())
	{
		return false;
	}

	g_GameLogger = g_Logging.CreateLogger("game");
	g_AssertLogger = g_Logging.CreateLogger("assert");
	CBaseEntity::Logger = g_Logging.CreateLogger("ent");
	CBasePlayerWeapon::WeaponsLogger = g_Logging.CreateLogger("ent.weapons");

	g_GameSystems.Invoke(&IGameSystem::PostInitialize);

	g_ConCommands.CreateCommand("log_setentlevels", [this](const auto& args)
		{ SetEntLogLevels(args); });

	return true;
}

void GameLibrary::Shutdown()
{
	CBasePlayerWeapon::WeaponsLogger.reset();
	CBaseEntity::Logger.reset();
	g_AssertLogger.reset();
	g_GameLogger.reset();

	g_GameSystems.InvokeReverse(&IGameSystem::Shutdown);
	g_GameSystems.RemoveAll();

	FileSystem_FreeFileSystem();
}

void GameLibrary::AddGameSystems()
{
	// Done separately before game systems to avoid tightly coupling logging to json.
	g_Logging.PreInitialize();

	// Logging must initialize after JSON so that it can load the config file immediately.
	// It must also initialize after concommands since it creates a few of those.
	// It is safe to use loggers after logging has shut down so having it shut down first is not an issue.
	g_GameSystems.Add(&g_ConCommands);
	g_GameSystems.Add(&g_JSON);
	g_GameSystems.Add(&g_Logging);
	g_GameSystems.Add(&g_NetworkData);
	g_GameSystems.Add(&g_ASManager);
	g_GameSystems.Add(&g_ReplacementMaps);
	g_GameSystems.Add(&g_MaterialSystem);
	g_GameSystems.Add(&g_ProjectInfo);
	g_GameSystems.Add(&g_WeaponData);
	g_GameSystems.Add(&g_AmmoTypes);
	g_GameSystems.Add(&g_Skill);
}

void GameLibrary::SetEntLogLevels(spdlog::level::level_enum level)
{
	const auto& levelName = spdlog::level::to_string_view(level);

	for (auto& logger : {CBaseEntity::Logger, CBasePlayerWeapon::WeaponsLogger})
	{
		logger->set_level(level);
		Con_Printf("Set \"%s\" log level to %s\n", logger->name().c_str(), levelName.data());
	}
}

void GameLibrary::SetEntLogLevels(const CommandArgs& args)
{
	if (args.Count() != 2)
	{
		Con_Printf("Usage: log_setentlevels log_level\n");
		return;
	}

	const auto level = spdlog::level::from_str(args.Argument(1));

	SetEntLogLevels(level);
}
