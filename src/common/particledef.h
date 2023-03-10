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

enum ptype_t
{
	pt_static,
	pt_grav,
	pt_slowgrav,
	pt_fire,
	pt_explode,
	pt_explode2,
	pt_blob,
	pt_blob2,
	pt_vox_slowgrav,
	pt_vox_grav,
	pt_clientcustom // Must have callback function specified
};

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
struct particle_t
{
	// driver-usable fields
	Vector org;
	short color;
	short packedColor;
	// drivers never touch the following fields
	particle_t* next;
	Vector vel;
	float ramp;
	float die;
	ptype_t type;
	void (*deathfunc)(particle_t* particle);

	// for pt_clientcusttom, we'll call this function each frame
	void (*callback)(particle_t* particle, float frametime);

	// For deathfunc, etc.
	unsigned char context;
};
