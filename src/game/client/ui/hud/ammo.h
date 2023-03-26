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

struct AmmoType;
struct WeaponInfo;

struct WEAPON
{
	const WeaponInfo* Info{};
	const AmmoType* AmmoType1{};
	const AmmoType* AmmoType2{};

	int AmmoInMagazine{0};

	int iCount{0}; // # of itesm in plist

	HSPRITE hActive{0};
	Rect rcActive;
	HSPRITE hInactive{0};
	Rect rcInactive;
	HSPRITE hAmmo{0};
	Rect rcAmmo;
	HSPRITE hAmmo2{0};
	Rect rcAmmo2;
	HSPRITE hCrosshair{0};
	Rect rcCrosshair;
	HSPRITE hAutoaim{0};
	Rect rcAutoaim;
	HSPRITE hZoomedCrosshair{0};
	Rect rcZoomedCrosshair;
	HSPRITE hZoomedAutoaim{0};
	Rect rcZoomedAutoaim;
};
