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

#pragma once

#include "cbase.h"

class CLaserSpot : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;

	int ObjectCaps() override { return FCAP_DONT_SAVE; }

	/**
	*	@brief make the laser sight invisible.
	*/
	void Suspend(float flSuspendTime);

	/**
	*	@brief bring a suspended laser sight back.
	*/
	void EXPORT Revive();

	static CLaserSpot* CreateSpot();
};
