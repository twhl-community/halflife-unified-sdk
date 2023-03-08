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
#include "cbase.h"

#include "entity_state.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"

#include "cl_dll.h"
#include "com_weapons.h"
#include "view.h"
#include "prediction/ClientPredictionSystem.h"

LINK_ENTITY_TO_CLASS(player, CBasePlayer);

// Local version of game .dll global variables ( time, etc. )
static globalvars_t Globals;

float g_flApplyVel = 0.0;
bool g_irunninggausspred = false;

Vector previousorigin;

int giTeamplay = 0;

/*
======================
AlertMessage

Print debug messages to console
======================
*/
void AlertMessage(ALERT_TYPE atype, const char* szFmt, ...)
{
	va_list argptr;
	static char string[8192];

	va_start(argptr, szFmt);
	vsprintf(string, szFmt, argptr);
	va_end(argptr);

	gEngfuncs.Con_DPrintf("cl:  ");
	gEngfuncs.Con_DPrintf(string);
}

void ServerPrint(const char* msg)
{
	gEngfuncs.Con_Printf("%s", msg);
}

void AddServerCommand(const char* cmd_name, void (*function)())
{
	gEngfuncs.pfnAddCommand(cmd_name, function);
}

// Just loads a v_ model.
void LoadVModel(const char* szViewModel, CBasePlayer* m_pPlayer)
{
	gEngfuncs.CL_LoadModel(szViewModel, reinterpret_cast<int*>(&m_pPlayer->pev->viewmodel.m_Value));
}

/*
=====================
CBaseEntity :: Killed

If weapons code "kills" an entity, just set its effects to EF_NODRAW
=====================
*/
void CBaseEntity::Killed(CBaseEntity* attacker, int iGib)
{
	pev->effects |= EF_NODRAW;
}

/*
=====================
CBasePlayerWeapon :: DefaultDeploy

=====================
*/
bool CBasePlayerWeapon::DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body)
{
	if (!CanDeploy())
		return false;

	gEngfuncs.CL_LoadModel(szViewModel, reinterpret_cast<int*>(&m_pPlayer->pev->viewmodel.m_Value));

	SendWeaponAnim(iAnim, body);

	g_irunninggausspred = false;
	m_pPlayer->m_flNextAttack = 0.5;
	m_flTimeWeaponIdle = 1.0;
	return true;
}

/*
=====================
CBasePlayerWeapon :: PlayEmptySound

=====================
*/
bool CBasePlayerWeapon::PlayEmptySound()
{
	if (m_iPlayEmptySound)
	{
		HUD_PlaySound("weapons/357_cock1.wav", 0.8);
		m_iPlayEmptySound = false;
		return false;
	}
	return false;
}

/*
=====================
CBasePlayerWeapon::Holster

Put away weapon
=====================
*/
void CBasePlayerWeapon::Holster()
{
	m_fInReload = false; // cancel any reload in progress.
	g_irunninggausspred = false;
	m_pPlayer->pev->viewmodel = string_t::Null;
}

/*
=====================
CBasePlayerWeapon::SendWeaponAnim

Animate weapon model
=====================
*/
void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body)
{
	m_pPlayer->pev->weaponanim = iAnim;

	HUD_SendWeaponAnim(iAnim, body, false);
}

/*
=====================
CBaseEntity::FireBulletsPlayer

Only produces random numbers to match the server ones.
=====================
*/
Vector CBaseEntity::FireBulletsPlayer(unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, CBaseEntity* attacker, int shared_rand)
{
	float x = 0, y = 0, z;

	for (unsigned int iShot = 1; iShot <= cShots; iShot++)
	{
		if (attacker == nullptr)
		{
			// get circular gaussian spread
			do
			{
				x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
				y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
				z = x * x + y * y;
			} while (z > 1);
		}
		else
		{
			// Use player's random seed.
			//  get circular gaussian spread
			x = UTIL_SharedRandomFloat(shared_rand + iShot, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (1 + iShot), -0.5, 0.5);
			y = UTIL_SharedRandomFloat(shared_rand + (2 + iShot), -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (3 + iShot), -0.5, 0.5);
			z = x * x + y * y;
		}
	}

	return Vector(x * vecSpread.x, y * vecSpread.y, 0.0);
}

/*
=====================
CBasePlayer::SelectItem

  Switch weapons
=====================
*/
void CBasePlayer::SelectItem(const char* pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem* pItem = nullptr;

	if (!pItem)
		return;


	if (pItem == m_pActiveItem)
		return;

	if (m_pActiveItem)
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy();
	}
}

/*
=====================
CBasePlayer::Killed

=====================
*/
void CBasePlayer::Killed(CBaseEntity* attacker, int iGib)
{
	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	g_irunninggausspred = false;
}

/*
=====================
CBasePlayer::Spawn

=====================
*/
void CBasePlayer::Spawn()
{
	if (m_pActiveItem)
		m_pActiveItem->Deploy();

	g_irunninggausspred = false;
}

/*
=====================
UTIL_TraceLine

Don't actually trace, but act like the trace didn't hit anything.
=====================
*/
void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, edict_t* pentIgnore, TraceResult* ptr)
{
	memset(ptr, 0, sizeof(*ptr));
	ptr->flFraction = 1.0;
}

/*
=====================
UTIL_ParticleBox

For debugging, draw a box around a player made out of particles
=====================
*/
void UTIL_ParticleBox(CBasePlayer* player, float* mins, float* maxs, float life, unsigned char r, unsigned char g, unsigned char b)
{
	int i;
	Vector mmin, mmax;

	for (i = 0; i < 3; i++)
	{
		mmin[i] = player->pev->origin[i] + mins[i];
		mmax[i] = player->pev->origin[i] + maxs[i];
	}

	gEngfuncs.pEfxAPI->R_ParticleBox(mmin, mmax, 5.0, 0, 255, 0);
}

/*
=====================
UTIL_ParticleBoxes

For debugging, draw boxes for other collidable players
=====================
*/
void UTIL_ParticleBoxes()
{
	int idx;
	physent_t* pe;
	cl_entity_t* player;
	Vector mins, maxs;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	player = gEngfuncs.GetLocalPlayer();
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(player->index - 1);

	for (idx = 1; idx < 100; idx++)
	{
		pe = gEngfuncs.pEventAPI->EV_GetPhysent(idx);
		if (!pe)
			break;

		if (pe->info >= 1 && pe->info <= gEngfuncs.GetMaxClients())
		{
			mins = pe->origin + pe->mins;
			maxs = pe->origin + pe->maxs;

			gEngfuncs.pEfxAPI->R_ParticleBox(mins, maxs, 0, 0, 255, 2.0);
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=====================
UTIL_ParticleLine

For debugging, draw a line made out of particles
=====================
*/
void UTIL_ParticleLine(CBasePlayer* player, float* start, float* end, float life, unsigned char r, unsigned char g, unsigned char b)
{
	gEngfuncs.pEfxAPI->R_ParticleLine(start, end, r, g, b, life);
}

void HUD_SetupServerEngineInterface()
{
	if (gpGlobals)
	{
		return;
	}

	// Set up pointer ( dummy object )
	gpGlobals = &Globals;

	// So strings use the correct base address.
	gpGlobals->pStringBase = "";

	// Fill in current time ( probably not needed )
	gpGlobals->time = gEngfuncs.GetClientTime();

	// Fake functions
	g_engfuncs.pfnPrecacheModel = stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound = stub_PrecacheSound;
	g_engfuncs.pfnPrecacheEvent = stub_PrecacheEvent;
	g_engfuncs.pfnNameForFunction = stub_NameForFunction;
	g_engfuncs.pfnSetModel = stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnPlaybackEvent = HUD_PlaybackEvent;
	g_engfuncs.pfnAlertMessage = AlertMessage;
	g_engfuncs.pfnServerPrint = ServerPrint;
	g_engfuncs.pfnAddServerCommand = AddServerCommand;
	g_engfuncs.pfnCreateEntity = CreateEntity;
	g_engfuncs.pfnFindEntityByVars = FindEntityByVars;

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent = gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat = gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong = gEngfuncs.pfnRandomLong;
	g_engfuncs.pfnCVarGetPointer = gEngfuncs.pfnGetCvarPointer;
	g_engfuncs.pfnCVarGetString = gEngfuncs.pfnGetCvarString;
	g_engfuncs.pfnCVarGetFloat = gEngfuncs.pfnGetCvarFloat;
	g_engfuncs.pfnCvar_DirectSet = [](cvar_t* cvar, const char* value)
	{ gEngfuncs.Cvar_Set(cvar->name, value); };
	g_engfuncs.pfnServerCommand = [](const char* cmd)
	{ gEngfuncs.pfnClientCmd(cmd); };
	g_engfuncs.pfnCheckParm = gEngfuncs.CheckParm;
	g_engfuncs.pfnCmd_Argc = gEngfuncs.Cmd_Argc;
	g_engfuncs.pfnCmd_Argv = gEngfuncs.Cmd_Argv;
}

/*
=====================
HUD_GetLastOrg

Retruns the last position that we stored for egon beam endpoint.
=====================
*/
Vector HUD_GetLastOrg()
{
	// Return last origin
	return previousorigin;
}

/*
=====================
HUD_SetLastOrg

Remember our exact predicted origin so we can draw the egon to the right position.
=====================
*/
void HUD_SetLastOrg()
{
	// Offset final origin by view_offset
	previousorigin = g_finalstate->playerstate.origin + g_finalstate->client.view_ofs;
}


void SetLocalBody(int id, int body)
{
	if (auto pWeapon = g_ClientPrediction.GetLocalWeapon(id); pWeapon)
	{
		pWeapon->pev->body = body;
	}
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void DLLEXPORT HUD_PostRunCmd(local_state_t* from, local_state_t* to, usercmd_t* cmd, int runfuncs, double time, unsigned int random_seed)
{
	g_runfuncs = 0 != runfuncs;

	g_ClientPrediction.InitializeEntities();

#if defined(CLIENT_WEAPONS)
	if (cl_lw && 0 != cl_lw->value)
	{
		g_ClientPrediction.WeaponsPostThink(from, to, cmd, time, random_seed);
	}
	else
#endif
	{
		to->client.fov = g_lastFOV;
	}

	if (g_irunninggausspred)
	{
		Vector forward;
		gEngfuncs.pfnAngleVectors(v_angles, forward, nullptr, nullptr);
		to->client.velocity = to->client.velocity - forward * g_flApplyVel * 5;
		g_irunninggausspred = false;
	}

	// All games can use FOV state
	g_lastFOV = to->client.fov;
}

bool UTIL_IsMultiplayer()
{
	return gEngfuncs.GetMaxClients() != 1;
}

bool UTIL_IsCTF()
{
	return giTeamplay == 2;
}
