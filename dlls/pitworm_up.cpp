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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "effects.h"
#include "soundent.h"
#include "weapons.h"
#include "decals.h"
#include "animation.h"

const auto PITWORM_UP_AE_HITGROUND = 1;
const auto PITWORM_UP_AE_SHOOTBEAM = 2;
const auto PITWORM_UP_AE_LOCKYAW = 4;

enum PITWORMUP_ANIM
{
	PITWORM_ANIM_IdleLong = 0,
	PITWORM_ANIM_IdleShort,
	PITWORM_ANIM_Level2AttackCenter,
	PITWORM_ANIM_Scream,
	PITWORM_ANIM_Level1AttackCenter,
	PITWORM_ANIM_Level2AttackLeft,
	PITWORM_ANIM_Level1AttackRight,
	PITWORM_ANIM_Level1AttackLeft,
	PITWORM_ANIM_RangeAttack,
	PITWORM_ANIM_Level3Attack1,
	PITWORM_ANIM_Level3Attack2,
	PITWORM_ANIM_Flinch1,
	PITWORM_ANIM_Flinch2,
	PITWORM_ANIM_Death,
};

const auto PITWORM_UP_NUM_LEVELS = 4;
const auto PITWORM_UP_EYE_HEIGHT = 300.0f;

const float PITWORM_UP_LEVELS[PITWORM_UP_NUM_LEVELS] =
	{
		PITWORM_UP_EYE_HEIGHT,
		PITWORM_UP_EYE_HEIGHT,
		PITWORM_UP_EYE_HEIGHT,
		PITWORM_UP_EYE_HEIGHT + 50};

const char* const PITWORM_UP_LEVEL_NAMES[PITWORM_UP_NUM_LEVELS] =
	{
		"pw_tleveldead",
		"pw_tlevel1",
		"pw_tlevel2",
		"pw_tlevel3"};

class COFPitWormUp : public CBaseMonster
{
public:
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void Precache() override;

	void Spawn() override;

	int BloodColor() override { return BLOOD_COLOR_GREEN; }

	int Classify() override { return CLASS_ALIEN_MILITARY; }

	int ObjectCaps() override { return 0; }

	//Don't gib ever
	void GibMonster() override {}

	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector(-400, -400, 0);
		pev->absmax = pev->origin + Vector(400, 400, 850);
	}

	void IdleSound() override;

	void PainSound() override;

	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	bool FVisible(CBaseEntity* pEntity) override;

	bool FVisible(const Vector& vecOrigin) override;

	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void EXPORT StartupThink();

	void EXPORT HuntThink();

	void EXPORT DyingThink();

	void EXPORT NullThink();

	void EXPORT HitTouch(CBaseEntity* pOther);

	void EXPORT CommandUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void EXPORT StartupUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void ChangeLevel();

	void ShootBeam();

	void StrafeBeam();

	void TrackEnemy();

	void NextActivity();

	bool ClawAttack();

	void LockTopLevel();

	static const char* pAttackSounds[];
	static const char* pAttackVoiceSounds[];
	static const char* pShootSounds[];
	static const char* pPainSounds[];
	static const char* pHitGroundSounds[];
	static const char* pIdleSounds[];

	float m_flNextPainSound;

	Vector m_vecTarget;
	Vector m_posTarget;
	Vector m_vecDesired;
	Vector m_posDesired;

	float m_offsetBeam;
	Vector m_posBeam;
	Vector m_vecBeam;
	Vector m_angleBeam;

	float m_flBeamExpireTime;
	float m_flBeamDir;

	float m_flTorsoYaw;
	float m_flHeadYaw;
	float m_flHeadPitch;
	float m_flIdealTorsoYaw;
	float m_flIdealHeadYaw;
	float m_flIdealHeadPitch;

	float m_flLevels[PITWORM_UP_NUM_LEVELS];
	float m_flTargetLevels[PITWORM_UP_NUM_LEVELS];

	float m_flLastSeen;
	float m_flPrevSeen;

	int m_iLevel;
	float m_flLevelSpeed;

	CBeam* m_pBeam;
	CSprite* m_pSprite;

	bool m_fAttacking;
	bool m_fLockHeight;
	bool m_fLockYaw;

	int m_iWasHit;
	float m_flTakeHitTime;
	float m_flHitTime;

	float m_flNextMeleeTime;
	float m_flNextRangeTime;
	float m_flDeathStartTime;

	bool m_fFirstSighting;
	bool m_fTopLevelLocked;

	float m_flLastBlinkTime;
	float m_flLastBlinkInterval;
	float m_flLastEventTime;
};

LINK_ENTITY_TO_CLASS(monster_pitworm_up, COFPitWormUp);

TYPEDESCRIPTION COFPitWormUp::m_SaveData[] =
	{
		DEFINE_FIELD(COFPitWormUp, m_flNextPainSound, FIELD_TIME),
		DEFINE_FIELD(COFPitWormUp, m_vecTarget, FIELD_VECTOR),
		DEFINE_FIELD(COFPitWormUp, m_posTarget, FIELD_POSITION_VECTOR),
		DEFINE_FIELD(COFPitWormUp, m_vecDesired, FIELD_VECTOR),
		DEFINE_FIELD(COFPitWormUp, m_posDesired, FIELD_POSITION_VECTOR),

		DEFINE_FIELD(COFPitWormUp, m_offsetBeam, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormUp, m_posBeam, FIELD_POSITION_VECTOR),
		DEFINE_FIELD(COFPitWormUp, m_vecBeam, FIELD_VECTOR),
		DEFINE_FIELD(COFPitWormUp, m_angleBeam, FIELD_VECTOR),
		DEFINE_FIELD(COFPitWormUp, m_flBeamExpireTime, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormUp, m_flBeamDir, FIELD_FLOAT),

		DEFINE_FIELD(COFPitWormUp, m_flTorsoYaw, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormUp, m_flHeadYaw, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormUp, m_flHeadPitch, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormUp, m_flIdealTorsoYaw, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormUp, m_flIdealHeadYaw, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormUp, m_flIdealHeadPitch, FIELD_FLOAT),

		DEFINE_ARRAY(COFPitWormUp, m_flLevels, FIELD_FLOAT, PITWORM_UP_NUM_LEVELS),
		DEFINE_ARRAY(COFPitWormUp, m_flTargetLevels, FIELD_FLOAT, PITWORM_UP_NUM_LEVELS),

		DEFINE_FIELD(COFPitWormUp, m_flLastSeen, FIELD_TIME),
		DEFINE_FIELD(COFPitWormUp, m_flPrevSeen, FIELD_TIME),

		DEFINE_FIELD(COFPitWormUp, m_iLevel, FIELD_INTEGER),
		DEFINE_FIELD(COFPitWormUp, m_flLevelSpeed, FIELD_FLOAT),

		DEFINE_FIELD(COFPitWormUp, m_pBeam, FIELD_CLASSPTR),
		DEFINE_FIELD(COFPitWormUp, m_pSprite, FIELD_CLASSPTR),

		DEFINE_FIELD(COFPitWormUp, m_fAttacking, FIELD_BOOLEAN),
		DEFINE_FIELD(COFPitWormUp, m_fLockHeight, FIELD_BOOLEAN),
		DEFINE_FIELD(COFPitWormUp, m_fLockYaw, FIELD_BOOLEAN),

		DEFINE_FIELD(COFPitWormUp, m_iWasHit, FIELD_INTEGER),

		DEFINE_FIELD(COFPitWormUp, m_flTakeHitTime, FIELD_TIME),
		DEFINE_FIELD(COFPitWormUp, m_flHitTime, FIELD_TIME),
		DEFINE_FIELD(COFPitWormUp, m_flNextMeleeTime, FIELD_TIME),
		DEFINE_FIELD(COFPitWormUp, m_flNextRangeTime, FIELD_TIME),
		DEFINE_FIELD(COFPitWormUp, m_flDeathStartTime, FIELD_TIME),

		DEFINE_FIELD(COFPitWormUp, m_fFirstSighting, FIELD_BOOLEAN),

		DEFINE_FIELD(COFPitWormUp, m_fTopLevelLocked, FIELD_BOOLEAN),

		DEFINE_FIELD(COFPitWormUp, m_flLastBlinkTime, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormUp, m_flLastBlinkInterval, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormUp, m_flLastEventTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(COFPitWormUp, CBaseMonster);

const char* COFPitWormUp::pAttackSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav"};

const char* COFPitWormUp::pAttackVoiceSounds[] =
	{
		"pitworm/pit_worm_attack_swipe1.wav",
		"pitworm/pit_worm_attack_swipe2.wav",
		"pitworm/pit_worm_attack_swipe3.wav"};

const char* COFPitWormUp::pShootSounds[] =
	{
		"debris/beamstart3.wav",
		"debris/beamstart8.wav"};

const char* COFPitWormUp::pPainSounds[] =
	{
		"pitworm/pit_worm_flinch1.wav",
		"pitworm/pit_worm_flinch2.wav"};

const char* COFPitWormUp::pHitGroundSounds[] =
	{
		"tentacle/te_strike1.wav",
		"tentacle/te_strike2.wav"};

const char* COFPitWormUp::pIdleSounds[] =
	{
		"pitworm/pit_worm_idle1.wav",
		"pitworm/pit_worm_idle2.wav",
		"pitworm/pit_worm_idle3.wav"};

void COFPitWormUp::Precache()
{
	PRECACHE_MODEL("models/pit_worm_up.mdl");
	PRECACHE_MODEL("sprites/tele1.spr");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAttackVoiceSounds);
	PRECACHE_SOUND_ARRAY(pShootSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pHitGroundSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);

	PRECACHE_SOUND("debris/beamstart7.wav");

	PRECACHE_SOUND("pitworm/clang1.wav");
	PRECACHE_SOUND("pitworm/clang2.wav");
	PRECACHE_SOUND("pitworm/clang3.wav");

	PRECACHE_SOUND("pitworm/pit_worm_alert.wav");

	PRECACHE_SOUND("pitworm/pit_worm_attack_eyeblast.wav");
	PRECACHE_SOUND("pitworm/pit_worm_attack_eyeblast_impact.wav");

	PRECACHE_SOUND("pitworm/pit_worm_attack_swipe1.wav");
	PRECACHE_SOUND("pitworm/pit_worm_attack_swipe2.wav");
	PRECACHE_SOUND("pitworm/pit_worm_attack_swipe3.wav");

	PRECACHE_SOUND("pitworm/pit_worm_death.wav");

	PRECACHE_SOUND("pitworm/pit_worm_flinch1.wav");
	PRECACHE_SOUND("pitworm/pit_worm_flinch2.wav");

	PRECACHE_MODEL("models/pit_worm_gibs.mdl");

	UTIL_PrecacheOther("pitworm_gib");
}

void COFPitWormUp::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(edict(), "models/pit_worm_up.mdl");

	UTIL_SetSize(pev, {-32, -32, 0}, {32, 32, 64});

	UTIL_SetOrigin(pev, pev->origin);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;

	pev->max_health = pev->health = gSkillData.pitWormHealth;

	pev->view_ofs = {0, 0, PITWORM_UP_EYE_HEIGHT};

	m_bloodColor = BLOOD_COLOR_GREEN;
	m_flFieldOfView = 0.5;

	pev->sequence = 0;

	// Force interpolation on.
	m_EFlags |= EFLAG_SLERP;

	ResetSequenceInfo();

	m_flTorsoYaw = 0;
	m_flHeadYaw = 0;
	m_flHeadPitch = 0;
	m_flIdealTorsoYaw = 0;
	m_flIdealHeadYaw = 0;
	m_flIdealHeadPitch = 0;

	InitBoneControllers();

	SetThink(&COFPitWormUp::StartupThink);
	SetTouch(&COFPitWormUp::HitTouch);

	pev->nextthink = gpGlobals->time + 0.1;

	m_vecDesired = {1, 0, 0};

	m_posDesired = pev->origin;

	m_fAttacking = false;
	m_fLockHeight = false;
	m_fFirstSighting = false;

	m_flBeamExpireTime = gpGlobals->time;

	m_iLevel = 0;
	m_fLockYaw = false;
	m_iWasHit = 0;

	m_flTakeHitTime = 0;
	m_flHitTime = 0;
	m_flLevelSpeed = 10;

	m_fTopLevelLocked = false;
	m_flLastBlinkTime = gpGlobals->time;
	m_flLastBlinkInterval = gpGlobals->time;
	m_flLastEventTime = gpGlobals->time;

	for (auto i = 0; i < PITWORM_UP_NUM_LEVELS; ++i)
	{
		m_flLevels[i] = pev->origin.z - PITWORM_UP_LEVELS[i];
	}

	for (auto& level : m_flTargetLevels)
	{
		level = pev->origin.z;
	}

	m_pBeam = nullptr;
}

void COFPitWormUp::StartupThink()
{
	for (auto i = 0; i < PITWORM_UP_NUM_LEVELS; ++i)
	{
		auto pTarget = UTIL_FindEntityByTargetname(nullptr, PITWORM_UP_LEVEL_NAMES[i]);

		if (pTarget)
		{
			ALERT(at_console, "level %d node set\n", i);
			m_flTargetLevels[i] = pTarget->pev->origin.z;
			m_flLevels[i] = pTarget->pev->origin.z - PITWORM_UP_LEVELS[i];
		}
	}

	if (!FStringNull(pev->target))
	{
		auto pszTarget = STRING(pev->target);

		if (!m_fTopLevelLocked && !stricmp("pw_level3", pszTarget))
		{
			m_iLevel = 3;
		}
		else if (!stricmp("pw_level2", pszTarget))
		{
			m_iLevel = 2;
		}
		else if (!stricmp("pw_level1", pszTarget))
		{
			m_iLevel = 1;
		}
		else if (!stricmp("pw_leveldead", pszTarget))
		{
			m_iLevel = 0;
		}

		m_posDesired.z = m_flLevels[m_iLevel];
	}

	Vector vecEyePos, vecEyeAng;

	GetAttachment(0, vecEyePos, vecEyeAng);

	pev->view_ofs = vecEyePos - pev->origin;

	m_flNextMeleeTime = gpGlobals->time;

	SetThink(&COFPitWormUp::HuntThink);
	SetUse(&COFPitWormUp::CommandUse);

	m_flNextRangeTime = gpGlobals->time;

	pev->nextthink = gpGlobals->time + 0.1;
}

void COFPitWormUp::HuntThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	DispatchAnimEvents();
	StudioFrameAdvance();

	UpdateShockEffect();

	if (m_pBeam)
	{
		if (m_hEnemy && m_flBeamExpireTime > gpGlobals->time)
		{
			StrafeBeam();
		}
		else
		{
			UTIL_Remove(m_pBeam);
			UTIL_Remove(m_pSprite);
			m_pBeam = nullptr;
			m_pSprite = nullptr;
		}
	}

	if (pev->health <= 0)
	{
		SetThink(&COFPitWormUp::DyingThink);
		m_fSequenceFinished = true;
	}
	else
	{
		const auto blinkInterval = gpGlobals->time - m_flLastBlinkTime;

		if (blinkInterval >= 6.0 && !m_pBeam && blinkInterval >= g_engfuncs.pfnRandomFloat(6.0, 9.0))
		{
			pev->skin = 1;
			m_flLastBlinkInterval = gpGlobals->time;
			m_flLastBlinkTime = gpGlobals->time;
		}

		if (pev->skin > 0 && gpGlobals->time - m_flLastBlinkInterval >= 0)
		{
			if (pev->skin == 5)
				pev->skin = 0;
			else
				pev->skin = pev->skin + 1;

			m_flLastBlinkInterval = gpGlobals->time;
		}

		if (m_iWasHit == 1)
		{
			int iDir = 1;
			pev->sequence = FindTransition(pev->sequence, PITWORM_ANIM_Flinch1 + RANDOM_LONG(0, 1), &iDir);

			if (iDir <= 0)
				pev->frame = 255;
			else
				pev->frame = 0;

			ResetSequenceInfo();

			m_iWasHit = 0;

			PainSound();
		}
		else if (m_fSequenceFinished)
		{
			const auto oldSequence = pev->sequence;

			if (m_fAttacking)
			{
				m_fLockHeight = false;
				m_fLockYaw = false;
				m_fAttacking = false;
				m_flNextMeleeTime = gpGlobals->time + 0.25;
			}

			NextActivity();

			if (!m_fSequenceLoops || pev->sequence != oldSequence)
			{
				pev->frame = 0;
				ResetSequenceInfo();
			}
		}
		if (m_hEnemy)
		{
			if (FVisible(m_hEnemy))
			{
				if (gpGlobals->time - 5.0 > m_flLastSeen)
				{
					m_flPrevSeen = gpGlobals->time;
				}

				m_flLastSeen = gpGlobals->time;

				m_posTarget = m_hEnemy->pev->origin;

				m_posTarget.z += 24;

				Vector vecEyePos, vecEyeAng;
				GetAttachment(0, vecEyePos, vecEyeAng);

				m_vecTarget = (m_posTarget - vecEyePos).Normalize();

				m_vecDesired = m_vecTarget;
			}
		}

		if (m_posDesired.z > m_flLevels[3])
			m_posDesired.z = m_flLevels[3];

		if (m_flLevels[0] > m_posDesired.z)
			m_posDesired.z = m_flLevels[0];

		ChangeLevel();

		if (m_hEnemy && !m_pBeam)
		{
			TrackEnemy();
		}
	}
}

void COFPitWormUp::DyingThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	DispatchAnimEvents();
	StudioFrameAdvance();

	if (pev->deadflag != DEAD_NO)
	{
		if (pev->deadflag == DEAD_DYING)
		{
			if (gpGlobals->time - m_flDeathStartTime > 3.0)
			{
				ChangeLevel();
			}

			if (fabs(pev->origin.z - m_flLevels[0]) < 16.0)
			{
				pev->velocity = g_vecZero;
				pev->deadflag = DEAD_DEAD;

				SetThink(&COFPitWormUp::SUB_Remove);
				pev->nextthink = gpGlobals->time + 0.1;
			}
		}
	}
	else
	{
		pev->deadflag = DEAD_DYING;

		m_posDesired.z = m_flLevels[0];

		int iDir = 1;
		pev->sequence = FindTransition(pev->sequence, PITWORM_ANIM_Death, &iDir);

		if (iDir <= 0)
			pev->frame = 255;
		else
			pev->frame = 0;

		ResetSequenceInfo();

		m_flLevelSpeed = 5;

		ClearShockEffect();

		EMIT_SOUND(edict(), CHAN_VOICE, "pitworm/pit_worm_death.wav", VOL_NORM, 0.1);

		m_flDeathStartTime = gpGlobals->time;

		if (m_pBeam)
		{
			UTIL_Remove(m_pBeam);
			m_pBeam = nullptr;
		}

		if (m_pSprite)
		{
			UTIL_Remove(m_pSprite);
			m_pSprite = nullptr;
		}

		SetTouch(nullptr);
		SetUse(nullptr);
	}
}

void COFPitWormUp::NullThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.5;
}

void COFPitWormUp::HitTouch(CBaseEntity* pOther)
{
	auto tr = UTIL_GetGlobalTrace();

	if (pOther->pev->modelindex != pev->modelindex && m_flHitTime <= gpGlobals->time && tr.pHit && pev->modelindex == tr.pHit->v.modelindex && pOther->pev->takedamage != DAMAGE_NO)
	{
		pOther->TakeDamage(pev, pev, 20, DMG_CRUSH | DMG_SLASH);

		pOther->pev->punchangle.z = 15;

		//TODO: maybe determine direction of velocity to apply?
		pOther->pev->velocity = pOther->pev->velocity + Vector{0, 0, 200};

		pOther->pev->flags &= ~FL_ONGROUND;


		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pAttackSounds[RANDOM_LONG(0, 2)], VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(-5, 5) + 100);

		if (tr.iHitgroup == 2)
			pOther->TakeDamage(pev, pev, 10, DMG_CRUSH | DMG_SLASH);

		m_flHitTime = gpGlobals->time + 1.0;
	}
}

void COFPitWormUp::CommandUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	switch (useType)
	{
	case USE_ON:
		if (pActivator)
			CSoundEnt::InsertSound(bits_SOUND_WORLD, pActivator->pev->origin, 1024, 1.0);
		break;
	case USE_TOGGLE:
		pev->takedamage = DAMAGE_NO;
		SetThink(&COFPitWormUp::DyingThink);
		break;
	case USE_OFF:
		pev->takedamage = DAMAGE_NO;
		SetThink(&COFPitWormUp::DyingThink);
		break;
	}
}

void COFPitWormUp::StartupUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&COFPitWormUp::HuntThink);
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse(&COFPitWormUp::CommandUse);
}

void COFPitWormUp::ChangeLevel()
{
	if (pev->origin.z != m_posDesired.z)
	{
		if (m_posDesired.z <= pev->origin.z)
		{
			pev->origin.z -= V_min(m_flLevelSpeed, pev->origin.z - m_posDesired.z);
		}
		else
		{
			pev->origin.z += V_min(m_flLevelSpeed, m_posDesired.z - pev->origin.z);
		}
	}

	if (m_flTorsoYaw != m_flIdealTorsoYaw)
	{
		if (m_flIdealTorsoYaw <= m_flTorsoYaw)
		{
			m_flTorsoYaw -= V_min(5, m_flTorsoYaw - m_flIdealTorsoYaw);
		}
		else
		{
			m_flTorsoYaw += V_min(5, m_flIdealTorsoYaw - m_flTorsoYaw);
		}

		SetBoneController(2, m_flTorsoYaw);
	}

	if (m_flHeadYaw != m_flIdealHeadYaw)
	{
		if (m_flIdealHeadYaw <= m_flHeadYaw)
		{
			m_flHeadYaw -= V_min(5, m_flHeadYaw - m_flIdealHeadYaw);
		}
		else
		{
			m_flHeadYaw += V_min(5, m_flIdealHeadYaw - m_flHeadYaw);
		}

		SetBoneController(0, -m_flHeadYaw);
	}

	if (m_flHeadPitch != m_flIdealHeadPitch)
	{
		if (m_flIdealHeadPitch <= m_flHeadPitch)
		{
			m_flHeadPitch -= V_min(5, m_flHeadPitch - m_flIdealHeadPitch);
		}
		else
		{
			m_flHeadPitch += V_min(5, m_flIdealHeadPitch - m_flHeadPitch);
		}

		SetBoneController(1, m_flHeadPitch);
	}
}

void COFPitWormUp::ShootBeam()
{
	if (m_hEnemy)
	{
		if (m_flHeadYaw > 0)
		{
			m_flBeamDir = -1;
		}
		else
		{
			m_flBeamDir = 1;
		}

		m_offsetBeam = -m_flBeamDir * 80;

		Vector vecEyePos, vecEyeAng;
		GetAttachment(0, vecEyePos, vecEyeAng);

		m_vecBeam = (m_posBeam - vecEyePos).Normalize();

		m_angleBeam = UTIL_VecToAngles(m_vecBeam);

		UTIL_MakeVectors(m_angleBeam);

		m_vecBeam = gpGlobals->v_forward;

		m_vecBeam.z = -m_vecBeam.z;

		TraceResult tr;
		UTIL_TraceLine(vecEyePos, vecEyePos + m_offsetBeam * gpGlobals->v_right + 1280 * m_vecBeam, dont_ignore_monsters, edict(), &tr);

		m_pBeam = CBeam::BeamCreate("sprites/laserbeam.spr", 80);

		if (m_pBeam)
		{
			m_pBeam->PointEntInit(tr.vecEndPos, entindex());
			m_pBeam->SetEndAttachment(1);
			m_pBeam->SetColor(0, 255, 32);
			m_pBeam->SetBrightness(128);
			m_pBeam->SetWidth(32);
			m_pBeam->pev->spawnflags |= SF_BEAM_SPARKSTART;

			auto pHit = Instance(tr.pHit);

			if (pHit && pHit->pev->takedamage != DAMAGE_NO)
			{
				ClearMultiDamage();
				pHit->TraceAttack(pev, gSkillData.pitWormDmgBeam, m_vecBeam, &tr, 1024);
				pHit->TakeDamage(pev, pev, gSkillData.pitWormDmgBeam, 1024);
			}
			else if (tr.flFraction != 1.0)
			{
				UTIL_DecalTrace(&tr, DECAL_GUNSHOT1 + RANDOM_LONG(0, 4));
				m_pBeam->DoSparks(tr.vecEndPos, tr.vecEndPos);
			}

			m_pBeam->DoSparks(vecEyePos, vecEyePos);

			m_flBeamExpireTime = gpGlobals->time + 0.9;

			const auto yaw = m_flHeadYaw - m_flBeamDir * 25.0;

			if (-45.0 <= yaw && yaw <= 45.0)
			{
				m_flHeadYaw = yaw;
			}

			m_flIdealHeadYaw += m_flBeamDir * 50.0;

			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pShootSounds[RANDOM_LONG(0, ARRAYSIZE(pShootSounds) - 1)], VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(-5, 5) + 100);
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_ELIGHT);
			WRITE_SHORT(entindex() | (1 << 12));
			WRITE_COORD(vecEyePos.x);
			WRITE_COORD(vecEyePos.y);
			WRITE_COORD(vecEyePos.z);
			WRITE_COORD(128.0);
			WRITE_BYTE(128);
			WRITE_BYTE(255);
			WRITE_BYTE(128);
			WRITE_BYTE(1);
			WRITE_COORD(2.0);
			MESSAGE_END();

			m_pSprite = CSprite::SpriteCreate("sprites/tele1.spr", vecEyePos, true);

			if (m_pSprite)
			{
				m_pSprite->SetTransparency(kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation);
				m_pSprite->SetColor(0, 255, 0);
				m_pSprite->SetAttachment(edict(), 1);
				m_pSprite->SetScale(0.75);
				m_pSprite->pev->framerate = 10;

				m_pSprite->TurnOn();
			}
		}
	}
}

void COFPitWormUp::StrafeBeam()
{
	m_offsetBeam += 20 * m_flBeamDir;

	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);

	m_vecBeam = (m_posBeam - vecEyePos).Normalize();

	m_angleBeam = UTIL_VecToAngles(m_vecBeam);

	UTIL_MakeVectors(m_angleBeam);

	m_vecBeam = gpGlobals->v_forward;

	m_vecBeam.z = -m_vecBeam.z;

	TraceResult tr;
	UTIL_TraceLine(vecEyePos, vecEyePos + gpGlobals->v_right * m_offsetBeam + m_vecBeam * 1280, dont_ignore_monsters, edict(), &tr);
	m_pBeam->DoSparks(vecEyePos, vecEyePos);

	m_pBeam->pev->origin = tr.vecEndPos;

	auto pHit = Instance(tr.pHit);

	if (pHit && pHit->pev->takedamage != DAMAGE_NO)
	{
		ClearMultiDamage();

		pHit->TraceAttack(pev, gSkillData.pitWormDmgBeam, m_vecBeam, &tr, DMG_ENERGYBEAM);
		pHit->TakeDamage(pev, pev, gSkillData.pitWormDmgBeam, DMG_ENERGYBEAM);

		//TODO: missing an ApplyMultiDamage call here
		//Should probably replace the TakeDamage call
		//ApplyMultiDamage( pev, pev );
	}
	else if (tr.flFraction != 1.0)
	{
		UTIL_DecalTrace(&tr, DECAL_GUNSHOT1 + g_engfuncs.pfnRandomLong(0, 4));
		m_pBeam->DoSparks(tr.vecEndPos, tr.vecEndPos);
	}

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_ELIGHT);
	WRITE_SHORT(entindex() | (1 << 12));
	WRITE_COORD(vecEyePos.x);
	WRITE_COORD(vecEyePos.y);
	WRITE_COORD(vecEyePos.z);
	WRITE_COORD(128);
	WRITE_BYTE(128);
	WRITE_BYTE(255);
	WRITE_BYTE(128);
	WRITE_BYTE(1);
	WRITE_COORD(2);
	MESSAGE_END();
}

void COFPitWormUp::TrackEnemy()
{
	auto pEnemy = m_hEnemy.Entity<CBaseEntity>();

	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);

	vecEyePos.x = pev->origin.x;
	vecEyePos.y = pev->origin.y;

	const auto vecDir = UTIL_VecToAngles(pEnemy->pev->origin + pEnemy->pev->view_ofs - vecEyePos);

	m_flIdealHeadPitch = V_min(45, V_max(-45, UTIL_AngleDiff(vecDir.x, pev->angles.x)));

	const auto yaw = UTIL_AngleDiff(VecToYaw(pEnemy->pev->origin + pEnemy->pev->view_ofs - vecEyePos), pev->angles.y);

	if (!m_fLockYaw)
	{
		if (yaw < 0)
		{
			m_flIdealTorsoYaw = V_max(yaw, m_iLevel == 1 ? -30 : -50);
		}

		if (yaw > 0)
		{
			m_flIdealTorsoYaw = V_min(yaw, m_iLevel == 2 ? 30 : 50);
		}
	}

	const auto headYaw = V_max(-45, V_min(45, m_flTorsoYaw - yaw));

	if (!m_fAttacking || m_pBeam)
		m_flIdealHeadYaw = headYaw;

	if (!m_fLockHeight)
	{
		m_iLevel = 0;

		for (auto i = m_fTopLevelLocked ? 2 : 3; i > 0; --i)
		{
			if (pEnemy->pev->origin.z > m_flTargetLevels[i])
			{
				m_iLevel = i;
				break;
			}
		}

		m_posDesired.z = m_flLevels[m_iLevel];
	}
}

void COFPitWormUp::NextActivity()
{
	UTIL_MakeAimVectors(pev->angles);

	//TODO: never used?
	const auto moveDistance = m_posDesired - pev->origin;

	if (m_hEnemy)
	{
		if (!m_hEnemy->IsAlive())
		{
			m_hEnemy = nullptr;
			m_flIdealHeadYaw = 0;
		}
	}

	if (gpGlobals->time > m_flLastSeen + 15.0)
	{
		if (m_hEnemy)
		{
			if ((pev->origin - m_hEnemy->pev->origin).Length2D() > 700.0)
				m_hEnemy = nullptr;
		}
	}

	if (!m_hEnemy)
	{
		Look(4096);
		m_hEnemy = BestVisibleEnemy();

		if (m_hEnemy)
		{
			EMIT_SOUND(edict(), CHAN_VOICE, "pitworm/pit_worm_alert.wav", VOL_NORM, 0.1);
		}
	}

	if (!m_hEnemy || m_fFirstSighting)
	{
		if (m_iWasHit == 1)
		{
			pev->sequence = PITWORM_ANIM_Flinch1 + RANDOM_LONG(0, 1);
			m_iWasHit = 0;

			PainSound();

			m_fLockHeight = false;
			m_fLockYaw = false;
			m_fAttacking = false;
		}
		else if (pev->origin.z == m_posDesired.z)
		{
			if (abs(static_cast<int>(m_flIdealTorsoYaw - m_flTorsoYaw)) > 10 || !ClawAttack())
			{
				if (!RANDOM_LONG(0, 2))
					IdleSound();

				pev->sequence = PITWORM_ANIM_IdleShort;

				m_fLockHeight = false;
				m_fLockYaw = false;
				m_fAttacking = false;
			}
		}
		else
		{
			if (!RANDOM_LONG(0, 2))
				IdleSound();

			pev->sequence = PITWORM_ANIM_IdleLong;

			m_fLockHeight = false;
			m_fLockYaw = false;
			m_fAttacking = false;
		}
	}
	else
	{
		pev->sequence = PITWORM_ANIM_Scream;
		m_fFirstSighting = true;
	}
}

bool COFPitWormUp::ClawAttack()
{
	if (!m_hEnemy || pev->origin.z != m_posDesired.z || m_flNextMeleeTime > gpGlobals->time)
		return false;

	const auto distance = (pev->origin - m_hEnemy->pev->origin).Length2D();

	if (!FVisible(m_posTarget))
	{
		if (distance >= 600.0)
			return false;

		const auto direction = UTIL_VecToAngles((m_posTarget - pev->origin).Normalize());

		const auto yaw = UTIL_AngleDiff(direction.y, pev->angles.y);

		if (m_iLevel == 2)
		{
			if (yaw < 30.0)
				return false;

			pev->sequence = PITWORM_ANIM_Level2AttackLeft;
			m_flIdealHeadYaw = 0;
		}
		else if (m_iLevel == 1)
		{
			if (yaw <= -30.0)
			{
				pev->sequence = PITWORM_ANIM_Level1AttackRight;
				m_flIdealHeadYaw = 0;
			}
			else if (yaw >= 30.0)
			{
				pev->sequence = PITWORM_ANIM_Level1AttackLeft;
				m_flIdealHeadYaw = 0;
			}
			else
			{
				return false;
			}
		}

		EMIT_SOUND(edict(), CHAN_VOICE, pAttackVoiceSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackVoiceSounds) - 1)], VOL_NORM, 0.1);

		m_fLockHeight = true;
		m_fLockYaw = true;
		m_fAttacking = true;
		return true;
	}

	m_fLockYaw = false;

	if (m_iLevel == 2)
	{
		const auto direction = UTIL_VecToAngles((m_posTarget - pev->origin).Normalize());

		const auto yaw = UTIL_AngleDiff(direction.y, pev->angles.y);

		if (yaw < 30.0)
		{
			if (distance > 425.0 || yaw <= -50.0)
			{
				pev->sequence = PITWORM_ANIM_RangeAttack;
				m_posBeam = m_posTarget;
				m_vecBeam = m_vecTarget;
				m_angleBeam = UTIL_VecToAngles(m_vecBeam);
			}
			else
			{
				pev->sequence = PITWORM_ANIM_Level2AttackCenter;
			}
		}
		else
		{
			pev->sequence = PITWORM_ANIM_Level2AttackLeft;
			m_flIdealHeadYaw = 0;
			m_fLockYaw = true;
		}
	}
	else if (m_iLevel == 3)
	{
		if (distance <= 425.0)
		{
			pev->sequence = PITWORM_ANIM_Level3Attack1 + RANDOM_LONG(0, 1);
		}
		else
		{
			pev->sequence = PITWORM_ANIM_RangeAttack;
			m_posBeam = m_posTarget;
			m_vecBeam = m_vecTarget;
			m_angleBeam = UTIL_VecToAngles(m_vecBeam);
		}
	}
	else
	{
		if (m_iLevel != 1)
			return false;

		const auto direction = UTIL_VecToAngles((m_posTarget - pev->origin).Normalize());

		const auto yaw = UTIL_AngleDiff(direction.y, pev->angles.y);

		if (yaw < 50.0)
		{
			if (yaw > -30.0)
			{
				if (distance > 425.0)
				{
					pev->sequence = PITWORM_ANIM_RangeAttack;
					m_posBeam = m_posTarget;
					m_vecBeam = m_vecTarget;
					m_angleBeam = UTIL_VecToAngles(m_vecBeam);
				}
				else
				{
					pev->sequence = PITWORM_ANIM_Level1AttackCenter;
				}
			}
			else
			{
				pev->sequence = PITWORM_ANIM_Level1AttackRight;
			}
		}
		else
		{
			pev->sequence = PITWORM_ANIM_Level1AttackLeft;
			m_flIdealHeadYaw = 0;
			m_fLockYaw = true;
		}
	}

	if (pev->sequence == PITWORM_ANIM_RangeAttack)
	{
		EMIT_SOUND(edict(), CHAN_VOICE, "pitworm/pit_worm_attack_eyeblast.wav", VOL_NORM, 0.1);
	}
	else
	{
		EMIT_SOUND(edict(), CHAN_VOICE, pAttackVoiceSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackVoiceSounds) - 1)], VOL_NORM, 0.1);
	}

	m_fAttacking = true;
	m_fLockHeight = true;

	return true;
}

void COFPitWormUp::LockTopLevel()
{
	if (m_iLevel == 3)
	{
		pev->health = pev->max_health;
		m_iWasHit = 1;
		m_iLevel = 2;
		m_flTakeHitTime = RANDOM_LONG(2, 4) + gpGlobals->time;
		m_posDesired.z = m_flLevels[2];
	}

	m_fTopLevelLocked = true;
}

void COFPitWormUp::IdleSound()
{
	EMIT_SOUND(edict(), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], VOL_NORM, 0.1);
}

void COFPitWormUp::PainSound()
{
	if (m_flNextPainSound <= gpGlobals->time)
	{
		m_flNextPainSound = RANDOM_FLOAT(2, 5) + gpGlobals->time;

		EMIT_SOUND(edict(), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], VOL_NORM, 0.1);
	}
}

bool COFPitWormUp::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	PainSound();
	return false;
}

void COFPitWormUp::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (ptr->iHitgroup == 1)
	{
		if (gpGlobals->time > m_flTakeHitTime)
		{
			pev->health -= flDamage;

			if (pev->health <= 0)
			{
				pev->health = pev->max_health;
				m_iWasHit = 1;
				m_flTakeHitTime = RANDOM_LONG(2, 4) + gpGlobals->time;
			}

			if (m_bloodColor != DONT_BLEED)
			{
				SpawnBlood(ptr->vecEndPos - vecDir * 4, m_bloodColor, flDamage * 10.0);
				TraceBleed(flDamage, vecDir, ptr, bitsDamageType);

				if (pevAttacker && !m_hEnemy)
				{
					auto pAttacker = Instance(pevAttacker);

					if (pAttacker && pAttacker->MyMonsterPointer())
					{
						m_hEnemy = pAttacker;
						EMIT_SOUND(edict(), CHAN_VOICE, "pitworm/pit_worm_alert.wav", VOL_NORM, 0.1);

						if (!m_fFirstSighting)
						{
							pev->sequence = PITWORM_ANIM_Scream;
							m_fFirstSighting = true;
							return;
						}
					}
				}
			}

			if (0 == pev->skin)
			{
				pev->skin = 1;
				m_flLastBlinkInterval = gpGlobals->time;
				m_flLastBlinkTime = gpGlobals->time;
			}
		}
	}
	else if (pev->dmgtime != gpGlobals->time || RANDOM_LONG(0, 10) <= 0)
	{
		UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
		pev->dmgtime = gpGlobals->time;
	}
}

bool COFPitWormUp::FVisible(CBaseEntity* pEntity)
{
	if ((pEntity->pev->flags & FL_NOTARGET) == 0)
	{
		if ((pev->waterlevel != WATERLEVEL_HEAD && pEntity->pev->waterlevel != WATERLEVEL_HEAD) || 0 != pEntity->pev->waterlevel)
		{
			return FVisible(pEntity->EyePosition());
		}
	}

	return false;
}

bool COFPitWormUp::FVisible(const Vector& vecOrigin)
{
	Vector vecLookerOrigin, vecLookerAngle;
	GetAttachment(0, vecLookerOrigin, vecLookerAngle);

	TraceResult tr;
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, edict(), &tr);

	return tr.flFraction == 1.0;
}

void COFPitWormUp::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case PITWORM_UP_AE_HITGROUND:
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pHitGroundSounds[RANDOM_LONG(0, ARRAYSIZE(pHitGroundSounds) - 1)], VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(-5, 5) + 100);

		if (pev->sequence == PITWORM_ANIM_Level2AttackCenter)
			UTIL_ScreenShake(pev->origin, 12.0, 100.0, 2.0, 1000.0);
		else
			UTIL_ScreenShake(pev->origin, 4.0, 3.0, 1.0, 750.0);
		break;

	case PITWORM_UP_AE_SHOOTBEAM:
		if (gpGlobals->time - m_flLastEventTime >= 1.1)
		{
			m_posBeam = m_hEnemy->pev->origin;
			m_posBeam.z += 24.0;

			Vector vecEyePos, vecEyeAng;
			GetAttachment(0, vecEyePos, vecEyeAng);

			m_vecBeam = (m_posBeam - vecEyePos).Normalize();

			m_angleBeam = UTIL_VecToAngles(m_vecBeam);

			ShootBeam();

			m_fLockYaw = true;
		}
		break;

	case PITWORM_UP_AE_LOCKYAW:
		m_fLockYaw = true;
		break;

	default:
		break;
	}
}

class COFPitWormSteamTrigger : public CPointEntity
{
public:
	void Spawn() override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

LINK_ENTITY_TO_CLASS(info_pitworm_steam_lock, COFPitWormSteamTrigger);

void COFPitWormSteamTrigger::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	UTIL_SetOrigin(pev, pev->origin);
}

void COFPitWormSteamTrigger::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	auto pPitworm = static_cast<COFPitWormUp*>(UTIL_FindEntityByClassname(nullptr, "monster_pitworm_up"));

	if (pPitworm)
	{
		if (pPitworm->m_iLevel == 3)
		{
			pPitworm->pev->health = pPitworm->pev->max_health;

			pPitworm->m_iWasHit = 1;
			pPitworm->m_iLevel = 2;

			pPitworm->m_flTakeHitTime = RANDOM_LONG(2, 4) + gpGlobals->time;

			pPitworm->m_posDesired.z = pPitworm->m_flLevels[2];
		}

		pPitworm->m_fTopLevelLocked = true;
	}
}

/**
*	@brief Part of the unfinished monster_pitworm entity
*	Used for navigation and sequence playback
*/
class COFInfoPW : public CPointEntity
{
public:
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool KeyValue(KeyValueData* pkvd) override;

	void Spawn() override;

	void EXPORT StartNode();

	int m_preSequence;
};

TYPEDESCRIPTION COFInfoPW::m_SaveData[] =
	{
		DEFINE_FIELD(COFInfoPW, m_preSequence, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(COFInfoPW, CPointEntity);

LINK_ENTITY_TO_CLASS(info_pitworm, COFInfoPW);

bool COFInfoPW::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("radius", pkvd->szKeyName))
	{
		pev->scale = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq("reachdelay", pkvd->szKeyName))
	{
		pev->speed = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq("reachtarget", pkvd->szKeyName))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq("reachsequence", pkvd->szKeyName))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq("presequence", pkvd->szKeyName))
	{
		m_preSequence = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void COFInfoPW::Spawn()
{
	SetThink(&COFInfoPW::StartNode);
	pev->nextthink = gpGlobals->time + 0.1;
}

void COFInfoPW::StartNode()
{
	pev->origin.z += 1.0;
	DROP_TO_FLOOR(edict());
}

class COFPitWormGib : public CBaseEntity
{
public:
	void Precache() override;
	void Spawn() override;

	void EXPORT GibFloat();
};

LINK_ENTITY_TO_CLASS(pitworm_gib, COFPitWormGib);

void COFPitWormGib::Precache()
{
	PRECACHE_MODEL("models/pit_worm_gibs.mdl");
}

void COFPitWormGib::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE;

	pev->friction = 0.55;
	pev->renderamt = 255;

	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_NOT;

	pev->classname = MAKE_STRING("pitworm_gib");

	SET_MODEL(edict(), "models/pit_worm_gibs.mdl");

	UTIL_SetSize(pev, {-8, -8, -4}, {8, 8, 16});

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&COFPitWormGib::GibFloat);
}

void COFPitWormGib::GibFloat()
{
	if (pev->waterlevel == WATERLEVEL_HEAD)
	{
		pev->movetype = MOVETYPE_FLY;

		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8.0;
	}
	else if (pev->waterlevel != WATERLEVEL_DRY)
	{
		pev->velocity.z -= 8.0;
	}
	else
	{
		pev->movetype = MOVETYPE_BOUNCE;
		pev->velocity.z -= 8.0;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class COFPitWormGibShooter : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT ShootThink();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	virtual COFPitWormGib* CreateGib();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flDelay;
	int m_iGibs;
	int m_iGibCapacity;
	int m_iGibMaterial;
	int m_iGibModelIndex;
	float m_flGibVelocity;
	float m_flVariance;
};

TYPEDESCRIPTION COFPitWormGibShooter::m_SaveData[] =
	{
		DEFINE_FIELD(COFPitWormGibShooter, m_iGibs, FIELD_INTEGER),
		DEFINE_FIELD(COFPitWormGibShooter, m_iGibCapacity, FIELD_INTEGER),
		DEFINE_FIELD(COFPitWormGibShooter, m_iGibMaterial, FIELD_INTEGER),
		DEFINE_FIELD(COFPitWormGibShooter, m_flGibVelocity, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWormGibShooter, m_flVariance, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(COFPitWormGibShooter, CBaseEntity);
LINK_ENTITY_TO_CLASS(pitworm_gibshooter, COFPitWormGibShooter);

void COFPitWormGibShooter::Precache()
{
	m_iGibModelIndex = PRECACHE_MODEL("models/pit_worm_gibs.mdl");
}

bool COFPitWormGibShooter::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iGibs"))
	{
		m_iGibs = m_iGibCapacity = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flVelocity"))
	{
		m_flGibVelocity = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flVariance"))
	{
		m_flVariance = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flDelay"))
	{
		m_flDelay = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void COFPitWormGibShooter::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&COFPitWormGibShooter::ShootThink);
	pev->nextthink = gpGlobals->time;
}

void COFPitWormGibShooter::Spawn()
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if (m_flDelay == 0)
	{
		m_flDelay = 0.1;
	}

	SetMovedir(pev);
	pev->body = MODEL_FRAMES(m_iGibModelIndex);
}


COFPitWormGib* COFPitWormGibShooter::CreateGib()
{
	if (CVAR_GET_FLOAT("violence_hgibs") == 0)
		return NULL;

	COFPitWormGib* pGib = GetClassPtr((COFPitWormGib*)NULL);
	pGib->Spawn();

	if (pev->body <= 1)
	{
		ALERT(at_aiconsole, "GibShooter Body is <= 1!\n");
	}

	pGib->pev->body = RANDOM_LONG(1, pev->body - 1); // avoid throwing random amounts of the 0th gib. (skull).

	return pGib;
}


void COFPitWormGibShooter::ShootThink()
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	Vector vecShootDir;

	vecShootDir = pev->movedir;

	vecShootDir = vecShootDir + gpGlobals->v_right * RANDOM_FLOAT(-1, 1) * m_flVariance;
	;
	vecShootDir = vecShootDir + gpGlobals->v_forward * RANDOM_FLOAT(-1, 1) * m_flVariance;
	;
	vecShootDir = vecShootDir + gpGlobals->v_up * RANDOM_FLOAT(-1, 1) * m_flVariance;
	;

	vecShootDir = vecShootDir.Normalize();
	COFPitWormGib* pGib = CreateGib();

	if (pGib)
	{
		pGib->pev->origin = pev->origin;
		pGib->pev->velocity = vecShootDir * m_flGibVelocity;
	}

	SetUse(nullptr);
	SetThink(&COFPitWormGibShooter::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

#define bits_MEMORY_ADVANCE_NODE (bits_MEMORY_CUSTOM2)
#define bits_MEMORY_COMPLETED_NODE (bits_MEMORY_CUSTOM3)

int gSpikeSprite, gSpikeDebrisSprite;

/**
*	@brief Unfinished, never used Pitworm NPC
*/
class COFPitWorm : public CBaseMonster
{
public:
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int Classify() override
	{
		return CLASS_ALIEN_MONSTER;
	}

	void SetYawSpeed() override
	{
		int yawSpeed = 100;
		if (m_Activity != ACT_IDLE)
			yawSpeed = 90;

		pev->yaw_speed = yawSpeed;
	}

	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void Precache() override;

	bool CheckMeleeAttack1(float flDot, float flDist) override
	{
		return flDot >= 0.7 && flDist <= 192.0;
	}

	void Activate() override
	{
		if (m_hTargetEnt == NULL)
			Remember(bits_MEMORY_ADVANCE_NODE); // Start 'er up
	}

	void StartMonster() override;

	void Spawn() override;

	bool CheckRangeAttack1(float flDot, float flDist) override
	{
		if (flDist <= 1024.0 && gpGlobals->time > m_spikeTime)
		{
			auto enemy = static_cast<CBaseEntity*>(m_hEnemy);

			if (enemy && FVisible(enemy))
			{
				Vector vecStart, vecAngles;
				GetAttachment(0, vecStart, vecAngles);

				const auto vecEnd = BodyTarget(vecStart);

				TraceResult tr;
				UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, edict(), &tr);

				return true;
			}
		}

		return false;
	}

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	void Move(float flInterval) override;

	void ShootBeam();

	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void StrafeBeam();

	void EXPORT StartupUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void UpdateEye();

	void TrackEnemy();

	void RunAI() override;

	void SetObjectCollisionBox() override;

	static const char* pChildDieSounds[];
	static const char* pSackSounds[];
	static const char* pDeathSounds[];
	static const char* pAttackSounds[];
	static const char* pAttackHitSounds[];
	static const char* pBirthSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pFootSounds[];

	float m_nodeTime;
	float m_spikeTime;
	float m_painSoundTime;
	Activity m_slowMode;
	float m_slowTime;
	float m_flHeadYaw;
	float m_flHeadPitch;
	float m_flIdealHeadYaw;
	float m_flIdealHeadPitch;
	Vector m_posTarget;
	Vector m_vecTarget;
	CBeam* m_pBeam;
	Vector m_posBeam;
	Vector m_vecBeam;
	Vector m_angleBeam;
	float m_offsetBeam;
	float m_flBeamExpireTime;
	float m_flBeamDir;
	Activity m_NextActivity;
};

TYPEDESCRIPTION COFPitWorm::m_SaveData[] =
	{
		DEFINE_FIELD(COFPitWorm, m_nodeTime, FIELD_TIME),
		DEFINE_FIELD(COFPitWorm, m_spikeTime, FIELD_TIME),
		DEFINE_FIELD(COFPitWorm, m_painSoundTime, FIELD_TIME),
		DEFINE_FIELD(COFPitWorm, m_slowMode, FIELD_INTEGER),
		DEFINE_FIELD(COFPitWorm, m_slowTime, FIELD_TIME),
		DEFINE_FIELD(COFPitWorm, m_flHeadYaw, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWorm, m_flHeadPitch, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWorm, m_flIdealHeadYaw, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWorm, m_flIdealHeadPitch, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWorm, m_posTarget, FIELD_VECTOR),
		DEFINE_FIELD(COFPitWorm, m_vecTarget, FIELD_VECTOR),
		DEFINE_FIELD(COFPitWorm, m_pBeam, FIELD_CLASSPTR),
		DEFINE_FIELD(COFPitWorm, m_posBeam, FIELD_VECTOR),
		DEFINE_FIELD(COFPitWorm, m_vecBeam, FIELD_VECTOR),
		DEFINE_FIELD(COFPitWorm, m_angleBeam, FIELD_VECTOR),
		DEFINE_FIELD(COFPitWorm, m_offsetBeam, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWorm, m_flBeamExpireTime, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWorm, m_flBeamDir, FIELD_FLOAT),
		DEFINE_FIELD(COFPitWorm, m_NextActivity, FIELD_INTEGER)};

IMPLEMENT_SAVERESTORE(COFPitWorm, CBaseMonster);

const char* COFPitWorm::pChildDieSounds[] =
	{
		"gonarch/gon_childdie1.wav",
		"gonarch/gon_childdie2.wav",
		"gonarch/gon_childdie3.wav",
};

const char* COFPitWorm::pSackSounds[] =
	{
		"gonarch/gon_sack1.wav",
		"gonarch/gon_sack2.wav",
		"gonarch/gon_sack3.wav",
};

const char* COFPitWorm::pDeathSounds[] =
	{
		"gonarch/gon_die1.wav",
};

const char* COFPitWorm::pAttackSounds[] =
	{
		"gonarch/gon_attack1.wav",
		"gonarch/gon_attack2.wav",
		"gonarch/gon_attack3.wav",
};
const char* COFPitWorm::pAttackHitSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav",
};

const char* COFPitWorm::pBirthSounds[] =
	{
		"gonarch/gon_birth1.wav",
		"gonarch/gon_birth2.wav",
		"gonarch/gon_birth3.wav",
};

const char* COFPitWorm::pAlertSounds[] =
	{
		"gonarch/gon_alert1.wav",
		"gonarch/gon_alert2.wav",
		"gonarch/gon_alert3.wav",
};

const char* COFPitWorm::pPainSounds[] =
	{
		"gonarch/gon_pain2.wav",
		"gonarch/gon_pain4.wav",
		"gonarch/gon_pain5.wav",
};

const char* COFPitWorm::pFootSounds[] =
	{
		"gonarch/gon_step1.wav",
		"gonarch/gon_step2.wav",
		"gonarch/gon_step3.wav",
};

LINK_ENTITY_TO_CLASS(monster_pitworm, COFPitWorm);

bool COFPitWorm::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
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
			ALERT(at_aiconsole, "BM: Finished node health!!!\n");
		}
	}

	return false;
}

void COFPitWorm::Precache()
{
	g_engfuncs.pfnPrecacheModel("models/pit_worm.mdl");
	PRECACHE_SOUND_ARRAY(pChildDieSounds);
	PRECACHE_SOUND_ARRAY(pSackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pBirthSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pFootSounds);

	g_engfuncs.pfnPrecacheModel("sprites/xspark1.spr");
	gSpikeSprite = g_engfuncs.pfnPrecacheModel("sprites/mommaspout.spr");
	gSpikeDebrisSprite = g_engfuncs.pfnPrecacheModel("sprites/mommablob.spr");
	g_engfuncs.pfnPrecacheSound("bullchicken/bc_acid1.wav");
	g_engfuncs.pfnPrecacheSound("bullchicken/bc_spithit1.wav");
	g_engfuncs.pfnPrecacheSound("bullchicken/bc_spithit2.wav");
}

void COFPitWorm::StartMonster()
{
	// update capabilities
	if (LookupActivity(ACT_RANGE_ATTACK1) != ACTIVITY_NOT_AVAILABLE)
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK1;
	}
	if (LookupActivity(ACT_RANGE_ATTACK2) != ACTIVITY_NOT_AVAILABLE)
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK2;
	}
	if (LookupActivity(ACT_MELEE_ATTACK1) != ACTIVITY_NOT_AVAILABLE)
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK1;
	}
	if (LookupActivity(ACT_MELEE_ATTACK2) != ACTIVITY_NOT_AVAILABLE)
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK2;
	}

	// Raise monster off the floor one unit, then drop to floor
	if (pev->movetype != MOVETYPE_FLY && !FBitSet(pev->spawnflags, SF_MONSTER_FALL_TO_GROUND))
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR(ENT(pev));
		// Try to move the monster to make sure it's not stuck in a brush.
		if (!WALK_MOVE(ENT(pev), 0, 0, WALKMOVE_NORMAL))
		{
			ALERT(at_error, "Monster %s stuck in wall--level design error", STRING(pev->classname));
			pev->effects = EF_BRIGHTFIELD;
		}
	}
	else
	{
		pev->flags &= ~FL_ONGROUND;
	}

	if (!FStringNull(pev->target)) // this monster has a target
	{
		// Find the monster's initial target entity, stash it
		m_pGoalEnt = CBaseEntity::Instance(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target)));

		if (!m_pGoalEnt)
		{
			ALERT(at_error, "ReadyMonster()--%s couldn't find target %s", STRING(pev->classname), STRING(pev->target));
		}
		else
		{
			// Monster will start turning towards his destination
			MakeIdealYaw(m_pGoalEnt->pev->origin);

			// JAY: How important is this error message?  Big Momma doesn't obey this rule, so I took it out.
#if 0
			// At this point, we expect only a path_corner as initial goal
			if (!FClassnameIs(m_pGoalEnt->pev, "path_corner"))
			{
				ALERT(at_warning, "ReadyMonster--monster's initial goal '%s' is not a path_corner", STRING(pev->target));
			}
#endif

			// set the monster up to walk a path corner path.
			// !!!BUGBUG - this is a minor bit of a hack.
			// JAYJAY
			m_movementGoal = MOVEGOAL_PATHCORNER;
			m_movementActivity = m_slowMode;

			if (!FRefreshRoute())
			{
				ALERT(at_aiconsole, "Can't Create Route!\n");
			}
			SetState(MONSTERSTATE_IDLE);
			//ChangeSchedule(GetScheduleOfType(SCHED_IDLE_WALK));
		}
	}

	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);

	pev->view_ofs = vecEyePos - pev->origin;

	//SetState ( m_IdealMonsterState );
	//SetActivity ( m_IdealActivity );

	// Delay drop to floor to make sure each door in the level has had its chance to spawn
	// Spread think times so that they don't all happen at the same time (Carmack)
	SetThink(&CBaseMonster::CallMonsterThink);
	pev->nextthink += RANDOM_FLOAT(0.1, 0.4); // spread think times.
}

void COFPitWorm::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/pit_worm.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = 150 * gSkillData.bigmommaHealthFactor;
	pev->view_ofs = Vector(0, 0, 128); // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.3;			   // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1 | bits_CAP_TURN_HEAD;
	m_slowMode = ACT_RUN;

	MonsterInit();

	m_flDistLook = 1024;
	m_slowTime = 0;
	m_pBeam = nullptr;
}

void COFPitWorm::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (ptr->iHitgroup != 1)
	{
		// didn't hit the sack?

		if (pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0, 10) < 1))
		{
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
			pev->dmgtime = gpGlobals->time;
		}
	}
	else if (gpGlobals->time > m_painSoundTime)
	{
		m_painSoundTime = gpGlobals->time + RANDOM_LONG(1, 3);
		EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds);

		if (gpGlobals->time > m_slowTime)
		{
			if (flDamage >= 50.0)
			{
				m_slowMode = ACT_WALK_HURT;
				m_IdealActivity = ACT_WALK_HURT;
				m_movementActivity = ACT_WALK_HURT;
				m_slowTime = gpGlobals->time + 3.0;
			}
			else
			{
				m_slowMode = ACT_WALK;
				m_IdealActivity = ACT_WALK;
				m_movementActivity = ACT_WALK;
				m_slowTime = gpGlobals->time + 3.0;
			}
			SetConditions(bits_COND_SPECIAL2);
		}

		UTIL_BloodStream(ptr->vecEndPos, vecDir, m_bloodColor, 128);
		UTIL_BloodDecalTrace(ptr, m_bloodColor);
	}


	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

#define DIST_TO_CHECK 200

void COFPitWorm::Move(float flInterval)
{
	float flWaypointDist;
	float flCheckDist;
	float flDist; // how far the lookahead check got before hitting an object.
	Vector vecDir;
	Vector vecApex;
	CBaseEntity* pTargetEnt;

	// Don't move if no valid route
	if (FRouteClear())
	{
		// If we still have a movement goal, then this is probably a route truncated by SimplifyRoute()
		// so refresh it.
		if (m_movementGoal == MOVEGOAL_NONE || !FRefreshRoute())
		{
			ALERT(at_aiconsole, "Tried to move with no route!\n");
			TaskFail();
			return;
		}
	}

	if (m_flMoveWaitFinished > gpGlobals->time)
		return;

		// Debug, test movement code
#if 0
//	if ( CVAR_GET_FLOAT("stopmove" ) != 0 )
	{
		if (m_movementGoal == MOVEGOAL_ENEMY)
			RouteSimplify(m_hEnemy);
		else
			RouteSimplify(m_hTargetEnt);
		FRefreshRoute();
		return;
	}
#else
// Debug, draw the route
//	DrawRoute( pev, m_Route, m_iRouteIndex, 0, 200, 0 );
#endif

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	// local move to waypoint.
	vecDir = (m_Route[m_iRouteIndex].vecLocation - pev->origin).Normalize();
	flWaypointDist = (m_Route[m_iRouteIndex].vecLocation - pev->origin).Length2D();

	if (flWaypointDist > 32)
	{
		MakeIdealYaw(m_Route[m_iRouteIndex].vecLocation);
		ChangeYaw(pev->yaw_speed);
	}

	// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
	if (flWaypointDist < DIST_TO_CHECK)
	{
		flCheckDist = flWaypointDist;
	}
	else
	{
		flCheckDist = DIST_TO_CHECK;
	}

	if ((m_Route[m_iRouteIndex].iType & (~bits_MF_NOT_TO_MASK)) == bits_MF_TO_ENEMY)
	{
		// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
		pTargetEnt = m_hEnemy;
	}
	else if ((m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_TARGETENT)
	{
		pTargetEnt = m_hTargetEnt;
	}

	// !!!BUGBUG - CheckDist should be derived from ground speed.
	// If this fails, it should be because of some dynamic entity blocking this guy.
	// We've already checked this path, so we should wait and time out if the entity doesn't move
	flDist = 0;
	if (CheckLocalMove(pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist) != LOCALMOVE_VALID)
	{
		CBaseEntity* pBlocker;

		// Can't move, stop
		Stop();
		// Blocking entity is in global trace_ent
		pBlocker = CBaseEntity::Instance(gpGlobals->trace_ent);
		if (pBlocker)
		{
			DispatchBlocked(edict(), pBlocker->edict());

			m_IdealActivity = m_movementActivity = ACT_MELEE_ATTACK1;
		}
		else if (m_Activity == ACT_MELEE_ATTACK1)
		{
			m_IdealActivity = m_movementActivity = ACT_MELEE_ATTACK1;
		}
		else
		{
			m_IdealActivity = m_movementActivity = ACT_IDLE;
		}

		if (pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time - m_flMoveWaitFinished) > 3.0)
		{
			// Can we still move toward our target?
			if (flDist < m_flGroundSpeed)
			{
				// No, Wait for a second
				m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
				return;
			}
			// Ok, still enough room to take a step

			m_IdealActivity = m_movementActivity = m_slowMode;
		}
	}

	// close enough to the target, now advance to the next target. This is done before actually reaching
	// the target so that we get a nice natural turn while moving.
	if (ShouldAdvanceRoute(flWaypointDist)) ///!!!BUGBUG- magic number
	{
		AdvanceRoute(flWaypointDist);
	}

	// Might be waiting for a door
	if (m_flMoveWaitFinished > gpGlobals->time)
	{
		Stop();
		return;
	}

	// UNDONE: this is a hack to quit moving farther than it has looked ahead.
	if (flCheckDist < m_flGroundSpeed * flInterval)
	{
		flInterval = flCheckDist / m_flGroundSpeed;
		// ALERT( at_console, "%.02f\n", flInterval );
	}
	MoveExecute(pTargetEnt, vecDir, flInterval);

	if (MovementIsComplete())
	{
		Stop();
		RouteClear();
	}
}

void COFPitWorm::ShootBeam()
{
	if (m_hEnemy)
	{
		const float beamDirection = g_engfuncs.pfnRandomLong(0, 1) != 0 ? 1 : -1;

		m_flBeamDir = -beamDirection;
		m_offsetBeam = beamDirection * 40.0;

		Vector vecEyePos, vecEyeAng;
		GetAttachment(0, vecEyePos, vecEyeAng);

		m_vecBeam = (m_posTarget - vecEyePos).Normalize();

		m_angleBeam = UTIL_VecToAngles(m_vecBeam);
		UTIL_MakeVectors(m_angleBeam);

		m_vecBeam = gpGlobals->v_forward;
		m_vecBeam.z = -m_vecBeam.z;

		const Vector vecEnd = m_offsetBeam * gpGlobals->v_right + 2048 * m_vecBeam;

		TraceResult tr;
		UTIL_TraceLine(vecEyePos, vecEnd, dont_ignore_monsters, edict(), &tr);

		m_pBeam = CBeam::BeamCreate("sprites/laserbeam.spr", 80);
		if (m_pBeam)
		{
			//EHANDLE::operator CBaseEntity* (&this->baseclass_0.m_hEnemy);
			m_pBeam->PointEntInit(tr.vecEndPos, entindex());

			m_pBeam->SetEndAttachment(1);

			m_pBeam->pev->rendercolor.x = 0;
			m_pBeam->pev->rendercolor.y = 255;
			m_pBeam->pev->rendercolor.z = 32;
			m_pBeam->pev->renderamt = 192;
			m_pBeam->pev->scale = 32;
			m_pBeam->pev->spawnflags |= SF_BEAM_SPARKSTART;

			auto target = CBaseEntity::Instance(tr.pHit);

			if (target && target->pev->takedamage != DAMAGE_NO)
			{
				ClearMultiDamage();
				target->TraceAttack(pev, 10, m_vecBeam, &tr, DMG_ENERGYBEAM);
				target->TakeDamage(pev, pev, 10, DMG_ENERGYBEAM);
			}
			else if (tr.flFraction != 1.0)
			{
				UTIL_DecalTrace(&tr, g_engfuncs.pfnRandomLong(0, 4));
				m_pBeam->DoSparks(tr.vecEndPos, tr.vecEndPos);
			}

			m_flBeamExpireTime = gpGlobals->time + 0.5;
			m_flIdealHeadYaw = 50.0 * m_flBeamDir + m_flIdealHeadYaw;
		}
	}
}

void COFPitWorm::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case 1:
	{
		UTIL_ScreenShake(pev->origin, 4.0, 3.0, 1.0, 750.0);

		Vector forward, right;

		UTIL_MakeVectorsPrivate(pev->angles, forward, right, NULL);

		Vector center = pev->origin + forward * 128;
		Vector mins = center - Vector(96, 96, 0);
		Vector maxs = center + Vector(96, 96, 128);

		CBaseEntity* pList[8];
		int count = UTIL_EntitiesInBox(pList, 8, mins, maxs, FL_MONSTER | FL_CLIENT);
		CBaseEntity* pHurt = NULL;

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
			pHurt->TakeDamage(pev, pev, gSkillData.bigmommaDmgSlash, DMG_CRUSH | DMG_SLASH);
			pHurt->pev->punchangle.x = 15;

			pHurt->pev->velocity = pHurt->pev->velocity + (forward * 220) + Vector(0, 0, 200);

			pHurt->pev->flags &= ~FL_ONGROUND;
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
	}
	break;

	case 2:
	{
		m_posTarget = m_hEnemy->pev->origin;
		m_posTarget.z += 24;

		Vector vecEyePos, vecEyeAng;
		GetAttachment(0, vecEyePos, vecEyeAng);

		m_vecTarget = (m_posTarget - vecEyePos).Normalize();
		m_posBeam = m_posTarget;
		m_vecBeam = m_vecTarget;
		m_angleBeam = UTIL_VecToAngles(m_vecBeam);

		ShootBeam();
	}
	break;

	case 3:
	{
		pev->flags &= ~FL_ONGROUND;

		UTIL_SetOrigin(pev, pev->origin + Vector(0, 0, 1));
		UTIL_MakeVectors(pev->angles);
		pev->velocity = gpGlobals->v_forward * 200 + gpGlobals->v_up * 500;
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void COFPitWorm::StrafeBeam()
{
	m_offsetBeam += 20 * m_flBeamDir;

	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);

	m_vecBeam = (m_posTarget - vecEyePos).Normalize();

	m_angleBeam = UTIL_VecToAngles(m_vecBeam);
	UTIL_MakeVectors(m_angleBeam);
	m_vecBeam = gpGlobals->v_forward;
	m_vecBeam.z = -m_vecBeam.z;

	const Vector vecEnd = vecEyePos + gpGlobals->v_right * m_offsetBeam + m_vecBeam * 2048;

	TraceResult tr;
	UTIL_TraceLine(vecEyePos, vecEnd, dont_ignore_monsters, edict(), &tr);

	m_pBeam->pev->origin = tr.vecEndPos;

	auto hit = CBaseEntity::Instance(tr.pHit);

	if (hit && hit->pev->takedamage != DAMAGE_NO)
	{
		ClearMultiDamage();
		hit->TraceAttack(pev, 0x41200000, m_vecBeam, &tr, 1024);
		hit->TakeDamage(pev, pev, 0x41200000, 1024);
	}
	else if (tr.flFraction != 1)
	{
		UTIL_DecalTrace(&tr, g_engfuncs.pfnRandomLong(0, 4));
		m_pBeam->DoSparks(tr.vecEndPos, tr.vecEndPos);
	}
}

void COFPitWorm::StartupUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&COFPitWorm::CallMonsterThink);
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse(nullptr);
}

void COFPitWorm::UpdateEye()
{
	if (m_flHeadYaw != m_flIdealHeadYaw)
	{
		if (m_flIdealHeadYaw <= m_flHeadYaw)
		{
			m_flHeadYaw -= V_min(5, m_flHeadYaw - m_flIdealHeadYaw);
		}
		else
		{
			m_flHeadYaw += V_min(5, m_flIdealHeadYaw - m_flHeadYaw);
		}

		SetBoneController(1, m_flHeadYaw);
	}

	if (m_flHeadPitch != m_flIdealHeadPitch)
	{
		if (m_flIdealHeadPitch <= m_flHeadPitch)
		{
			m_flHeadPitch -= V_min(5, m_flHeadPitch - m_flIdealHeadPitch);
		}
		else
		{
			m_flHeadPitch += V_min(5, m_flIdealHeadPitch - m_flHeadPitch);
		}

		SetBoneController(0, m_flHeadPitch);
	}
}

void COFPitWorm::TrackEnemy()
{
	auto enemy = static_cast<CBaseEntity*>(m_hEnemy);

	Vector vecEyePos, vecEyeAng;
	GetAttachment(0, vecEyePos, vecEyeAng);

	vecEyePos.x = pev->origin.x;
	vecEyePos.y = pev->origin.y;

	const Vector vec = enemy->pev->origin + enemy->pev->view_ofs - vecEyePos;

	const Vector vecDir = UTIL_VecToAngles(vec);

	m_flHeadPitch = vecDir.x - pev->angles.x;

	if (m_flHeadPitch < 0)
		m_flHeadPitch += 360;

	m_flHeadPitch = -m_flHeadPitch;

	if (m_flHeadPitch > 180)
		m_flHeadPitch -= 360;

	if (m_flHeadPitch < -180)
		m_flHeadPitch += m_flHeadPitch;

	if (m_flHeadPitch < 0)
	{
		m_flHeadPitch = V_max(-45, m_flHeadPitch);
	}
	else if (m_flHeadPitch > 0)
	{
		m_flHeadPitch = V_min(45, m_flHeadPitch);
	}

	m_flHeadYaw = -VecToYaw(vec) - pev->angles.y;

	if (m_flHeadYaw < 0)
		m_flHeadYaw += 360;

	if (m_flHeadYaw > 180)
		m_flHeadYaw -= 360;

	if (m_flHeadYaw < -180)
		m_flHeadYaw += 360;

	if (m_flHeadYaw < 0)
	{
		m_flHeadYaw = V_max(-45, m_flHeadYaw);
	}
	else if (m_flHeadYaw > 0)
	{
		m_flHeadYaw = V_min(45, m_flHeadYaw);
	}
}

void COFPitWorm::RunAI()
{
	if (!m_hEnemy)
	{
		Look(1024);

		if (HasConditions(bits_COND_SEE_DISLIKE | bits_COND_SEE_HATE | bits_COND_SEE_NEMESIS))
		{
			auto enemy = BestVisibleEnemy();
			if (static_cast<CBaseEntity*>(m_hEnemy) != enemy)
			{
				if (enemy)
				{
					PushEnemy(static_cast<CBaseEntity*>(m_hEnemy), m_vecEnemyLKP);
					SetConditions(bits_COND_NEW_ENEMY);
					m_hEnemy = enemy;
					m_vecEnemyLKP = m_hEnemy->pev->origin;
				}
			}
		}

		if (!m_hEnemy && PopEnemy())
		{
			SetConditions(bits_COND_NEW_ENEMY);
		}
	}

	if (!m_pGoalEnt)
	{
		if (!FStringNull(pev->target))
		{
			m_movementGoal = MOVEGOAL_PATHCORNER;
			m_pGoalEnt = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));
			m_movementActivity = m_slowMode;

			if (!FRefreshRoute())
			{
				ALERT(at_aiconsole, "Can't Create Route!\n");
			}
		}
	}

	if (m_hEnemy)
	{
		SetConditions(bits_COND_SEE_DISLIKE | bits_COND_SEE_HATE);

		CheckEnemy(m_hEnemy);

		if (m_hEnemy->IsAlive())
		{
			if (m_hEnemy && !m_pBeam)
			{
				TrackEnemy();
			}
		}
		else
		{
			m_hEnemy = nullptr;
		}
	}

	UpdateEye();

	if (m_pBeam)
	{
		if (!m_hEnemy || m_flBeamExpireTime <= gpGlobals->time)
		{
			UTIL_Remove(m_pBeam);
			m_pBeam = nullptr;
		}
		else
		{
			StrafeBeam();
		}
	}

	if (!m_fSequenceFinished)
	{
		if (m_Activity != m_IdealActivity)
		{
			m_movementActivity = m_IdealActivity;
			SetActivity(m_IdealActivity);
		}
		return;
	}

	if (m_hEnemy && HasConditions(bits_COND_CAN_RANGE_ATTACK1))
	{
		m_IdealActivity = ACT_RANGE_ATTACK1;
		m_spikeTime = gpGlobals->time + g_engfuncs.pfnRandomFloat(7, 10);
	}
	else if (m_hEnemy && HasConditions(bits_COND_CAN_MELEE_ATTACK1))
	{
		m_IdealActivity = ACT_MELEE_ATTACK1;
	}
	else if (m_Activity == ACT_MELEE_ATTACK1 || m_Activity == ACT_RANGE_ATTACK1)
	{
		if (0 == m_movementGoal)
		{
			m_IdealActivity = ACT_IDLE;
		}
		else
		{
			m_IdealActivity = m_slowMode;
		}
	}
	else
	{
		if (m_Activity == ACT_IDLE)
		{
			if (0 == m_movementGoal)
			{
				m_IdealActivity = ACT_IDLE;
				return;
			}
		}
		else if (0 == m_movementGoal)
		{
			m_IdealActivity = ACT_IDLE;
			m_movementActivity = m_IdealActivity;
			SetActivity(m_IdealActivity);
			return;
		}

		if (HasConditions(bits_COND_SPECIAL2) && gpGlobals->time > m_slowTime)
		{
			ClearConditions(bits_COND_SPECIAL2);
			m_IdealActivity = m_slowMode = ACT_RUN;
		}
		else
		{
			m_IdealActivity = m_slowMode;
		}
	}

	if (m_Activity != m_IdealActivity)
	{
		m_movementActivity = m_IdealActivity;
		SetActivity(m_IdealActivity);
	}
	else
	{
		if (m_Activity == ACT_MELEE_ATTACK1 || m_Activity == ACT_RANGE_ATTACK1)
		{
			m_IdealActivity = ACT_IDLE;

			if (m_Activity != m_IdealActivity)
			{
				m_movementActivity = m_IdealActivity;
				SetActivity(m_IdealActivity);
			}
		}
	}
}

void COFPitWorm::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-64, -64, 0);
	pev->absmax = pev->origin + Vector(64, 96, 128);
}
