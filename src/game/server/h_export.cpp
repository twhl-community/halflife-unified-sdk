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
/*
 *	@file
 *	Functions used by the engine to access the server dll.
 */

#include "cbase.h"
#include "client.h"
#include "pm_shared.h"

#ifdef WIN32
#define GIVEFNPTRSTODLL_DLLEXPORT __stdcall
#else
#define GIVEFNPTRSTODLL_DLLEXPORT DLLEXPORT
#endif

void DummySpectatorFunction(edict_t*)
{
	// Nothing
}

static DLL_FUNCTIONS gFunctionTable =
	{
		GameDLLInit,			   // pfnGameInit
		DispatchSpawn,			   // pfnSpawn
		DispatchThink,			   // pfnThink
		DispatchUse,			   // pfnUse
		DispatchTouch,			   // pfnTouch
		DispatchBlocked,		   // pfnBlocked
		DispatchKeyValue,		   // pfnKeyValue
		DispatchSave,			   // pfnSave
		DispatchRestore,		   // pfnRestore
		DispatchObjectCollsionBox, // pfnAbsBox

		SaveWriteFields, // pfnSaveWriteFields
		SaveReadFields,	 // pfnSaveReadFields

		SaveGlobalState,	// pfnSaveGlobalState
		RestoreGlobalState, // pfnRestoreGlobalState
		ResetGlobalState,	// pfnResetGlobalState

		ClientConnect,		   // pfnClientConnect
		ClientDisconnect,	   // pfnClientDisconnect
		ClientKill,			   // pfnClientKill
		ClientPutInServer,	   // pfnClientPutInServer
		ExecuteClientCommand,  // pfnClientCommand
		ClientUserInfoChanged, // pfnClientUserInfoChanged
		ServerActivate,		   // pfnServerActivate
		ServerDeactivate,	   // pfnServerDeactivate

		PlayerPreThink,	 // pfnPlayerPreThink
		PlayerPostThink, // pfnPlayerPostThink

		StartFrame,		  // pfnStartFrame
		ParmsNewLevel,	  // pfnParmsNewLevel
		ParmsChangeLevel, // pfnParmsChangeLevel

		GetGameDescription,	 // pfnGetGameDescription    Returns string describing current .dll game.
		PlayerCustomization, // pfnPlayerCustomization   Notifies .dll of new customization for player.

		DummySpectatorFunction, // pfnSpectatorConnect      Called when spectator joins server
		DummySpectatorFunction, // pfnSpectatorDisconnect   Called when spectator leaves the server
		DummySpectatorFunction, // pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

		Sys_Error, // pfnSys_Error				Called when engine has encountered an error

		PM_Move, // pfnPM_Move
		PM_Init, // pfnPM_Init				Server version of player movement initialization
		[](const char* name)
		{ return g_MaterialSystem.FindTextureType(name); }, // pfnPM_FindTextureType

		SetupVisibility,		  // pfnSetupVisibility        Set up PVS and PAS for networking for this client
		UpdateClientData,		  // pfnUpdateClientData       Set up data sent only to specific client
		AddToFullPack,			  // pfnAddToFullPack
		CreateBaseline,			  // pfnCreateBaseline			Tweak entity baseline for network encoding, allows setup of player baselines, too.
		RegisterEncoders,		  // pfnRegisterEncoders		Callbacks for network encoding
		GetWeaponData,			  // pfnGetWeaponData
		CmdStart,				  // pfnCmdStart
		CmdEnd,					  // pfnCmdEnd
		ConnectionlessPacket,	  // pfnConnectionlessPacket
		GetHullBounds,			  // pfnGetHullBounds
		CreateInstancedBaselines, // pfnCreateInstancedBaselines
		InconsistentFile,		  // pfnInconsistentFile
		AllowLagCompensation,	  // pfnAllowLagCompensation
};

NEW_DLL_FUNCTIONS gNewDLLFunctions =
	{
		OnFreeEntPrivateData, // pfnOnFreeEntPrivateData
		GameDLLShutdown,
};

extern "C" {
void GIVEFNPTRSTODLL_DLLEXPORT GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t* pGlobals)
{
	memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
	gpGlobals = pGlobals;
}

int DLLEXPORT GetEntityAPI(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion)
{
	if (!pFunctionTable || interfaceVersion != INTERFACE_VERSION)
	{
		return 0;
	}

	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));
	return 1;
}

int DLLEXPORT GetEntityAPI2(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != INTERFACE_VERSION)
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return 0;
	}

	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));
	return 1;
}

int DLLEXPORT GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return 0;
	}

	memcpy(pFunctionTable, &gNewDLLFunctions, sizeof(gNewDLLFunctions));
	return 1;
}

struct TITLECOMMENT
{
	const char* BSPName;
	const char* TitleName;
};

static constexpr TITLECOMMENT gTitleComments[66] =
	{
		{"t0a0", "#T0A0TITLE"},
		{"c0a0", "#C0A0TITLE"},
		{"c1a0", "#C0A1TITLE"},
		{"c1a1", "#C1A1TITLE"},
		{"c1a2", "#C1A2TITLE"},
		{"c1a3", "#C1A3TITLE"},
		{"c1a4", "#C1A4TITLE"},
		{"c2a1", "#C2A1TITLE"},
		{"c2a2", "#C2A2TITLE"},
		{"c2a3", "#C2A3TITLE"},
		{"c2a4d", "#C2A4TITLE2"},
		{"c2a4e", "#C2A4TITLE2"},
		{"c2a4f", "#C2A4TITLE2"},
		{"c2a4g", "#C2A4TITLE2"},
		{"c2a4", "#C2A4TITLE1"},
		{"c2a5", "#C2A5TITLE"},
		{"c3a1", "#C3A1TITLE"},
		{"c3a2", "#C3A2TITLE"},
		{"c4a1a", "#C4A1ATITLE"},
		{"c4a1b", "#C4A1ATITLE"},
		{"c4a1c", "#C4A1ATITLE"},
		{"c4a1d", "#C4A1ATITLE"},
		{"c4a1e", "#C4A1ATITLE"},
		{"c4a1", "#C4A1TITLE"},
		{"c4a2", "#C4A2TITLE"},
		{"c4a3", "#C4A3TITLE"},
		{"c5a1", "#C5TITLE"},
		{"ofboot", "#OF_BOOT0TITLE"},
		{"of0a", "#OF1A1TITLE"},
		{"of1a1", "#OF1A3TITLE"},
		{"of1a2", "#OF1A3TITLE"},
		{"of1a3", "#OF1A3TITLE"},
		{"of1a4", "#OF1A3TITLE"},
		{"of1a", "#OF1A5TITLE"},
		{"of2a1", "#OF2A1TITLE"},
		{"of2a2", "#OF2A1TITLE"},
		{"of2a3", "#OF2A1TITLE"},
		{"of2a", "#OF2A4TITLE"},
		{"of3a1", "#OF3A1TITLE"},
		{"of3a2", "#OF3A1TITLE"},
		{"of3a", "#OF3A3TITLE"},
		{"of4a1", "#OF4A1TITLE"},
		{"of4a2", "#OF4A1TITLE"},
		{"of4a3", "#OF4A1TITLE"},
		{"of4a", "#OF4A4TITLE"},
		{"of5a", "#OF5A1TITLE"},
		{"of6a1", "#OF6A1TITLE"},
		{"of6a2", "#OF6A1TITLE"},
		{"of6a3", "#OF6A1TITLE"},
		{"of6a4b", "#OF6A4TITLE"},
		{"of6a4", "#OF6A1TITLE"},
		{"of6a5", "#OF6A4TITLE"},
		{"of6a", "#OF6A4TITLE"},
		{"of7a", "#OF7A0TITLE"},
		{"ba_tram", "#BA_TRAMTITLE"},
		{"ba_security", "#BA_SECURITYTITLE"},
		{"ba_main", "#BA_SECURITYTITLE"},
		{"ba_elevator", "#BA_SECURITYTITLE"},
		{"ba_canal", "#BA_CANALSTITLE"},
		{"ba_yard", "#BA_YARDTITLE"},
		{"ba_xen", "#BA_XENTITLE"},
		{"ba_hazard", "#BA_HAZARD"},
		{"ba_power", "#BA_POWERTITLE"},
		{"ba_teleport1", "#BA_YARDTITLE"},
		{"ba_teleport", "#BA_TELEPORTTITLE"},
		{"ba_outro", "#BA_OUTRO"}};

/**
 *	@brief Called by the engine to get the level name shown in the save and load game dialogs.
 *	@param buffer Filled in by this function.
 *		Contains either a localizable token loaded from @c resource/valve_%language%.txt or a plain text title.
 */
void DLLEXPORT SV_SaveGameComment(char* buffer, int bufferSize)
{
	const char* mapName = STRING(gpGlobals->mapname);

	const char* titleName = nullptr;

	// See if the current map is in the hardcoded list of titles
	if (mapName)
	{
		for (const auto& comment : gTitleComments)
		{
			// See if the bsp name starts with this prefix
			if (!strnicmp(mapName, comment.BSPName, strlen(comment.BSPName)))
			{
				if (comment.TitleName)
				{
					titleName = comment.TitleName;
					break;
				}
			}
		}
	}

	if (!titleName)
	{
		// Slightly different behavior here compared to the engine
		// Instead of using maps/mapname.bsp we just use an empty string
		if (mapName)
		{
			titleName = mapName;
		}
		else
		{
			titleName = "";
		}
	}

	strncpy(buffer, titleName, bufferSize - 1);
	buffer[bufferSize - 1] = '\0';
}
}
