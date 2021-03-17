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
#ifndef WEAPONS_CM249_H
#define WEAPONS_CM249_H

enum M249Anim
{
	M249_SLOWIDLE = 0,
	M249_IDLE2,
	M249_RELOAD_START,
	M249_RELOAD_END,
	M249_HOLSTER,
	M249_DRAW,
	M249_SHOOT1,
	M249_SHOOT2,
	M249_SHOOT3
};

class CM249 : public CBasePlayerWeapon
{
public:
	using BaseClass = CBasePlayerWeapon;

#ifndef CLIENT_DLL
	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Precache() override;

	void Spawn() override;

	BOOL AddToPlayer( CBasePlayer* pPlayer ) override;

	BOOL Deploy() override;

	void Holster( int skiplocal = 0 ) override;

	void WeaponIdle() override;

	void PrimaryAttack() override;

	void Reload() override;

	int iItemSlot() override;

	int GetItemInfo( ItemInfo* p ) override;

	void IncrementAmmo(CBasePlayer* pPlayer) override;

	BOOL UseDecrement() override
	{
#if defined( CLIENT_WEAPONS )
		return UTIL_DefaultUseDecrement();
#else
		return FALSE;
#endif
	}

private:
	static int RecalculateBody( int iClip );

private:
	unsigned short m_usFireM249;

	float m_flNextAnimTime;

	int m_iShell;

	//Used to alternate between ejecting shells and links. - Solokiller
	BOOL m_bAlternatingEject;
	int m_iLink;
	int m_iSmoke;
	int m_iFire;

	BOOL m_bReloading;
	float m_flReloadStartTime;
	float m_flReloadStart;
};

#endif
