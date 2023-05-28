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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "squadmonster.h"
#include "weapons.h"
#include "soundent.h"
#include "hornet.h"
#include "decals.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_VOLTIGORE_THREAT_DISPLAY = LAST_COMMON_SCHEDULE + 1,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define VOLTIGORE_AE_BOLT1 (1)
#define VOLTIGORE_AE_BOLT2 (2)
#define VOLTIGORE_AE_BOLT3 (3)
#define VOLTIGORE_AE_BOLT4 (4)
#define VOLTIGORE_AE_BOLT5 (5)
// some events are set up in the QC file that aren't recognized by the code yet.
#define VOLTIGORE_AE_PUNCH (6)
#define VOLTIGORE_AE_BITE (7)

#define VOLTIGORE_AE_LEFT_PUNCH (12)
#define VOLTIGORE_AE_RIGHT_PUNCH (13)



#define VOLTIGORE_MELEE_DIST 128

constexpr auto VOLTIGORE_BEAM_COUNT = 8;

class COFChargedBolt : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override;

	int Classify() override { return CLASS_NONE; }

	void InitBeams();
	void ClearBeams();

	void EXPORT ShutdownChargedBolt();

	static COFChargedBolt* ChargedBoltCreate();

	void LaunchChargedBolt(const Vector& vecAim, edict_t* pOwner, int nSpeed, int nAcceleration);

	void SetAttachment(CBaseAnimating* pAttachEnt, int iAttachIdx);

	void ArmBeam(int side);

	void EXPORT AttachThink();

	void EXPORT FlyThink();

	void EXPORT ChargedBoltTouch(CBaseEntity* pOther);

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iShowerSparks;
	EHANDLE m_pBeam[VOLTIGORE_BEAM_COUNT];
	int m_iBeams;
	CBaseAnimating* m_pAttachEnt;
	int m_iAttachIdx;
};

LINK_ENTITY_TO_CLASS(charged_bolt, COFChargedBolt);

TYPEDESCRIPTION COFChargedBolt::m_SaveData[] =
	{
		DEFINE_FIELD(COFChargedBolt, m_iShowerSparks, FIELD_INTEGER),
		DEFINE_ARRAY(COFChargedBolt, m_pBeam, FIELD_EHANDLE, VOLTIGORE_BEAM_COUNT),
		DEFINE_FIELD(COFChargedBolt, m_iBeams, FIELD_INTEGER),
		DEFINE_FIELD(COFChargedBolt, m_pAttachEnt, FIELD_CLASSPTR),
		DEFINE_FIELD(COFChargedBolt, m_iAttachIdx, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(COFChargedBolt, CBaseEntity);

void COFChargedBolt::Precache()
{
	PRECACHE_MODEL("sprites/blueflare2.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	m_iShowerSparks = PRECACHE_MODEL("sprites/spark1.spr");
}

void COFChargedBolt::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(edict(), "sprites/blueflare2.spr");

	UTIL_SetOrigin(pev, pev->origin);

	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->scale = 0.75;

	InitBeams();
}

void COFChargedBolt::InitBeams()
{
	memset(m_pBeam, 0, sizeof(m_pBeam));

	m_iBeams = 0;
	pev->skin = 0;
}

void COFChargedBolt::ClearBeams()
{
	for (auto& pBeam : m_pBeam)
	{
		if (pBeam)
		{
			UTIL_Remove(pBeam);
			pBeam = nullptr;
		}
	}

	m_iBeams = 0;
	pev->skin = 0;
}

void COFChargedBolt::ShutdownChargedBolt()
{
	ClearBeams();

	UTIL_Remove(this);
}

COFChargedBolt* COFChargedBolt::ChargedBoltCreate()
{
	auto pBolt = GetClassPtr<COFChargedBolt>(nullptr);

	pBolt->pev->classname = MAKE_STRING("charged_bolt");

	pBolt->Spawn();

	return pBolt;
}

void COFChargedBolt::LaunchChargedBolt(const Vector& vecAim, edict_t* pOwner, int nSpeed, int nAcceleration)
{
	pev->angles = vecAim;
	pev->owner = pOwner;
	pev->velocity = vecAim * nSpeed;

	pev->speed = nSpeed;

	SetTouch(&COFChargedBolt::ChargedBoltTouch);
	SetThink(&COFChargedBolt::FlyThink);

	pev->nextthink = gpGlobals->time + 0.15;
}

void COFChargedBolt::SetAttachment(CBaseAnimating* pAttachEnt, int iAttachIdx)
{
	Vector vecOrigin;
	Vector vecAngles;

	m_iAttachIdx = iAttachIdx;
	m_pAttachEnt = pAttachEnt;

	pAttachEnt->GetAttachment(iAttachIdx, vecOrigin, vecAngles);

	UTIL_SetOrigin(pev, vecOrigin);

	SetThink(&COFChargedBolt::AttachThink);

	pev->nextthink = gpGlobals->time + 0.05;
}

//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void COFChargedBolt::ArmBeam(int side)
{
	TraceResult tr;
	float flDist = 1.0;

	if (m_iBeams >= VOLTIGORE_BEAM_COUNT)
		m_iBeams = 0;

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT(pev), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	auto pBeam = m_pBeam[m_iBeams].Entity<CBeam>();

	if (!pBeam)
	{
		m_pBeam[m_iBeams] = pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 30);
	}

	if (!pBeam)
		return;

	auto pHit = Instance(tr.pHit);

	if (pHit && pHit->pev->takedamage != DAMAGE_NO)
	{
		pBeam->EntsInit(entindex(), pHit->entindex());
		pBeam->SetColor(255, 16, 255);
		pBeam->SetBrightness(255);
		pBeam->SetNoise(20);
	}
	else
	{
		pBeam->PointEntInit(tr.vecEndPos, entindex());
		pBeam->SetColor(180, 16, 255);
		pBeam->SetBrightness(255);
		pBeam->SetNoise(80);
	}

	++m_iBeams;
}

void COFChargedBolt::AttachThink()
{
	Vector vecOrigin;
	Vector vecAngles;

	m_pAttachEnt->GetAttachment(m_iAttachIdx, vecOrigin, vecAngles);
	UTIL_SetOrigin(pev, vecOrigin);

	pev->nextthink = gpGlobals->time + 0.05;
}

void COFChargedBolt::FlyThink()
{
	ArmBeam(-1);
	ArmBeam(1);
	pev->nextthink = gpGlobals->time + 0.05;
}

void COFChargedBolt::ChargedBoltTouch(CBaseEntity* pOther)
{
	SetTouch(nullptr);
	SetThink(nullptr);

	if (pOther->pev->takedamage == DAMAGE_NO)
	{
		if (0 == strcmp("worldspawn", STRING(pOther->pev->classname)))
		{
			TraceResult tr;
			UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, edict(), &tr);
			UTIL_DecalTrace(&tr, DECAL_SCORCH1 + RANDOM_LONG(0, 1));
		}
	}
	else
	{
		ClearMultiDamage();
		pOther->TakeDamage(pev, pev, gSkillData.voltigoreDmgBeam, DMG_ALWAYSGIB | DMG_SHOCK);
	}

	pev->velocity = g_vecZero;

	auto pevOwner = VARS(pev->owner);

	//Null out the owner to avoid issues with radius damage
	pev->owner = nullptr;

	ClearMultiDamage();

	RadiusDamage(pev->origin, pev, pevOwner, gSkillData.voltigoreDmgBeam, 128.0, CLASS_NONE, DMG_ALWAYSGIB | DMG_SHOCK);

	SetThink(&COFChargedBolt::ShutdownChargedBolt);
	pev->nextthink = gpGlobals->time + 0.5;
}

class COFVoltigore : public CSquadMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	int ISoundMask() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector(-80, -80, 0);
		pev->absmax = pev->origin + Vector(80, 80, 90);
	}

	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	bool FCanCheckAttacks() override;
	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;
	void AlertSound() override;
	void PainSound() override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	int IRelationship(CBaseEntity* pTarget) override;
	void StopTalking();
	bool ShouldSpeak();

	void ClearBeams();

	void EXPORT CallDeathGibThink();

	virtual void DeathGibThink();

	void GibMonster() override;

	void Killed(entvars_t* pevAttacker, int iGib) override;

	CUSTOM_SCHEDULES;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pPainSounds[];
	static const char* pAlertSounds[];

	EHANDLE m_pBeam[VOLTIGORE_BEAM_COUNT];
	int m_iBeams;

	float m_flNextBeamAttackCheck;

	float m_flNextPainTime;

	// three hacky fields for speech stuff. These don't really need to be saved.
	float m_flNextSpeakTime;
	float m_flNextWordTime;
	int m_iLastWord;

	EHANDLE m_pChargedBolt;

	int m_iVoltigoreGibs;
	bool m_fDeathCharge;
	float m_flDeathStartTime;
};
LINK_ENTITY_TO_CLASS(monster_alien_voltigore, COFVoltigore);

TYPEDESCRIPTION COFVoltigore::m_SaveData[] =
	{
		DEFINE_ARRAY(COFVoltigore, m_pBeam, FIELD_EHANDLE, VOLTIGORE_BEAM_COUNT),
		DEFINE_FIELD(COFVoltigore, m_iBeams, FIELD_INTEGER),
		DEFINE_FIELD(COFVoltigore, m_flNextBeamAttackCheck, FIELD_TIME),
		DEFINE_FIELD(COFVoltigore, m_flNextPainTime, FIELD_TIME),
		DEFINE_FIELD(COFVoltigore, m_flNextSpeakTime, FIELD_TIME),
		DEFINE_FIELD(COFVoltigore, m_flNextWordTime, FIELD_TIME),
		DEFINE_FIELD(COFVoltigore, m_iLastWord, FIELD_INTEGER),
		DEFINE_FIELD(COFVoltigore, m_pChargedBolt, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(COFVoltigore, CSquadMonster);

const char* COFVoltigore::pAttackHitSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav",
};

const char* COFVoltigore::pAttackMissSounds[] =
	{
		"zombie/claw_miss1.wav",
		"zombie/claw_miss2.wav",
};

const char* COFVoltigore::pPainSounds[] =
	{
		"voltigore/voltigore_pain1.wav",
		"voltigore/voltigore_pain2.wav",
		"voltigore/voltigore_pain3.wav",
		"voltigore/voltigore_pain4.wav",
};

const char* COFVoltigore::pAlertSounds[] =
	{
		"voltigore/voltigore_alert1.wav",
		"voltigore/voltigore_alert2.wav",
		"voltigore/voltigore_alert3.wav",
};

//=========================================================
// IRelationship - overridden because Human Grunts are
// Alien Grunt's nemesis.
//=========================================================
int COFVoltigore::IRelationship(CBaseEntity* pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_human_grunt"))
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship(pTarget);
}

//=========================================================
// ISoundMask
//=========================================================
int COFVoltigore::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_PLAYER |
		   bits_SOUND_DANGER;
}

//=========================================================
// TraceAttack
//=========================================================
void COFVoltigore::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	//Ignore shock damage since we have a shock based attack
	//TODO: use a filter based on attacker to identify self harm
	if ((bitsDamageType & DMG_SHOCK) == 0)
	{
		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage); // a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
	}
}

//=========================================================
// StopTalking - won't speak again for 10-20 seconds.
//=========================================================
void COFVoltigore::StopTalking()
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);
}

//=========================================================
// ShouldSpeak - Should this voltigore be talking?
//=========================================================
bool COFVoltigore::ShouldSpeak()
{
	if (m_flNextSpeakTime > gpGlobals->time)
	{
		// my time to talk is still in the future.
		return false;
	}

	if ((pev->spawnflags & SF_MONSTER_GAG) != 0)
	{
		if (m_MonsterState != MONSTERSTATE_COMBAT)
		{
			// if gagged, don't talk outside of combat.
			// if not going to talk because of this, put the talk time
			// into the future a bit, so we don't talk immediately after
			// going into combat
			m_flNextSpeakTime = gpGlobals->time + 3;
			return false;
		}
	}

	return true;
}

//=========================================================
// AlertSound
//=========================================================
void COFVoltigore::AlertSound()
{
	StopTalking();

	EMIT_SOUND(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM);
}

//=========================================================
// PainSound
//=========================================================
void COFVoltigore::PainSound()
{
	if (m_flNextPainTime > gpGlobals->time)
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	StopTalking();

	EMIT_SOUND(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM);
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int COFVoltigore::Classify()
{
	return CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COFVoltigore::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 80;
		break;
	default:
		ys = 70;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void COFVoltigore::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case VOLTIGORE_AE_BOLT1:
	case VOLTIGORE_AE_BOLT2:
	case VOLTIGORE_AE_BOLT3:
	case VOLTIGORE_AE_BOLT4:
	case VOLTIGORE_AE_BOLT5:
	{
		if (m_pChargedBolt)
		{
			UTIL_MakeVectors(pev->angles);

			const auto shootPosition = pev->origin + gpGlobals->v_forward * 50 + gpGlobals->v_up * 32;

			const auto direction = ShootAtEnemy(shootPosition);

			TraceResult tr;
			UTIL_TraceLine(shootPosition, shootPosition + direction * 1024, dont_ignore_monsters, edict(), &tr);

			COFChargedBolt* bolt = m_pChargedBolt.Entity<COFChargedBolt>();

			bolt->LaunchChargedBolt(direction, edict(), 1000, 10);

			//We no longer have to manage the bolt now
			m_pChargedBolt = nullptr;

			ClearBeams();
		}
	}
	break;

	case VOLTIGORE_AE_LEFT_PUNCH:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(VOLTIGORE_MELEE_DIST, gSkillData.voltigoreDmgPunch, DMG_CLUB);

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

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);
			SpawnBlood(vecArmPos, pHurt->BloodColor(), 25); // a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
	}
	break;

	case VOLTIGORE_AE_RIGHT_PUNCH:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(VOLTIGORE_MELEE_DIST, gSkillData.voltigoreDmgPunch, DMG_CLUB);

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

			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);
			SpawnBlood(vecArmPos, pHurt->BloodColor(), 25); // a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
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
void COFVoltigore::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/voltigore.mdl");
	UTIL_SetSize(pev, Vector(-80, -80, 0), Vector(80, 80, 90));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
	pev->health = gSkillData.voltigoreHealth;
	m_flFieldOfView = 0.2; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = 0;
	m_afCapability |= bits_CAP_SQUAD | bits_CAP_TURN_HEAD;

	m_HackedGunPos = Vector(24, 64, 48);

	m_flNextSpeakTime = m_flNextWordTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);

	m_flNextBeamAttackCheck = gpGlobals->time;
	m_fDeathCharge = false;
	m_flDeathStartTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COFVoltigore::Precache()
{
	int i;

	PRECACHE_MODEL("models/voltigore.mdl");

	for (i = 0; i < ARRAYSIZE(pAttackHitSounds); i++)
		PRECACHE_SOUND((char*)pAttackHitSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackMissSounds); i++)
		PRECACHE_SOUND((char*)pAttackMissSounds[i]);

	for (i = 0; i < ARRAYSIZE(pPainSounds); i++)
		PRECACHE_SOUND((char*)pPainSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAlertSounds); i++)
		PRECACHE_SOUND((char*)pAlertSounds[i]);

	PRECACHE_SOUND("voltigore/voltigore_attack_melee1.wav");
	PRECACHE_SOUND("voltigore/voltigore_attack_melee2.wav");
	PRECACHE_SOUND("voltigore/voltigore_attack_shock.wav");

	PRECACHE_SOUND("voltigore/voltigore_communicate1.wav");
	PRECACHE_SOUND("voltigore/voltigore_communicate2.wav");
	PRECACHE_SOUND("voltigore/voltigore_communicate3.wav");

	PRECACHE_SOUND("voltigore/voltigore_die1.wav");
	PRECACHE_SOUND("voltigore/voltigore_die2.wav");
	PRECACHE_SOUND("voltigore/voltigore_die3.wav");

	PRECACHE_SOUND("voltigore/voltigore_footstep1.wav");
	PRECACHE_SOUND("voltigore/voltigore_footstep2.wav");
	PRECACHE_SOUND("voltigore/voltigore_footstep3.wav");

	PRECACHE_SOUND("voltigore/voltigore_run_grunt1.wav");
	PRECACHE_SOUND("voltigore/voltigore_run_grunt2.wav");

	PRECACHE_SOUND("hassault/hw_shoot1.wav");

	PRECACHE_SOUND("debris/beamstart2.wav");

	UTIL_PrecacheOther("charged_bolt");

	m_iVoltigoreGibs = PRECACHE_MODEL("models/vgibs.mdl");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail Schedule
//=========================================================
Task_t tlVoltigoreFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slVoltigoreFail[] =
	{
		{tlVoltigoreFail,
			ARRAYSIZE(tlVoltigoreFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1,
			0,
			"Voltigore Fail"},
};

//=========================================================
// Combat Fail Schedule
//=========================================================
Task_t tlVoltigoreCombatFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slVoltigoreCombatFail[] =
	{
		{tlVoltigoreCombatFail,
			ARRAYSIZE(tlVoltigoreCombatFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1,
			0,
			"Voltigore Combat Fail"},
};

//=========================================================
// Standoff schedule. Used in combat when a monster is
// hiding in cover or the enemy has moved out of sight.
// Should we look around in this schedule?
//=========================================================
Task_t tlVoltigoreStandoff[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)2},
};

Schedule_t slVoltigoreStandoff[] =
	{
		{tlVoltigoreStandoff,
			ARRAYSIZE(tlVoltigoreStandoff),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_SEE_ENEMY |
				bits_COND_NEW_ENEMY |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"Voltigore Standoff"}};

//=========================================================
// primary range attacks
//=========================================================
Task_t tlVoltigoreRangeAttack1[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_RANGE_ATTACK1_NOTURN, (float)0},
		{TASK_WAIT, 0.5},
};

Schedule_t slVoltigoreRangeAttack1[] =
	{
		{tlVoltigoreRangeAttack1,
			ARRAYSIZE(tlVoltigoreRangeAttack1),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_ENEMY_OCCLUDED |
				bits_COND_HEAR_SOUND,

			0,
			"Voltigore Range Attack1"},
};

//=========================================================
// Take cover from enemy! Tries lateral cover before node
// cover!
//=========================================================
Task_t tlVoltigoreTakeCoverFromEnemy[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_WAIT, (float)0.2},
		{TASK_FIND_COVER_FROM_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
		{TASK_FACE_ENEMY, (float)0},
};

Schedule_t slVoltigoreTakeCoverFromEnemy[] =
	{
		{tlVoltigoreTakeCoverFromEnemy,
			ARRAYSIZE(tlVoltigoreTakeCoverFromEnemy),
			bits_COND_NEW_ENEMY,
			0,
			"VoltigoreTakeCoverFromEnemy"},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlVoltigoreVictoryDance[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_VOLTIGORE_THREAT_DISPLAY},
		{TASK_WAIT, (float)0.2},
		{TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_CROUCH},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_STAND},
		{TASK_PLAY_SEQUENCE, (float)ACT_THREAT_DISPLAY},
		{TASK_PLAY_SEQUENCE, (float)ACT_CROUCH},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
		{TASK_PLAY_SEQUENCE, (float)ACT_STAND},
};

Schedule_t slVoltigoreVictoryDance[] =
	{
		{tlVoltigoreVictoryDance,
			ARRAYSIZE(tlVoltigoreVictoryDance),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE,
			0,
			"VoltigoreVictoryDance"},
};

//=========================================================
//=========================================================
Task_t tlVoltigoreThreatDisplay[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_THREAT_DISPLAY},
};

Schedule_t slVoltigoreThreatDisplay[] =
	{
		{tlVoltigoreThreatDisplay,
			ARRAYSIZE(tlVoltigoreThreatDisplay),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE,

			bits_SOUND_PLAYER |
				bits_SOUND_COMBAT |
				bits_SOUND_WORLD,
			"VoltigoreThreatDisplay"},
};

DEFINE_CUSTOM_SCHEDULES(COFVoltigore){
	slVoltigoreFail,
	slVoltigoreCombatFail,
	slVoltigoreStandoff,
	slVoltigoreRangeAttack1,
	slVoltigoreTakeCoverFromEnemy,
	slVoltigoreVictoryDance,
	slVoltigoreThreatDisplay,
};

IMPLEMENT_CUSTOM_SCHEDULES(COFVoltigore, CSquadMonster);

//=========================================================
// FCanCheckAttacks - this is overridden for alien grunts
// because they can use their smart weapons against unseen
// enemies. Base class doesn't attack anyone it can't see.
//=========================================================
bool COFVoltigore::FCanCheckAttacks()
{
	if (!HasConditions(bits_COND_ENEMY_TOOFAR))
	{
		return true;
	}
	else
	{
		return false;
	}
}

//=========================================================
// CheckMeleeAttack1 - alien grunts zap the crap out of
// any enemy that gets too close.
//=========================================================
bool COFVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= VOLTIGORE_MELEE_DIST && flDot >= 0.6 && m_hEnemy != NULL)
	{
		return true;
	}
	return false;
}

//=========================================================
// CheckRangeAttack1
//
// !!!LATER - we may want to load balance this. Several
// tracelines are done, so we may not want to do this every
// server frame. Definitely not while firing.
//=========================================================
bool COFVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	if (IsMoving() && flDist >= 512)
	{
		return false;
	}

	if (flDist >= VOLTIGORE_MELEE_DIST && flDist <= 1024 && flDot >= 0.5 && gpGlobals->time >= m_flNextBeamAttackCheck)
	{
		TraceResult tr;
		Vector vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		UTIL_MakeVectors(pev->angles);
		GetAttachment(0, vecArmPos, vecArmDir);
		//		UTIL_TraceLine( vecArmPos, vecArmPos + gpGlobals->v_forward * 256, ignore_monsters, ENT(pev), &tr);
		UTIL_TraceLine(vecArmPos, m_hEnemy->BodyTarget(vecArmPos), dont_ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction == 1.0 || tr.pHit == m_hEnemy->edict())
		{
			m_flNextBeamAttackCheck = gpGlobals->time + RANDOM_FLOAT(20, 30);
			return true;
		}

		m_flNextBeamAttackCheck = gpGlobals->time + 0.2; // don't check for half second if this check wasn't successful
	}

	return false;
}

//=========================================================
// StartTask
//=========================================================
void COFVoltigore::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE:
	{
		ClearBeams();

		UTIL_MakeVectors(pev->angles);
		if (BuildRoute(m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, NULL))
		{
			TaskComplete();
		}
		else
		{
			ALERT(at_aiconsole, "VoltigoreGetPathToEnemyCorpse failed!!\n");
			TaskFail();
		}
	}
	break;

	case TASK_DIE:
	{
		SetThink(&COFVoltigore::CallDeathGibThink);
		CSquadMonster::StartTask(pTask);
	}
	break;

	case TASK_RANGE_ATTACK1_NOTURN:
	{
		ClearBeams();

		UTIL_MakeVectors(pev->angles);

		const auto vecConverge = pev->origin + gpGlobals->v_forward * 50 + gpGlobals->v_up * 32;

		for (auto i = 0; i < 3; ++i)
		{
			CBeam* pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 50);
			m_pBeam[m_iBeams] = pBeam;

			if (!pBeam)
				return;

			pBeam->PointEntInit(vecConverge, entindex());
			pBeam->SetEndAttachment(i + 1);
			pBeam->SetColor(180, 16, 255);
			pBeam->SetBrightness(255);
			pBeam->SetNoise(20);

			++m_iBeams;
		}

		m_pChargedBolt = COFChargedBolt::ChargedBoltCreate();

		UTIL_SetOrigin(m_pChargedBolt->pev, vecConverge);

		UTIL_EmitAmbientSound(edict(), pev->origin, "debris/beamstart2.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));
		CBaseMonster::StartTask(pTask);
	}
	break;

	default:
		ClearBeams();

		CSquadMonster::StartTask(pTask);
		break;
	}
}

void COFVoltigore::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_DIE:
	{
		if (m_fSequenceFinished)
		{
			if (pev->frame >= 255)
			{
				pev->deadflag = DEAD_DEAD;

				pev->framerate = 0;

				//Flatten the bounding box so players can step on it
				if (BBoxFlat())
				{
					const auto maxs = Vector(pev->maxs.x, pev->maxs.y, pev->mins.z + 1);
					UTIL_SetSize(pev, pev->mins, maxs);
				}
				else
				{
					UTIL_SetSize(pev, {-4, -4, 0}, {4, 4, 1});
				}

				if (ShouldFadeOnDeath())
				{
					SUB_StartFadeOut();
				}
				else
				{
					CSoundEnt::InsertSound(bits_SOUND_CARCASS, pev->origin, 384, 30.0);
				}
			}
		}
	}
	break;

	default:
		CSquadMonster::RunTask(pTask);
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t* COFVoltigore::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER) != 0)
		{
			// dangerous sound nearby!
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
		}
	}

	switch (m_MonsterState)
	{
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

		// zap player!
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		if (HasConditions(bits_COND_HEAVY_DAMAGE))
		{
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		// can attack
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		if (OccupySlot(bits_SLOT_AGRUNT_CHASE))
		{
			return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}

		return GetScheduleOfType(SCHED_STANDOFF);
	}
	}

	return CSquadMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* COFVoltigore::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return &slVoltigoreTakeCoverFromEnemy[0];
		break;

	case SCHED_RANGE_ATTACK1:
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			//normal attack
			return &slVoltigoreRangeAttack1[0];
		}
		else
		{
			// attack an unseen enemy
			// return &slVoltigoreHiddenRangeAttack[ 0 ];
			return &slVoltigoreCombatFail[0];
		}
		break;

	case SCHED_CHASE_ENEMY_FAILED:
		return &slVoltigoreRangeAttack1[0];

	case SCHED_VOLTIGORE_THREAT_DISPLAY:
		return &slVoltigoreThreatDisplay[0];
		break;

	case SCHED_STANDOFF:
		return &slVoltigoreStandoff[0];
		break;

	case SCHED_VICTORY_DANCE:
		return &slVoltigoreVictoryDance[0];
		break;

	case SCHED_FAIL:
		// no fail schedule specified, so pick a good generic one.
		{
			if (m_hEnemy != NULL)
			{
				// I have an enemy
				// !!!LATER - what if this enemy is really far away and i'm chasing him?
				// this schedule will make me stop, face his last known position for 2
				// seconds, and then try to move again
				return &slVoltigoreCombatFail[0];
			}

			return &slVoltigoreFail[0];
		}
		break;
	}

	return CSquadMonster::GetScheduleOfType(Type);
}

void COFVoltigore::ClearBeams()
{
	for (auto& pBeam : m_pBeam)
	{
		if (pBeam)
		{
			UTIL_Remove(pBeam);
			pBeam = nullptr;
		}
	}

	m_iBeams = 0;

	pev->skin = 0;

	if (m_pChargedBolt)
	{
		UTIL_Remove(m_pChargedBolt);
		m_pChargedBolt = nullptr;
	}
}

void COFVoltigore::CallDeathGibThink()
{
	DeathGibThink();
}

void COFVoltigore::DeathGibThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	DispatchAnimEvents(0.1);
	StudioFrameAdvance(0.0);

	if (m_fSequenceFinished)
	{
		GibMonster();
	}
	else
	{
		for (auto i = 0; i < 2; ++i)
		{
			const auto side = i == 0 ? -1 : 1;

			UTIL_MakeAimVectors(pev->angles);

			TraceResult tr;

			const auto vecSrc = pev->origin + gpGlobals->v_forward * 32 + side * gpGlobals->v_right * 16 + gpGlobals->v_up * 36;

			float closest = 1;

			//Do 3 ray traces and use the closest one to make a beam
			for (auto ray = 0; ray < 3; ++ray)
			{
				TraceResult tr1;
				UTIL_TraceLine(vecSrc, vecSrc + (side * gpGlobals->v_right * RANDOM_FLOAT(0, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1)) * 512, dont_ignore_monsters, edict(), &tr1);

				if (tr1.flFraction < closest)
				{
					tr = tr1;
					closest = tr1.flFraction;
				}
			}

			//No nearby objects found
			if (closest == 1)
			{
				return;
			}

			auto pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 30);

			if (!pBeam)
				return;

			auto pHit = Instance(tr.pHit);

			if (pHit && pHit->pev->takedamage != DAMAGE_NO)
			{
				pBeam->PointEntInit(pev->origin + Vector(0, 0, 32), pHit->entindex());

				pBeam->SetColor(180, 16, 255);
				pBeam->SetBrightness(255);
				pBeam->SetNoise(128);
			}
			else
			{
				pBeam->PointsInit(tr.vecEndPos, pev->origin + Vector(0, 0, 32));

				pBeam->SetColor(180, 16, 255);
				pBeam->SetBrightness(255);
				pBeam->SetNoise(192);
			}

			pBeam->SetThink(&CBaseEntity::SUB_Remove);
			pBeam->pev->nextthink = gpGlobals->time + 0.6;
		}

		ClearMultiDamage();

		::RadiusDamage(pev->origin, pev, pev, gSkillData.voltigoreDmgBeam, 160.0, CLASS_NONE, DMG_ALWAYSGIB | DMG_SHOCK);
	}
}

const GibLimit VoltigoreGibLimits[] =
	{
		{1},
		{1},
		{1},
		{1},
		{2},
		{1},
		{2},
		{1},
		{2},
};

const GibData VoltigoreGibs = {"models/vgibs.mdl", 0, 9, VoltigoreGibLimits};

void COFVoltigore::GibMonster()
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);

	pev->renderfx = kRenderFxExplode;

	pev->rendercolor.x = 255;
	pev->rendercolor.y = 0;
	pev->rendercolor.z = 0;
	pev->framerate = 0;

	// don't remove players!
	SetThink(&CBaseMonster::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.15;

	//Note: the original didn't have the violence check
	if (CVAR_GET_FLOAT("violence_agibs") != 0) // Should never get here, but someone might call it directly
	{
		//Gib spawning has been rewritten so the logic for limiting gib submodels is generalized
		CGib::SpawnRandomGibs(pev, 12, VoltigoreGibs); // Throw alien gibs
	}
}

void COFVoltigore::Killed(entvars_t* pevAttacker, int iGib)
{
	ClearBeams();

	CSquadMonster::Killed(pevAttacker, iGib);
}
