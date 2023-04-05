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

struct AmmoType;

/**
*	@brief a single entity that can store weapons and ammo.
*/
class CWeaponBox : public CBaseEntity
{
	DECLARE_CLASS(CWeaponBox, CBaseEntity);
	DECLARE_DATAMAP();

public:
	void OnCreate() override;
	void Precache() override;
	void Spawn() override;

	/**
	*	@brief try to add my contents to the toucher if the toucher is a player.
	*/
	void Touch(CBaseEntity* pOther) override;
	bool KeyValue(KeyValueData* pkvd) override;

	/**
	*	@brief is there anything in this box?
	*/
	bool IsEmpty();
	int GiveAmmo(int iCount, const char* szName, int* pIndex = nullptr);
	int GiveAmmo(int iCount, const AmmoType* type, int* pIndex = nullptr);
	void SetObjectCollisionBox() override;

	void UpdateOnRemove() override;

	void RemoveWeapons();

	/**
	*	@brief the think function that removes the box from the world.
	*/
	void Kill();

	/**
	*	@brief is a weapon of this type already packed in this box?
	*/
	bool HasWeapon(CBasePlayerWeapon* checkWeapon);

	/**
	*	@brief Add this weapon to the box
	*/
	bool PackWeapon(CBasePlayerWeapon* weapon);
	bool PackAmmo(string_t iszName, int iCount);

	CBasePlayerWeapon* m_rgpPlayerWeapons[MAX_WEAPON_SLOTS]; // one slot for each

	string_t m_rgiszAmmo[MAX_AMMO_TYPES]; // ammo names
	int m_rgAmmo[MAX_AMMO_TYPES];		  // ammo quantities

	int m_cAmmoTypes; // how many ammo types packed into this box (if packed by a level designer)
};
