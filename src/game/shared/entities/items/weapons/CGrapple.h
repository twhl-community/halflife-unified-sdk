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

#pragma once

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
	DECLARE_CLASS(CGrapple, CBasePlayerWeapon);
	DECLARE_DATAMAP();

private:
	enum class FireState
	{
		OFF = 0,
		CHARGE = 1
	};

public:
	void OnCreate() override;

	void Precache() override;

	bool Deploy() override;

	void Holster() override;

	void WeaponIdle() override;

	void PrimaryAttack() override;

	void SecondaryAttack() override;

	bool GetWeaponInfo(WeaponInfo& info) override;

	bool UseDecrement() override
	{
#if defined(CLIENT_WEAPONS)
		return true;
#else
		return false;
#endif
	}

private:
	void Fire(const Vector& vecOrigin, const Vector& vecDir);
	void EndAttack();

	void CreateEffect();
	void UpdateEffect();
	void DestroyEffect();

private:
	CGrappleTip* m_pTip = nullptr;

	CBeam* m_pBeam;

	float m_flShootTime;
	float m_flDamageTime;

	FireState m_FireState;

	bool m_bGrappling = false;

	bool m_bMissed;

	bool m_bMomentaryStuck;
};
