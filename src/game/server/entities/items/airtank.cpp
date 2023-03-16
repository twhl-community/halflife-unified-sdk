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
#include "cbase.h"

class CAirtank : public CGrenade
{
public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	void EXPORT TankThink();
	void EXPORT TankTouch(CBaseEntity* pOther);
	int BloodColor() override { return DONT_BLEED; }
	void Killed(CBaseEntity* attacker, int iGib) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	bool m_state;
};


LINK_ENTITY_TO_CLASS(item_airtank, CAirtank);
TYPEDESCRIPTION CAirtank::m_SaveData[] =
	{
		DEFINE_FIELD(CAirtank, m_state, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CAirtank, CGrenade);

void CAirtank::OnCreate()
{
	CGrenade::OnCreate();

	pev->health = 20;
	pev->model = MAKE_STRING("models/w_oxygen.mdl");
}

void CAirtank::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SetModel(STRING(pev->model));
	SetSize(Vector(-16, -16, 0), Vector(16, 16, 36));
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CAirtank::TankTouch);
	SetThink(&CAirtank::TankThink);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_YES;
	pev->dmg = 50;
	m_state = true;
}

void CAirtank::Precache()
{
	PrecacheModel(STRING(pev->model));
	PrecacheSound("doors/aliendoor3.wav");
}


void CAirtank::Killed(CBaseEntity* attacker, int iGib)
{
	SetOwner(attacker);

	// UNDONE: this should make a big bubble cloud, not an explosion

	Explode(pev->origin, Vector(0, 0, -1));
}


void CAirtank::TankThink()
{
	// Fire trigger
	m_state = true;
	SUB_UseTargets(this, USE_TOGGLE, 0);
}


void CAirtank::TankTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
		return;

	if (!m_state)
	{
		// "no oxygen" sound
		EmitSound(CHAN_BODY, "player/pl_swim2.wav", 1.0, ATTN_NORM);
		return;
	}

	// give player 12 more seconds of air
	pOther->pev->air_finished = gpGlobals->time + 12;

	// suit recharge sound
	EmitSound(CHAN_VOICE, "doors/aliendoor3.wav", 1.0, ATTN_NORM);

	// recharge airtank in 30 seconds
	pev->nextthink = gpGlobals->time + 30;
	m_state = false;
	SUB_UseTargets(this, USE_TOGGLE, 1);
}
