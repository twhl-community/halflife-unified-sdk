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
#include "demo.h"

#include "com_weapons.h"
#include "demo_api.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"

#include "pm_defs.h"
#include "event_api.h"
#include "entity_types.h"
#include "r_efx.h"

extern BEAM* pBeam;
extern BEAM* pBeam2;
extern TEMPENTITY* pFlare; // Vit_amiN: egon's energy flare

void UpdateBeams()
{
	Vector forward, vecSrc, vecEnd, angles, right, up;
	Vector view_ofs;
	pmtrace_t tr;
	cl_entity_t* pthisplayer = gEngfuncs.GetLocalPlayer();
	int idx = pthisplayer->index;

	// Get our exact viewangles from engine
	gEngfuncs.GetViewAngles(angles);

	// Determine our last predicted origin
	const Vector origin = HUD_GetLastOrg();

	AngleVectors(angles, forward, right, up);

	VectorCopy(origin, vecSrc);

	VectorMA(vecSrc, 2048, forward, vecEnd);

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);

	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

	gEngfuncs.pEventAPI->EV_PopPMStates();

	if (pBeam)
	{
		pBeam->target = tr.endpos;
		pBeam->die = gEngfuncs.GetClientTime() + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}

	if (pBeam2)
	{
		pBeam2->target = tr.endpos;
		pBeam2->die = gEngfuncs.GetClientTime() + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}

	if (pFlare) // Vit_amiN: beam flare
	{
		pFlare->entity.origin = tr.endpos;
		pFlare->die = gEngfuncs.GetClientTime() + 0.1f; // We keep it alive just a little bit forward in the future, just in case.

		if (gEngfuncs.GetMaxClients() != 1) // Singleplayer always draws the egon's energy beam flare
		{
			pFlare->flags |= FTENT_NOMODEL;

			if (!(0 != tr.allsolid || tr.ent <= 0 || tr.fraction == 1.0f)) // Beam hit some non-world entity
			{
				physent_t* pEntity = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);

				// Not the world, let's assume that we hit something organic ( dog, cat, uncle joe, etc )
				if (pEntity && !(pEntity->solid == SOLID_BSP || pEntity->movetype == MOVETYPE_PUSHSTEP))
				{
					pFlare->flags &= ~FTENT_NOMODEL;
				}
			}
		}
	}
}

/*
=====================
Game_AddObjects

Add game specific, client-side objects here
=====================
*/
void Game_AddObjects()
{
	if (pBeam || pBeam2 || pFlare)
		UpdateBeams();
}
