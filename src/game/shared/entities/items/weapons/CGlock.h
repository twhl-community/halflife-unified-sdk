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

enum glock_e
{
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

class CGlock : public CBasePlayerWeapon
{
public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	bool GetItemInfo(ItemInfo* p) override;

	void IncrementAmmo(CBasePlayer* pPlayer) override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void GlockFire(float flSpread, float flCycleTime, bool fUseAutoAim);
	bool Deploy() override;
	void Reload() override;
	void WeaponIdle() override;

	bool UseDecrement() override
	{
#if defined(CLIENT_WEAPONS)
		return true;
#else
		return false;
#endif
	}

private:
	int m_iShell;


	unsigned short m_usFireGlock1;
	unsigned short m_usFireGlock2;
};
