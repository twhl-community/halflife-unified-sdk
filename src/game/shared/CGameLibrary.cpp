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

bool CGameLibrary::InitializeCommon()
{
	if (!FileSystem_LoadFileSystem())
	{
		g_engfuncs.pfnServerPrint("Could not load filesystem library\n");
		return false;
	}

	if (!g_LogSystem.Initialize())
	{
		g_engfuncs.pfnServerPrint("Could not initialize logging system\n");
		return false;
	}

	return true;
}

void CGameLibrary::ShutdownCommon()
{
	g_LogSystem.Shutdown();
	FileSystem_FreeFileSystem();
}

