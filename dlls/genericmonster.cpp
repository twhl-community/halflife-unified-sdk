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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "soundent.h"

// For holograms, make them not solid so the player can walk through them
#define SF_GENERICMONSTER_NOTSOLID 4

const int SF_GENERICMONSTER_CONTROLLER = 8;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	int ISoundMask() override;

	void PlayScriptedSentence(const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener) override;

	void MonsterThink() override;
	void IdleHeadTurn(Vector& vecFriend);

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	float m_talkTime;
	EHANDLE m_hTalkTarget;
	float m_flIdealYaw;
	float m_flCurrentYaw;
};
LINK_ENTITY_TO_CLASS(monster_generic, CGenericMonster);

TYPEDESCRIPTION CGenericMonster::m_SaveData[] =
	{
		//TODO: should be FIELD_TIME
		DEFINE_FIELD(CGenericMonster, m_talkTime, FIELD_FLOAT),
		DEFINE_FIELD(CGenericMonster, m_hTalkTarget, FIELD_EHANDLE),
		DEFINE_FIELD(CGenericMonster, m_flIdealYaw, FIELD_FLOAT),
		DEFINE_FIELD(CGenericMonster, m_flCurrentYaw, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CGenericMonster, CBaseMonster);

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CGenericMonster::Classify()
{
	return CLASS_PLAYER_ALLY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericMonster::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster::ISoundMask()
{
	return bits_SOUND_NONE;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), STRING(pev->model));

	/*
	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) )
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if (FStrEq(STRING(pev->model), "models/player.mdl") || FStrEq(STRING(pev->model), "models/holo.mdl"))
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	m_flFieldOfView = 0.5; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();

	if ((pev->spawnflags & SF_GENERICMONSTER_NOTSOLID) != 0)
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

	if (pev->spawnflags & SF_GENERICMONSTER_CONTROLLER)
	{
		m_afCapability = bits_CAP_TURN_HEAD;
	}

	m_flCurrentYaw = 0;
	m_flIdealYaw = 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster::Precache()
{
	PRECACHE_MODEL((char*)STRING(pev->model));
}

void CGenericMonster::PlayScriptedSentence(const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener)
{
	m_talkTime = gpGlobals->time + duration;
	PlaySentence(pszSentence, duration, volume, attenuation);

	m_hTalkTarget = pListener;
}

void CGenericMonster::MonsterThink()
{
	if (m_afCapability & bits_CAP_TURN_HEAD)
	{
		if (m_hTalkTarget)
		{
			if (gpGlobals->time > m_talkTime)
			{
				m_flIdealYaw = 0;
				m_hTalkTarget = nullptr;
			}
			else
			{
				IdleHeadTurn(m_hTalkTarget->pev->origin);
			}
		}

		if (m_flCurrentYaw != m_flIdealYaw)
		{
			if (m_flCurrentYaw <= m_flIdealYaw)
			{
				m_flCurrentYaw += V_min(20.0, m_flIdealYaw - m_flCurrentYaw);
			}
			else
			{
				m_flCurrentYaw -= V_min(20.0, m_flCurrentYaw - m_flIdealYaw);
			}

			SetBoneController(0, m_flCurrentYaw);
		}
	}

	CBaseMonster::MonsterThink();
}

// turn head towards supplied origin
void CGenericMonster::IdleHeadTurn(Vector& vecFriend)
{
	// turn head in desired direction only if ent has a turnable head
	if (m_afCapability & bits_CAP_TURN_HEAD)
	{
		float yaw = VecToYaw(vecFriend - pev->origin) - pev->angles.y;

		if (yaw > 180)
			yaw -= 360;
		if (yaw < -180)
			yaw += 360;

		// turn towards vector
		m_flIdealYaw = yaw;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
