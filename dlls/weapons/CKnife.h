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
#ifndef WEAPONS_CKNIFE_H
#define WEAPONS_CKNIFE_H

enum KnifeAnim
{
	KNIFE_IDLE1 = 0,
	KNIFE_DRAW,
	KNIFE_HOLSTER,
	KNIFE_ATTACK1,
	KNIFE_ATTACK1MISS,
	KNIFE_ATTACK2,
	KNIFE_ATTACK2HIT,
	KNIFE_ATTACK3,
	KNIFE_ATTACK3HIT,
	KNIFE_IDLE2,
	KNIFE_IDLE3,
	KNIFE_CHARGE,
	KNIFE_STAB
};

class CKnife : public CBasePlayerWeapon
{
public:
	using BaseClass = CBasePlayerWeapon;

	void Precache() override;

	void Spawn() override;

	BOOL Deploy() override;

	void Holster( int skiplocal = 0 ) override;

	void PrimaryAttack() override;

	bool Swing( const bool bFirst );

	void EXPORT SwingAgain();

	void EXPORT Smack();

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
	unsigned short m_usKnife;

	int m_iSwing;
	TraceResult m_trHit;
};

#endif
