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
#ifndef WEAPONS_CEAGLELASER_H
#define WEAPONS_CEAGLELASER_H

/**
*	@brief Identical to CLaserSpot, different class to avoid RPG laser confusion logic. - Solokiller
*/
class CEagleLaser : public CLaserSpot
{
public:
	using BaseClass = CLaserSpot;

	static CEagleLaser* CreateSpot();
};
#endif
