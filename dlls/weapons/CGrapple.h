/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#ifndef WEAPONS_CGRAPPLE_H
#define WEAPONS_CGRAPPLE_H

class CGrappleTip;

enum BarnacleGrappleAnim
{
	BGRAPPLE_BREATHE = 0,
	BGRAPPLE_LONGIDLE,
	BGRAPPLE_SHORTIDLE,
	BGRAPPLE_COUGH,
	BGRAPPLE_DOWN,
	BGRAPPLE_UP,
	BGRAPPLE_FIRE,
	BGRAPPLE_FIREWAITING,
	BGRAPPLE_FIREREACHED,
	BGRAPPLE_FIRETRAVEL,
	BGRAPPLE_FIRERELEASE
};

class CGrapple : public CBasePlayerWeapon
{
private:
	enum class FireState
	{
		OFF		= 0,
		CHARGE	= 1
	};

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

	void SecondaryAttack() override;

	int iItemSlot() override;

	int GetItemInfo( ItemInfo* p ) override;

	BOOL UseDecrement() override
	{
#if defined( CLIENT_WEAPONS )
		return UTIL_DefaultUseDecrement();
#else
		return FALSE;
#endif
	}

private:
	void Fire( const Vector& vecOrigin, const Vector& vecDir );
	void EndAttack();

	void CreateEffect();
	void UpdateEffect();
	void DestroyEffect();

private:
	CGrappleTip* m_pTip;

	CBeam* m_pBeam;

	float m_flShootTime;
	float m_flDamageTime;

	FireState m_FireState;

	bool m_bGrappling;

	bool m_bMissed;

	bool m_bMomentaryStuck;
};

#endif
