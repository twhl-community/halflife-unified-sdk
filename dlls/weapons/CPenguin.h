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

enum PenguinAnim
{
	PENGUIN_IDLE1 = 0,
	PENGUIN_FIDGETFIT,
	PENGUIN_FIDGETNIP,
	PENGUIN_DOWN,
	PENGUIN_UP,
	PENGUIN_THROW,
};

class CPenguin : public CBasePlayerWeapon
{
public:
	using BaseClass = CBasePlayerWeapon;

#ifndef CLIENT_DLL
	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Precache() override;

	void Spawn() override;

	BOOL Deploy() override;

	void Holster(int skiplocal = 0) override;

	void WeaponIdle() override;

	void PrimaryAttack() override;

	void SecondaryAttack() override;

	int iItemSlot() override;

	int GetItemInfo(ItemInfo* p) override;

	BOOL UseDecrement() override
	{
#if defined( CLIENT_WEAPONS )
		return UTIL_DefaultUseDecrement();
#else
		return FALSE;
#endif
	}

private:
	int m_fJustThrown;
	unsigned short m_usPenguinFire;
};
