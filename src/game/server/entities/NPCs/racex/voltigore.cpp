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

#include "cbase.h"
#include "squadmonster.h"
#include "voltigore.h"

class COFChargedBolt : public CBaseEntity
{
	DECLARE_CLASS(COFChargedBolt, CBaseEntity);
	DECLARE_DATAMAP();

public:
	void Precache() override;
	void Spawn() override;

	void InitBeams();
	void ClearBeams();

	void ShutdownChargedBolt();

	static COFChargedBolt* ChargedBoltCreate();

	void LaunchChargedBolt(const Vector& vecAim, CBaseEntity* owner, int nSpeed, int nAcceleration);

	void SetAttachment(CBaseAnimating* pAttachEnt, int iAttachIdx);

	/**
	 *	@brief small beam from arm to nearby geometry
	 */
	void ArmBeam(int side);

	void AttachThink();

	void FlyThink();

	void ChargedBoltTouch(CBaseEntity* pOther);

	int m_iShowerSparks;
	EntityHandle<CBeam> m_pBeam[VOLTIGORE_BEAM_COUNT];
	int m_iBeams;
	CBaseAnimating* m_pAttachEnt;
	int m_iAttachIdx;
};

LINK_ENTITY_TO_CLASS(charged_bolt, COFChargedBolt);

BEGIN_DATAMAP(COFChargedBolt)
DEFINE_FIELD(m_iShowerSparks, FIELD_INTEGER),
	DEFINE_ARRAY(m_pBeam, FIELD_EHANDLE, VOLTIGORE_BEAM_COUNT),
	DEFINE_FIELD(m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(m_pAttachEnt, FIELD_CLASSPTR),
	DEFINE_FIELD(m_iAttachIdx, FIELD_INTEGER),
	DEFINE_FUNCTION(ShutdownChargedBolt),
	DEFINE_FUNCTION(AttachThink),
	DEFINE_FUNCTION(FlyThink),
	DEFINE_FUNCTION(ChargedBoltTouch),
	END_DATAMAP();

void COFChargedBolt::Precache()
{
	PrecacheModel("sprites/blueflare2.spr");
	PrecacheModel("sprites/lgtning.spr");
	m_iShowerSparks = PrecacheModel("sprites/spark1.spr");
}

void COFChargedBolt::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SetModel("sprites/blueflare2.spr");

	SetOrigin(pev->origin);

	SetSize(g_vecZero, g_vecZero);

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
	auto pBolt = g_EntityDictionary->Create<COFChargedBolt>("charged_bolt");

	pBolt->Spawn();

	return pBolt;
}

void COFChargedBolt::LaunchChargedBolt(const Vector& vecAim, CBaseEntity* owner, int nSpeed, int nAcceleration)
{
	pev->angles = vecAim;
	SetOwner(owner);
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

	SetOrigin(vecOrigin);

	SetThink(&COFChargedBolt::AttachThink);

	pev->nextthink = gpGlobals->time + 0.05;
}

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
		UTIL_TraceLine(vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, edict(), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if (flDist == 1.0)
		return;

	CBeam* pBeam = m_pBeam[m_iBeams];

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
	SetOrigin(vecOrigin);

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
		pOther->TakeDamage(this, this, GetSkillFloat("voltigore_dmg_beam"sv), DMG_ALWAYSGIB | DMG_SHOCK);
	}

	pev->velocity = g_vecZero;

	auto pevOwner = GetOwner();

	// Null out the owner to avoid issues with radius damage
	pev->owner = nullptr;

	ClearMultiDamage();

	RadiusDamage(pev->origin, this, pevOwner, GetSkillFloat("voltigore_dmg_beam"sv), 128.0, DMG_ALWAYSGIB | DMG_SHOCK);

	SetThink(&COFChargedBolt::ShutdownChargedBolt);
	pev->nextthink = gpGlobals->time + 0.5;
}

LINK_ENTITY_TO_CLASS(monster_alien_voltigore, COFVoltigore);

BEGIN_DATAMAP(COFVoltigore)
DEFINE_ARRAY(m_pBeam, FIELD_EHANDLE, VOLTIGORE_BEAM_COUNT),
	DEFINE_FIELD(m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(m_flNextBeamAttackCheck, FIELD_TIME),
	DEFINE_FIELD(m_flNextPainTime, FIELD_TIME),
	DEFINE_FIELD(m_flNextSpeakTime, FIELD_TIME),
	DEFINE_FIELD(m_flNextWordTime, FIELD_TIME),
	DEFINE_FIELD(m_iLastWord, FIELD_INTEGER),
	DEFINE_FIELD(m_pChargedBolt, FIELD_EHANDLE),
	DEFINE_FUNCTION(CallDeathGibThink),
	END_DATAMAP();

void COFVoltigore::OnCreate()
{
	CSquadMonster::OnCreate();

	pev->health = GetSkillFloat("voltigore_health"sv);
	pev->model = MAKE_STRING("models/voltigore.mdl");

	SetClassification("alien_military");
}

Relationship COFVoltigore::IRelationship(CBaseEntity* pTarget)
{
	if (pTarget->ClassnameIs("monster_human_grunt"))
	{
		return Relationship::Nemesis;
	}

	return CSquadMonster::IRelationship(pTarget);
}

int COFVoltigore::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_PLAYER |
		   bits_SOUND_DANGER;
}

void COFVoltigore::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// Ignore shock damage since we have a shock based attack
	// TODO: use a filter based on attacker to identify self harm
	if ((bitsDamageType & DMG_SHOCK) == 0)
	{
		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage); // a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		AddMultiDamage(attacker, this, flDamage, bitsDamageType);
	}
}

void COFVoltigore::StopTalking()
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);
}

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

void COFVoltigore::AlertSound()
{
	StopTalking();

	EmitSound(CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, std::size(pAlertSounds) - 1)], 1.0, ATTN_NORM);
}

void COFVoltigore::PainSound()
{
	if (m_flNextPainTime > gpGlobals->time)
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	StopTalking();

	EmitSound(CHAN_VOICE, pPainSounds[RANDOM_LONG(0, std::size(pPainSounds) - 1)], 1.0, ATTN_NORM);
}

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

			COFChargedBolt* bolt = m_pChargedBolt;

			bolt->LaunchChargedBolt(direction, this, 1000, 10);

			// We no longer have to manage the bolt now
			m_pChargedBolt = nullptr;

			ClearBeams();
		}
	}
	break;

	case VOLTIGORE_AE_LEFT_PUNCH:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(GetMeleeDistance(), GetSkillFloat("voltigore_dmg_punch"sv), DMG_CLUB);

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

			EmitSoundDyn(CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, std::size(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);
			SpawnBlood(vecArmPos, pHurt->BloodColor(), 25); // a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EmitSoundDyn(CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, std::size(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
	}
	break;

	case VOLTIGORE_AE_RIGHT_PUNCH:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(GetMeleeDistance(), GetSkillFloat("voltigore_dmg_punch"sv), DMG_CLUB);

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

			EmitSoundDyn(CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, std::size(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

			Vector vecArmPos, vecArmAng;
			GetAttachment(0, vecArmPos, vecArmAng);
			SpawnBlood(vecArmPos, pHurt->BloodColor(), 25); // a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EmitSoundDyn(CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, std::size(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
	}
	break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void COFVoltigore::SpawnCore(const Vector& mins, const Vector& maxs)
{
	Precache();

	SetModel(STRING(pev->model));
	SetSize(mins, maxs);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
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

void COFVoltigore::Spawn()
{
	SpawnCore({-80, -80, 0}, {80, 80, 90});
}

void COFVoltigore::Precache()
{
	PrecacheModel(STRING(pev->model));

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);

	PrecacheSound("voltigore/voltigore_attack_shock.wav");

	PrecacheSound("voltigore/voltigore_communicate1.wav");
	PrecacheSound("voltigore/voltigore_communicate2.wav");
	PrecacheSound("voltigore/voltigore_communicate3.wav");

	PrecacheSound("voltigore/voltigore_die1.wav");
	PrecacheSound("voltigore/voltigore_die2.wav");
	PrecacheSound("voltigore/voltigore_die3.wav");

	PrecacheSound("voltigore/voltigore_footstep1.wav");
	PrecacheSound("voltigore/voltigore_footstep2.wav");
	PrecacheSound("voltigore/voltigore_footstep3.wav");

	PrecacheSound("voltigore/voltigore_run_grunt1.wav");
	PrecacheSound("voltigore/voltigore_run_grunt2.wav");

	PrecacheSound("voltigore/voltigore_eat.wav");

	PrecacheSound("hassault/hw_shoot1.wav");

	PrecacheSound("debris/beamstart2.wav");

	UTIL_PrecacheOther("charged_bolt");

	m_iVoltigoreGibs = PrecacheModel("models/vgibs.mdl");
}

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
			std::size(tlVoltigoreFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1,
			0,
			"Voltigore Fail"},
};

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
			std::size(tlVoltigoreCombatFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1,
			0,
			"Voltigore Combat Fail"},
};

Task_t tlVoltigoreStandoff[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)2},
};

/**
 *	@brief Used in combat when a monster is hiding in cover or the enemy has moved out of sight.
 *	Should we look around in this schedule?
 */
Schedule_t slVoltigoreStandoff[] =
	{
		{tlVoltigoreStandoff,
			std::size(tlVoltigoreStandoff),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_SEE_ENEMY |
				bits_COND_NEW_ENEMY |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"Voltigore Standoff"}};

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
			std::size(tlVoltigoreRangeAttack1),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_ENEMY_OCCLUDED |
				bits_COND_HEAR_SOUND,

			0,
			"Voltigore Range Attack1"},
};

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
			std::size(tlVoltigoreTakeCoverFromEnemy),
			bits_COND_NEW_ENEMY,
			0,
			"VoltigoreTakeCoverFromEnemy"},
};

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
			std::size(tlVoltigoreVictoryDance),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE,
			0,
			"VoltigoreVictoryDance"},
};

Task_t tlVoltigoreThreatDisplay[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_THREAT_DISPLAY},
};

Schedule_t slVoltigoreThreatDisplay[] =
	{
		{tlVoltigoreThreatDisplay,
			std::size(tlVoltigoreThreatDisplay),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE,

			bits_SOUND_PLAYER |
				bits_SOUND_COMBAT |
				bits_SOUND_WORLD,
			"VoltigoreThreatDisplay"},
};

BEGIN_CUSTOM_SCHEDULES(COFVoltigore)
slVoltigoreFail,
	slVoltigoreCombatFail,
	slVoltigoreStandoff,
	slVoltigoreRangeAttack1,
	slVoltigoreTakeCoverFromEnemy,
	slVoltigoreVictoryDance,
	slVoltigoreThreatDisplay
	END_CUSTOM_SCHEDULES();

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

bool COFVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= GetMeleeDistance() && flDot >= 0.6 && m_hEnemy != nullptr)
	{
		return true;
	}
	return false;
}

bool COFVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	//!!!LATER - we may want to load balance this.Several
	// tracelines are done, so we may not want to do this every
	// server frame. Definitely not while firing.
	if (IsMoving() && flDist >= 512)
	{
		return false;
	}

	if (flDist >= GetMeleeDistance() && flDist <= 1024 && flDot >= 0.5 && gpGlobals->time >= m_flNextBeamAttackCheck)
	{
		TraceResult tr;
		Vector vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		UTIL_MakeVectors(pev->angles);
		GetAttachment(0, vecArmPos, vecArmDir);
		//		UTIL_TraceLine( vecArmPos, vecArmPos + gpGlobals->v_forward * 256, ignore_monsters, edict(), &tr);
		UTIL_TraceLine(vecArmPos, m_hEnemy->BodyTarget(vecArmPos), dont_ignore_monsters, edict(), &tr);

		if (tr.flFraction == 1.0 || tr.pHit == m_hEnemy->edict())
		{
			m_flNextBeamAttackCheck = gpGlobals->time + RANDOM_FLOAT(20, 30);
			return true;
		}

		m_flNextBeamAttackCheck = gpGlobals->time + 0.2; // don't check for half second if this check wasn't successful
	}

	return false;
}

void COFVoltigore::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE:
	{
		ClearBeams();

		UTIL_MakeVectors(pev->angles);
		if (BuildRoute(m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, nullptr))
		{
			TaskComplete();
		}
		else
		{
			AILogger->debug("VoltigoreGetPathToEnemyCorpse failed!!");
			TaskFail();
		}
	}
	break;

	case TASK_DIE:
	{
		if (BlowsUpOnDeath())
		{
			SetThink(&COFVoltigore::CallDeathGibThink);
		}

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

		if (CanUseRangeAttacks())
		{
			m_pChargedBolt = COFChargedBolt::ChargedBoltCreate();

			m_pChargedBolt->SetOrigin(vecConverge);
		}

		EmitAmbientSound(pev->origin, "debris/beamstart2.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG(140, 160));
		CBaseMonster::StartTask(pTask);
	}
	break;

	default:
		ClearBeams();

		CSquadMonster::StartTask(pTask);
		break;
	}
}

void COFVoltigore::RunTask(const Task_t* pTask)
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

				// Flatten the bounding box so players can step on it
				if (BBoxFlat())
				{
					const auto maxs = Vector(pev->maxs.x, pev->maxs.y, pev->mins.z + 1);
					SetSize(pev->mins, maxs);
				}
				else
				{
					SetSize({-4, -4, 0}, {4, 4, 1});
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

const Schedule_t* COFVoltigore::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != nullptr);
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
			AttackSound(); // this is a total hack. Should be parto f the schedule
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

const Schedule_t* COFVoltigore::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return &slVoltigoreTakeCoverFromEnemy[0];
		break;

	case SCHED_RANGE_ATTACK1:
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			// normal attack
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
			if (m_hEnemy != nullptr)
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
			const int side = i == 0 ? -1 : 1;

			UTIL_MakeAimVectors(pev->angles);

			TraceResult tr;

			const auto vecSrc = pev->origin + gpGlobals->v_forward * 32 + side * gpGlobals->v_right * 16 + gpGlobals->v_up * 36;

			float closest = 1;

			// Do 3 ray traces and use the closest one to make a beam
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

			// No nearby objects found
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

		::RadiusDamage(pev->origin, this, this, GetSkillFloat("voltigore_dmg_beam"sv), 160.0, DMG_ALWAYSGIB | DMG_SHOCK);
	}
}

void COFVoltigore::GibMonster()
{
	pev->renderfx = kRenderFxExplode;

	pev->rendercolor.x = 255;
	pev->rendercolor.y = 0;
	pev->rendercolor.z = 0;
	pev->framerate = 0;

	// don't remove players!
	SetThink(&CBaseMonster::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.15;

	/*
	EmitSound(CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);

	// Note: the original didn't have the violence check
	if (CVAR_GET_FLOAT("violence_agibs") != 0) // Should never get here, but someone might call it directly
	{
		// Gib spawning has been rewritten so the logic for limiting gib submodels is generalized
		CGib::SpawnRandomGibs(this, 12, VoltigoreGibs); // Throw alien gibs
	}
	*/

	CGib::SpawnClientGibs(this, GibType::Voltigore, 12, true, false);
}

void COFVoltigore::Killed(CBaseEntity* attacker, int iGib)
{
	ClearBeams();

	CSquadMonster::Killed(attacker, iGib);
}
