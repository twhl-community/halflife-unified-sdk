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

#include "PlatformHeaders.h"

#include <delayimp.h>

#include <fmt/format.h>

#include "cbase.h"
#include "utils/filesystem_utils.h"

FARPROC DelayLoad_LoadGameLib(const char* dllName)
{
	ASSERT(dllName);

	const auto& gameDir = FileSystem_GetModDirectoryName();

	const auto path = fmt::format("{}/{}", gameDir, dllName);

	return reinterpret_cast<FARPROC>(LoadLibraryA(path.c_str()));
}

/*
 *	@brief Handles loading of shared delay loaded libraries
 */
HMODULE DelayLoad_HandleSharedLibs(unsigned dliNotify, PDelayLoadInfo pdli)
{
	return nullptr;
}

FARPROC WINAPI DelayHook(
	unsigned dliNotify,
	PDelayLoadInfo pdli)
{
	if (dliNotify == dliNotePreLoadLibrary)
	{
		if (strcmp(pdli->szDll, "openal-hlu.dll") == 0)
		{
			return DelayLoad_LoadGameLib("cl_dlls/openal-hlu.dll");
		}
	}

	return nullptr;
}

ExternC const PfnDliHook __pfnDliNotifyHook2 = DelayHook;

ExternC const PfnDliHook __pfnDliFailureHook2 = nullptr;
