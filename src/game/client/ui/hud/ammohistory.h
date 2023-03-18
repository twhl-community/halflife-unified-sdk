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
//
// ammohistory.h
//

#pragma once

#include <array>

struct AmmoType;

// this is the max number of items in each bucket
#define MAX_WEAPON_POSITIONS MAX_WEAPON_SLOTS

class WeaponsResource
{
private:
	// Information about weapons & ammo
	std::array<WEAPON, MAX_WEAPONS> rgWeapons; // Weapons Array

	// counts of weapons * ammo
	WEAPON* rgSlots[MAX_WEAPON_SLOTS + 1][MAX_WEAPON_POSITIONS + 1]; // The slots currently in use by weapons.  The value is a pointer to the weapon;  if it's nullptr, no weapon is there
	int riAmmo[MAX_AMMO_TYPES];										 // count of each ammo type

public:
	void Init()
	{
		Reset();
	}

	void Reset()
	{
		iOldWeaponBits = 0;
		memset(rgSlots, 0, sizeof rgSlots);
		memset(riAmmo, 0, sizeof riAmmo);
	}

	///// WEAPON /////
	std::uint64_t iOldWeaponBits;

	WEAPON* GetWeapon(int iId)
	{
		return &rgWeapons[iId];
	}

	void PickupWeapon(WEAPON* wp);

	void DropWeapon(WEAPON* wp);

	void DropAllWeapons();

	WEAPON* GetWeaponSlot(int slot, int pos) { return rgSlots[slot][pos]; }

	void InitializeWeapons();

	void LoadWeaponSprites(WEAPON* wp);
	WEAPON* GetFirstPos(int iSlot);
	void SelectSlot(int iSlot, bool fAdvance, int iDirection);
	WEAPON* GetNextActivePos(int iSlot, int iSlotPos);

	bool HasAmmo(WEAPON* p);

	///// AMMO /////
	void SetAmmo(int iId, int iCount) { riAmmo[iId] = iCount; }

	int CountAmmo(const AmmoType* type);

	HSPRITE* GetAmmoPicFromWeapon(int iAmmoId, Rect& rect);
};

inline WeaponsResource gWR;


#define MAX_HISTORY 12
enum
{
	HISTSLOT_EMPTY,
	HISTSLOT_AMMO,
	HISTSLOT_WEAP,
	HISTSLOT_ITEM,
};

class HistoryResource
{
private:
	struct HIST_ITEM
	{
		int type;
		float DisplayTime; // the time at which this item should be removed from the history
		int iCount;
		int iId;
	};

	HIST_ITEM rgAmmoHistory[MAX_HISTORY];

public:
	void Init()
	{
		Reset();
	}

	void Reset()
	{
		memset(rgAmmoHistory, 0, sizeof rgAmmoHistory);
	}

	int iHistoryGap;
	int iCurrentHistorySlot;

	void AddToHistory(int iType, int iId, int iCount = 0);
	void AddToHistory(int iType, const char* szName, int iCount = 0);

	void CheckClearHistory();
	bool DrawAmmoHistory(float flTime);
};

inline HistoryResource gHR;
