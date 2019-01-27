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
#ifndef CTFDEFS_H
#define CTFDEFS_H

/**
*	@file CTF gamemode definitions
*/

enum class CTFTeam
{
	None = 0,
	BlackMesa,
	OpposingForce
};

namespace CTFItem
{
enum CTFItem : unsigned int
{
	None = 0,

	BlackMesaFlag = 1 << 0,
	OpposingForceFlag = 1 << 1,

	LongJump = 1 << 2,
	PortableHEV = 1 << 3,
	Backpack = 1 << 4,
	Acceleration = 1 << 5,
	Unknown = 1 << 6, //Appears to be some non-existent item
	Regeneration = 1 << 7,

	ItemsMask = LongJump | PortableHEV | Backpack | Acceleration | Unknown | Regeneration
};
}

#endif //CTFDEFS_H
