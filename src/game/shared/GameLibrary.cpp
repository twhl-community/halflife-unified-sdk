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

#include "GameLibrary.h"

#include "config/GameConfigLoader.h"

#include "scripting/AS/ASManager.h"

#include "utils/ConCommandSystem.h"
#include "utils/GameSystem.h"
#include "utils/JSONSystem.h"
#include "utils/ReplacementMaps.h"

void GameLibrary::AddGameSystems()
{
	//Done separately before game systems to avoid tightly coupling logging to json.
	g_Logging.PreInitialize();

	//Logging must initialize after JSON so that it can load the config file immediately.
	//It must also initialize after concommands since it creates a few of those.
	//It is safe to use loggers after logging has shut down so having it shut down first is not an issue.
	g_GameSystems.Add(&g_ConCommands);
	g_GameSystems.Add(&g_JSON);
	g_GameSystems.Add(&g_Logging);
	g_GameSystems.Add(&g_ASManager);
	//Depends on Angelscript
	g_GameSystems.Add(&g_GameConfigLoader);
	g_GameSystems.Add(&g_ReplacementMaps);
}

bool GameLibrary::InitializeCommon()
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

	g_GameSystems.Invoke(&IGameSystem::PostInitialize);

	return true;
}

void GameLibrary::ShutdownCommon()
{
	g_GameSystems.InvokeReverse(&IGameSystem::Shutdown);
	g_GameSystems.RemoveAll();

	FileSystem_FreeFileSystem();
}
