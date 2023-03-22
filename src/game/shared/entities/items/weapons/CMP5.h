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

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
};

class CMP5 : public CBasePlayerWeapon
{
public:
	void OnCreate() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo& info) override;
	void IncrementAmmo(CBasePlayer* pPlayer) override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float m_flNextAnimTime;
	int m_iShell;

	bool UseDecrement() override
	{
#if defined(CLIENT_WEAPONS)
		return true;
#else
		return false;
#endif
	}

private:
	float m_flNextGrenadeLoad;
	unsigned short m_usMP5;
	unsigned short m_usMP52;
};
