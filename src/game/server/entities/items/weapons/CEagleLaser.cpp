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

#include "CEagleLaser.h"

LINK_ENTITY_TO_CLASS(eagle_laser, CEagleLaser);

CEagleLaser* CEagleLaser::CreateSpot()
{
	auto pSpot = static_cast<CEagleLaser*>(g_EntityDictionary->Create("eagle_laser"));
	pSpot->Spawn();

	// Eagle laser is smaller
	pSpot->pev->scale = 0.5;

	return pSpot;
}
