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

#include "extdll.h"
#include "util.h"

#include "CGameLibrary.h"

#include "config/GameConfigLoader.h"

#include "scripting/AS/CASManager.h"

#include "utils/command_utils.h"
#include "utils/json_utils.h"

bool CGameLibrary::InitializeCommon()
{
	if (!FileSystem_LoadFileSystem())
	{
		Con_Printf("Could not load filesystem library\n");
		return false;
	}

	g_pDeveloper = g_ConCommands.GetCVar("developer");

	//These systems have to initialize in a specific order because they depend on each-other
	{
		if (!g_Logging->Initialize())
		{
			Con_Printf("Could not initialize logging system\n");
			return false;
		}

		if (!g_ConCommands.Initialize())
		{
			Con_Printf("Could not initialize console command system\n");
			return false;
		}

		if (!g_JSON.Initialize())
		{
			Con_Printf("Could not initialize JSON system\n");
			return false;
		}

		if (!g_Logging->PostInitialize())
		{
			Con_Printf("Could not post-initialize logging system\n");
			return false;
		}

		if (!g_ConCommands.PostInitialize())
		{
			Con_Printf("Could not post-initialize console command system\n");
			return false;
		}

		if (!g_JSON.PostInitialize())
		{
			Con_Printf("Could not post-initialize JSON system\n");
			return false;
		}
	}

	if (!g_ASManager.Initialize())
	{
		Con_Printf("Could not initialize Angelscript engine manager\n");
		return false;
	}

	//Depends on Angelscript
	if (!g_GameConfigLoader.Initialize())
	{
		Con_Printf("Could not initialize game configuration loader\n");
		return false;
	}

	return true;
}

void CGameLibrary::ShutdownCommon()
{
	g_GameConfigLoader.Shutdown();
	g_ASManager.Shutdown();
	g_JSON.Shutdown();
	g_ConCommands.Shutdown();
	g_Logging->Shutdown();
	FileSystem_FreeFileSystem();
}
