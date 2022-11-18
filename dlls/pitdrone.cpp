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
// pit drone - medium sized, fires sharp teeth like spikes and swipes with sharp appendages
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "nodes.h"
#include "effects.h"
#include "decals.h"
#include "soundent.h"
#include "game.h"

#define SQUID_SPRINT_DIST 256 // how close the squid has to get before starting to sprint and refusing to swerve

int iSpikeTrail;
int iPitdroneSpitSprite;


//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_PITDRONE_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_PITDRONE_SMELLFOOD,
	SCHED_PITDRONE_EAT,
	SCHED_PITDRONE_SNIFF_AND_EAT,
	SCHED_PITDRONE_WALLOW,
	SCHED_PITDRONE_COVER_AND_RELOAD,
	SCHED_PITDRONE_WAIT_FACE_ENEMY,
	SCHED_PITDRONE_TAKECOVER_FAILED,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_PITDRONE_HOPTURN = LAST_COMMON_TASK + 1,
};

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CPitdroneSpike : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override;

	int Classify() override { return CLASS_NONE; }

	static void Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles);
	void EXPORT SpikeTouch(CBaseEntity* pOther);

	void EXPORT StartTrail();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_maxFrame;
};

LINK_ENTITY_TO_CLASS(pitdronespike, CPitdroneSpike);

TYPEDESCRIPTION CPitdroneSpike::m_SaveData[] =
	{
		DEFINE_FIELD(CPitdroneSpike, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CPitdroneSpike, CBaseEntity);

void CPitdroneSpike::Precache()
{
	PRECACHE_MODEL("models/pit_drone_spike.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");

	iSpikeTrail = PRECACHE_MODEL("sprites/spike_trail.spr");
}

void CPitdroneSpike::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("pitdronespike");

	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags |= FL_MONSTER;
	pev->health = 1;

	SET_MODEL(ENT(pev), "models/pit_drone_spike.mdl");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CPitdroneSpike::Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles)
{
	CPitdroneSpike* pSpit = GetClassPtr((CPitdroneSpike*)NULL);

	pSpit->pev->angles = vecAngles;
	UTIL_SetOrigin(pSpit->pev, vecStart);

	pSpit->Spawn();

	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink(&CPitdroneSpike::StartTrail);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CPitdroneSpike::SpikeTouch(CBaseEntity* pOther)
{
	// splat sound
	int iPitch = RANDOM_FLOAT(120, 140);

	if (0 == pOther->pev->takedamage)
	{
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "weapons/xbow_hit1.wav", VOL_NORM, ATTN_NORM, 0, iPitch);
	}
	else
	{
		pOther->TakeDamage(pev, pev, gSkillData.pitdroneDmgSpit, DMG_GENERIC);
		EMIT_SOUND_DYN(edict(), CHAN_VOICE, "weapons/xbow_hitbod1.wav", VOL_NORM, ATTN_NORM, 0, iPitch);
	}

	SetTouch(nullptr);

	//Stick it in the world for a bit
	//TODO: maybe stick it on any entity that reports FL_WORLDBRUSH too?
	if (0 == strcmp("worldspawn", STRING(pOther->pev->classname)))
	{
		const auto vecDir = pev->velocity.Normalize();

		const auto vecOrigin = pev->origin - vecDir * 6;

		UTIL_SetOrigin(pev, vecOrigin);

		auto v41 = UTIL_VecToAngles(vecDir);

		pev->angles = UTIL_VecToAngles(vecDir);
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_FLY;

		pev->angles.z = RANDOM_LONG(0, 360);

		pev->velocity = g_vecZero;
		pev->avelocity = g_vecZero;

		SetThink(&CBaseEntity::SUB_FadeOut);
		pev->nextthink = gpGlobals->time + 90.0;
	}
	else
	{
		//Hit something else, remove
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CPitdroneSpike::StartTrail()
{
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex());
	WRITE_SHORT(iSpikeTrail);
	WRITE_BYTE(2);
	WRITE_BYTE(1);
	WRITE_BYTE(197);
	WRITE_BYTE(194);
	WRITE_BYTE(11);
	WRITE_BYTE(192);
	MESSAGE_END();

	SetTouch(&CPitdroneSpike::SpikeTouch);
	SetThink(nullptr);
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define PITDRONE_AE_SPIT (1)
#define PITDRONE_AE_BITE (2)
#define PITDRONE_AE_TAILWHIP (4)
#define PITDRONE_AE_HOP (5)
#define PITDRONE_AE_THROW (6)
#define PITDRONE_AE_RELOAD 7

namespace PitdroneBodygroup
{
enum PitdroneBodygroup
{
	Weapons = 1
};
}

namespace PitdroneWeapon
{
enum PitdroneWeapon
{
	Empty = 0,
	Full,
	One = 6,
};
}

class CPitdrone : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int ISoundMask() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void IdleSound() override;
	void PainSound() override;
	void AlertSound() override;
	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;
	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckMeleeAttack2(float flDot, float flDist) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	void RunAI() override;
	bool FValidateHintType(short sHint) override;
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	int IRelationship(CBaseEntity* pTarget) override;
	int IgnoreConditions() override;

	void CheckAmmo() override;
	void GibMonster() override;
	bool KeyValue(KeyValueData* pkvd) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flLastHurtTime;	 // we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpikeTime; // last time the pit drone used the spike attack.
	int m_iInitialAmmo;
	float m_flNextEatTime;
};
LINK_ENTITY_TO_CLASS(monster_pitdrone, CPitdrone);

TYPEDESCRIPTION CPitdrone::m_SaveData[] =
	{
		DEFINE_FIELD(CPitdrone, m_flLastHurtTime, FIELD_TIME),
		DEFINE_FIELD(CPitdrone, m_flNextSpikeTime, FIELD_TIME),
		DEFINE_FIELD(CPitdrone, m_flNextEatTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CPitdrone, CBaseMonster);

//=========================================================
// IgnoreConditions
//=========================================================
int CPitdrone::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (gpGlobals->time - m_flLastHurtTime <= 20)
	{
		// haven't been hurt in 20 seconds, so let the drone care about stink.
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	return iIgnore;
}

int CPitdrone::IRelationship(CBaseEntity* pTarget)
{
	//Always mark pit drones as allies
	if (FClassnameIs(pTarget->pev, "monster_pitdrone"))
	{
		return R_AL;
	}

	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
bool CPitdrone::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_iInitialAmmo == -1 || GetBodygroup(PitdroneBodygroup::Weapons) == PitdroneWeapon::Empty || (IsMoving() && flDist >= 512))
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return false;
	}

	if (flDist > 128 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpikeTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return false;
			}
		}

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpikeTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpikeTime = gpGlobals->time + 0.5;
		}

		return true;
	}

	return false;
}

bool CPitdrone::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 64 && flDot >= 0.7)
	{
		return RANDOM_LONG(0, 3) == 0;
	}
	return false;
}

bool CPitdrone::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= 64 && flDot >= 0.7 && !HasConditions(bits_COND_CAN_MELEE_ATTACK1))
	{
		return true;
	}
	return false;
}

//=========================================================
//  FValidateHintType
//=========================================================
bool CPitdrone::FValidateHintType(short sHint)
{
	int i;

	static short sSquidHints[] =
		{
			HINT_WORLD_HUMAN_BLOOD,
		};

	for (i = 0; i < ARRAYSIZE(sSquidHints); i++)
	{
		if (sSquidHints[i] == sHint)
		{
			return true;
		}
	}

	ALERT(at_aiconsole, "Couldn't validate hint type");
	return false;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CPitdrone::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_CARCASS |
		   bits_SOUND_MEAT |
		   bits_SOUND_GARBAGE |
		   bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CPitdrone::Classify()
{
	return CLASS_ALIEN_PREDATOR;
}

//=========================================================
// IdleSound
//=========================================================
void CPitdrone::IdleSound()
{
}

//=========================================================
// PainSound
//=========================================================
void CPitdrone::PainSound()
{
	int iPitch = RANDOM_LONG(85, 120);

	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_pain1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_pain2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_pain3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_pain4.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CPitdrone::AlertSound()
{
	int iPitch = RANDOM_LONG(140, 160);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_alert1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_alert2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_alert3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPitdrone::SetYawSpeed()
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case ACT_WALK:
		ys = 90;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	case ACT_IDLE:
		ys = 90;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 90;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CPitdrone::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case PITDRONE_AE_SPIT:
	{
		if (m_iInitialAmmo != -1 && GetBodygroup(PitdroneBodygroup::Weapons) != PitdroneWeapon::Empty)
		{
			Vector vecSpitOffset;
			Vector vecSpitDir;

			UTIL_MakeVectors(pev->angles);

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecSpitOffset = (gpGlobals->v_forward * 15 + gpGlobals->v_up * 36);
			vecSpitOffset = (pev->origin + vecSpitOffset);
			vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();

			vecSpitDir.x += RANDOM_FLOAT(-0.05, 0.05);
			vecSpitDir.y += RANDOM_FLOAT(-0.05, 0.05);
			vecSpitDir.z += RANDOM_FLOAT(-0.05, 0);

			// spew the spittle temporary ents.
			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpitOffset);
			WRITE_BYTE(TE_SPRITE_SPRAY);
			WRITE_COORD(vecSpitOffset.x); // pos
			WRITE_COORD(vecSpitOffset.y);
			WRITE_COORD(vecSpitOffset.z);
			WRITE_COORD(vecSpitDir.x); // dir
			WRITE_COORD(vecSpitDir.y);
			WRITE_COORD(vecSpitDir.z);
			WRITE_SHORT(iPitdroneSpitSprite); // model
			WRITE_BYTE(10);					  // count
			WRITE_BYTE(110);				  // speed
			WRITE_BYTE(25);					  // noise ( client will divide by 100 )
			MESSAGE_END();

			CPitdroneSpike::Shoot(pev, vecSpitOffset, vecSpitDir * 900, UTIL_VecToAngles(vecSpitDir));

			auto ammoSubModel = GetBodygroup(PitdroneBodygroup::Weapons);

			if (ammoSubModel == PitdroneWeapon::One)
			{
				ammoSubModel = PitdroneWeapon::Empty;
			}
			else
			{
				++ammoSubModel;
			}

			SetBodygroup(PitdroneBodygroup::Weapons, ammoSubModel);
			--m_cAmmoLoaded;
		}
	}
	break;

	case PITDRONE_AE_BITE:
	{
		// SOUND HERE!
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.pitdroneDmgBite, DMG_SLASH);

		if (pHurt)
		{
			//pHurt->pev->punchangle.z = -15;
			//pHurt->pev->punchangle.x = -45;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
		}
	}
	break;

	case PITDRONE_AE_TAILWHIP:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.pitdroneDmgWhip, DMG_CLUB | DMG_ALWAYSGIB);
		if (pHurt)
		{
			pHurt->pev->punchangle.z = -20;
			pHurt->pev->punchangle.x = 20;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
		}
	}
	break;

	case PITDRONE_AE_HOP:
	{
		float flGravity = g_psv_gravity->value;

		// throw the squid up into the air on this frame.
		if (FBitSet(pev->flags, FL_ONGROUND))
		{
			pev->flags -= FL_ONGROUND;
		}

		// jump into air for 0.8 (24/30) seconds
		//			pev->velocity.z += (0.875 * flGravity) * 0.5;
		pev->velocity.z += (0.625 * flGravity) * 0.5;
	}
	break;

	case PITDRONE_AE_THROW:
	{
		int iPitch;

		// squid throws its prey IF the prey is a client.
		CBaseEntity* pHurt = CheckTraceHullAttack(70, 0, 0);


		if (pHurt)
		{
			// croonchy bite sound
			iPitch = RANDOM_FLOAT(90, 110);
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite2.wav", 1, ATTN_NORM, 0, iPitch);
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite3.wav", 1, ATTN_NORM, 0, iPitch);
				break;
			}


			//pHurt->pev->punchangle.x = RANDOM_LONG(0,34) - 5;
			//pHurt->pev->punchangle.z = RANDOM_LONG(0,49) - 25;
			//pHurt->pev->punchangle.y = RANDOM_LONG(0,89) - 45;

			// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
			UTIL_ScreenShake(pHurt->pev->origin, 25.0, 1.5, 0.7, 2);

			if (pHurt->IsPlayer())
			{
				UTIL_MakeVectors(pev->angles);
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300 + gpGlobals->v_up * 300;
			}
		}
	}
	break;

	case PITDRONE_AE_RELOAD:
	{
		if (m_iInitialAmmo == -1)
		{
			m_cAmmoLoaded = 0;
		}
		else
		{
			SetBodygroup(PitdroneBodygroup::Weapons, PitdroneWeapon::Full);
			m_cAmmoLoaded = 6;
		}

		ClearConditions(bits_COND_NO_AMMO_LOADED);
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CPitdrone::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/pit_drone.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 48));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
	pev->health = gSkillData.pitdroneHealth;
	m_flFieldOfView = VIEW_FIELD_WIDE; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_flNextSpikeTime = gpGlobals->time;

	if (m_iInitialAmmo == -1)
	{
		SetBodygroup(PitdroneBodygroup::Weapons, PitdroneWeapon::Full);
	}
	else if (m_iInitialAmmo <= 0)
	{
		m_iInitialAmmo = -1;
		SetBodygroup(PitdroneBodygroup::Weapons, PitdroneWeapon::Empty);
	}
	else
	{
		//TODO: constrain value to valid range
		SetBodygroup(PitdroneBodygroup::Weapons, (PitdroneWeapon::One + 1) - m_iInitialAmmo);
	}

	if (m_iInitialAmmo == -1)
		m_iInitialAmmo = 0;

	m_cAmmoLoaded = m_iInitialAmmo;

	m_flNextEatTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPitdrone::Precache()
{
	PRECACHE_MODEL("models/pit_drone.mdl");
	PRECACHE_MODEL("models/pit_drone_gibs.mdl");

	UTIL_PrecacheOther("pitdronespike");

	iPitdroneSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr"); // client side spittle.

	PRECACHE_SOUND("zombie/claw_miss2.wav"); // because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("pitdrone/pit_drone_alert1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_alert2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_alert3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_attack_spike1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_attack_spike2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate4.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_die1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_die2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_die3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_idle1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_idle2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_idle3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_melee_attack1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_melee_attack2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_pain1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_pain2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_pain3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_pain4.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_run_on_grate.wav");
	PRECACHE_SOUND("bullchicken/bc_bite2.wav");
	PRECACHE_SOUND("bullchicken/bc_bite3.wav");
}

//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CPitdrone::RunAI()
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if (m_hEnemy != NULL && m_Activity == ACT_RUN)
	{
		// chasing enemy. Sprint for last bit
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() < SQUID_SPRINT_DIST)
		{
			pev->framerate = 1.25;
		}
	}
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t tlPitdroneRangeAttack1[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
};

Schedule_t slPitdroneRangeAttack1[] =
	{
		{tlPitdroneRangeAttack1,
			ARRAYSIZE(tlPitdroneRangeAttack1),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_ENEMY_OCCLUDED |
				bits_COND_NO_AMMO_LOADED,
			0,
			"Squid Range Attack1"},
};

// Chase enemy schedule
Task_t tlPitdroneChaseEnemy1[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1}, // !!!OEM - this will stop nasty squid oscillation.
		{TASK_GET_PATH_TO_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
};

Schedule_t slPitdroneChaseEnemy[] =
	{
		{tlPitdroneChaseEnemy1,
			ARRAYSIZE(tlPitdroneChaseEnemy1),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_SMELL_FOOD |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK2 |
				bits_COND_TASK_FAILED |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER |
				bits_SOUND_MEAT,
			"Squid Chase Enemy"},
};

Task_t tlPitdroneHurtHop[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SOUND_WAKE, (float)0},
		{TASK_PITDRONE_HOPTURN, (float)0},
		{TASK_FACE_ENEMY, (float)0}, // in case squid didn't turn all the way in the air.
};

Schedule_t slPitdroneHurtHop[] =
	{
		{tlPitdroneHurtHop,
			ARRAYSIZE(tlPitdroneHurtHop),
			0,
			0,
			"SquidHurtHop"}};

// squid walks to something tasty and eats it.
Task_t tlPitdroneEat[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_EAT, (float)10}, // this is in case the squid can't get to the food
		{TASK_STORE_LASTPOSITION, (float)0},
		{TASK_GET_PATH_TO_BESTSCENT, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_EAT},
		{TASK_PLAY_SEQUENCE, (float)ACT_EAT},
		{TASK_PLAY_SEQUENCE, (float)ACT_EAT},
		{TASK_EAT, (float)50},
		{TASK_GET_PATH_TO_LASTPOSITION, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_CLEAR_LASTPOSITION, (float)0},
};

Schedule_t slPitdroneEat[] =
	{
		{tlPitdroneEat,
			ARRAYSIZE(tlPitdroneEat),
			bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_NEW_ENEMY,

			// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
			// here or the monster won't detect these sounds at ALL while running this schedule.
			bits_SOUND_MEAT |
				bits_SOUND_CARCASS,
			"SquidEat"}};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the squid. This schedule plays a sniff animation before going to the source of food.
Task_t tlPitdroneSniffAndEat[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_EAT, (float)10}, // this is in case the squid can't get to the food
		{TASK_PLAY_SEQUENCE, (float)ACT_DETECT_SCENT},
		{TASK_STORE_LASTPOSITION, (float)0},
		{TASK_GET_PATH_TO_BESTSCENT, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_EAT},
		{TASK_PLAY_SEQUENCE, (float)ACT_EAT},
		{TASK_PLAY_SEQUENCE, (float)ACT_EAT},
		{TASK_EAT, (float)50},
		{TASK_GET_PATH_TO_LASTPOSITION, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_CLEAR_LASTPOSITION, (float)0},
};

Schedule_t slPitdroneSniffAndEat[] =
	{
		{tlPitdroneSniffAndEat,
			ARRAYSIZE(tlPitdroneSniffAndEat),
			bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_NEW_ENEMY,

			// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
			// here or the monster won't detect these sounds at ALL while running this schedule.
			bits_SOUND_MEAT |
				bits_SOUND_CARCASS,
			"SquidSniffAndEat"}};

// squid does this to stinky things.
Task_t tlPitdroneWallow[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_EAT, (float)10}, // this is in case the squid can't get to the stinkiness
		{TASK_STORE_LASTPOSITION, (float)0},
		{TASK_GET_PATH_TO_BESTSCENT, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_INSPECT_FLOOR},
		{TASK_EAT, (float)50}, // keeps squid from eating or sniffing anything else for a while.
		{TASK_GET_PATH_TO_LASTPOSITION, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_CLEAR_LASTPOSITION, (float)0},
};

Schedule_t slPitdroneWallow[] =
	{
		{tlPitdroneWallow,
			ARRAYSIZE(tlPitdroneWallow),
			bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_NEW_ENEMY,

			// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
			// here or the monster won't detect these sounds at ALL while running this schedule.
			bits_SOUND_GARBAGE,

			"SquidWallow"}};

Task_t tlPitdroneHideReload[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_FAIL_SCHEDULE, ACT_MELEE_ATTACK1},
		{TASK_FIND_COVER_FROM_ENEMY, 0},
		{TASK_RUN_PATH, 0},
		{TASK_WAIT_FOR_MOVEMENT, 0},
		{TASK_REMEMBER, bits_MEMORY_INCOVER},
		{TASK_FACE_ENEMY, 0},
		{TASK_PLAY_SEQUENCE, ACT_RELOAD},
};

Schedule_t slPitdroneHideReload[] =
	{
		{tlPitdroneHideReload,
			ARRAYSIZE(tlPitdroneHideReload),
			bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,
			bits_SOUND_DANGER,

			"PitdroneHideReload"}};

Task_t tlPitdroneWaitInCover[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, 1},
};

Schedule_t slPitdroneWaitInCover[] =
	{
		{tlPitdroneWaitInCover,
			ARRAYSIZE(tlPitdroneWaitInCover),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_CAN_MELEE_ATTACK2 |
				bits_COND_NEW_ENEMY |
				bits_COND_HEAR_SOUND,
			bits_SOUND_DANGER,

			"PitdroneWaitInCover"}};

DEFINE_CUSTOM_SCHEDULES(CPitdrone){
	slPitdroneRangeAttack1,
	slPitdroneChaseEnemy,
	slPitdroneHurtHop,
	slPitdroneEat,
	slPitdroneSniffAndEat,
	slPitdroneWallow,
	slPitdroneHideReload,
	slPitdroneWaitInCover};

IMPLEMENT_CUSTOM_SCHEDULES(CPitdrone, CBaseMonster);

//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CPitdrone::GetSchedule()
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	{
		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			return GetScheduleOfType(SCHED_PITDRONE_HURTHOP);
		}

		if (m_flNextEatTime <= gpGlobals->time)
		{
			if (HasConditions(bits_COND_SMELL_FOOD))
			{
				CSound* pSound;

				pSound = PBestScent();

				if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
				{
					m_flNextEatTime = gpGlobals->time + 90;

					// scent is behind or occluded
					return GetScheduleOfType(SCHED_PITDRONE_SNIFF_AND_EAT);
				}

				m_flNextEatTime = gpGlobals->time + 90;

				// food is right out in the open. Just go get it.
				return GetScheduleOfType(SCHED_PITDRONE_EAT);
			}

			if (HasConditions(bits_COND_SMELL))
			{
				// there's something stinky.
				CSound* pSound;

				pSound = PBestScent();
				if (pSound)
				{
					m_flNextEatTime = gpGlobals->time + 90;
					return GetScheduleOfType(SCHED_PITDRONE_WALLOW);
				}
			}
		}

		break;
	}
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if (HasConditions(bits_COND_NO_AMMO_LOADED) && m_iInitialAmmo != -1)
		{
			return GetScheduleOfType(SCHED_PITDRONE_COVER_AND_RELOAD);
		}

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK2))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK2);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	}

	return CBaseMonster::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CPitdrone::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slPitdroneRangeAttack1[0];
		break;
	case SCHED_PITDRONE_HURTHOP:
		return &slPitdroneHurtHop[0];
		break;
	case SCHED_PITDRONE_EAT:
		return &slPitdroneEat[0];
		break;
	case SCHED_PITDRONE_SNIFF_AND_EAT:
		return &slPitdroneSniffAndEat[0];
		break;
	case SCHED_PITDRONE_WALLOW:
		return &slPitdroneWallow[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slPitdroneChaseEnemy[0];
		break;

	case SCHED_PITDRONE_COVER_AND_RELOAD:
		return &slPitdroneHideReload[0];
		break;

	case SCHED_PITDRONE_WAIT_FACE_ENEMY:
		return &slPitdroneWaitInCover[0];
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CPitdrone::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_PITDRONE_HOPTURN:
	{
		SetActivity(ACT_HOP);
		MakeIdealYaw(m_vecEnemyLKP);
		break;
	}
	case TASK_GET_PATH_TO_ENEMY:
	{
		if (BuildRoute(m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy))
		{
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
		else
		{
			ALERT(at_aiconsole, "GetPathToEnemy failed!!\n");
			TaskFail();
		}
		break;
	}
	default:
	{
		CBaseMonster::StartTask(pTask);
		break;
	}
	}
}

//=========================================================
// RunTask
//=========================================================
void CPitdrone::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_PITDRONE_HOPTURN:
	{
		MakeIdealYaw(m_vecEnemyLKP);
		ChangeYaw(pev->yaw_speed);

		if (m_fSequenceFinished)
		{
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
		break;
	}
	default:
	{
		CBaseMonster::RunTask(pTask);
		break;
	}
	}
}

void CPitdrone::CheckAmmo()
{
	if (m_iInitialAmmo != -1 && m_cAmmoLoaded <= 0)
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

const GibData PitDroneGibs = {"models/pit_drone_gibs.mdl", 0, 7};

void CPitdrone::GibMonster()
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);

	if (CVAR_GET_FLOAT("violence_agibs") != 0) // Should never get here, but someone might call it directly
	{
		//Note: the original doesn't check for German censorship
		CGib::SpawnRandomGibs(pev, 6, PitDroneGibs); // Throw alien gibs
	}

	// don't remove players!
	SetThink(&CBaseMonster::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

bool CPitdrone::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("initammo", pkvd->szKeyName))
	{
		m_iInitialAmmo = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}
