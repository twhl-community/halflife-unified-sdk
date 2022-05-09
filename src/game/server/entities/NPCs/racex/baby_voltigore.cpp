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
//=========================================================
// Voltigore - Tank like alien
//=========================================================

#include "cbase.h"
#include "squadmonster.h"
#include "voltigore.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define BABYVOLTIGORE_AE_LEFT_FOOT (10)
#define BABYVOLTIGORE_AE_RIGHT_FOOT (11)
#define BABYVOLTIGORE_AE_RUN 14

class COFBabyVoltigore : public COFVoltigore
{
public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector(-16, -16, 0);
		pev->absmax = pev->origin + Vector(16, 16, 32);
	}

	void AlertSound() override;
	void PainSound() override;

	CBaseEntity* CheckTraceHullAttack(float flDist, int iDamage, int iDmgType);

protected:
	//So babies can't fire off attacks if they happen to have an activity for it
	bool CanUseRangeAttacks() const override { return false; }

	//Babies don't blow up
	bool BlowsUpOnDeath() const override { return false; }
};
LINK_ENTITY_TO_CLASS(monster_alien_babyvoltigore, COFBabyVoltigore);

void COFBabyVoltigore::OnCreate()
{
	COFVoltigore::OnCreate();

	pev->health = GetSkillFloat("babyvoltigore_health"sv);
}

//=========================================================
// AlertSound
//=========================================================
void COFBabyVoltigore::AlertSound()
{
	StopTalking();

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, std::size(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, 180);
}

//=========================================================
// PainSound
//=========================================================
void COFBabyVoltigore::PainSound()
{
	if (m_flNextPainTime > gpGlobals->time)
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	StopTalking();

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, std::size(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, 180);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void COFBabyVoltigore::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BABYVOLTIGORE_AE_LEFT_FOOT:
	case BABYVOLTIGORE_AE_RIGHT_FOOT:
		switch (RANDOM_LONG(0, 2))
		{
			// left foot
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "voltigore/voltigore_footstep1.wav", 1, ATTN_IDLE, 0, 130);
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "voltigore/voltigore_footstep2.wav", 1, ATTN_IDLE, 0, 130);
			break;
		case 2:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "voltigore/voltigore_footstep3.wav", 1, ATTN_IDLE, 0, 130);
			break;
		}
		break;

	case VOLTIGORE_AE_LEFT_PUNCH:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(VOLTIGORE_MELEE_DIST, GetSkillFloat("babyvoltigore_dmg_punch"sv), DMG_CLUB);

		if (pHurt)
		{
			pHurt->pev->punchangle.y = -25;
			pHurt->pev->punchangle.x = 8;

			// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
			if (pHurt->IsPlayer())
			{
				// this is a player. Knock him around.
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 250;
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, std::size(pAttackHitSounds) - 1)], 1.0, ATTN_IDLE, 0, 130);

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);
			SpawnBlood(vecArmPos, pHurt->BloodColor(), 25); // a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, std::size(pAttackMissSounds) - 1)], 1.0, ATTN_IDLE, 0, 130);
		}
	}
	break;

	case VOLTIGORE_AE_RIGHT_PUNCH:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(VOLTIGORE_MELEE_DIST, GetSkillFloat("babyvoltigore_dmg_punch"sv), DMG_CLUB);

		if (pHurt)
		{
			pHurt->pev->punchangle.y = 25;
			pHurt->pev->punchangle.x = 8;

			// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
			if (pHurt->IsPlayer())
			{
				// this is a player. Knock him around.
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -250;
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, std::size(pAttackHitSounds) - 1)], 1.0, ATTN_IDLE, 0, 130);

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);
			SpawnBlood(vecArmPos, pHurt->BloodColor(), 25); // a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, std::size(pAttackMissSounds) - 1)], 1.0, ATTN_IDLE, 0, 130);
		}
	}
	break;

	case BABYVOLTIGORE_AE_RUN:
		switch (RANDOM_LONG(0, 1))
		{
			// left foot
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "voltigore/voltigore_run_grunt1.wav", 1, ATTN_NORM, 0, 180);
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "voltigore/voltigore_run_grunt2.wav", 1, ATTN_NORM, 0, 180);
			break;
		}
		break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void COFBabyVoltigore::Spawn()
{
	SpawnCore("models/baby_voltigore.mdl", {-16, -16, 0}, {16, 16, 32});
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COFBabyVoltigore::Precache()
{
	PrecacheCore("models/baby_voltigore.mdl");
}

//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
//
// Used for many contact-range melee attacks. Bites, claws, etc.
//=========================================================
CBaseEntity* COFBabyVoltigore::CheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	TraceResult tr;

	if (IsPlayer())
		UTIL_MakeVectors(pev->angles);
	else
		UTIL_MakeAimVectors(pev->angles);

	Vector vecStart = pev->origin;
	//Don't rescale the Z size for us since we're just a baby
	vecStart.z += pev->size.z;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (iDamage > 0)
		{
			pEntity->TakeDamage(pev, pev, iDamage, iDmgType);
		}

		return pEntity;
	}

	return nullptr;
}
