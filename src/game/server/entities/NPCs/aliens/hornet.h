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

#define HORNET_TYPE_RED 0
#define HORNET_TYPE_ORANGE 1
#define HORNET_RED_SPEED (float)600
#define HORNET_ORANGE_SPEED (float)800
#define HORNET_BUZZ_VOLUME (float)0.8

extern int iHornetPuff;

/**
 *	@brief this is the projectile that the Alien Grunt fires.
 */
class CHornet : public CBaseMonster
{
	DECLARE_CLASS(CHornet, CBaseMonster);
	DECLARE_DATAMAP();

public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;

	bool IsBioWeapon() const override { return true; }

	/**
	 *	@brief hornets will never get mad at each other, no matter who the owner is.
	 */
	Relationship IRelationship(CBaseEntity* pTarget) override;

	void IgniteTrail();

	/**
	 *	@brief starts a hornet out tracking its target
	 */
	void StartTrack();

	/**
	 *	@brief starts a hornet out just flying straight.
	 */
	void StartDart();

	/**
	 *	@brief Hornet is flying, gently tracking target
	 */
	void TrackTarget();
	void TrackTouch(CBaseEntity* pOther);
	void DartTouch(CBaseEntity* pOther);
	void DieTouch(CBaseEntity* pOther);

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	float m_flStopAttack;
	int m_iHornetType;
	float m_flFlySpeed;
};
