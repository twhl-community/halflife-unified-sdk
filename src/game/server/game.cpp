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
#include "ServerLibrary.h"
#include "UserMessages.h"

cvar_t displaysoundlist = {"displaysoundlist", "0"};

// multiplayer server rules
cvar_t fragsleft = {"mp_fragsleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED}; // Don't spam console/log files/users with this changing
cvar_t timeleft = {"mp_timeleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED};	 // "      "

// multiplayer server rules
cvar_t fraglimit = {"mp_fraglimit", "0", FCVAR_SERVER};
cvar_t timelimit = {"mp_timelimit", "0", FCVAR_SERVER};
cvar_t friendlyfire = {"mp_friendlyfire", "0", FCVAR_SERVER};
cvar_t forcerespawn = {"mp_forcerespawn", "1", FCVAR_SERVER};
cvar_t aimcrosshair = {"mp_autocrosshair", "1", FCVAR_SERVER};
cvar_t decalfrequency = {"decalfrequency", "30", FCVAR_SERVER};
cvar_t teamlist = {"mp_teamlist", "hgrunt;scientist", FCVAR_SERVER};
cvar_t teamoverride = {"mp_teamoverride", "1"};
cvar_t defaultteam = {"mp_defaultteam", "0"};

cvar_t allow_spectators = {"allow_spectators", "0.0", FCVAR_SERVER}; // 0 prevents players from being spectators

cvar_t mp_chattime = {"mp_chattime", "10", FCVAR_SERVER};

cvar_t sv_allowbunnyhopping = {"sv_allowbunnyhopping", "0", FCVAR_SERVER};

// BEGIN Opposing Force variables

cvar_t ctfplay = {"mp_ctfplay", "0", FCVAR_SERVER};
cvar_t ctf_autoteam = {"mp_ctf_autoteam", "0", FCVAR_SERVER};
cvar_t ctf_capture = {"mp_ctf_capture", "0", FCVAR_SERVER};

cvar_t spamdelay = {"sv_spamdelay", "3.0", FCVAR_SERVER};
cvar_t multipower = {"mp_multipower", "0", FCVAR_SERVER};

// END Opposing Force variables

cvar_t sv_entityinfo_enabled{"sv_entityinfo_enabled", "0", FCVAR_SERVER};
cvar_t sv_entityinfo_eager{"sv_entityinfo_eager", "1", FCVAR_SERVER};

cvar_t sv_schedule_debug{"sv_schedule_debug", "0", FCVAR_SERVER};

static bool SV_InitServer()
{
	if (!FileSystem_LoadFileSystem())
	{
		return false;
	}

	if (UTIL_IsValveGameDirectory())
	{
		g_engfuncs.pfnServerPrint("This mod has detected that it is being run from a Valve game directory which is not supported\n"
								  "Run this mod from its intended location\n\nThe game will now shut down\n");
		return false;
	}

	if (!g_Server.Initialize())
	{
		return false;
	}

	return true;
}

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit()
{
	// Register cvars here:

	g_psv_gravity = CVAR_GET_POINTER("sv_gravity");
	g_psv_aim = CVAR_GET_POINTER("sv_aim");
	g_footsteps = CVAR_GET_POINTER("mp_footsteps");
	g_psv_cheats = CVAR_GET_POINTER("sv_cheats");

	if (!SV_InitServer())
	{
		g_engfuncs.pfnServerPrint("Error initializing server\n");
		// Shut the game down as soon as possible.
		SERVER_COMMAND("quit\n");
		return;
	}

	CVAR_REGISTER(&displaysoundlist);
	CVAR_REGISTER(&allow_spectators);

	CVAR_REGISTER(&fraglimit);
	CVAR_REGISTER(&timelimit);

	CVAR_REGISTER(&fragsleft);
	CVAR_REGISTER(&timeleft);

	CVAR_REGISTER(&friendlyfire);
	CVAR_REGISTER(&forcerespawn);
	CVAR_REGISTER(&aimcrosshair);
	CVAR_REGISTER(&decalfrequency);
	CVAR_REGISTER(&teamlist);
	CVAR_REGISTER(&teamoverride);
	CVAR_REGISTER(&defaultteam);

	CVAR_REGISTER(&mp_chattime);

	CVAR_REGISTER(&sv_allowbunnyhopping);

	// BEGIN REGISTER CVARS FOR OPPOSING FORCE

	CVAR_REGISTER(&ctfplay);
	CVAR_REGISTER(&ctf_autoteam);
	CVAR_REGISTER(&ctf_capture);

	CVAR_REGISTER(&spamdelay);
	CVAR_REGISTER(&multipower);

	CVAR_REGISTER(&sv_entityinfo_enabled);
	CVAR_REGISTER(&sv_entityinfo_eager);

	// END REGISTER CVARS FOR OPPOSING FORCE

	// Default to on in debug builds to match original behavior.
#ifdef DEBUG
	sv_schedule_debug.string = "1";
#endif

	CVAR_REGISTER(&sv_schedule_debug);

	// Link user messages immediately so there are no race conditions.
	LinkUserMessages();
}

void GameDLLShutdown()
{
	g_Server.Shutdown();
}
