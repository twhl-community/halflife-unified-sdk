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

#include "CBaseEntity.h"

#define SF_WORLD_DARK 0x0001	  // Fade from black at startup
#define SF_WORLD_FORCETEAM 0x0004 // Force teams

// this moved here from world.cpp, to allow classes to be derived from it
/**
 *	@brief This spawns first when each level begins.
 */
class CWorld : public CBaseEntity
{
public:
	CWorld();
	~CWorld() override;

	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
};
