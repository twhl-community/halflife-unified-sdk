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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

#include "CEagleLaser.h"

LINK_ENTITY_TO_CLASS( eagle_laser, CEagleLaser );

//=========================================================
//=========================================================
CEagleLaser *CEagleLaser::CreateSpot()
{
	auto pSpot = GetClassPtr( reinterpret_cast<CEagleLaser*>( VARS( CREATE_NAMED_ENTITY( MAKE_STRING( "eagle_laser" ) ) ) ) );
	pSpot->Spawn();

	//Eagle laser is smaller
	pSpot->pev->scale = 0.5;

	pSpot->pev->classname = MAKE_STRING( "eagle_laser" );

	return pSpot;
}
