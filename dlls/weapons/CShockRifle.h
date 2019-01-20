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
#ifndef WEAPONS_CSHOCKRIFLE_H
#define WEAPONS_CSHOCKRIFLE_H

enum ShockRifleAnim
{
	SHOCKRIFLE_IDLE1 = 0,
	SHOCKRIFLE_FIRE,
	SHOCKRIFLE_DRAW,
	SHOCKRIFLE_HOLSTER,
	SHOCKRIFLE_IDLE3
};

#ifndef WEAPONS_NO_CLASSES
class CShockRifle : public CBasePlayerWeapon
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

	void AttachToPlayer( CBasePlayer* pPlayer ) override;

	BOOL CanDeploy() override;

	BOOL Deploy() override;

	void Holster( int skiplocal = 0 ) override;

	void WeaponIdle() override;

	void PrimaryAttack() override;

	void SecondaryAttack() override;

	void Reload() override;

	//TODO: implement later
	//void WeaponHolsterPreFrame() override;

	void ItemPostFrame() override;

	int iItemSlot() override;

	int GetItemInfo( ItemInfo* p ) override;

	BOOL UseDecrement() override
	{
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void RechargeAmmo( bool bLoud );

private:
	int m_iSpriteTexture;

	unsigned short m_usShockRifle;

	float m_flRechargeTime;
	float m_flSoundDelay;
};
#endif

#endif
