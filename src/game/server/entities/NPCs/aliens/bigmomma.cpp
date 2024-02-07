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

#define SF_INFOBM_RUN 0x0001
#define SF_INFOBM_WAIT 0x0002

/**
 *	@brief AI Nodes for Big Momma
 */
class CInfoBM : public CPointEntity
{
	DECLARE_CLASS(CInfoBM, CPointEntity);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;

	// name in pev->targetname
	// next in pev->target
	// radius in pev->scale
	// health in pev->health
	// Reach target in pev->message
	// Reach delay in pev->speed
	// Reach sequence in pev->netname

	string_t m_preSequence;
};

LINK_ENTITY_TO_CLASS(info_bigmomma, CInfoBM);

BEGIN_DATAMAP(CInfoBM)
DEFINE_FIELD(m_preSequence, FIELD_STRING),
	END_DATAMAP();

void CInfoBM::Spawn()
{
}

bool CInfoBM::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->scale = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "reachdelay"))
	{
		pev->speed = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "reachtarget"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "reachsequence"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "presequence"))
	{
		m_preSequence = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

/**
 *	@brief Mortar shot entity
 */
class CBMortar : public CBaseEntity
{
	DECLARE_CLASS(CBMortar, CBaseEntity);
	DECLARE_DATAMAP();

public:
	void Spawn() override;

	static CBMortar* Shoot(CBaseEntity* owner, Vector vecStart, Vector vecVelocity);
	void Touch(CBaseEntity* pOther) override;
	void Animate();

	int m_maxFrame;
};

LINK_ENTITY_TO_CLASS(bmortar, CBMortar);

BEGIN_DATAMAP(CBMortar)
DEFINE_FIELD(m_maxFrame, FIELD_INTEGER),
	DEFINE_FUNCTION(Animate),
	END_DATAMAP();

#define BIG_AE_STEP1 1		// Footstep left
#define BIG_AE_STEP2 2		// Footstep right
#define BIG_AE_STEP3 3		// Footstep back left
#define BIG_AE_STEP4 4		// Footstep back right
#define BIG_AE_SACK 5		// Sack slosh
#define BIG_AE_DEATHSOUND 6 // Death sound

#define BIG_AE_MELEE_ATTACKBR 8	 // Leg attack
#define BIG_AE_MELEE_ATTACKBL 9	 // Leg attack
#define BIG_AE_MELEE_ATTACK1 10	 // Leg attack
#define BIG_AE_MORTAR_ATTACK1 11 // Launch a mortar
#define BIG_AE_LAY_CRAB 12		 // Lay a headcrab
#define BIG_AE_JUMP_FORWARD 13	 // Jump up and forward
#define BIG_AE_SCREAM 14		 // alert sound
#define BIG_AE_PAIN_SOUND 15	 // pain sound
#define BIG_AE_ATTACK_SOUND 16	 // attack sound
#define BIG_AE_BIRTH_SOUND 17	 // birth sound
#define BIG_AE_EARLY_TARGET 50	 // Fire target early

// User defined conditions
#define bits_COND_NODE_SEQUENCE (bits_COND_SPECIAL1) // pev->netname contains the name of a sequence to play

// Attack distance constants
#define BIG_ATTACKDIST 170
#define BIG_MORTARDIST 800
#define BIG_MAXCHILDREN 20 // Max # of live headcrab children

#define bits_MEMORY_CHILDPAIR (bits_MEMORY_CUSTOM1)
#define bits_MEMORY_ADVANCE_NODE (bits_MEMORY_CUSTOM2)
#define bits_MEMORY_COMPLETED_NODE (bits_MEMORY_CUSTOM3)
#define bits_MEMORY_FIRED_NODE (bits_MEMORY_CUSTOM4)

int gSpitSprite, gSpitDebrisSprite;
Vector VecCheckSplatToss(CBaseEntity* entity, const Vector& vecSpot1, Vector vecSpot2, float maxHeight);
void MortarSpray(const Vector& position, const Vector& direction, int spriteModel, int count);

#define BIG_CHILDCLASS "monster_babycrab"

class CBigMomma : public CBaseMonster
{
	DECLARE_CLASS(CBigMomma, CBaseMonster);
	DECLARE_DATAMAP();
	DECLARE_CUSTOM_SCHEDULES();

public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	bool SharedKeyValue( const char* szKey ) override;
	void Activate() override;
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	bool HasAlienGibs() override { return true; }

	void RunTask(const Task_t* pTask) override;
	void StartTask(const Task_t* pTask) override;
	const Schedule_t* GetSchedule() override;
	const Schedule_t* GetScheduleOfType(int Type) override;
	void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	void NodeStart(string_t iszNextNode);
	void NodeReach();
	bool ShouldGoToNode();

	void SetYawSpeed() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void LayHeadcrab();

	string_t GetNodeSequence()
	{
		CBaseEntity* pTarget = m_hTargetEnt;
		if (pTarget)
		{
			return pTarget->pev->netname; // netname holds node sequence
		}
		return string_t::Null;
	}


	string_t GetNodePresequence()
	{
		CInfoBM* pTarget = m_hTargetEnt.Get<CInfoBM>();
		if (pTarget)
		{
			return pTarget->m_preSequence;
		}
		return string_t::Null;
	}

	float GetNodeDelay()
	{
		CBaseEntity* pTarget = m_hTargetEnt;
		if (pTarget)
		{
			return pTarget->pev->speed; // Speed holds node delay
		}
		return 0;
	}

	float GetNodeRange()
	{
		CBaseEntity* pTarget = m_hTargetEnt;
		if (pTarget)
		{
			return pTarget->pev->scale; // Scale holds node delay
		}
		return 1e6;
	}

	float GetNodeYaw()
	{
		CBaseEntity* pTarget = m_hTargetEnt;
		if (pTarget)
		{
			if (pTarget->pev->angles.y != 0)
				return pTarget->pev->angles.y;
		}
		return pev->angles.y;
	}

	// Restart the crab count on each new level
	void OverrideReset() override
	{
		m_crabCount = 0;
	}

	void DeathNotice(CBaseEntity* child) override;

	bool CanLayCrab()
	{
		if (m_crabTime < gpGlobals->time && m_crabCount < BIG_MAXCHILDREN)
		{
			// Don't spawn crabs inside each other
			Vector mins = pev->origin - Vector(32, 32, 0);
			Vector maxs = pev->origin + Vector(32, 32, 0);

			CBaseEntity* pList[2];
			int count = UTIL_EntitiesInBox(pList, 2, mins, maxs, FL_MONSTER);
			for (int i = 0; i < count; i++)
			{
				if (pList[i] != this) // Don't hurt yourself!
					return false;
			}
			return true;
		}

		return false;
	}

	void LaunchMortar();

	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector(-95, -95, 0);
		pev->absmax = pev->origin + Vector(95, 95, 190);
	}

	bool CheckMeleeAttack1(float flDot, float flDist) override; // Slash
	bool CheckMeleeAttack2(float flDot, float flDist) override; // Lay a crab
	bool CheckRangeAttack1(float flDot, float flDist) override; // Mortar launch

	static const char* pChildDieSounds[];
	static const char* pSackSounds[];
	static const char* pDeathSounds[];
	static const char* pAttackSounds[];
	static const char* pAttackHitSounds[];
	static const char* pBirthSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pFootSounds[];

private:
	float m_nodeTime;
	float m_crabTime;
	float m_mortarTime;
	float m_painSoundTime;
	int m_crabCount;
};
LINK_ENTITY_TO_CLASS(monster_bigmomma, CBigMomma);

BEGIN_DATAMAP(CBigMomma)
DEFINE_FIELD(m_nodeTime, FIELD_TIME),
	DEFINE_FIELD(m_crabTime, FIELD_TIME),
	DEFINE_FIELD(m_mortarTime, FIELD_TIME),
	DEFINE_FIELD(m_painSoundTime, FIELD_TIME),
	DEFINE_FIELD(m_crabCount, FIELD_INTEGER),
	END_DATAMAP();

const char* CBigMomma::pChildDieSounds[] =
	{
		"gonarch/gon_childdie1.wav",
		"gonarch/gon_childdie2.wav",
		"gonarch/gon_childdie3.wav",
};

const char* CBigMomma::pSackSounds[] =
	{
		"gonarch/gon_sack1.wav",
		"gonarch/gon_sack2.wav",
		"gonarch/gon_sack3.wav",
};

const char* CBigMomma::pDeathSounds[] =
	{
		"gonarch/gon_die1.wav",
};

const char* CBigMomma::pAttackSounds[] =
	{
		"gonarch/gon_attack1.wav",
		"gonarch/gon_attack2.wav",
		"gonarch/gon_attack3.wav",
};
const char* CBigMomma::pAttackHitSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav",
};

const char* CBigMomma::pBirthSounds[] =
	{
		"gonarch/gon_birth1.wav",
		"gonarch/gon_birth2.wav",
		"gonarch/gon_birth3.wav",
};

const char* CBigMomma::pAlertSounds[] =
	{
		"gonarch/gon_alert1.wav",
		"gonarch/gon_alert2.wav",
		"gonarch/gon_alert3.wav",
};

const char* CBigMomma::pPainSounds[] =
	{
		"gonarch/gon_pain2.wav",
		"gonarch/gon_pain4.wav",
		"gonarch/gon_pain5.wav",
};

const char* CBigMomma::pFootSounds[] =
	{
		"gonarch/gon_step1.wav",
		"gonarch/gon_step2.wav",
		"gonarch/gon_step3.wav",
};

void CBigMomma::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = 150 * GetSkillFloat("bigmomma_health_factor"sv);
	pev->model = MAKE_STRING("models/big_mom.mdl");

	SetClassification("alien_monster");
}

bool CBigMomma::KeyValue(KeyValueData* pkvd)
{
#if 0
	if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_volume = atof(pkvd->szValue);
		return true;
	}
#endif
	return CBaseMonster::KeyValue(pkvd);
}

bool CBigMomma :: SharedKeyValue( const char* szKey )
{
	return ( FStrEq( szKey, "model_replacement_filename" )
		  || FStrEq( szKey, "sound_replacement_filename" )
	);
}

void CBigMomma::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 100;
		break;
	default:
		ys = 90;
	}
	pev->yaw_speed = ys;
}

void CBigMomma::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BIG_AE_MELEE_ATTACKBR:
	case BIG_AE_MELEE_ATTACKBL:
	case BIG_AE_MELEE_ATTACK1:
	{
		Vector forward, right;

		AngleVectors(pev->angles, &forward, &right, nullptr);

		Vector center = pev->origin + forward * 128;
		Vector mins = center - Vector(64, 64, 0);
		Vector maxs = center + Vector(64, 64, 64);

		CBaseEntity* pList[8];
		int count = UTIL_EntitiesInBox(pList, 8, mins, maxs, FL_MONSTER | FL_CLIENT);
		CBaseEntity* pHurt = nullptr;

		for (int i = 0; i < count && !pHurt; i++)
		{
			if (pList[i] != this)
			{
				if (pList[i]->pev->owner != edict())
					pHurt = pList[i];
			}
		}

		if (pHurt)
		{
			pHurt->TakeDamage(this, this, GetSkillFloat("bigmomma_dmg_slash"sv), DMG_CRUSH | DMG_SLASH);
			pHurt->pev->punchangle.x = 15;
			switch (pEvent->event)
			{
			case BIG_AE_MELEE_ATTACKBR:
				pHurt->pev->velocity = pHurt->pev->velocity + (forward * 150) + Vector(0, 0, 250) - (right * 200);
				break;

			case BIG_AE_MELEE_ATTACKBL:
				pHurt->pev->velocity = pHurt->pev->velocity + (forward * 150) + Vector(0, 0, 250) + (right * 200);
				break;

			case BIG_AE_MELEE_ATTACK1:
				pHurt->pev->velocity = pHurt->pev->velocity + (forward * 220) + Vector(0, 0, 200);
				break;
			}

			pHurt->pev->flags &= ~FL_ONGROUND;
			EmitSoundDyn(CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
	}
	break;

	case BIG_AE_SCREAM:
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pAlertSounds);
		break;

	case BIG_AE_PAIN_SOUND:
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);
		break;

	case BIG_AE_ATTACK_SOUND:
		EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pAttackSounds);
		break;

	case BIG_AE_BIRTH_SOUND:
		EMIT_SOUND_ARRAY_DYN(CHAN_BODY, pBirthSounds);
		break;

	case BIG_AE_SACK:
		if (RANDOM_LONG(0, 100) < 30)
			EMIT_SOUND_ARRAY_DYN(CHAN_BODY, pSackSounds);
		break;

	case BIG_AE_DEATHSOUND:
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pDeathSounds);
		break;

	case BIG_AE_STEP1: // Footstep left
	case BIG_AE_STEP3: // Footstep back left
		EMIT_SOUND_ARRAY_DYN(CHAN_ITEM, pFootSounds);
		break;

	case BIG_AE_STEP4: // Footstep back right
	case BIG_AE_STEP2: // Footstep right
		EMIT_SOUND_ARRAY_DYN(CHAN_BODY, pFootSounds);
		break;

	case BIG_AE_MORTAR_ATTACK1:
		LaunchMortar();
		break;

	case BIG_AE_LAY_CRAB:
		LayHeadcrab();
		break;

	case BIG_AE_JUMP_FORWARD:
		ClearBits(pev->flags, FL_ONGROUND);

		SetOrigin(pev->origin + Vector(0, 0, 1)); // take him off ground so engine doesn't instantly reset onground
		UTIL_MakeVectors(pev->angles);

		pev->velocity = (gpGlobals->v_forward * 200) + gpGlobals->v_up * 500;
		break;

	case BIG_AE_EARLY_TARGET:
	{
		CBaseEntity* pTarget = m_hTargetEnt;
		if (pTarget && !FStringNull(pTarget->pev->message))
			FireTargets(STRING(pTarget->pev->message), this, this, USE_TOGGLE, 0);
		Remember(bits_MEMORY_FIRED_NODE);
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CBigMomma::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (ptr->iHitgroup != 1)
	{
		// didn't hit the sack?

		if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0, 10) < 1))
		{
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
			pev->dmgtime = gpGlobals->time;
		}

		flDamage = 0.1; // don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}
	else if (gpGlobals->time > m_painSoundTime)
	{
		m_painSoundTime = gpGlobals->time + RANDOM_LONG(1, 3);
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);
	}


	CBaseMonster::TraceAttack(attacker, flDamage, vecDir, ptr, bitsDamageType);
}

bool CBigMomma::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	// Don't take any acid damage -- BigMomma's mortar is acid
	if ((bitsDamageType & DMG_ACID) != 0)
		flDamage = 0;

	if (!HasMemory(bits_MEMORY_PATH_FINISHED))
	{
		if (pev->health <= flDamage)
		{
			pev->health = flDamage + 1;
			Remember(bits_MEMORY_ADVANCE_NODE | bits_MEMORY_COMPLETED_NODE);
			AILogger->debug("BM: Finished node health!!!");
		}
	}

	return CBaseMonster::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);
}

void CBigMomma::LayHeadcrab()
{
	CBaseEntity* pChild = CBaseEntity::Create(BIG_CHILDCLASS, pev->origin, pev->angles, this, false);

	MaybeSetChildClassification(pChild);

	UTIL_InitializeKeyValues( pChild, m_SharedKey, m_SharedValue, m_SharedKeyValues );

	DispatchSpawn(pChild->edict());

	pChild->pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;

	// Is this the second crab in a pair?
	if (HasMemory(bits_MEMORY_CHILDPAIR))
	{
		m_crabTime = gpGlobals->time + RANDOM_FLOAT(5, 10);
		Forget(bits_MEMORY_CHILDPAIR);
	}
	else
	{
		m_crabTime = gpGlobals->time + RANDOM_FLOAT(0.5, 2.5);
		Remember(bits_MEMORY_CHILDPAIR);
	}

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
	UTIL_DecalTrace(&tr, DECAL_MOMMABIRTH);

	EmitSoundDyn(CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBirthSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	m_crabCount++;
}

void CBigMomma::DeathNotice(CBaseEntity* child)
{
	if (m_crabCount > 0) // Some babies may cross a transition, but we reset the count then
		m_crabCount--;
	if (IsAlive())
	{
		// Make the "my baby's dead" noise!
		EMIT_SOUND_ARRAY_DYN(CHAN_WEAPON, pChildDieSounds);
	}
}

void CBigMomma::LaunchMortar()
{
	m_mortarTime = gpGlobals->time + RANDOM_FLOAT(2, 15);

	Vector startPos = pev->origin;
	startPos.z += 180;

	EmitSoundDyn(CHAN_WEAPON, RANDOM_SOUND_ARRAY(pSackSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	CBMortar* pBomb = CBMortar::Shoot(this, startPos, pev->movedir);
	pBomb->pev->gravity = 1.0;
	MortarSpray(startPos, Vector(0, 0, 1), gSpitSprite, 24);
}

void CBigMomma::Spawn()
{
	Precache();

	SetModel(STRING(pev->model));
	SetSize(Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->view_ofs = Vector(0, 0, 128); // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.3;			   // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

void CBigMomma::Precache()
{
	PrecacheModel(STRING(pev->model));

	PRECACHE_SOUND_ARRAY(pChildDieSounds);
	PRECACHE_SOUND_ARRAY(pSackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pBirthSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pFootSounds);

	UTIL_PrecacheOther( BIG_CHILDCLASS, m_SharedKey, m_SharedValue, m_SharedKeyValues );

	// TEMP: Squid
	PrecacheModel("sprites/mommaspit.spr");				   // spit projectile.
	gSpitSprite = PrecacheModel("sprites/mommaspout.spr"); // client side spittle.
	gSpitDebrisSprite = PrecacheModel("sprites/mommablob.spr");

	PrecacheSound("bullchicken/bc_acid1.wav");
	PrecacheSound("bullchicken/bc_spithit1.wav");
	PrecacheSound("bullchicken/bc_spithit2.wav");
}

void CBigMomma::Activate()
{
	if (m_hTargetEnt == nullptr)
		Remember(bits_MEMORY_ADVANCE_NODE); // Start 'er up
}

void CBigMomma::NodeStart(string_t iszNextNode)
{
	pev->netname = iszNextNode;

	CBaseEntity* pTarget = nullptr;

	if (!FStringNull(pev->netname))
	{
		pTarget = UTIL_FindEntityByTargetname(nullptr, STRING(pev->netname));
	}

	if (!pTarget)
	{
		AILogger->debug("BM: Finished the path!!");
		Remember(bits_MEMORY_PATH_FINISHED);
		return;
	}
	Remember(bits_MEMORY_ON_PATH);
	m_hTargetEnt = pTarget;
}

void CBigMomma::NodeReach()
{
	CBaseEntity* pTarget = m_hTargetEnt;

	Forget(bits_MEMORY_ADVANCE_NODE);

	if (!pTarget)
		return;

	if (0 != pTarget->pev->health)
		pev->max_health = pev->health = pTarget->pev->health * GetSkillFloat("bigmomma_health_factor"sv);

	if (!HasMemory(bits_MEMORY_FIRED_NODE))
	{
		if (!FStringNull(pTarget->pev->message))
			FireTargets(STRING(pTarget->pev->message), this, this, USE_TOGGLE, 0);
	}
	Forget(bits_MEMORY_FIRED_NODE);

	pev->netname = pTarget->pev->target;
	if (pTarget->pev->health == 0)
		Remember(bits_MEMORY_ADVANCE_NODE); // Move on if no health at this node
}

bool CBigMomma::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDot >= 0.7)
	{
		if (flDist <= BIG_ATTACKDIST)
			return true;
	}
	return false;
}

bool CBigMomma::CheckMeleeAttack2(float flDot, float flDist)
{
	return CanLayCrab();
}

bool CBigMomma::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= BIG_MORTARDIST && m_mortarTime < gpGlobals->time)
	{
		CBaseEntity* pEnemy = m_hEnemy;

		if (pEnemy)
		{
			Vector startPos = pev->origin;
			startPos.z += 180;
			pev->movedir = VecCheckSplatToss(this, startPos, pEnemy->BodyTarget(pev->origin), RANDOM_FLOAT(150, 500));
			if (pev->movedir != g_vecZero)
				return true;
		}
	}
	return false;
}

enum
{
	SCHED_BIG_NODE = LAST_COMMON_SCHEDULE + 1,
	SCHED_NODE_FAIL,
};

enum
{
	TASK_MOVE_TO_NODE_RANGE = LAST_COMMON_TASK + 1, // Move within node range
	TASK_FIND_NODE,									// Find my next node
	TASK_PLAY_NODE_PRESEQUENCE,						// Play node pre-script
	TASK_PLAY_NODE_SEQUENCE,						// Play node script
	TASK_PROCESS_NODE,								// Fire targets, etc.
	TASK_WAIT_NODE,									// Wait at the node
	TASK_NODE_DELAY,								// Delay walking toward node for a bit. You've failed to get there
	TASK_NODE_YAW,									// Get the best facing direction for this node
};

Task_t tlBigNode[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_NODE_FAIL},
		{TASK_STOP_MOVING, (float)0},
		{TASK_FIND_NODE, (float)0},				// Find my next node
		{TASK_PLAY_NODE_PRESEQUENCE, (float)0}, // Play the pre-approach sequence if any
		{TASK_MOVE_TO_NODE_RANGE, (float)0},	// Move within node range
		{TASK_STOP_MOVING, (float)0},
		{TASK_NODE_YAW, (float)0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_WAIT_NODE, (float)0},			 // Wait for node delay
		{TASK_PLAY_NODE_SEQUENCE, (float)0}, // Play the sequence if one exists
		{TASK_PROCESS_NODE, (float)0},		 // Fire targets, etc.
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
};

Schedule_t slBigNode[] =
	{
		{tlBigNode,
			std::size(tlBigNode),
			0,
			0,
			"Big Node"},
};

Task_t tlNodeFail[] =
	{
		{TASK_NODE_DELAY, (float)10}, // Try to do something else for 10 seconds
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
};

Schedule_t slNodeFail[] =
	{
		{tlNodeFail,
			std::size(tlNodeFail),
			0,
			0,
			"NodeFail"},
};

BEGIN_CUSTOM_SCHEDULES(CBigMomma)
slBigNode,
	slNodeFail
	END_CUSTOM_SCHEDULES();

const Schedule_t* CBigMomma::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_BIG_NODE:
		return slBigNode;
		break;

	case SCHED_NODE_FAIL:
		return slNodeFail;
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

bool CBigMomma::ShouldGoToNode()
{
	if (HasMemory(bits_MEMORY_ADVANCE_NODE))
	{
		if (m_nodeTime < gpGlobals->time)
			return true;
	}
	return false;
}

const Schedule_t* CBigMomma::GetSchedule()
{
	if (ShouldGoToNode())
	{
		return GetScheduleOfType(SCHED_BIG_NODE);
	}

	return CBaseMonster::GetSchedule();
}

void CBigMomma::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_FIND_NODE:
	{
		CBaseEntity* pTarget = m_hTargetEnt;
		if (!HasMemory(bits_MEMORY_ADVANCE_NODE))
		{
			if (pTarget)
				pev->netname = m_hTargetEnt->pev->target;
		}
		NodeStart(pev->netname);
		TaskComplete();
		AILogger->debug("BM: Found node {}", STRING(pev->netname));
	}
	break;

	case TASK_NODE_DELAY:
		m_nodeTime = gpGlobals->time + pTask->flData;
		TaskComplete();
		AILogger->debug("BM: FAIL! Delay {:.2f}", pTask->flData);
		break;

	case TASK_PROCESS_NODE:
		AILogger->debug("BM: Reached node {}", STRING(pev->netname));
		NodeReach();
		TaskComplete();
		break;

	case TASK_PLAY_NODE_PRESEQUENCE:
	case TASK_PLAY_NODE_SEQUENCE:
	{
		string_t sequence;
		if (pTask->iTask == TASK_PLAY_NODE_SEQUENCE)
			sequence = GetNodeSequence();
		else
			sequence = GetNodePresequence();

		AILogger->debug("BM: Playing node sequence {}", STRING(sequence));
		if (!FStringNull(sequence))
		{
			const int sequenceIndex = LookupSequence(STRING(sequence));
			if (sequenceIndex != -1)
			{
				pev->sequence = sequenceIndex;
				pev->frame = 0;
				ResetSequenceInfo();
				AILogger->debug("BM: Sequence {}", STRING(GetNodeSequence()));
				return;
			}
		}
		TaskComplete();
	}
	break;

	case TASK_NODE_YAW:
		pev->ideal_yaw = GetNodeYaw();
		TaskComplete();
		break;

	case TASK_WAIT_NODE:
		m_flWait = gpGlobals->time + GetNodeDelay();
		if ((m_hTargetEnt->pev->spawnflags & SF_INFOBM_WAIT) != 0)
			AILogger->debug("BM: Wait at node {} forever", STRING(pev->netname));
		else
			AILogger->debug("BM: Wait at node {} for {:.2f}", STRING(pev->netname), GetNodeDelay());
		break;


	case TASK_MOVE_TO_NODE_RANGE:
	{
		CBaseEntity* pTarget = m_hTargetEnt;
		if (!pTarget)
			TaskFail();
		else
		{
			if ((pTarget->pev->origin - pev->origin).Length() < GetNodeRange())
				TaskComplete();
			else
			{
				Activity act = ACT_WALK;
				if ((pTarget->pev->spawnflags & SF_INFOBM_RUN) != 0)
					act = ACT_RUN;

				m_vecMoveGoal = pTarget->pev->origin;
				if (!MoveToTarget(act, 2))
				{
					TaskFail();
				}
			}
		}
	}
		AILogger->debug("BM: Moving to node {}", STRING(pev->netname));

		break;

	case TASK_MELEE_ATTACK1:
		// Play an attack sound here
		EmitSound(CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_NORM);
		CBaseMonster::StartTask(pTask);
		break;

	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

void CBigMomma::RunTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_MOVE_TO_NODE_RANGE:
	{
		float distance;

		if (m_hTargetEnt == nullptr)
			TaskFail();
		else
		{
			distance = (m_vecMoveGoal - pev->origin).Length2D();
			// Set the appropriate activity based on an overlapping range
			// overlap the range to prevent oscillation
			if ((distance < GetNodeRange()) || MovementIsComplete())
			{
				AILogger->debug("BM: Reached node!");
				TaskComplete();
				RouteClear(); // Stop moving
			}
		}
	}

	break;

	case TASK_WAIT_NODE:
		if (m_hTargetEnt != nullptr && (m_hTargetEnt->pev->spawnflags & SF_INFOBM_WAIT) != 0)
			return;

		if (gpGlobals->time > m_flWaitFinished)
			TaskComplete();
		AILogger->debug("BM: The WAIT is over!");
		break;

	case TASK_PLAY_NODE_PRESEQUENCE:
	case TASK_PLAY_NODE_SEQUENCE:
		if (m_fSequenceFinished)
		{
			m_Activity = ACT_RESET;
			TaskComplete();
		}
		break;

	default:
		CBaseMonster::RunTask(pTask);
		break;
	}
}

Vector VecCheckSplatToss(CBaseEntity* entity, const Vector& vecSpot1, Vector vecSpot2, float maxHeight)
{
	TraceResult tr;
	Vector vecMidPoint; // halfway point between Spot1 and Spot2
	Vector vecApex;		// highest point
	Vector vecGrenadeVel;
	float flGravity = g_psv_gravity->value;

	// calculate the midpoint and apex of the 'triangle'
	vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	UTIL_TraceLine(vecMidPoint, vecMidPoint + Vector(0, 0, maxHeight), ignore_monsters, entity->edict(), &tr);
	vecApex = tr.vecEndPos;

	UTIL_TraceLine(vecSpot1, vecApex, dont_ignore_monsters, entity->edict(), &tr);
	if (tr.flFraction != 1.0)
	{
		// fail!
		return g_vecZero;
	}

	// Don't worry about actually hitting the target, this won't hurt us!

	// How high should the grenade travel (subtract 15 so the grenade doesn't hit the ceiling)?
	float height = (vecApex.z - vecSpot1.z) - 15;
	// How fast does the grenade need to travel to reach that height given gravity?
	float speed = sqrt(2 * flGravity * height);

	// How much time does it take to get there?
	float time = speed / flGravity;
	vecGrenadeVel = (vecSpot2 - vecSpot1);
	vecGrenadeVel.z = 0;

	// Travel half the distance to the target in that time (apex is at the midpoint)
	vecGrenadeVel = vecGrenadeVel * (0.5 / time);
	// Speed to offset gravity at the desired height
	vecGrenadeVel.z = speed;

	return vecGrenadeVel;
}

void MortarSpray(const Vector& position, const Vector& direction, int spriteModel, int count)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, position);
	WRITE_BYTE(TE_SPRITE_SPRAY);
	WRITE_COORD(position.x); // pos
	WRITE_COORD(position.y);
	WRITE_COORD(position.z);
	WRITE_COORD(direction.x); // dir
	WRITE_COORD(direction.y);
	WRITE_COORD(direction.z);
	WRITE_SHORT(spriteModel); // model
	WRITE_BYTE(count);		  // count
	WRITE_BYTE(130);		  // speed
	WRITE_BYTE(80);			  // noise ( client will divide by 100 )
	MESSAGE_END();
}

// UNDONE: right now this is pretty much a copy of the squid spit with minor changes to the way it does damage
void CBMortar::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SetModel("sprites/mommaspit.spr");
	pev->frame = 0;
	pev->scale = 0.5;

	SetSize(Vector(0, 0, 0), Vector(0, 0, 0));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
	pev->dmgtime = gpGlobals->time + 0.4;
}

void CBMortar::Animate()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (gpGlobals->time > pev->dmgtime)
	{
		pev->dmgtime = gpGlobals->time + 0.2;
		MortarSpray(pev->origin, -pev->velocity.Normalize(), gSpitSprite, 3);
	}
	if (0 != pev->frame++)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = 0;
		}
	}
}

CBMortar* CBMortar::Shoot(CBaseEntity* owner, Vector vecStart, Vector vecVelocity)
{
	CBMortar* pSpit = g_EntityDictionary->Create<CBMortar>("bmortar");

	UTIL_InitializeKeyValues( static_cast<CBaseEntity*>( pSpit ), owner->m_SharedKey, owner->m_SharedValue, owner->m_SharedKeyValues );

	pSpit->Spawn();

	pSpit->SetOrigin(vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->SetOwner(owner);
	pSpit->pev->scale = 2.5;
	pSpit->SetThink(&CBMortar::Animate);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;

	return pSpit;
}

void CBMortar::Touch(CBaseEntity* pOther)
{
	TraceResult tr;
	int iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT(90, 110);

	EmitSoundDyn(CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch);

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EmitSoundDyn(CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EmitSoundDyn(CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}

	if (pOther->IsBSPModel())
	{

		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, edict(), &tr);
		UTIL_DecalTrace(&tr, DECAL_MOMMASPLAT);
	}
	else
	{
		tr.vecEndPos = pev->origin;
		tr.vecPlaneNormal = -1 * pev->velocity.Normalize();
	}
	// make some flecks
	MortarSpray(tr.vecEndPos, tr.vecPlaneNormal, gSpitSprite, 24);

	auto owner = GetOwner();

	RadiusDamage(pev->origin, this, owner, GetSkillFloat("bigmomma_dmg_blast"sv), GetSkillFloat("bigmomma_radius_blast"sv), DMG_ACID);
	UTIL_Remove(this);
}
