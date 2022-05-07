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

// Com_Weapons.cpp
// Shared weapons common/shared functions
#include "hud.h"
#include "com_weapons.h"

#include "const.h"
#include "entity_state.h"
#include "r_efx.h"

// g_runfuncs is true if this is the first time we've "predicated" a particular movement/firing
//  command.  If it is 1, then we should play events/sounds etc., otherwise, we just will be
//  updating state info, but not firing events
bool g_runfuncs = false;

// During our weapon prediction processing, we'll need to reference some data that is part of
//  the final state passed into the postthink functionality.  We'll set this pointer and then
//  reset it to nullptr as appropriate
struct local_state_s* g_finalstate = nullptr;

/*
====================
COM_Log

Log debug messages to file ( appends )
====================
*/
void COM_Log(const char* pszFile, const char* fmt, ...)
{
	va_list argptr;
	char string[1024];
	FILE* fp;
	const char* pfilename;

	if (!pszFile)
	{
		pfilename = "c:\\hllog.txt";
	}
	else
	{
		pfilename = pszFile;
	}

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	fp = fopen(pfilename, "a+t");
	if (fp)
	{
		fprintf(fp, "%s", string);
		fclose(fp);
	}
}

// remember the current animation for the view model, in case we get out of sync with
//  server.
static int g_currentanim;

/*
=====================
HUD_SendWeaponAnim

Change weapon model animation
=====================
*/
void HUD_SendWeaponAnim(int iAnim, int body, bool force)
{
	// Don't actually change it.
	if (!g_runfuncs && !force)
		return;

	g_currentanim = iAnim;

	// Tell animation system new info
	gEngfuncs.pfnWeaponAnim(iAnim, body);
}

/*
=====================
HUD_GetWeaponAnim

Retrieve current predicted weapon animation
=====================
*/
int HUD_GetWeaponAnim()
{
	return g_currentanim;
}

/*
=====================
HUD_PlaySound

Play a sound, if we are seeing this command for the first time
=====================
*/
void HUD_PlaySound(const char* sound, float volume)
{
	if (!g_runfuncs || !g_finalstate)
		return;

	gEngfuncs.pfnPlaySoundByNameAtLocation(sound, volume, g_finalstate->playerstate.origin);
}

/*
=====================
HUD_PlaybackEvent

Directly queue up an event on the client
=====================
*/
void HUD_PlaybackEvent(int flags, const edict_t* pInvoker, unsigned short eventindex, float delay,
	const float* origin, const float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2)
{
	Vector org;
	Vector ang;

	if (!g_runfuncs || !g_finalstate)
		return;

	// Weapon prediction events are assumed to occur at the player's origin
	org = g_finalstate->playerstate.origin;
	ang = v_client_aimangles;
	gEngfuncs.pfnPlaybackEvent(flags, pInvoker, eventindex, delay, org, ang, fparam1, fparam2, iparam1, iparam2, bparam1, bparam2);
}

/*
=====================
HUD_SetMaxSpeed

=====================
*/
void HUD_SetMaxSpeed(const edict_t* ed, float speed)
{
}


/*
=====================
UTIL_WeaponTimeBase

Always 0.0 on client, even if not predicting weapons ( won't get called
 in that case )
=====================
*/
float UTIL_WeaponTimeBase()
{
	return 0.0;
}

/*
======================
stub_*

stub functions for such things as precaching.  So we don't have to modify weapons code that
 is compiled into both game and client .dlls.
======================
*/
int stub_PrecacheModel(const char* s) { return 0; }
int stub_PrecacheSound(const char* s) { return 0; }
unsigned short stub_PrecacheEvent(int type, const char* s) { return 0; }
const char* stub_NameForFunction(uint32 function) { return "func"; }
void stub_SetModel(edict_t* e, const char* m) {}
