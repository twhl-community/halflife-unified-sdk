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

class CBeam;

#define SF_WAITFORTRIGGER (0x04 | 0x40) // UNDONE: Fix!
#define SF_NOWRECKAGE 0x08

class CApache : public CBaseMonster
{
	DECLARE_CLASS(CApache, CBaseMonster);
	DECLARE_DATAMAP();

public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	int BloodColor() override { return DONT_BLEED; }
	void Killed(CBaseEntity* attacker, int iGib) override;
	void GibMonster() override;

	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector(-300, -300, -172);
		pev->absmax = pev->origin + Vector(300, 300, 8);
	}

	void HuntThink();
	void FlyTouch(CBaseEntity* pOther);
	void CrashTouch(CBaseEntity* pOther);
	void DyingThink();
	void StartupUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void NullThink();

	void ShowDamage();
	void Flight();
	void FireRocket();
	bool FireGun();

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	int m_iRockets;
	float m_flForce;
	float m_flNextRocket;

	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	Vector m_vecGoal;

	Vector m_angGun;
	float m_flLastSeen;
	float m_flPrevSeen;

	int m_iSoundState; // don't save this

	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBodyGibs;

	float m_flGoalSpeed;

	int m_iDoSmokePuff;
	CBeam* m_pBeam;
};
