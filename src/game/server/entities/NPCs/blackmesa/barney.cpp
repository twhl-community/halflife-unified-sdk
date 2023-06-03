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

// UNDONE: Holster weapon?

#include "cbase.h"
#include "talkmonster.h"
#include "defaultai.h"
#include "scripted.h"
#include "barney.h"

LINK_ENTITY_TO_CLASS(monster_barney, CBarney);

BEGIN_DATAMAP(CBarney)
DEFINE_FIELD(m_painTime, FIELD_TIME),
	DEFINE_FIELD(m_checkAttackTime, FIELD_TIME),
	DEFINE_FIELD(m_lastAttackCheck, FIELD_BOOLEAN),
	END_DATAMAP();

Task_t tlBaFollow[] =
	{
		{TASK_MOVE_TO_TARGET_RANGE, (float)128}, // Move within 128 of target ent (client)
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE},
};

Schedule_t slBaFollow[] =
	{
		{tlBaFollow,
			std::size(tlBaFollow),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"Follow"},
};

Task_t tlBarneyEnemyDraw[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_ENEMY, 0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_ARM},
};

/**
 *	@brief much better looking draw schedule for when barney knows who he's gonna attack.
 */
Schedule_t slBarneyEnemyDraw[] =
	{
		{tlBarneyEnemyDraw,
			std::size(tlBarneyEnemyDraw),
			0,
			0,
			"Barney Enemy Draw"}};

Task_t tlBaFaceTarget[] =
	{
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_TARGET, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE},
};

Schedule_t slBaFaceTarget[] =
	{
		{tlBaFaceTarget,
			std::size(tlBaFaceTarget),
			bits_COND_CLIENT_PUSH |
				bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"FaceTarget"},
};

Task_t tlIdleBaStand[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},			// repick IDLESTAND every two seconds.
		{TASK_TLK_HEADRESET, (float)0}, // reset head position
};

Schedule_t slIdleBaStand[] =
	{
		{tlIdleBaStand,
			std::size(tlIdleBaStand),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_SMELL |
				bits_COND_PROVOKED,

			bits_SOUND_COMBAT | // sound flags - change these, and you'll break the talking code.
								// bits_SOUND_PLAYER		|
								// bits_SOUND_WORLD		|

				bits_SOUND_DANGER |
				bits_SOUND_MEAT | // scents
				bits_SOUND_CARCASS |
				bits_SOUND_GARBAGE,
			"IdleStand"},
};

BEGIN_CUSTOM_SCHEDULES(CBarney)
slBaFollow,
	slBarneyEnemyDraw,
	slBaFaceTarget,
	slIdleBaStand
	END_CUSTOM_SCHEDULES();

void CBarney::OnCreate()
{
	CTalkMonster::OnCreate();

	pev->health = GetSkillFloat("barney_health"sv);
	pev->model = MAKE_STRING("models/barney.mdl");

	SetClassification("player_ally");

	m_iszUse = MAKE_STRING("BA_OK");
	m_iszUnUse = MAKE_STRING("BA_WAIT");
}

void CBarney::StartTask(const Task_t* pTask)
{
	CTalkMonster::StartTask(pTask);
}

void CBarney::RunTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != nullptr && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask(pTask);
		break;
	default:
		CTalkMonster::RunTask(pTask);
		break;
	}
}

int CBarney::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_CARCASS |
		   bits_SOUND_MEAT |
		   bits_SOUND_GARBAGE |
		   bits_SOUND_DANGER |
		   bits_SOUND_PLAYER;
}

void CBarney::AlertSound()
{
	if (m_hEnemy != nullptr)
	{
		if (FOkToSpeak())
		{
			PlaySentence("BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}
}

void CBarney::SetYawSpeed()
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

bool CBarney::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= 1024 && flDot >= 0.5)
	{
		if (gpGlobals->time > m_checkAttackTime)
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector(0, 0, 55);
			CBaseEntity* pEnemy = m_hEnemy;
			Vector shootTarget = ((pEnemy->BodyTarget(shootOrigin) - pEnemy->pev->origin) + m_vecEnemyLKP);
			UTIL_TraceLine(shootOrigin, shootTarget, dont_ignore_monsters, edict(), &tr);
			m_checkAttackTime = gpGlobals->time + 1;
			if (tr.flFraction == 1.0 || (tr.pHit != nullptr && CBaseEntity::Instance(tr.pHit) == pEnemy))
				m_lastAttackCheck = true;
			else
				m_lastAttackCheck = false;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return false;
}

void CBarney::GuardFirePistol()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EmitSoundDyn(CHAN_WEAPON, "barney/ba_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--; // take away a bullet!
}

void CBarney::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BARNEY_AE_SHOOT:
		GuardFirePistol();
		break;

	case BARNEY_AE_DRAW:
		// guard's bodygroup switches here so he can pull gun from holster
		if (GetBodygroup(GuardBodyGroup::Weapons) == NPCWeaponState::Holstered)
		{
			SetBodygroup(GuardBodyGroup::Weapons, NPCWeaponState::Drawn);
		}
		break;

	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		if (GetBodygroup(GuardBodyGroup::Weapons) == NPCWeaponState::Drawn)
		{
			SetBodygroup(GuardBodyGroup::Weapons, NPCWeaponState::Holstered);
		}
		break;

	default:
		CTalkMonster::HandleAnimEvent(pEvent);
	}
}

void CBarney::Spawn()
{
	Precache();

	SetModel(STRING(pev->model));
	SetSize(VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->view_ofs = Vector(0, 0, 50);  // position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	SetBodygroup(GuardBodyGroup::Weapons, NPCWeaponState::Holstered + m_iGuardBody);

	MonsterInit();
}

void CBarney::Precache()
{
	PrecacheModel(STRING(pev->model));

	PrecacheSound("barney/ba_attack1.wav");
	PrecacheSound("barney/ba_attack2.wav");

	PrecacheSound("barney/ba_pain1.wav");
	PrecacheSound("barney/ba_pain2.wav");
	PrecacheSound("barney/ba_pain3.wav");

	PrecacheSound("barney/ba_die1.wav");
	PrecacheSound("barney/ba_die2.wav");
	PrecacheSound("barney/ba_die3.wav");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

void CBarney::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "BA_ANSWER";
	m_szGrp[TLK_QUESTION] = "BA_QUESTION";
	m_szGrp[TLK_IDLE] = "BA_IDLE";
	m_szGrp[TLK_STARE] = "BA_STARE";
	m_szGrp[TLK_STOP] = "BA_STOP";

	m_szGrp[TLK_NOSHOOT] = "BA_SCARED";
	m_szGrp[TLK_HELLO] = "BA_HELLO";

	m_szGrp[TLK_PLHURT1] = "!BA_CUREA";
	m_szGrp[TLK_PLHURT2] = "!BA_CUREB";
	m_szGrp[TLK_PLHURT3] = "!BA_CUREC";

	m_szGrp[TLK_PHELLO] = nullptr;		  //"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = nullptr;		  //"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "BA_PQUEST"; // UNDONE

	m_szGrp[TLK_SMELL] = "BA_SMELL";

	m_szGrp[TLK_WOUND] = "BA_WOUND";
	m_szGrp[TLK_MORTAL] = "BA_MORTAL";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}

bool CBarney::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	bool ret = CTalkMonster::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (attacker->pev->flags & FL_CLIENT) != 0)
	{
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == nullptr)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) != 0 || IsFacing(attacker, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence("BA_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(true);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("BA_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("BA_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}

void CBarney::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EmitSoundDyn(CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1:
		EmitSoundDyn(CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2:
		EmitSoundDyn(CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

void CBarney::DeathSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EmitSoundDyn(CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1:
		EmitSoundDyn(CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2:
		EmitSoundDyn(CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

void CBarney::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST)) != 0)
		{
			flDamage = flDamage / 2;
		}
		break;
		// TODO: Otis doesn't have a helmet, probably don't want his dome being bulletproof
	case 10:
		if ((bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)) != 0)
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet(ptr->vecEndPos, 1.0);
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack(attacker, flDamage, vecDir, ptr, bitsDamageType);
}

void CBarney::DropWeapon()
{
	Vector vecGunPos, vecGunAngles;
	GetAttachment(0, vecGunPos, vecGunAngles);
	DropItem("weapon_9mmhandgun", vecGunPos, vecGunAngles);
}

void CBarney::Killed(CBaseEntity* attacker, int iGib)
{
	if (GetBodygroup(GuardBodyGroup::Weapons) != NPCWeaponState::Blank)
	{ // drop the gun!
		SetBodygroup(GuardBodyGroup::Weapons, NPCWeaponState::Blank);
		DropWeapon();
	}

	SetUse(nullptr);
	CTalkMonster::Killed(attacker, iGib);
}

const Schedule_t* CBarney::GetScheduleOfType(int Type)
{
	const Schedule_t* psched;

	switch (Type)
	{
	case SCHED_ARM_WEAPON:
		if (m_hEnemy != nullptr)
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;

		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used'
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBaFaceTarget; // override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

void CBarney::SpeakKilledEnemy()
{
	PlaySentence("BA_KILL", 4, VOL_NORM, ATTN_NORM);
}

const Schedule_t* CBarney::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != nullptr);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER) != 0)
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		SpeakKilledEnemy();
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

		// always act surprized with a new enemy
		if (HasConditions(bits_COND_NEW_ENEMY) && HasConditions(bits_COND_LIGHT_DAMAGE))
			return GetScheduleOfType(SCHED_SMALL_FLINCH);

		// wait for one schedule to draw gun
		if (GetBodygroup(GuardBodyGroup::Weapons) != NPCWeaponState::Drawn)
			return GetScheduleOfType(SCHED_ARM_WEAPON);

		if (HasConditions(bits_COND_HEAVY_DAMAGE))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		if (m_hEnemy == nullptr && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(false);
				break;
			}
			else
			{
				if (HasConditions(bits_COND_CLIENT_PUSH))
				{
					return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CBarney::GetIdealState()
{
	return CTalkMonster::GetIdealState();
}

void CBarney::DeclineFollowing()
{
	PlaySentence("BA_POK", 2, VOL_NORM, ATTN_NORM);
}

bool CBarney::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("bodystate", pkvd->szKeyName))
	{
		m_iGuardBody = atoi(pkvd->szValue);
		return true;
	}

	return CTalkMonster::KeyValue(pkvd);
}

/**
 *	@brief DEAD BARNEY PROP
 *	@details Designer selects a pose in worldcraft, 0 through num_poses-1
 *	this value is added to what is selected as the 'first dead pose' among the monster's normal animations.
 *	All dead poses must appear sequentially in the model file.
 *	Be sure and set the m_iFirstPose properly!
 */
class CDeadBarney : public CBaseMonster
{
public:
	void OnCreate() override;
	void Spawn() override;

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	static const char* m_szPoses[3];
};

const char* CDeadBarney::m_szPoses[] = {"lying_on_back", "lying_on_side", "lying_on_stomach"};

void CDeadBarney::OnCreate()
{
	CBaseMonster::OnCreate();

	// Corpses have less health
	pev->health = 8; // GetSkillFloat("barney_health"sv);
	pev->model = MAKE_STRING("models/barney.mdl");

	SetClassification("player_ally");
}

bool CDeadBarney::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_barney_dead, CDeadBarney);

void CDeadBarney::Spawn()
{
	PrecacheModel(STRING(pev->model));
	SetModel(STRING(pev->model));

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		AILogger->debug("Dead barney with bad pose");
	}

	MonsterInitDead();
}
