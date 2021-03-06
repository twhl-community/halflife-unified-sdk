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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"

#define PENGUIN_DETONATE_DELAY	15.0

enum MonsterPenguinAnim
{
	MONSTERPENGUIN_IDLE1 = 0,
	MONSTERPENGUIN_FIDGET,
	MONSTERPENGUIN_JUMP,
	MONSTERPENGUIN_RUN,
};

class CPenguinGrenade : public CGrenade
{
public:
	int Save(CSave& save) override;
	int Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	void Precache() override;
	void GibMonster() override;
	void EXPORT SuperBounceTouch(CBaseEntity* pOther);
	void Spawn() override;

	int Classify() override;
	int IRelationship(CBaseEntity* pTarget) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void EXPORT HuntThink();
	void Smoke();
	int BloodColor() override;

	static float m_flNextBounceSoundTime;

	float m_flDie;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	Vector m_posPrev;
	EHANDLE m_hOwner;
	int m_iMyClass;
};

float CPenguinGrenade::m_flNextBounceSoundTime = 0;

TYPEDESCRIPTION CPenguinGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CPenguinGrenade, m_flDie, FIELD_TIME),
	DEFINE_FIELD(CPenguinGrenade, m_vecTarget, FIELD_VECTOR),
	DEFINE_FIELD(CPenguinGrenade, m_flNextHunt, FIELD_TIME),
	DEFINE_FIELD(CPenguinGrenade, m_flNextHit, FIELD_TIME),
	DEFINE_FIELD(CPenguinGrenade, m_posPrev, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CPenguinGrenade, m_hOwner, FIELD_EHANDLE)
};

IMPLEMENT_SAVERESTORE(CPenguinGrenade, CGrenade);

LINK_ENTITY_TO_CLASS(monster_penguin, CPenguinGrenade);

void CPenguinGrenade::Precache()
{
	g_engfuncs.pfnPrecacheModel("models/w_penguin.mdl");
	g_engfuncs.pfnPrecacheSound("squeek/sqk_blast1.wav");
	g_engfuncs.pfnPrecacheSound("common/bodysplat.wav");
	g_engfuncs.pfnPrecacheSound("squeek/sqk_die1.wav");
	g_engfuncs.pfnPrecacheSound("squeek/sqk_hunt1.wav");
	g_engfuncs.pfnPrecacheSound("squeek/sqk_hunt2.wav");
	g_engfuncs.pfnPrecacheSound("squeek/sqk_hunt3.wav");
	g_engfuncs.pfnPrecacheSound("squeek/sqk_deploy1.wav");
}

void CPenguinGrenade::GibMonster()
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);
}

void CPenguinGrenade::SuperBounceTouch(CBaseEntity* pOther)
{
	TraceResult tr = UTIL_GetGlobalTrace();

	// don't hit the guy that launched this grenade
	if (pev->owner && pOther->edict() == pev->owner)
		return;

	// at least until we've bounced once
	pev->owner = NULL;

	pev->angles.x = 0;
	pev->angles.z = 0;

	// avoid bouncing too much
	if (m_flNextHit > gpGlobals->time)
		return;

	// higher pitch as squeeker gets closer to detonation time
	const float flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / PENGUIN_DETONATE_DELAY);

	if (pOther->pev->takedamage && m_flNextAttack < gpGlobals->time)
	{
		// attack!

		bool hurtTarget = true;

		if (g_pGameRules->IsMultiplayer())
		{
			//TODO: set to null earlier on, so this can never be valid
			auto owner = CBaseEntity::Instance(pev->owner);

			auto ownerPlayer = owner->IsPlayer() ? owner : nullptr;

			hurtTarget = true;

			if (pOther->IsPlayer())
			{
				if (ownerPlayer)
				{
					hurtTarget = false;
					if (ownerPlayer != pOther)
					{
						hurtTarget = g_pGameRules->FPlayerCanTakeDamage(static_cast<CBasePlayer*>(pOther), ownerPlayer) != 0;
					}
				}
			}
		}

		// make sure it's me who has touched them
		if (tr.pHit == pOther->edict())
		{
			// and it's not another squeakgrenade
			if (tr.pHit->v.modelindex != pev->modelindex)
			{
				// ALERT( at_console, "hit enemy\n");
				ClearMultiDamage();
				pOther->TraceAttack(pev, gSkillData.snarkDmgBite, gpGlobals->v_forward, &tr, DMG_SLASH);
				if (m_hOwner != NULL)
					ApplyMultiDamage(pev, m_hOwner->pev);
				else
					ApplyMultiDamage(pev, pev);

				// add more explosion damage
				if (hurtTarget)
					pev->dmg += gSkillData.plrDmgHandGrenade;
				else
					pev->dmg += gSkillData.plrDmgHandGrenade / 5.0;

				if (pev->dmg > 500.0)
				{
					pev->dmg = 500.0;
				}

				// m_flDie += 2.0; // add more life

				// make bite sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "squeek/sqk_deploy1.wav", 1.0, ATTN_NORM, 0, (int)flpitch);
				m_flNextAttack = gpGlobals->time + 0.5;
			}
		}
		else
		{
			// ALERT( at_console, "been hit\n");
		}
	}

	m_flNextHit = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time;

	if (g_pGameRules->IsMultiplayer())
	{
		// in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
		if (gpGlobals->time < m_flNextBounceSoundTime)
		{
			// too soon!
			return;
		}
	}

	if (!(pev->flags & FL_ONGROUND))
	{
		// play bounce sound
		float flRndSound = RANDOM_FLOAT(0, 1);

		if (flRndSound <= 0.33)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt1.wav", 1, ATTN_NORM, 0, (int)flpitch);
		else if (flRndSound <= 0.66)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt2.wav", 1, ATTN_NORM, 0, (int)flpitch);
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_hunt3.wav", 1, ATTN_NORM, 0, (int)flpitch);
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 256, 0.25);
	}
	else
	{
		// skittering sound
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 100, 0.1);
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5;// half second.
}

void CPenguinGrenade::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_penguin.mdl");
	UTIL_SetSize(pev, Vector(-4, -4, 0), Vector(4, 4, 8));
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CPenguinGrenade::SuperBounceTouch);
	SetThink(&CPenguinGrenade::HuntThink);
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time + 1E6;

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.snarkHealth;
	pev->gravity = 0.5;
	pev->friction = 0.5;

	pev->dmg = gSkillData.plrDmgHandGrenade;

	m_flDie = gpGlobals->time + PENGUIN_DETONATE_DELAY;

	m_flFieldOfView = 0; // 180 degrees

	if (pev->owner)
		m_hOwner = Instance(pev->owner);

	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a snark is spawned.

	//TODO: shouldn't use index
	pev->sequence = MONSTERPENGUIN_RUN;
	ResetSequenceInfo();
}

int CPenguinGrenade::Classify()
{
	if (m_iMyClass != 0)
		return m_iMyClass; // protect against recursion

	if (m_hEnemy != NULL)
	{
		m_iMyClass = CLASS_INSECT; // no one cares about it
		switch (m_hEnemy->Classify())
		{
		case CLASS_PLAYER:
		case CLASS_HUMAN_PASSIVE:
		case CLASS_HUMAN_MILITARY:
			m_iMyClass = 0;
			return CLASS_ALIEN_MILITARY; // barney's get mad, grunts get mad at it
		}
		m_iMyClass = 0;
	}

	return CLASS_ALIEN_BIOWEAPON;
}

int CPenguinGrenade::IRelationship(CBaseEntity* pTarget)
{
	if (pTarget->Classify() == CLASS_ALIEN_MILITARY)
	{
		return R_DL;
	}
	else if (pTarget->Classify() == CLASS_PLAYER_ALLY)
	{
		return R_AL;
	}
	else
	{
		return CBaseMonster::IRelationship(pTarget);
	}
}

void CPenguinGrenade::Killed(entvars_t* pevAttacker, int iGib)
{
	if (m_hOwner != NULL)
		pev->owner = m_hOwner->edict();

	Detonate();

	UTIL_BloodDrips(pev->origin, g_vecZero, BloodColor(), 80);

	// reset owner so death message happens
	if (m_hOwner != NULL)
		pev->owner = m_hOwner->edict();
}

void CPenguinGrenade::HuntThink()
{
	// ALERT( at_console, "think\n" );

	if (!IsInWorld())
	{
		SetTouch(NULL);
		UTIL_Remove(this);
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	// explode when ready
	if (gpGlobals->time >= m_flDie)
	{
		g_vecAttackDir = pev->velocity.Normalize();
		pev->health = -1;
		Killed(pev, 0);
		return;
	}

	// float
	if (pev->waterlevel != 0)
	{
		if (pev->movetype == MOVETYPE_BOUNCE)
		{
			pev->movetype = MOVETYPE_FLY;
		}
		pev->velocity = pev->velocity * 0.9;
		pev->velocity.z += 8.0;
	}
	else if (pev->movetype = MOVETYPE_FLY)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}

	// return if not time to hunt
	if (m_flNextHunt > gpGlobals->time)
		return;

	m_flNextHunt = gpGlobals->time + 2.0;

	CBaseEntity* pOther = NULL;
	Vector vecDir;
	TraceResult tr;

	Vector vecFlat = pev->velocity;
	vecFlat.z = 0;
	vecFlat = vecFlat.Normalize();

	UTIL_MakeVectors(pev->angles);

	if (m_hEnemy == NULL || !m_hEnemy->IsAlive())
	{
		// find target, bounce a bit towards it.
		Look(512);
		m_hEnemy = BestVisibleEnemy();
	}

	// squeek if it's about time blow up
	if ((m_flDie - gpGlobals->time <= 0.5) && (m_flDie - gpGlobals->time >= 0.3))
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "squeek/sqk_die1.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG(0, 0x3F));
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 256, 0.25);
	}

	// higher pitch as squeeker gets closer to detonation time
	float flpitch = 155.0 - 60.0 * ((m_flDie - gpGlobals->time) / PENGUIN_DETONATE_DELAY);
	if (flpitch < 80)
		flpitch = 80;

	if (m_hEnemy != NULL)
	{
		if (FVisible(m_hEnemy))
		{
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = vecDir.Normalize();
		}

		float flVel = pev->velocity.Length();
		float flAdj = 50.0 / (flVel + 10.0);

		if (flAdj > 1.2)
			flAdj = 1.2;

		// ALERT( at_console, "think : enemy\n");

		// ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

		pev->velocity = pev->velocity * flAdj + m_vecTarget * 300;
	}

	if (pev->flags & FL_ONGROUND)
	{
		pev->avelocity = Vector(0, 0, 0);
	}
	else
	{
		if (pev->avelocity == Vector(0, 0, 0))
		{
			pev->avelocity.x = RANDOM_FLOAT(-100, 100);
			pev->avelocity.z = RANDOM_FLOAT(-100, 100);
		}
	}

	if ((pev->origin - m_posPrev).Length() < 1.0)
	{
		pev->velocity.x = RANDOM_FLOAT(-100, 100);
		pev->velocity.y = RANDOM_FLOAT(-100, 100);
	}
	m_posPrev = pev->origin;

	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->angles.z = 0;
	pev->angles.x = 0;
}

void CPenguinGrenade::Smoke()
{
	if (UTIL_PointContents(pev->origin) == CONTENTS_WATER)
	{
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);
		UTIL_Remove(this);
	}
	else
	{
		g_engfuncs.pfnMessageBegin(MSG_PVS, SVC_TEMPENTITY, pev->origin, 0);
		g_engfuncs.pfnWriteByte(TE_SMOKE);
		g_engfuncs.pfnWriteCoord(pev->origin.x);
		g_engfuncs.pfnWriteCoord(pev->origin.y);
		g_engfuncs.pfnWriteCoord(pev->origin.z);
		g_engfuncs.pfnWriteShort(g_sModelIndexSmoke);
		g_engfuncs.pfnWriteByte((pev->dmg - 50.0) * 0.8);
		g_engfuncs.pfnWriteByte(12);
		g_engfuncs.pfnMessageEnd();

		UTIL_Remove(this);
	}
}

int CPenguinGrenade::BloodColor()
{
	return BLOOD_COLOR_RED;
}
