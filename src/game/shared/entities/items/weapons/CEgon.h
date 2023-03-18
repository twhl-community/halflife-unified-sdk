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

enum egon_e
{
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRE1,
	EGON_FIRE2,
	EGON_FIRE3,
	EGON_FIRE4,
	EGON_DRAW,
	EGON_HOLSTER
};

enum EGON_FIRESTATE
{
	FIRE_OFF,
	FIRE_CHARGE
};

enum EGON_FIREMODE
{
	FIRE_NARROW,
	FIRE_WIDE
};

#define EGON_PRIMARY_VOLUME 450
#define EGON_BEAM_SPRITE "sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE "sprites/XSpark1.spr"
#define EGON_SOUND_OFF "weapons/egon_off1.wav"
#define EGON_SOUND_RUN "weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP "weapons/egon_windup2.wav"

class CEgon : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif

	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	bool GetWeaponInfo(WeaponInfo& info) override;
	void IncrementAmmo(CBasePlayer* pPlayer) override;

	bool Deploy() override;
	void Holster() override;

	void UpdateEffect(const Vector& startPoint, const Vector& endPoint, float timeBlend);

	void CreateEffect();
	void DestroyEffect();

	void EndAttack();
	void Attack();
	void PrimaryAttack() override;
	bool ShouldWeaponIdle() override { return true; }
	void WeaponIdle() override;

	float m_flAmmoUseTime; // since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval();
	float GetDischargeInterval();

	void Fire(const Vector& vecOrigSrc, const Vector& vecDir);

	bool HasAmmo();

	void UseAmmo(int count);

	CBeam* m_pBeam;
	CBeam* m_pNoise;
	CSprite* m_pSprite;

	bool UseDecrement() override
	{
#if defined(CLIENT_WEAPONS)
		return true;
#else
		return false;
#endif
	}

	unsigned short m_usEgonStop;

private:
	float m_shootTime;
	EGON_FIREMODE m_fireMode;
	float m_shakeTime;
	bool m_deployed;

	unsigned short m_usEgonFire;
};
