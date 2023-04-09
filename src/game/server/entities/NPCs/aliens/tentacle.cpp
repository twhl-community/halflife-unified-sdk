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

#define ACT_T_IDLE 1010
#define ACT_T_TAP 1020
#define ACT_T_STRIKE 1030
#define ACT_T_REARIDLE 1040

constexpr int TENTACLE_NUM_HEIGHTS = 4;

/**
 *	@brief Amount of units to subtract from a height value to get its equivalent level.
 */
constexpr float TENTACLE_HEIGHT_LEVEL_OFFSET = 40;

/**
*	@brief silo of death tentacle monster
*/
class CTentacle : public CBaseMonster
{
	DECLARE_CLASS(CTentacle, CBaseMonster);
	DECLARE_DATAMAP();

public:
	CTentacle();

	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	// Don't allow the tentacle to go across transitions!!!
	int ObjectCaps() override { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector(-400, -400, 0);
		pev->absmax = pev->origin + Vector(400, 400, 850);
	}

	void Cycle();
	void CommandUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void Start();
	void DieThink();

	void Test();

	void HitTouch(CBaseEntity* pOther);

	float HearingSensitivity() override { return 2.0; }

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void Killed(CBaseEntity* attacker, int iGib) override;

	MONSTERSTATE GetIdealState() override { return MONSTERSTATE_IDLE; }
	// TODO: should override base, but has different signature
	bool CanPlaySequence(bool fDisregardState) { return true; }

	int Classify() override;

	int Level(float dz);
	int MyLevel();
	float MyHeight();

	float m_flInitialYaw;
	int m_iGoalAnim;
	int m_iLevel;
	int m_iDir;
	float m_flFramerateAdj;
	float m_flSoundYaw;
	int m_iSoundLevel;
	float m_flSoundTime;
	float m_flSoundRadius;
	int m_iHitDmg;
	float m_flHitTime;

	float m_flTapRadius;

	float m_flNextSong;
	static bool g_fFlySound;
	static bool g_fSquirmSound;

	float m_flMaxYaw;
	int m_iTapSound;

	Vector m_vecPrevSound;
	float m_flPrevSoundTime;

	float m_Heights[TENTACLE_NUM_HEIGHTS] =
		{
			0,
			256,
			448,
			640};

	static const char* pHitSilo[];
	static const char* pHitDirt[];
	static const char* pHitWater[];
};

bool CTentacle::g_fFlySound;
bool CTentacle::g_fSquirmSound;

LINK_ENTITY_TO_CLASS(monster_tentacle, CTentacle);

// stike sounds
#define TE_NONE -1
#define TE_SILO 0
#define TE_DIRT 1
#define TE_WATER 2

const char* CTentacle::pHitSilo[] =
	{
		"tentacle/te_strike1.wav",
		"tentacle/te_strike2.wav",
};

const char* CTentacle::pHitDirt[] =
	{
		"player/pl_dirt1.wav",
		"player/pl_dirt2.wav",
		"player/pl_dirt3.wav",
		"player/pl_dirt4.wav",
};

const char* CTentacle::pHitWater[] =
	{
		"player/pl_slosh1.wav",
		"player/pl_slosh2.wav",
		"player/pl_slosh3.wav",
		"player/pl_slosh4.wav",
};

BEGIN_DATAMAP(CTentacle)
DEFINE_FIELD(m_flInitialYaw, FIELD_FLOAT),
	DEFINE_FIELD(m_iGoalAnim, FIELD_INTEGER),
	DEFINE_FIELD(m_iLevel, FIELD_INTEGER),
	DEFINE_FIELD(m_iDir, FIELD_INTEGER),
	DEFINE_FIELD(m_flFramerateAdj, FIELD_FLOAT),
	DEFINE_FIELD(m_flSoundYaw, FIELD_FLOAT),
	DEFINE_FIELD(m_iSoundLevel, FIELD_INTEGER),
	DEFINE_FIELD(m_flSoundTime, FIELD_TIME),
	DEFINE_FIELD(m_flSoundRadius, FIELD_FLOAT),
	DEFINE_FIELD(m_iHitDmg, FIELD_INTEGER),
	DEFINE_FIELD(m_flHitTime, FIELD_TIME),
	DEFINE_FIELD(m_flTapRadius, FIELD_FLOAT),
	DEFINE_FIELD(m_flNextSong, FIELD_TIME),
	DEFINE_FIELD(m_iTapSound, FIELD_INTEGER),
	DEFINE_FIELD(m_flMaxYaw, FIELD_FLOAT),
	DEFINE_FIELD(m_vecPrevSound, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_flPrevSoundTime, FIELD_TIME),
	DEFINE_ARRAY(m_Heights, FIELD_FLOAT, TENTACLE_NUM_HEIGHTS),
	DEFINE_FUNCTION(Cycle),
	DEFINE_FUNCTION(CommandUse),
	DEFINE_FUNCTION(Start),
	DEFINE_FUNCTION(DieThink),
	DEFINE_FUNCTION(Test),
	DEFINE_FUNCTION(HitTouch),
	END_DATAMAP();

// animation sequence aliases
enum TENTACLE_ANIM
{
	TENTACLE_ANIM_Pit_Idle,

	TENTACLE_ANIM_rise_to_Temp1,
	TENTACLE_ANIM_Temp1_to_Floor,
	TENTACLE_ANIM_Floor_Idle,
	TENTACLE_ANIM_Floor_Fidget_Pissed,
	TENTACLE_ANIM_Floor_Fidget_SmallRise,
	TENTACLE_ANIM_Floor_Fidget_Wave,
	TENTACLE_ANIM_Floor_Strike,
	TENTACLE_ANIM_Floor_Tap,
	TENTACLE_ANIM_Floor_Rotate,
	TENTACLE_ANIM_Floor_Rear,
	TENTACLE_ANIM_Floor_Rear_Idle,
	TENTACLE_ANIM_Floor_to_Lev1,

	TENTACLE_ANIM_Lev1_Idle,
	TENTACLE_ANIM_Lev1_Fidget_Claw,
	TENTACLE_ANIM_Lev1_Fidget_Shake,
	TENTACLE_ANIM_Lev1_Fidget_Snap,
	TENTACLE_ANIM_Lev1_Strike,
	TENTACLE_ANIM_Lev1_Tap,
	TENTACLE_ANIM_Lev1_Rotate,
	TENTACLE_ANIM_Lev1_Rear,
	TENTACLE_ANIM_Lev1_Rear_Idle,
	TENTACLE_ANIM_Lev1_to_Lev2,

	TENTACLE_ANIM_Lev2_Idle,
	TENTACLE_ANIM_Lev2_Fidget_Shake,
	TENTACLE_ANIM_Lev2_Fidget_Swing,
	TENTACLE_ANIM_Lev2_Fidget_Tut,
	TENTACLE_ANIM_Lev2_Strike,
	TENTACLE_ANIM_Lev2_Tap,
	TENTACLE_ANIM_Lev2_Rotate,
	TENTACLE_ANIM_Lev2_Rear,
	TENTACLE_ANIM_Lev2_Rear_Idle,
	TENTACLE_ANIM_Lev2_to_Lev3,

	TENTACLE_ANIM_Lev3_Idle,
	TENTACLE_ANIM_Lev3_Fidget_Shake,
	TENTACLE_ANIM_Lev3_Fidget_Side,
	TENTACLE_ANIM_Lev3_Fidget_Swipe,
	TENTACLE_ANIM_Lev3_Strike,
	TENTACLE_ANIM_Lev3_Tap,
	TENTACLE_ANIM_Lev3_Rotate,
	TENTACLE_ANIM_Lev3_Rear,
	TENTACLE_ANIM_Lev3_Rear_Idle,

	TENTACLE_ANIM_Lev1_Door_reach,

	TENTACLE_ANIM_Lev3_to_Engine,
	TENTACLE_ANIM_Engine_Idle,
	TENTACLE_ANIM_Engine_Sway,
	TENTACLE_ANIM_Engine_Swat,
	TENTACLE_ANIM_Engine_Bob,
	TENTACLE_ANIM_Engine_Death1,
	TENTACLE_ANIM_Engine_Death2,
	TENTACLE_ANIM_Engine_Death3,

	TENTACLE_ANIM_none
};

void CTentacle::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = 75;
	pev->model = MAKE_STRING("models/tentacle2.mdl");
}

int CTentacle::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

void CTentacle::Spawn()
{
	Precache();

	// Sort the list to ensure it's in the correct order, since it is expected that each value is larger than the last.
	std::sort(std::begin(m_Heights), std::end(m_Heights));

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->effects = 0;
	pev->max_health = pev->health;
	pev->sequence = 0;

	// Always interpolate tentacles since they don't actually move.
	m_EFlags |= EFLAG_SLERP;

	SetModel(STRING(pev->model));
	SetSize(Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->takedamage = DAMAGE_AIM;
	pev->flags |= FL_MONSTER;

	m_bloodColor = BLOOD_COLOR_GREEN;

	SetThink(&CTentacle::Start);
	SetTouch(&CTentacle::HitTouch);
	SetUse(&CTentacle::CommandUse);

	pev->nextthink = gpGlobals->time + 0.2;

	ResetSequenceInfo();
	m_iDir = 1;

	pev->yaw_speed = 18;
	m_flInitialYaw = pev->angles.y;
	pev->ideal_yaw = m_flInitialYaw;

	g_fFlySound = false;
	g_fSquirmSound = false;

	m_iHitDmg = 20;

	if (m_flMaxYaw <= 0)
		m_flMaxYaw = 65;

	m_MonsterState = MONSTERSTATE_IDLE;

	// SetThink( Test );
	SetOrigin(pev->origin);
}

void CTentacle::Precache()
{
	PrecacheModel(STRING(pev->model));

	PrecacheSound("ambience/flies.wav");
	PrecacheSound("ambience/squirm2.wav");

	PrecacheSound("tentacle/te_alert1.wav");
	PrecacheSound("tentacle/te_alert2.wav");
	PrecacheSound("tentacle/te_flies1.wav");
	PrecacheSound("tentacle/te_move1.wav");
	PrecacheSound("tentacle/te_move2.wav");
	PrecacheSound("tentacle/te_roar1.wav");
	PrecacheSound("tentacle/te_roar2.wav");
	PrecacheSound("tentacle/te_search1.wav");
	PrecacheSound("tentacle/te_search2.wav");
	PrecacheSound("tentacle/te_sing1.wav");
	PrecacheSound("tentacle/te_sing2.wav");
	PrecacheSound("tentacle/te_squirm2.wav");
	PrecacheSound("tentacle/te_strike1.wav");
	PrecacheSound("tentacle/te_strike2.wav");
	PrecacheSound("tentacle/te_swing1.wav");
	PrecacheSound("tentacle/te_swing2.wav");

	PRECACHE_SOUND_ARRAY(pHitSilo);
	PRECACHE_SOUND_ARRAY(pHitDirt);
	PRECACHE_SOUND_ARRAY(pHitWater);
}

CTentacle::CTentacle()
{
	// TODO: use in-class initializer
	m_flMaxYaw = 65;
	m_iTapSound = 0;
}

bool CTentacle::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sweeparc"))
	{
		m_flMaxYaw = atof(pkvd->szValue) / 2.0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "sound"))
	{
		m_iTapSound = atoi(pkvd->szValue);
		return true;
	}
	else
	{
		std::string_view name;
		int index;

		if (UTIL_ParseStringWithArrayIndex(pkvd->szKeyName, name, index))
		{
			if (name == "height")
			{
				if (index < TENTACLE_NUM_HEIGHTS)
				{
					m_Heights[index] = atof(pkvd->szValue);
				}
				else
				{
					AILogger->debug("Invalid tentacle height index \"{}\"", pkvd->szKeyName);
				}

				return true;
			}
		}
	}

	return CBaseMonster::KeyValue(pkvd);
}

int CTentacle::Level(float dz)
{
	for (std::size_t level = 1; level < std::size(m_Heights); ++level)
	{
		if (dz < (m_Heights[level] - TENTACLE_HEIGHT_LEVEL_OFFSET))
		{
			return level - 1;
		}
	}

	return 3;
}

float CTentacle::MyHeight()
{
	const int level = MyLevel();

	if (level >= 1 && level <= 3)
	{
		return m_Heights[level];
	}

	return m_Heights[0];
}

int CTentacle::MyLevel()
{
	switch (pev->sequence)
	{
	case TENTACLE_ANIM_Pit_Idle:
		return -1;

	case TENTACLE_ANIM_rise_to_Temp1:
	case TENTACLE_ANIM_Temp1_to_Floor:
	case TENTACLE_ANIM_Floor_to_Lev1:
		return 0;

	case TENTACLE_ANIM_Floor_Idle:
	case TENTACLE_ANIM_Floor_Fidget_Pissed:
	case TENTACLE_ANIM_Floor_Fidget_SmallRise:
	case TENTACLE_ANIM_Floor_Fidget_Wave:
	case TENTACLE_ANIM_Floor_Strike:
	case TENTACLE_ANIM_Floor_Tap:
	case TENTACLE_ANIM_Floor_Rotate:
	case TENTACLE_ANIM_Floor_Rear:
	case TENTACLE_ANIM_Floor_Rear_Idle:
		return 0;

	case TENTACLE_ANIM_Lev1_Idle:
	case TENTACLE_ANIM_Lev1_Fidget_Claw:
	case TENTACLE_ANIM_Lev1_Fidget_Shake:
	case TENTACLE_ANIM_Lev1_Fidget_Snap:
	case TENTACLE_ANIM_Lev1_Strike:
	case TENTACLE_ANIM_Lev1_Tap:
	case TENTACLE_ANIM_Lev1_Rotate:
	case TENTACLE_ANIM_Lev1_Rear:
	case TENTACLE_ANIM_Lev1_Rear_Idle:
		return 1;

	case TENTACLE_ANIM_Lev1_to_Lev2:
		return 1;

	case TENTACLE_ANIM_Lev2_Idle:
	case TENTACLE_ANIM_Lev2_Fidget_Shake:
	case TENTACLE_ANIM_Lev2_Fidget_Swing:
	case TENTACLE_ANIM_Lev2_Fidget_Tut:
	case TENTACLE_ANIM_Lev2_Strike:
	case TENTACLE_ANIM_Lev2_Tap:
	case TENTACLE_ANIM_Lev2_Rotate:
	case TENTACLE_ANIM_Lev2_Rear:
	case TENTACLE_ANIM_Lev2_Rear_Idle:
		return 2;

	case TENTACLE_ANIM_Lev2_to_Lev3:
		return 2;

	case TENTACLE_ANIM_Lev3_Idle:
	case TENTACLE_ANIM_Lev3_Fidget_Shake:
	case TENTACLE_ANIM_Lev3_Fidget_Side:
	case TENTACLE_ANIM_Lev3_Fidget_Swipe:
	case TENTACLE_ANIM_Lev3_Strike:
	case TENTACLE_ANIM_Lev3_Tap:
	case TENTACLE_ANIM_Lev3_Rotate:
	case TENTACLE_ANIM_Lev3_Rear:
	case TENTACLE_ANIM_Lev3_Rear_Idle:
		return 3;

	case TENTACLE_ANIM_Lev1_Door_reach:
		return -1;
	}
	return -1;
}

void CTentacle::Test()
{
	pev->sequence = TENTACLE_ANIM_Floor_Strike;
	pev->framerate = 0;
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;
}

void CTentacle::Cycle()
{
	// AILogger->debug("{} {:.2f} {} {}", STRING( pev->targetname ), pev->origin.z, m_MonsterState, m_IdealMonsterState);
	pev->nextthink = gpGlobals->time + 0.1;

	// AILogger->debug("{} {} {} {} {} {}", STRING(pev->targetname), pev->sequence, m_iGoalAnim, m_iDir, pev->framerate, pev->health);

	if (m_MonsterState == MONSTERSTATE_SCRIPT || m_IdealMonsterState == MONSTERSTATE_SCRIPT)
	{
		pev->angles.y = m_flInitialYaw;
		pev->ideal_yaw = m_flInitialYaw;
		ClearConditions(IgnoreConditions());
		MonsterThink();
		m_iGoalAnim = TENTACLE_ANIM_Pit_Idle;
		return;
	}

	DispatchAnimEvents();
	StudioFrameAdvance();

	UpdateShockEffect();

	ChangeYaw(pev->yaw_speed);

	CSound* pSound;

	Listen();

	// Listen will set this if there's something in my sound list
	if (HasConditions(bits_COND_HEAR_SOUND))
		pSound = PBestSound();
	else
		pSound = nullptr;

	if (pSound)
	{
		Vector vecDir;
		if (gpGlobals->time - m_flPrevSoundTime < 0.5)
		{
			float dt = gpGlobals->time - m_flPrevSoundTime;
			vecDir = pSound->m_vecOrigin + (pSound->m_vecOrigin - m_vecPrevSound) / dt - pev->origin;
		}
		else
		{
			vecDir = pSound->m_vecOrigin - pev->origin;
		}
		m_flPrevSoundTime = gpGlobals->time;
		m_vecPrevSound = pSound->m_vecOrigin;

		m_flSoundYaw = VectorToYaw(vecDir) - m_flInitialYaw;
		m_iSoundLevel = Level(vecDir.z);

		if (m_flSoundYaw < -180)
			m_flSoundYaw += 360;
		if (m_flSoundYaw > 180)
			m_flSoundYaw -= 360;

		// AILogger->debug("sound {} {:.0f}", m_iSoundLevel, m_flSoundYaw);
		if (m_flSoundTime < gpGlobals->time)
		{
			// play "I hear new something" sound
			const char* sound;

			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				sound = "tentacle/te_alert1.wav";
				break;
			case 1:
				sound = "tentacle/te_alert2.wav";
				break;
			}

			// EmitAmbientSound(pev->origin + Vector( 0, 0, MyHeight()), sound, 1.0, ATTN_NORM, 0, 100);
		}
		m_flSoundTime = gpGlobals->time + RANDOM_FLOAT(5.0, 10.0);
	}

	// clip ideal_yaw
	float dy = m_flSoundYaw;
	switch (pev->sequence)
	{
	case TENTACLE_ANIM_Floor_Rear:
	case TENTACLE_ANIM_Floor_Rear_Idle:
	case TENTACLE_ANIM_Lev1_Rear:
	case TENTACLE_ANIM_Lev1_Rear_Idle:
	case TENTACLE_ANIM_Lev2_Rear:
	case TENTACLE_ANIM_Lev2_Rear_Idle:
	case TENTACLE_ANIM_Lev3_Rear:
	case TENTACLE_ANIM_Lev3_Rear_Idle:
		if (dy < 0 && dy > -m_flMaxYaw)
			dy = -m_flMaxYaw;
		if (dy > 0 && dy < m_flMaxYaw)
			dy = m_flMaxYaw;
		break;
	default:
		if (dy < -m_flMaxYaw)
			dy = -m_flMaxYaw;
		if (dy > m_flMaxYaw)
			dy = m_flMaxYaw;
	}
	pev->ideal_yaw = m_flInitialYaw + dy;

	if (m_fSequenceFinished)
	{
		// AILogger->debug("{} done {} {}", STRING(pev->targetname), pev->sequence, m_iGoalAnim);
		if (pev->health <= 1)
		{
			m_iGoalAnim = TENTACLE_ANIM_Pit_Idle;
			if (pev->sequence == TENTACLE_ANIM_Pit_Idle)
			{
				pev->health = pev->max_health;
			}
		}
		else if (m_flSoundTime > gpGlobals->time)
		{
			if (m_flSoundYaw >= -(m_flMaxYaw + 30) && m_flSoundYaw <= (m_flMaxYaw + 30))
			{
				// strike
				m_iGoalAnim = LookupActivity(ACT_T_STRIKE + m_iSoundLevel);
			}
			else if (m_flSoundYaw >= -m_flMaxYaw * 2 && m_flSoundYaw <= m_flMaxYaw * 2)
			{
				// tap
				m_iGoalAnim = LookupActivity(ACT_T_TAP + m_iSoundLevel);
			}
			else
			{
				// go into rear idle
				m_iGoalAnim = LookupActivity(ACT_T_REARIDLE + m_iSoundLevel);
			}
		}
		else if (pev->sequence == TENTACLE_ANIM_Pit_Idle)
		{
			// stay in pit until hear noise
			m_iGoalAnim = TENTACLE_ANIM_Pit_Idle;
		}
		else if (pev->sequence == m_iGoalAnim)
		{
			if (MyLevel() >= 0 && gpGlobals->time < m_flSoundTime)
			{
				if (RANDOM_LONG(0, 9) < m_flSoundTime - gpGlobals->time)
				{
					// continue stike
					m_iGoalAnim = LookupActivity(ACT_T_STRIKE + m_iSoundLevel);
				}
				else
				{
					// tap
					m_iGoalAnim = LookupActivity(ACT_T_TAP + m_iSoundLevel);
				}
			}
			else if (MyLevel() < 0)
			{
				m_iGoalAnim = LookupActivity(ACT_T_IDLE + 0);
			}
			else
			{
				if (m_flNextSong < gpGlobals->time)
				{
					// play "I hear new something" sound
					const char* sound;

					switch (RANDOM_LONG(0, 1))
					{
					case 0:
						sound = "tentacle/te_sing1.wav";
						break;
					case 1:
						sound = "tentacle/te_sing2.wav";
						break;
					}

					EmitSound(CHAN_VOICE, sound, 1.0, ATTN_NORM);

					m_flNextSong = gpGlobals->time + RANDOM_FLOAT(10, 20);
				}

				if (RANDOM_LONG(0, 15) == 0)
				{
					// idle on new level
					m_iGoalAnim = LookupActivity(ACT_T_IDLE + RANDOM_LONG(0, 3));
				}
				else if (RANDOM_LONG(0, 3) == 0)
				{
					// tap
					m_iGoalAnim = LookupActivity(ACT_T_TAP + MyLevel());
				}
				else
				{
					// idle
					m_iGoalAnim = LookupActivity(ACT_T_IDLE + MyLevel());
				}
			}
			if (m_flSoundYaw < 0)
				m_flSoundYaw += RANDOM_FLOAT(2, 8);
			else
				m_flSoundYaw -= RANDOM_FLOAT(2, 8);
		}

		pev->sequence = FindTransition(pev->sequence, m_iGoalAnim, &m_iDir);

		if (m_iDir > 0)
		{
			pev->frame = 0;
		}
		else
		{
			m_iDir = -1; // just to safe
			pev->frame = 255;
		}
		ResetSequenceInfo();

		m_flFramerateAdj = RANDOM_FLOAT(-0.2, 0.2);
		pev->framerate = m_iDir * 1.0 + m_flFramerateAdj;

		switch (pev->sequence)
		{
		case TENTACLE_ANIM_Floor_Tap:
		case TENTACLE_ANIM_Lev1_Tap:
		case TENTACLE_ANIM_Lev2_Tap:
		case TENTACLE_ANIM_Lev3_Tap:
		{
			Vector vecSrc;
			UTIL_MakeVectors(pev->angles);

			TraceResult tr1, tr2;

			vecSrc = pev->origin + Vector(0, 0, MyHeight() - 4);
			UTIL_TraceLine(vecSrc, vecSrc + gpGlobals->v_forward * 512, ignore_monsters, ENT(pev), &tr1);

			vecSrc = pev->origin + Vector(0, 0, MyHeight() + 8);
			UTIL_TraceLine(vecSrc, vecSrc + gpGlobals->v_forward * 512, ignore_monsters, ENT(pev), &tr2);

			// AILogger->debug("{} {}", tr1.flFraction * 512, tr2.flFraction * 512);

			m_flTapRadius = SetBlending(0, RANDOM_FLOAT(tr1.flFraction * 512, tr2.flFraction * 512));
		}
		break;
		default:
			m_flTapRadius = 336; // 400 - 64
			break;
		}
		pev->view_ofs.z = MyHeight();
		// AILogger->debug("seq {}", pev->sequence);
	}

	if (m_flPrevSoundTime + 2.0 > gpGlobals->time)
	{
		// 1.5 normal speed if hears sounds
		pev->framerate = m_iDir * 1.5 + m_flFramerateAdj;
	}
	else if (m_flPrevSoundTime + 5.0 > gpGlobals->time)
	{
		// slowdown to normal
		pev->framerate = m_iDir + m_iDir * (5 - (gpGlobals->time - m_flPrevSoundTime)) / 2 + m_flFramerateAdj;
	}
}

void CTentacle::CommandUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// AILogger->debug("{} triggered {}", STRING(pev->targetname), useType);
	switch (useType)
	{
	case USE_OFF:
		pev->takedamage = DAMAGE_NO;
		SetThink(&CTentacle::DieThink);
		m_iGoalAnim = TENTACLE_ANIM_Engine_Death1;
		break;
	case USE_ON:
		if (pActivator)
		{
			// AILogger->debug("insert sound");
			CSoundEnt::InsertSound(bits_SOUND_WORLD, pActivator->pev->origin, 1024, 1.0);
		}
		break;
	case USE_SET:
		break;
	case USE_TOGGLE:
		pev->takedamage = DAMAGE_NO;
		SetThink(&CTentacle::DieThink);
		m_iGoalAnim = TENTACLE_ANIM_Engine_Idle;
		break;
	}
}

void CTentacle::DieThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	DispatchAnimEvents();
	StudioFrameAdvance();

	ChangeYaw(24);

	if (m_fSequenceFinished)
	{
		if (pev->sequence == m_iGoalAnim)
		{
			switch (m_iGoalAnim)
			{
			case TENTACLE_ANIM_Engine_Idle:
			case TENTACLE_ANIM_Engine_Sway:
			case TENTACLE_ANIM_Engine_Swat:
			case TENTACLE_ANIM_Engine_Bob:
				m_iGoalAnim = TENTACLE_ANIM_Engine_Sway + RANDOM_LONG(0, 2);
				break;
			case TENTACLE_ANIM_Engine_Death1:
			case TENTACLE_ANIM_Engine_Death2:
			case TENTACLE_ANIM_Engine_Death3:
				UTIL_Remove(this);
				return;
			}
		}

		// const int previousSequence = pev->sequence;
		pev->sequence = FindTransition(pev->sequence, m_iGoalAnim, &m_iDir);
		// AILogger->debug("{} : {} => {}", previousSequence, m_iGoalAnim, pev->sequence);

		if (m_iDir > 0)
		{
			pev->frame = 0;
		}
		else
		{
			pev->frame = 255;
		}
		ResetSequenceInfo();

		float dy;
		switch (pev->sequence)
		{
		case TENTACLE_ANIM_Floor_Rear:
		case TENTACLE_ANIM_Floor_Rear_Idle:
		case TENTACLE_ANIM_Lev1_Rear:
		case TENTACLE_ANIM_Lev1_Rear_Idle:
		case TENTACLE_ANIM_Lev2_Rear:
		case TENTACLE_ANIM_Lev2_Rear_Idle:
		case TENTACLE_ANIM_Lev3_Rear:
		case TENTACLE_ANIM_Lev3_Rear_Idle:
		case TENTACLE_ANIM_Engine_Idle:
		case TENTACLE_ANIM_Engine_Sway:
		case TENTACLE_ANIM_Engine_Swat:
		case TENTACLE_ANIM_Engine_Bob:
		case TENTACLE_ANIM_Engine_Death1:
		case TENTACLE_ANIM_Engine_Death2:
		case TENTACLE_ANIM_Engine_Death3:
			pev->framerate = RANDOM_FLOAT(m_iDir - 0.2, m_iDir + 0.2);
			dy = 180;
			break;
		default:
			pev->framerate = 1.5;
			dy = 0;
			break;
		}
		pev->ideal_yaw = m_flInitialYaw + dy;
	}
}

void CTentacle::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	const char* sound;

	switch (pEvent->event)
	{
	case 1: // bang
	{
		Vector vecSrc, vecAngles;
		GetAttachment(0, vecSrc, vecAngles);

		// Vector vecSrc = pev->origin + m_flTapRadius * Vector( cos( pev->angles.y * (3.14192653 / 180.0) ), sin( pev->angles.y * (PI / 180.0) ), 0.0 );

		// vecSrc.z += MyHeight( );

		switch (m_iTapSound)
		{
		case TE_SILO:
			EmitAmbientSound(vecSrc, RANDOM_SOUND_ARRAY(pHitSilo), 1.0, ATTN_NORM, 0, 100);
			break;
		case TE_NONE:
			break;
		case TE_DIRT:
			EmitAmbientSound(vecSrc, RANDOM_SOUND_ARRAY(pHitDirt), 1.0, ATTN_NORM, 0, 100);
			break;
		case TE_WATER:
			EmitAmbientSound(vecSrc, RANDOM_SOUND_ARRAY(pHitWater), 1.0, ATTN_NORM, 0, 100);
			break;
		}
		gpGlobals->force_retouch++;
	}
	break;

	case 3: // start killing swing
		m_iHitDmg = 200;
		// EmitAmbientSound(pev->origin + Vector( 0, 0, MyHeight()), "tentacle/te_swing1.wav", 1.0, ATTN_NORM, 0, 100);
		break;

	case 4: // end killing swing
		m_iHitDmg = 25;
		break;

	case 5: // just "whoosh" sound
		// EmitAmbientSound(pev->origin + Vector( 0, 0, MyHeight()), "tentacle/te_swing2.wav", 1.0, ATTN_NORM, 0, 100);
		break;

	case 2: // tap scrape
	case 6: // light tap
	{
		Vector vecSrc = pev->origin + m_flTapRadius * Vector(cos(pev->angles.y * (PI / 180.0)), sin(pev->angles.y * (PI / 180.0)), 0.0);

		vecSrc.z += MyHeight();

		float flVol = RANDOM_FLOAT(0.3, 0.5);

		switch (m_iTapSound)
		{
		case TE_SILO:
			EmitAmbientSound(vecSrc, RANDOM_SOUND_ARRAY(pHitSilo), flVol, ATTN_NORM, 0, 100);
			break;
		case TE_NONE:
			break;
		case TE_DIRT:
			EmitAmbientSound(vecSrc, RANDOM_SOUND_ARRAY(pHitDirt), flVol, ATTN_NORM, 0, 100);
			break;
		case TE_WATER:
			EmitAmbientSound(vecSrc, RANDOM_SOUND_ARRAY(pHitWater), flVol, ATTN_NORM, 0, 100);
			break;
		}
	}
	break;


	case 7: // roar
		switch (RANDOM_LONG(0, 1))
		{
		default:
		case 0:
			sound = "tentacle/te_roar1.wav";
			break;
		case 1:
			sound = "tentacle/te_roar2.wav";
			break;
		}

		EmitAmbientSound(pev->origin + Vector(0, 0, MyHeight()), sound, 1.0, ATTN_NORM, 0, 100);
		break;

	case 8: // search
		switch (RANDOM_LONG(0, 1))
		{
		default:
		case 0:
			sound = "tentacle/te_search1.wav";
			break;
		case 1:
			sound = "tentacle/te_search2.wav";
			break;
		}

		EmitAmbientSound(pev->origin + Vector(0, 0, MyHeight()), sound, 1.0, ATTN_NORM, 0, 100);
		break;

	case 9: // swing
		switch (RANDOM_LONG(0, 1))
		{
		default:
		case 0:
			sound = "tentacle/te_move1.wav";
			break;
		case 1:
			sound = "tentacle/te_move2.wav";
			break;
		}

		EmitAmbientSound(pev->origin + Vector(0, 0, MyHeight()), sound, 1.0, ATTN_NORM, 0, 100);
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

void CTentacle::Start()
{
	SetThink(&CTentacle::Cycle);

	if (!g_fFlySound)
	{
		EmitSound(CHAN_BODY, "ambience/flies.wav", 1, ATTN_NORM);
		g_fFlySound = true;
		//		pev->nextthink = gpGlobals-> time + 0.1;
	}
	else if (!g_fSquirmSound)
	{
		EmitSound(CHAN_BODY, "ambience/squirm2.wav", 1, ATTN_NORM);
		g_fSquirmSound = true;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CTentacle::HitTouch(CBaseEntity* pOther)
{
	TraceResult tr = UTIL_GetGlobalTrace();

	if (pOther->pev->modelindex == pev->modelindex)
		return;

	if (m_flHitTime > gpGlobals->time)
		return;

	// only look at the ones where the player hit me
	if (tr.pHit == nullptr || tr.pHit->v.modelindex != pev->modelindex)
		return;

	if (tr.iHitgroup >= 3)
	{
		pOther->TakeDamage(this, this, m_iHitDmg, DMG_CRUSH);
		// AILogger->debug("wack {:3d} : ", m_iHitDmg);
	}
	else if (tr.iHitgroup != 0)
	{
		pOther->TakeDamage(this, this, 20, DMG_CRUSH);
		// AILogger->debug("tap  {:3d} : ", 20);
	}
	else
	{
		return; // Huh?
	}

	m_flHitTime = gpGlobals->time + 0.5;

	// AILogger->debug("{} : {:.0f} : {} : {}", STRING(tr.pHit->v.classname), pev->angles.y, STRING(pOther->pev->classname), tr.iHitgroup);
}

bool CTentacle::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (flDamage > pev->health)
	{
		pev->health = 1;
	}
	else
	{
		pev->health -= flDamage;
	}
	return true;
}

void CTentacle::Killed(CBaseEntity* attacker, int iGib)
{
	m_iGoalAnim = TENTACLE_ANIM_Pit_Idle;
	ClearShockEffect();
	return;
}

class CTentacleMaw : public CBaseMonster
{
public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
};

LINK_ENTITY_TO_CLASS(monster_tentaclemaw, CTentacleMaw);

void CTentacleMaw::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = 75;
	pev->model = MAKE_STRING("models/maw.mdl");
}

void CTentacleMaw::Spawn()
{
	Precache();
	SetModel(STRING(pev->model));
	SetSize(Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_STEP;
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;

	pev->angles.x = 90;
	// ResetSequenceInfo( );
}

void CTentacleMaw::Precache()
{
	PrecacheModel(STRING(pev->model));
}
