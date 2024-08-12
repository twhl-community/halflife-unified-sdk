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
#include "hud.h"

#include "cbase.h"

#include "CCrossbow.h"
#include "CCrowbar.h"
#include "CDisplacer.h"
#include "CEagle.h"
#include "CEgon.h"
#include "CGauss.h"
#include "CGlock.h"
#include "CHgun.h"
#include "CKnife.h"
#include "CMP5.h"
#include "CPipewrench.h"
#include "CPython.h"
#include "CM249.h"
#include "CPenguin.h"
#include "CRpg.h"
#include "CShockRifle.h"
#include "CShotgun.h"
#include "CSporeLauncher.h"
#include "CSniperRifle.h"
#include "CSqueak.h"
#include "CTripmine.h"

#include "ClientLibrary.h"
#include "com_weapons.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"

#include "r_studioint.h"
#include "com_model.h"
#include "sound/ISoundSystem.h"

extern engine_studio_api_t IEngineStudio;

static int g_tracerCount[MAX_PLAYERS];

#include "pm_shared.h"

void V_PunchAxis(int axis, float punch);

extern cvar_t* cl_lw;
extern cvar_t* r_decals;

static inline bool EV_HLDM_IsBSPModel(physent_t* pe)
{
	return pe != nullptr && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP);
}

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound(int idx, pmtrace_t* ptr, const Vector& vecSrc, const Vector& vecEnd, int iBulletType)
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	float fvolbar;
	const char* rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;

	const int entity = gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr);

	// FIXME check if playtexture sounds movevar is set
	//

	chTextureType = 0;

	// Player
	if (entity >= 1 && entity <= gEngfuncs.GetMaxClients())
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else if (entity == 0)
	{
		// get texture from entity or world (world is ent(0))
		const char* pTextureName = gEngfuncs.pEventAPI->EV_TraceTexture(ptr->ent, vecSrc, vecEnd);

		if (pTextureName)
		{
			pTextureName = g_MaterialSystem.StripTexturePrefix(pTextureName);

			// get texture type
			chTextureType = g_MaterialSystem.FindTextureType(pTextureName);
		}
	}

	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		fvol = 0.9;
		fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL:
		fvol = 0.9;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:
		fvol = 0.9;
		fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:
		fvol = 0.5;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE:
		fvol = 0.9;
		fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH:
		fvol = 0.9;
		fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD:
		fvol = 0.9;
		fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if (iBulletType == BULLET_PLAYER_CROWBAR)
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;
		fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	case CHAR_TEX_SNOW:
		fvol = 0.9;
		fvolbar = 0.1;
		rgsz[0] = "player/pl_snow1.wav";
		rgsz[1] = "player/pl_snow2.wav";
		rgsz[2] = "player/pl_snow3.wav";
		cnt = 3;
		break;
	}

	// play material hit sound
	EV_PlaySound(0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0, cnt - 1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0, 0xf));
	return fvolbar;
}

char* EV_HLDM_DamageDecal(physent_t* pe)
{
	static char decalname[32];
	int idx;

	if (pe->rendermode == kRenderTransAlpha)
		return nullptr;

	if (pe->classnumber == 1)
	{
		idx = gEngfuncs.pfnRandomLong(0, 2);
		sprintf(decalname, "{break%i", idx + 1);
	}
	else if (pe->rendermode != kRenderNormal)
	{
		sprintf(decalname, "{bproof1");
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong(0, 4);
		sprintf(decalname, "{shot%i", idx + 1);
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace(pmtrace_t* pTrace, char* decalName)
{
	int iRand;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles(pTrace->endpos);

	iRand = gEngfuncs.pfnRandomLong(0, 0x7FFF);
	if (iRand < (0x7fff / 2)) // not every bullet makes a sound.
	{
		switch (iRand % 5)
		{
		case 0:
			EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 1:
			EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 2:
			EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 3:
			EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		case 4:
			EV_PlaySound(-1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
			break;
		}
	}

	// Only decal brush models such as the world etc.
	if (decalName && '\0' != decalName[0] && r_decals->value > 0)
	{
		gEngfuncs.pEfxAPI->R_DecalShoot(
			gEngfuncs.pEfxAPI->Draw_DecalIndex(gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalName)),
			gEngfuncs.pEventAPI->EV_IndexFromTrace(pTrace), 0, pTrace->endpos, 0);
	}
}

void EV_HLDM_DecalGunshot(pmtrace_t* pTrace, int iBulletType)
{
	physent_t* pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent(pTrace->ent);

	if (EV_HLDM_IsBSPModel(pe))
	{
		switch (iBulletType)
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		case BULLET_PLAYER_556:
		case BULLET_PLAYER_762:
		case BULLET_PLAYER_EAGLE:
		default:
			// smoke and decal
			EV_HLDM_GunshotDecalTrace(pTrace, EV_HLDM_DamageDecal(pe));
			break;
		}
	}
}

void EV_HLDM_CheckTracer(int idx, const Vector& vecSrc, const Vector& end, const Vector& forward, const Vector& right,
	int iBulletType, int iTracerFreq, int* tracerCount)
{
	int i;
	const bool player = idx >= 1 && idx <= gEngfuncs.GetMaxClients();

	if (iTracerFreq != 0 && ((*tracerCount)++ % iTracerFreq) == 0)
	{
		Vector vecTracerSrc;

		if (player)
		{
			Vector offset(0, 0, -4);

			// adjust tracer position for player
			for (i = 0; i < 3; i++)
			{
				vecTracerSrc[i] = vecSrc[i] + offset[i] + right[i] * 2 + forward[i] * 16;
			}
		}
		else
		{
			vecTracerSrc = vecSrc;
		}

		switch (iBulletType)
		{
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_MONSTER_9MM:
		case BULLET_MONSTER_12MM:
		case BULLET_PLAYER_556:
		case BULLET_PLAYER_762:
		case BULLET_PLAYER_EAGLE:
		default:
			EV_CreateTracer(vecTracerSrc, end);
			break;
		}
	}
}


/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets(int idx,
	const Vector& forward, const Vector& right, const Vector& up,
	int cShots, const Vector& vecSrc, const Vector& vecDirShooting,
	float flDistance, int iBulletType, int iTracerFreq, int* tracerCount, float flSpreadX, float flSpreadY)
{
	int i;
	pmtrace_t tr;
	int iShot;

	for (iShot = 1; iShot <= cShots; iShot++)
	{
		Vector vecDir, vecEnd;

		float x, y, z;
		// We randomize for the Shotgun.
		if (iBulletType == BULLET_PLAYER_BUCKSHOT)
		{
			do
			{
				x = gEngfuncs.pfnRandomFloat(-0.5, 0.5) + gEngfuncs.pfnRandomFloat(-0.5, 0.5);
				y = gEngfuncs.pfnRandomFloat(-0.5, 0.5) + gEngfuncs.pfnRandomFloat(-0.5, 0.5);
				z = x * x + y * y;
			} while (z > 1);

			for (i = 0; i < 3; i++)
			{
				vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[i] + y * flSpreadY * up[i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		} // But other guns already have their spread randomized in the synched spread.
		else
		{

			for (i = 0; i < 3; i++)
			{
				vecDir[i] = vecDirShooting[i] + flSpreadX * right[i] + flSpreadY * up[i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
		gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

		EV_HLDM_CheckTracer(idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount);

		// do damage, paint decals
		if (tr.fraction != 1.0)
		{
			switch (iBulletType)
			{
			default:
			case BULLET_PLAYER_9MM:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);

				break;
			case BULLET_PLAYER_MP5:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);
				break;
			case BULLET_PLAYER_BUCKSHOT:

				EV_HLDM_DecalGunshot(&tr, iBulletType);

				break;
			case BULLET_PLAYER_357:

				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);

				break;

			case BULLET_PLAYER_EAGLE:
				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);
				break;

			case BULLET_PLAYER_762:
				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);
				break;

			case BULLET_PLAYER_556:
				EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, iBulletType);
				EV_HLDM_DecalGunshot(&tr, iBulletType);
				break;
			}
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

//======================
//	    GLOCK START
//======================
void EV_FireGlock1(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;
	velocity = args->velocity;

	const bool empty = 0 != args->bparam1;
	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(empty ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, 0);

		V_PunchAxis(0, -2.0);
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);

	EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));

	EV_GetGunPosition(args, vecSrc, origin);

	vecAiming = forward;

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, &g_tracerCount[idx - 1], args->fparam1, args->fparam2);
}

void EV_FireGlock2(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;
	velocity = args->velocity;

	const bool empty = 0 != args->bparam1;
	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(empty ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, 0);

		V_PunchAxis(0, -2.0);
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);

	EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));

	EV_GetGunPosition(args, vecSrc, origin);

	vecAiming = forward;

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, &g_tracerCount[idx - 1], args->fparam1, args->fparam2);
}
//======================
//	   GLOCK END
//======================

//======================
//	  SHOTGUN START
//======================
void EV_FireShotGunDouble(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	int j;
	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;
	velocity = args->velocity;

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shotgunshell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SHOTGUN_FIRE2, 0);
		V_PunchAxis(0, -10.0);
	}

	for (j = 0; j < 2; j++)
	{
		EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6);

		EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHOTSHELL);
	}

	EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/dbarrel1.wav", gEngfuncs.pfnRandomFloat(0.98, 1.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong(0, 0x1f));

	EV_GetGunPosition(args, vecSrc, origin);
	vecAiming = forward;

	if (g_Skill.GetValue("shotgun_double_wide_spread") != 0)
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 8, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0,
			&g_tracerCount[idx - 1], VECTOR_CONE_DM_DOUBLESHOTGUN.x, VECTOR_CONE_DM_DOUBLESHOTGUN.y);
	}
	else
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 12, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0,
			&g_tracerCount[idx - 1], VECTOR_CONE_10DEGREES.x, VECTOR_CONE_10DEGREES.y);
	}
}

void EV_FireShotGunSingle(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;
	velocity = args->velocity;

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shotgunshell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SHOTGUN_FIRE, 0);

		V_PunchAxis(0, -5.0);
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHOTSHELL);

	EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0x1f));

	EV_GetGunPosition(args, vecSrc, origin);
	vecAiming = forward;

	if (g_Skill.GetValue("shotgun_single_tight_spread") != 0)
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 4, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0,
			&g_tracerCount[idx - 1], VECTOR_CONE_DM_SHOTGUN.x, VECTOR_CONE_DM_SHOTGUN.y);
	}
	else
	{
		EV_HLDM_FireBullets(idx, forward, right, up, 6, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0,
			&g_tracerCount[idx - 1], VECTOR_CONE_10DEGREES.x, VECTOR_CONE_10DEGREES.y);
	}
}
//======================
//	   SHOTGUN END
//======================

//======================
//	    MP5 START
//======================
void EV_FireMP5(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector ShellVelocity;
	Vector ShellOrigin;
	int shell;
	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;
	velocity = args->velocity;

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl"); // brass shell

	if (EV_IsLocal(idx))
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(MP5_FIRE1 + gEngfuncs.pfnRandomLong(0, 2), 0);

		V_PunchAxis(0, gEngfuncs.pfnRandomFloat(-2, 2));
	}

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL);

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	case 1:
		EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	}

	EV_GetGunPosition(args, vecSrc, origin);
	vecAiming = forward;

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, &g_tracerCount[idx - 1], args->fparam1, args->fparam2);
}

// We only predict the animation and sound
// The grenade is still launched from the server.
void EV_FireMP52(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	origin = args->origin;

	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(MP5_LAUNCH, 0);
		V_PunchAxis(0, -10);
	}

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/glauncher.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	case 1:
		EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/glauncher2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 0xf));
		break;
	}
}
//======================
//		 MP5 END
//======================

//======================
//	   PHYTON START
//	     ( .357 )
//======================
void EV_FirePython(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	Vector vecSrc, vecAiming;
	Vector up, right, forward;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;
	velocity = args->velocity;

	AngleVectors(angles, forward, right, up);

	if (EV_IsLocal(idx))
	{
		const int body = g_Skill.GetValue("revolver_laser_sight") != 0 ? 1 : 0;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(PYTHON_FIRE1, body);

		V_PunchAxis(0, -10.0);

		SetLocalBody(WEAPON_PYTHON, body);
	}

	switch (gEngfuncs.pfnRandomLong(0, 1))
	{
	case 0:
		EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/357_shot1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	case 1:
		EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/357_shot2.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		break;
	}

	EV_GetGunPosition(args, vecSrc, origin);

	vecAiming = forward;

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_357, 0, &g_tracerCount[idx - 1], args->fparam1, args->fparam2);
}
//======================
//	    PHYTON END
//	     ( .357 )
//======================

//======================
//	   GAUSS START
//======================
void EV_SpinGauss(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;
	int iSoundState = 0;

	int pitch;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;
	velocity = args->velocity;

	pitch = args->iparam1;

	iSoundState = 0 != args->bparam1 ? SND_CHANGE_PITCH : 0;

	EV_PlaySound(idx, origin, CHAN_WEAPON, "ambience/pulsemachine.wav", 1.0, ATTN_NORM, iSoundState, pitch);
}

/*
==============================
EV_StopPreviousGauss

==============================
*/
void EV_StopPreviousGauss(int idx)
{
	// Make sure we don't have a gauss spin event in the queue for this guy
	gEngfuncs.pEventAPI->EV_KillEvents(idx, "events/gaussspin.sc");
	EV_StopSound(idx, CHAN_WEAPON, "ambience/pulsemachine.wav");
}

extern float g_flApplyVel;

void EV_FireGauss(event_args_t* args)
{
	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;
	float flDamage = args->fparam1;

	const bool m_fPrimaryFire = 0 != args->bparam1;
	Vector vecSrc;
	Vector vecDest;
	edict_t* pentIgnore;
	pmtrace_t tr, beam_tr;
	float flMaxFrac = 1.0;
	bool fHasPunched = false;
	bool fFirstBeam = true;
	int nMaxHits = 10;
	physent_t* pEntity;
	int m_iBeam, m_iGlow, m_iBalls;
	Vector up, right, forward;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;
	velocity = args->velocity;

	if (0 != args->bparam2)
	{
		EV_StopPreviousGauss(idx);
		return;
	}

	//	Con_Printf( "Firing gauss with %f\n", flDamage );
	EV_GetGunPosition(args, vecSrc, origin);

	m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke.spr");
	m_iBalls = m_iGlow = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/hotglow.spr");

	AngleVectors(angles, forward, right, up);

	vecDest = vecSrc + (8192 * forward);

	if (EV_IsLocal(idx))
	{
		V_PunchAxis(0, -2.0);
		gEngfuncs.pEventAPI->EV_WeaponAnimation(GAUSS_FIRE2, 0);

		if (!m_fPrimaryFire)
			g_flApplyVel = flDamage;
	}

	EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/gauss2.wav", 0.5 + flDamage * (1.0 / 400.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong(0, 0x1f));

	while (flDamage > 10 && nMaxHits > 0)
	{
		nMaxHits--;

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
		gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecDest, PM_STUDIO_BOX, -1, &tr);

		gEngfuncs.pEventAPI->EV_PopPMStates();

		if (0 != tr.allsolid)
			break;

		if (fFirstBeam)
		{
			if (EV_IsLocal(idx))
			{
				// Add muzzle flash to current weapon model
				EV_MuzzleFlash();
			}
			fFirstBeam = false;

			gEngfuncs.pEfxAPI->R_BeamEntPoint(
				idx | 0x1000,
				tr.endpos,
				m_iBeam,
				0.1,
				m_fPrimaryFire ? 1.0 : 2.5,
				0.0,
				(m_fPrimaryFire ? 128.0 : flDamage) / 255.0,
				0,
				0,
				0,
				(m_fPrimaryFire ? 255 : 255) / 255.0,
				(m_fPrimaryFire ? 128 : 255) / 255.0,
				(m_fPrimaryFire ? 0 : 255) / 255.0);
		}
		else
		{
			gEngfuncs.pEfxAPI->R_BeamPoints(vecSrc,
				tr.endpos,
				m_iBeam,
				0.1,
				m_fPrimaryFire ? 1.0 : 2.5,
				0.0,
				(m_fPrimaryFire ? 128.0 : flDamage) / 255.0,
				0,
				0,
				0,
				(m_fPrimaryFire ? 255 : 255) / 255.0,
				(m_fPrimaryFire ? 128 : 255) / 255.0,
				(m_fPrimaryFire ? 0 : 255) / 255.0);
		}

		pEntity = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);
		if (pEntity == nullptr)
			break;

		if (EV_HLDM_IsBSPModel(pEntity))
		{
			float n;

			pentIgnore = nullptr;

			n = -DotProduct(tr.plane.normal, forward);

			if (n < 0.5) // 60 degrees
			{
				// CBasePlayerWeapon::WeaponsLogger->debug("reflect {}", n);
				// reflect
				Vector r;

				r = forward + ((2.0 * n) * tr.plane.normal);

				flMaxFrac = flMaxFrac - tr.fraction;

				forward = r;

				vecSrc = tr.endpos + (8.0 * forward);
				vecDest = vecSrc + (8192.0 * forward);

				gEngfuncs.pEfxAPI->R_TempSprite(tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage * n / 255.0, flDamage * n * 0.5 * 0.1, FTENT_FADEOUT);

				Vector fwd;
				fwd = tr.endpos + tr.plane.normal;

				gEngfuncs.pEfxAPI->R_Sprite_Trail(TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat(10, 20) / 100.0, 100,
					255, 100);

				// lose energy
				if (n == 0)
				{
					n = 0.1;
				}

				flDamage = flDamage * (1 - n);
			}
			else
			{
				// tunnel
				EV_HLDM_DecalGunshot(&tr, BULLET_MONSTER_12MM);

				gEngfuncs.pEfxAPI->R_TempSprite(tr.endpos, vec3_origin, 1.0, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT);

				// limit it to one hole punch
				if (fHasPunched)
				{
					break;
				}
				fHasPunched = true;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if (!m_fPrimaryFire)
				{
					Vector start;

					start = tr.endpos + (8.0 * forward);

					// Store off the old count
					gEngfuncs.pEventAPI->EV_PushPMStates();

					// Now add in all of the players.
					gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

					gEngfuncs.pEventAPI->EV_SetTraceHull(2);
					gEngfuncs.pEventAPI->EV_PlayerTrace(start, vecDest, PM_STUDIO_BOX, -1, &beam_tr);

					if (0 == beam_tr.allsolid)
					{
						// trace backwards to find exit point

						gEngfuncs.pEventAPI->EV_PlayerTrace(beam_tr.endpos, tr.endpos, PM_STUDIO_BOX, -1, &beam_tr);

						const Vector delta = beam_tr.endpos - tr.endpos;

						float deltaLength = delta.Length();

						if (deltaLength < flDamage)
						{
							if (deltaLength == 0)
								deltaLength = 1;
							flDamage -= deltaLength;

							// absorption balls
							{
								Vector fwd;
								fwd = tr.endpos - forward;
								gEngfuncs.pEfxAPI->R_Sprite_Trail(TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat(10, 20) / 100.0, 100,
									255, 100);
							}

							//////////////////////////////////// WHAT TO DO HERE
							// CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

							EV_HLDM_DecalGunshot(&beam_tr, BULLET_MONSTER_12MM);

							gEngfuncs.pEfxAPI->R_TempSprite(beam_tr.endpos, vec3_origin, 0.1, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT);

							// balls
							{
								Vector fwd;
								fwd = beam_tr.endpos - forward;
								gEngfuncs.pEfxAPI->R_Sprite_Trail(TE_SPRITETRAIL, beam_tr.endpos, fwd, m_iBalls, (int)(flDamage * 0.3), 0.1, gEngfuncs.pfnRandomFloat(10, 20) / 100.0, 200,
									255, 40);
							}

							vecSrc = beam_tr.endpos + forward;
						}
					}
					else
					{
						flDamage = 0;
					}

					gEngfuncs.pEventAPI->EV_PopPMStates();
				}
				else
				{
					if (m_fPrimaryFire)
					{
						// slug doesn't punch through ever with primary
						// fire, so leave a little glowy bit and make some balls
						gEngfuncs.pEfxAPI->R_TempSprite(tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 200.0 / 255.0, 0.3, FTENT_FADEOUT);

						{
							Vector fwd;
							fwd = tr.endpos + tr.plane.normal;
							gEngfuncs.pEfxAPI->R_Sprite_Trail(TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 8, 0.6, gEngfuncs.pfnRandomFloat(10, 20) / 100.0, 100,
								255, 200);
						}
					}

					flDamage = 0;
				}
			}
		}
		else
		{
			vecSrc = tr.endpos + forward;
		}
	}
}
//======================
//	   GAUSS END
//======================

//======================
//	   CROWBAR START
//======================
int g_iSwing;

// Only predict the miss sounds, hit sounds are still played
// server side, so players don't get the wrong idea.
void EV_Crowbar(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	origin = args->origin;

	// Play Swing sound
	EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM);

	if (EV_IsLocal(idx))
	{
		switch ((g_iSwing++) % 3)
		{
		case 0:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROWBAR_ATTACK1MISS, 0);
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROWBAR_ATTACK2MISS, 0);
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROWBAR_ATTACK3MISS, 0);
			break;
		}
	}
}
//======================
//	   CROWBAR END
//======================

//======================
//	  CROSSBOW START
//======================
//=====================
// EV_BoltCallback
// This function is used to correct the origin and angles
// of the bolt, so it looks like it's stuck on the wall.
//=====================
void EV_BoltCallback(TEMPENTITY* ent, float frametime, float currenttime)
{
	ent->entity.origin = ent->entity.baseline.vuser1;
	ent->entity.angles = ent->entity.baseline.vuser2;
}

void EV_FireCrossbow2(event_args_t* args)
{
	Vector vecSrc, vecEnd;
	Vector up, right, forward;
	pmtrace_t tr;

	int idx;
	Vector origin;
	Vector angles;
	Vector velocity;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;

	velocity = args->velocity;

	AngleVectors(angles, forward, right, up);

	EV_GetGunPosition(args, vecSrc, origin);

	vecEnd = vecSrc + (8192 * forward);

	EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));
	EV_PlaySound(idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));

	if (EV_IsLocal(idx))
	{
		if (0 != args->iparam1)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROSSBOW_FIRE1, 0);
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROSSBOW_FIRE3, 0);
	}

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

	// We hit something
	if (tr.fraction < 1.0)
	{
		physent_t* pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);

		// Not the world, let's assume we hit something organic ( dog, cat, uncle joe, etc ).
		if (!EV_HLDM_IsBSPModel(pe))
		{
			switch (gEngfuncs.pfnRandomLong(0, 1))
			{
			case 0:
				EV_PlaySound(idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			case 1:
				EV_PlaySound(idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			}
		}
		// Stick to world but don't stick to glass, it might break and leave the bolt floating. It can still stick to other non-transparent breakables though.
		else if (pe->rendermode == kRenderNormal)
		{
			EV_PlaySound(0, tr.endpos, CHAN_BODY, "weapons/xbow_hit1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, PITCH_NORM);

			// Not underwater, do some sparks...
			if (gEngfuncs.PM_PointContents(tr.endpos, nullptr) != CONTENTS_WATER)
				gEngfuncs.pEfxAPI->R_SparkShower(tr.endpos);

			Vector vBoltAngles;
			int iModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex("models/crossbow_bolt.mdl");

			VectorAngles(forward, vBoltAngles);

			TEMPENTITY* bolt = gEngfuncs.pEfxAPI->R_TempModel(tr.endpos - forward * 10, Vector(0, 0, 0), vBoltAngles, 5, iModelIndex, TE_BOUNCE_NULL);

			if (bolt)
			{
				bolt->flags |= (FTENT_CLIENTCUSTOM);					 // So it calls the callback function.
				bolt->entity.baseline.vuser1 = tr.endpos - forward * 10; // Pull out a little bit
				bolt->entity.baseline.vuser2 = vBoltAngles;				 // Look forward!
				bolt->callback = EV_BoltCallback;						 // So we can set the angles and origin back. (Stick the bolt to the wall)
			}
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

// TODO: Fully predict the fliying bolt.
void EV_FireCrossbow(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	origin = args->origin;

	EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));
	EV_PlaySound(idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0, 0xF));

	// Only play the weapon anims if I shot it.
	if (EV_IsLocal(idx))
	{
		if (0 != args->iparam1)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROSSBOW_FIRE1, 0);
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation(CROSSBOW_FIRE3, 0);

		V_PunchAxis(0, -2.0);
	}
}
//======================
//	   CROSSBOW END
//======================

//======================
//	    RPG START
//======================
void EV_FireRpg(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	origin = args->origin;

	EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9, ATTN_NORM, 0, PITCH_NORM);
	EV_PlaySound(idx, origin, CHAN_ITEM, "weapons/glauncher.wav", 0.7, ATTN_NORM, 0, PITCH_NORM);

	// Only play the weapon anims if I shot it.
	if (EV_IsLocal(idx))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(RPG_FIRE2, 0);

		V_PunchAxis(0, -5.0);
	}
}
//======================
//	     RPG END
//======================

//======================
//	    EGON END
//======================
int g_fireAnims1[] = {EGON_FIRE1, EGON_FIRE2, EGON_FIRE3, EGON_FIRE4};
int g_fireAnims2[] = {EGON_ALTFIRECYCLE};

BEAM* pBeam;
BEAM* pBeam2;
TEMPENTITY* pFlare; // Vit_amiN: egon's beam flare

void EV_EgonFlareCallback(TEMPENTITY* ent, float frametime, float currenttime)
{
	float delta = currenttime - ent->tentOffset.z; // time past since the last scale
	if (delta >= ent->tentOffset.y)
	{
		ent->entity.curstate.scale += ent->tentOffset.x * delta;
		ent->tentOffset.z = currenttime;
	}
}

void EV_EgonFire(event_args_t* args)
{
	int idx, iFireMode;
	Vector origin;

	idx = args->entindex;
	origin = args->origin;
	iFireMode = args->iparam2;
	const bool iStartup = 0 != args->bparam1;


	if (iStartup)
	{
		if (iFireMode == FIRE_WIDE)
			EV_PlaySound(idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.98, ATTN_NORM, 0, 125);
		else
			EV_PlaySound(idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.9, ATTN_NORM, 0, 100);
	}
	else
	{
		// If there is any sound playing already, kill it.
		// This is necessary because multiple sounds can play on the same channel at the same time.
		// In some cases, more than 1 run sound plays when the egon stops firing, in which case only the earliest entry in the list is stopped.
		// This ensures no more than 1 of those is ever active at the same time.
		EV_StopSound(idx, CHAN_STATIC, EGON_SOUND_RUN);

		if (iFireMode == FIRE_WIDE)
			EV_PlaySound(idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.98, ATTN_NORM, 0, 125);
		else
			EV_PlaySound(idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.9, ATTN_NORM, 0, 100);
	}

	// Only play the weapon anims if I shot it.
	if (EV_IsLocal(idx))
		gEngfuncs.pEventAPI->EV_WeaponAnimation(g_fireAnims1[gEngfuncs.pfnRandomLong(0, 3)], 0);

	if (iStartup && EV_IsLocal(idx) && !pBeam && !pBeam2 && !pFlare && 0 != cl_lw->value) // Adrian: Added the cl_lw check for those lital people that hate weapon prediction.
	{
		Vector vecSrc, vecEnd, angles, forward, right, up;
		pmtrace_t tr;

		cl_entity_t* pl = gEngfuncs.GetEntityByIndex(idx);

		if (pl)
		{
			angles = gHUD.m_vecAngles;

			AngleVectors(angles, forward, right, up);

			EV_GetGunPosition(args, vecSrc, pl->origin);

			vecEnd = vecSrc + (2048 * forward);

			gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

			// Store off the old count
			gEngfuncs.pEventAPI->EV_PushPMStates();

			// Now add in all of the players.
			gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

			gEngfuncs.pEventAPI->EV_SetTraceHull(2);
			gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

			gEngfuncs.pEventAPI->EV_PopPMStates();

			int iBeamModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex(EGON_BEAM_SPRITE);

			float r = 50.0f;
			float g = 50.0f;
			float b = 125.0f;

			// if ( IEngineStudio.IsHardware() )
			{
				r /= 255.0f;
				g /= 255.0f;
				b /= 255.0f;
			}


			pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint(idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 3.5, 0.2, 0.7, 55, 0, 0, r, g, b);

			if (pBeam)
				pBeam->flags |= (FBEAM_SINENOISE);

			pBeam2 = gEngfuncs.pEfxAPI->R_BeamEntPoint(idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 5.0, 0.08, 0.7, 25, 0, 0, r, g, b);

			// Vit_amiN: egon beam flare
			pFlare = gEngfuncs.pEfxAPI->R_TempSprite(tr.endpos, vec3_origin, 1.0,
				gEngfuncs.pEventAPI->EV_FindModelIndex(EGON_FLARE_SPRITE),
				kRenderGlow, kRenderFxNoDissipation, 1.0, 99999, FTENT_SPRCYCLE | FTENT_PERSIST);
		}
	}

	if (pFlare) // Vit_amiN: store the last mode for EV_EgonStop()
	{
		pFlare->tentOffset.x = (iFireMode == FIRE_WIDE) ? 1.0f : 0.0f;
	}
}

void EV_EgonStop(event_args_t* args)
{
	int idx;
	Vector origin;

	idx = args->entindex;
	origin = args->origin;

	EV_StopSound(idx, CHAN_STATIC, EGON_SOUND_RUN);

	// Only stop the sound if the event was sent by the same source as the owner of the egon.
	// If the local player owns the egon then only the local event should play this sound.
	// If another player owns it, only the server event should play it.
	if (0 != args->iparam1)
		EV_PlaySound(idx, origin, CHAN_WEAPON, EGON_SOUND_OFF, 0.98, ATTN_NORM, 0, 100);

	if (EV_IsLocal(idx))
	{
		if (pBeam)
		{
			pBeam->die = 0.0;
			pBeam = nullptr;
		}


		if (pBeam2)
		{
			pBeam2->die = 0.0;
			pBeam2 = nullptr;
		}

		if (pFlare) // Vit_amiN: egon beam flare
		{
			pFlare->die = gEngfuncs.GetClientTime();

			if (gEngfuncs.GetMaxClients() == 1 || (pFlare->flags & FTENT_NOMODEL) == 0)
			{
				if (pFlare->tentOffset.x != 0.0f) // true for iFireMode == FIRE_WIDE
				{
					pFlare->callback = &EV_EgonFlareCallback;
					pFlare->fadeSpeed = 2.0;			// fade out will take 0.5 sec
					pFlare->tentOffset.x = 10.0;		// scaling speed per second
					pFlare->tentOffset.y = 0.1;			// min time between two scales
					pFlare->tentOffset.z = pFlare->die; // the last callback run time
					pFlare->flags = FTENT_FADEOUT | FTENT_CLIENTCUSTOM;
				}
			}

			pFlare = nullptr;
		}

		// HACK: only reset animation if the Egon is still equipped.
		if (g_CurrentWeaponId == WEAPON_EGON)
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(EGON_IDLE1, 0);
		}
	}
}
//======================
//	    EGON END
//======================

//======================
//	   HORNET START
//======================
void EV_HornetGunFire(event_args_t* args)
{
	int idx;
	Vector origin, angles;

	idx = args->entindex;
	origin = args->origin;
	angles = args->angles;

	// Only play the weapon anims if I shot it.
	if (EV_IsLocal(idx))
	{
		V_PunchAxis(0, gEngfuncs.pfnRandomLong(0, 2));
		gEngfuncs.pEventAPI->EV_WeaponAnimation(HGUN_SHOOT, 0);
	}

	switch (gEngfuncs.pfnRandomLong(0, 2))
	{
	case 0:
		EV_PlaySound(idx, origin, CHAN_WEAPON, "agrunt/ag_fire1.wav", 1, ATTN_NORM, 0, 100);
		break;
	case 1:
		EV_PlaySound(idx, origin, CHAN_WEAPON, "agrunt/ag_fire2.wav", 1, ATTN_NORM, 0, 100);
		break;
	case 2:
		EV_PlaySound(idx, origin, CHAN_WEAPON, "agrunt/ag_fire3.wav", 1, ATTN_NORM, 0, 100);
		break;
	}
}
//======================
//	   HORNET END
//======================

//======================
//	   TRIPMINE START
//======================
// We only check if it's possible to put a trip mine
// and if it is, then we play the animation. Server still places it.
void EV_TripmineFire(event_args_t* args)
{
	int idx;
	Vector vecSrc, angles, view_ofs, forward;
	pmtrace_t tr;

	idx = args->entindex;
	vecSrc = args->origin;
	angles = args->angles;

	AngleVectors(angles, &forward, nullptr, nullptr);

	if (!EV_IsLocal(idx))
		return;

	// Grab predicted result for local player
	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(view_ofs);

	vecSrc = vecSrc + view_ofs;

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecSrc + forward * 128, PM_NORMAL, -1, &tr);

	// Hit something solid
	if (tr.fraction < 1.0)
		gEngfuncs.pEventAPI->EV_WeaponAnimation(TRIPMINE_DRAW, 0);

	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   TRIPMINE END
//======================

//======================
//	   SQUEAK START
//======================
void EV_SnarkFire(event_args_t* args)
{
	int idx;
	Vector vecSrc, angles, forward;
	pmtrace_t tr;

	idx = args->entindex;
	vecSrc = args->origin;
	angles = args->angles;

	AngleVectors(angles, &forward, nullptr, nullptr);

	if (!EV_IsLocal(idx))
		return;

	if (0 != args->ducking)
		vecSrc = vecSrc - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc + forward * 20, vecSrc + forward * 64, PM_NORMAL, -1, &tr);

	// Find space to drop the thing.
	if (tr.allsolid == 0 && tr.startsolid == 0 && tr.fraction > 0.25)
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SQUEAK_THROW, 0);

	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   SQUEAK END
//======================

void EV_FireEagle(event_args_t* args)
{
	const bool bEmpty = args->bparam1 != 0;

	Vector up, right, forward;

	AngleVectors(args->angles, forward, right, up);

	const int iShell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");

	if (EV_IsLocal(args->entindex))
	{
		EV_MuzzleFlash();

		gEngfuncs.pEventAPI->EV_WeaponAnimation(bEmpty ? EAGLE_SHOOT_EMPTY : EAGLE_SHOOT, 0);
		V_PunchAxis(0, -4.0);
	}

	Vector ShellVelocity;
	Vector ShellOrigin;

	EV_GetDefaultShellInfo(
		args,
		args->origin, args->velocity,
		ShellVelocity,
		ShellOrigin,
		forward, right, up,
		-9.0, 14.0, 9.0);

	EV_EjectBrass(ShellOrigin, ShellVelocity, args->angles[1], iShell, TE_BOUNCE_SHELL);

	EV_PlaySound(
		args->entindex,
		args->origin, CHAN_WEAPON, "weapons/desert_eagle_fire.wav",
		gEngfuncs.pfnRandomFloat(0.92, 1), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));

	Vector vecSrc;

	EV_GetGunPosition(args, vecSrc, args->origin);

	Vector vecAiming = forward;

	EV_HLDM_FireBullets(
		args->entindex,
		forward, right, up,
		1,
		vecSrc, vecAiming,
		8192.0,
		BULLET_PLAYER_EAGLE,
		0, nullptr,
		args->fparam1, args->fparam2);
}

//======================
//	PIPE WRENCH START
//======================
int g_iClub;

// Only predict the miss sounds, hit sounds are still played
// server side, so players don't get the wrong idea.
void EV_Pipewrench(event_args_t* args)
{
	const int idx = args->entindex;
	Vector origin = args->origin;
	const bool iBigSwing = 0 != args->bparam1;
	const bool hitSomething = 0 != args->bparam2;

	if (!EV_IsLocal(idx))
	{
		return;
	}

	// Play Swing sound
	if (iBigSwing)
	{
		if (hitSomething)
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_BIG_SWING_HIT, 0);
		}
		else
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_BIG_SWING_MISS, 0);
		}

		EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pwrench_big_miss.wav", 1, ATTN_NORM, 0, PITCH_NORM);
	}
	else
	{
		if (hitSomething)
		{
			switch (g_iClub % 3)
			{
			case 0:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK1HIT, 0);
				break;
			case 1:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK2HIT, 0);
				break;
			case 2:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK3HIT, 0);
				break;
			}
		}
		else
		{
			switch (g_iClub % 3)
			{
			case 0:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK1MISS, 0);
				break;
			case 1:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK2MISS, 0);
				break;
			case 2:
				gEngfuncs.pEventAPI->EV_WeaponAnimation(PIPEWRENCH_ATTACK3MISS, 0);
				break;
			}

			switch (g_iClub % 2)
			{
			case 0:
				EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pwrench_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			case 1:
				EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/pwrench_miss2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			}
		}

		++g_iClub;
	}
}
//======================
//	 PIPE WRENCH END
//======================

void EV_FireM249(event_args_t* args)
{
	int iBody = args->iparam1;

	const bool bAlternatingEject = args->bparam1 != 0;

	Vector up, right, forward;

	AngleVectors(args->angles, forward, right, up);

	int iShell =
		bAlternatingEject ? gEngfuncs.pEventAPI->EV_FindModelIndex("models/saw_link.mdl") : gEngfuncs.pEventAPI->EV_FindModelIndex("models/saw_shell.mdl");

	if (EV_IsLocal(args->entindex))
	{
		SetLocalBody(WEAPON_M249, iBody);
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(gEngfuncs.pfnRandomLong(0, 2) + M249_SHOOT1, iBody);
		V_PunchAxis(0, gEngfuncs.pfnRandomFloat(-2, 2));
		V_PunchAxis(1, gEngfuncs.pfnRandomFloat(-1, 1));
	}

	Vector ShellVelocity;
	Vector ShellOrigin;

	EV_GetDefaultShellInfo(
		args,
		args->origin, args->velocity,
		ShellVelocity,
		ShellOrigin,
		forward, right, up,
		-28.0, 24.0, 4.0);

	EV_EjectBrass(ShellOrigin, ShellVelocity, args->angles[1], iShell, TE_BOUNCE_SHELL);

	EV_PlaySound(
		args->entindex,
		args->origin, CHAN_WEAPON, "weapons/saw_fire1.wav",
		VOL_NORM, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 15));

	Vector vecSrc;

	EV_GetGunPosition(args, vecSrc, args->origin);

	Vector vecAiming = forward;

	EV_HLDM_FireBullets(
		args->entindex,
		forward, right, up,
		1,
		vecSrc, vecAiming,
		8192.0,
		BULLET_PLAYER_556,
		0, nullptr,
		args->fparam1, args->fparam2);
}

void EV_FireDisplacer(event_args_t* args)
{
	const auto mode = static_cast<DisplacerMode>(args->iparam1);

	switch (mode)
	{
	case DisplacerMode::SPINNING_UP:
	{
		int iAttach = 0;

		int iStartAttach, iEndAttach;

		for (size_t uiIndex = 0; uiIndex < DISPLACER_NUM_BEAMS; ++uiIndex)
		{
			if (iAttach <= 2)
			{
				iStartAttach = iAttach++ + 2;
				iEndAttach = iAttach % 2 + 2;
			}
			else
			{
				iStartAttach = 0;
				iEndAttach = 0;
			}

			gEngfuncs.pEfxAPI->R_BeamEnts(
				args->entindex | (iStartAttach << 12), args->entindex | (iEndAttach << 12),
				gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/lgtning.spr"),
				1,
				1, 60 * 0.01, 190 / 255.0, 30, 0, 10,
				96 / 255.0, 128 / 255.0, 16 / 255.0);
		}

		break;
	}

	case DisplacerMode::FIRED:
	{
		// bparam1 indicates whether it's a primary or secondary attack.
		if (0 == args->bparam1)
		{
			EV_PlaySound(
				args->entindex, args->origin,
				CHAN_WEAPON, "weapons/displacer_fire.wav",
				gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		}
		else
		{
			EV_PlaySound(
				args->entindex, args->origin,
				CHAN_WEAPON, "weapons/displacer_self.wav",
				gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM);
		}

		if (EV_IsLocal(args->entindex))
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(DISPLACER_FIRE, 0);
			V_PunchAxis(0, -2);
		}

		break;
	}

	default:
		break;
	}
}

void EV_FireShockRifle(event_args_t* args)
{
	EV_PlaySound(args->entindex, args->origin, CHAN_WEAPON, "weapons/shock_fire.wav", 0.9, ATTN_NORM, 0, PITCH_NORM);

	if (EV_IsLocal(args->entindex))
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SHOCKRIFLE_FIRE, 0);

	for (size_t uiIndex = 0; uiIndex < 3; ++uiIndex)
	{
		gEngfuncs.pEfxAPI->R_BeamEnts(
			args->entindex | 0x1000, args->entindex | ((uiIndex + 2) << 12),
			gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/lgtning.spr"),
			0.08,
			1, 75 * 0.01, 190 / 255.0, 30, 0, 10,
			0, 253 / 255.0, 253 / 255.0);
	}
}

void EV_FireSpore(event_args_t* args)
{
	EV_PlaySound(
		args->entindex, args->origin,
		CHAN_WEAPON, "weapons/splauncher_fire.wav",
		0.9,
		ATTN_NORM, 0, PITCH_NORM);

	if (EV_IsLocal(args->entindex))
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(SPLAUNCHER_FIRE, 0);

		V_PunchAxis(0, -3.0);

		if (cl_entity_t* pViewModel = gEngfuncs.GetViewModel())
		{
			Vector vecSrc = pViewModel->attachment[1];

			Vector forward;

			AngleVectors(args->angles, &forward, nullptr, nullptr);

			gEngfuncs.pEfxAPI->R_Sprite_Spray(
				vecSrc, forward,
				gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/tinyspit.spr"),
				10, 10, 180);
		}
	}
}

void EV_SniperRifle(event_args_t* args)
{
	const int idx = args->entindex;
	Vector vecOrigin = args->origin;
	Vector vecAngles = args->angles;

	const int iClip = args->iparam1;

	Vector up, right, forward;

	AngleVectors(vecAngles, forward, right, up);

	if (EV_IsLocal(idx))
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(iClip <= 0 ? SNIPERRIFLE_FIRELASTROUND : SNIPERRIFLE_FIRE, 0);
		V_PunchAxis(0, -2.0);
	}

	EV_PlaySound(idx, vecOrigin,
		CHAN_WEAPON, "weapons/sniper_fire.wav",
		gEngfuncs.pfnRandomFloat(0.9f, 1.0f), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong(0, 3));

	Vector vecSrc;
	Vector vecAiming = forward;

	EV_GetGunPosition(args, vecSrc, vecOrigin);

	EV_HLDM_FireBullets(
		idx,
		forward,
		right,
		up,
		1,
		vecSrc,
		vecAiming,
		8192.0,
		BULLET_PLAYER_762,
		0,
		nullptr,
		args->fparam1,
		args->fparam2);
}

// Only predict the miss sounds, hit sounds are still played
// server side, so players don't get the wrong idea.
void EV_Knife(event_args_t* args)
{
	const int idx = args->entindex;
	Vector origin = args->origin;

	const char* pszSwingSound;

	switch (g_iSwing)
	{
	default:
	case 0:
		pszSwingSound = "weapons/knife1.wav";
		break;
	case 1:
		pszSwingSound = "weapons/knife2.wav";
		break;
	case 2:
		pszSwingSound = "weapons/knife3.wav";
		break;
	}

	// Play Swing sound
	EV_PlaySound(idx, origin, CHAN_WEAPON, pszSwingSound, 1, ATTN_NORM, 0, PITCH_NORM);

	if (EV_IsLocal(idx))
	{
		switch ((g_iSwing++) % 3)
		{
		case 0:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(KNIFE_ATTACK1MISS, 0);
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(KNIFE_ATTACK2, 0);
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_WeaponAnimation(KNIFE_ATTACK3, 0);
			break;
		}
	}
}

void EV_PenguinFire(event_args_t* args)
{
	Vector origin = args->origin;
	Vector angles = args->angles;
	Vector forward;
	AngleVectors(angles, &forward, nullptr, nullptr);

	if (EV_IsLocal(args->entindex))
	{
		if (0 != args->ducking)
			origin.z += 18;

		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers(args->entindex - 1);
		gEngfuncs.pEventAPI->EV_SetTraceHull(2);

		Vector start = origin + (forward * 20);
		Vector end = origin + (forward * 64);

		pmtrace_t tr;
		gEngfuncs.pEventAPI->EV_PlayerTrace(start, end, PM_NORMAL, -1, &tr);

		if (0 == tr.allsolid && 0 == tr.startsolid && tr.fraction > 0.25)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(PENGUIN_THROW, 0);

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

int EV_TFC_IsAllyTeam(int iTeam1, int iTeam2)
{
	return 0;
}
