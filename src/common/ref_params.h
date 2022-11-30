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

#pragma once

struct movevars_t;
struct usercmd_t;

struct ref_params_t
{
	// Output
	Vector vieworg;
	Vector viewangles;

	Vector forward;
	Vector right;
	Vector up;

	// Client frametime;
	float frametime;
	// Client time
	float time;

	// Misc
	int intermission;
	int paused;
	int spectator;
	int onground;
	WaterLevel waterlevel;

	Vector simvel;
	Vector simorg;

	Vector viewheight;
	float idealpitch;

	Vector cl_viewangles;

	int health;
	Vector crosshairangle;
	float viewsize;

	Vector punchangle;
	int maxclients;
	int viewentity;
	int playernum;
	int max_entities;
	int demoplayback;
	int hardware;

	int smoothing;

	// Last issued usercmd
	usercmd_t* cmd;

	// Movevars
	movevars_t* movevars;

	int viewport[4]; // the viewport coordinates x ,y , width, height

	int nextView;		// the renderer calls ClientDLL_CalcRefdef() and Renderview
						// so long in cycles until this value is 0 (multiple views)
	int onlyClientDraw; // if !=0 nothing is drawn by the engine except clientDraw functions
};
