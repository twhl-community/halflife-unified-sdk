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

cvar_t displaysoundlist = {"displaysoundlist", "0"};

// multiplayer server rules
cvar_t fragsleft = {"mp_fragsleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED}; // Don't spam console/log files/users with this changing
cvar_t timeleft = {"mp_timeleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED};	 // "      "

// multiplayer server rules
cvar_t teamplay = {"mp_teamplay", "0", FCVAR_SERVER};
cvar_t fraglimit = {"mp_fraglimit", "0", FCVAR_SERVER};
cvar_t timelimit = {"mp_timelimit", "0", FCVAR_SERVER};
cvar_t friendlyfire = {"mp_friendlyfire", "0", FCVAR_SERVER};
cvar_t falldamage = {"mp_falldamage", "0", FCVAR_SERVER};
cvar_t weaponstay = {"mp_weaponstay", "0", FCVAR_SERVER};
cvar_t forcerespawn = {"mp_forcerespawn", "1", FCVAR_SERVER};
cvar_t flashlight = {"mp_flashlight", "0", FCVAR_SERVER};
cvar_t aimcrosshair = {"mp_autocrosshair", "1", FCVAR_SERVER};
cvar_t decalfrequency = {"decalfrequency", "30", FCVAR_SERVER};
cvar_t teamlist = {"mp_teamlist", "hgrunt;scientist", FCVAR_SERVER};
cvar_t teamoverride = {"mp_teamoverride", "1"};
cvar_t defaultteam = {"mp_defaultteam", "0"};
cvar_t allowmonsters = {"mp_allowmonsters", "0", FCVAR_SERVER};

cvar_t allow_spectators = {"allow_spectators", "0.0", FCVAR_SERVER}; // 0 prevents players from being spectators

cvar_t mp_chattime = {"mp_chattime", "10", FCVAR_SERVER};

// BEGIN Opposing Force variables

cvar_t ctfplay = {"mp_ctfplay", "0", FCVAR_SERVER};
cvar_t ctf_autoteam = {"mp_ctf_autoteam", "0", FCVAR_SERVER};
cvar_t ctf_capture = {"mp_ctf_capture", "0", FCVAR_SERVER};
cvar_t coopplay = {"mp_coopplay", "0", FCVAR_SERVER};
cvar_t defaultcoop = {"mp_defaultcoop", "0", FCVAR_SERVER};
cvar_t coopweprespawn = {"mp_coopweprespawn", "0", FCVAR_SERVER};

cvar_t oldgrapple = {"sv_oldgrapple", "0", FCVAR_SERVER};

cvar_t spamdelay = {"sv_spamdelay", "3.0", FCVAR_SERVER};
cvar_t multipower = {"mp_multipower", "0", FCVAR_SERVER};

// END Opposing Force variables

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit()
{
	// Register cvars here:

	g_psv_gravity = CVAR_GET_POINTER("sv_gravity");
	g_psv_aim = CVAR_GET_POINTER("sv_aim");
	g_footsteps = CVAR_GET_POINTER("mp_footsteps");
	g_psv_cheats = CVAR_GET_POINTER("sv_cheats");

	CVAR_REGISTER(&displaysoundlist);
	CVAR_REGISTER(&allow_spectators);

	CVAR_REGISTER(&teamplay);
	CVAR_REGISTER(&fraglimit);
	CVAR_REGISTER(&timelimit);

	CVAR_REGISTER(&fragsleft);
	CVAR_REGISTER(&timeleft);

	CVAR_REGISTER(&friendlyfire);
	CVAR_REGISTER(&falldamage);
	CVAR_REGISTER(&weaponstay);
	CVAR_REGISTER(&forcerespawn);
	CVAR_REGISTER(&flashlight);
	CVAR_REGISTER(&aimcrosshair);
	CVAR_REGISTER(&decalfrequency);
	CVAR_REGISTER(&teamlist);
	CVAR_REGISTER(&teamoverride);
	CVAR_REGISTER(&defaultteam);
	CVAR_REGISTER(&allowmonsters);

	CVAR_REGISTER(&mp_chattime);

	// BEGIN REGISTER CVARS FOR OPPOSING FORCE

	CVAR_REGISTER(&ctfplay);
	CVAR_REGISTER(&ctf_autoteam);
	CVAR_REGISTER(&ctf_capture);
	CVAR_REGISTER(&coopplay);
	CVAR_REGISTER(&defaultcoop);
	CVAR_REGISTER(&coopweprespawn);

	CVAR_REGISTER(&oldgrapple);

	CVAR_REGISTER(&spamdelay);
	CVAR_REGISTER(&multipower);

	// END REGISTER CVARS FOR OPPOSING FORCE

	if (!g_Server.Initialize())
	{
		//Shut the game down ASAP
		SERVER_COMMAND("quit\n");
		return;
	}
}

void GameDLLShutdown()
{
	g_Server.Shutdown();
}
