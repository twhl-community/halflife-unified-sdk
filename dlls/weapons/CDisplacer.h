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
#ifndef WEAPONS_CDISPLACER_H
#define WEAPONS_CDISPLACER_H

enum DisplacerAnim
{
	DISPLACER_IDLE1 = 0,
	DISPLACER_IDLE2,
	DISPLACER_SPINUP,
	DISPLACER_SPIN,
	DISPLACER_FIRE,
	DISPLACER_DRAW,
	DISPLACER_HOLSTER1
};

enum class DisplacerMode
{
	STARTED = 0,
	SPINNING_UP,
	SPINNING,
	FIRED
};

static const size_t DISPLACER_NUM_BEAMS = 4;

class CDisplacer : public CBasePlayerWeapon
{
public:
	using BaseClass = CBasePlayerWeapon;

	void Precache() override;

	void Spawn() override;

	BOOL AddToPlayer( CBasePlayer* pPlayer ) override;

	BOOL Deploy() override;

	void Holster( int skiplocal = 0 ) override;

	void WeaponIdle() override;

	void PrimaryAttack() override;

	void SecondaryAttack() override;

	void Reload() override;

	void EXPORT SpinupThink();

	void EXPORT AltSpinupThink();

	void EXPORT FireThink();

	void EXPORT AltFireThink();

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
	int m_iSpriteTexture;

	float m_flStartTime;
	float m_flSoundDelay;

	DisplacerMode m_Mode;

	int m_iImplodeCounter;
	int m_iSoundState;

	unsigned short m_usFireDisplacer;
};

#endif
