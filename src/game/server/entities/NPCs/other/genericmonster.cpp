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

/**
*	@brief For holograms, make them not solid so the player can walk through them
*/
#define SF_GENERICMONSTER_NOTSOLID 4

const int SF_GENERICMONSTER_CONTROLLER = 8;

/**
*	@brief purely for scripted sequence work.
*/
class CGenericMonster : public CBaseMonster
{
	DECLARE_CLASS(CGenericMonster, CBaseMonster);
	DECLARE_DATAMAP();

public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	int ISoundMask() override;

	void PlayScriptedSentence(const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener) override;

	void MonsterThink() override;
	void IdleHeadTurn(Vector& vecFriend);

	float m_talkTime;
	EHANDLE m_hTalkTarget;
	float m_flIdealYaw;
	float m_flCurrentYaw;
};
LINK_ENTITY_TO_CLASS(monster_generic, CGenericMonster);

BEGIN_DATAMAP(CGenericMonster)
// TODO: should be FIELD_TIME
DEFINE_FIELD(m_talkTime, FIELD_FLOAT),
	DEFINE_FIELD(m_hTalkTarget, FIELD_EHANDLE),
	DEFINE_FIELD(m_flIdealYaw, FIELD_FLOAT),
	DEFINE_FIELD(m_flCurrentYaw, FIELD_FLOAT),
	END_DATAMAP();

void CGenericMonster::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = 8;
}

int CGenericMonster::Classify()
{
	return CLASS_PLAYER_ALLY;
}

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

int CGenericMonster::ISoundMask()
{
	// generic monster can't hear.
	return bits_SOUND_NONE;
}

void CGenericMonster::Spawn()
{
	Precache();

	SetModel(STRING(pev->model));
	SetSize(VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView = 0.5; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();

	if ((pev->spawnflags & SF_GENERICMONSTER_NOTSOLID) != 0)
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

	if ((pev->spawnflags & SF_GENERICMONSTER_CONTROLLER) != 0)
	{
		m_afCapability = bits_CAP_TURN_HEAD;
	}

	m_flCurrentYaw = 0;
	m_flIdealYaw = 0;
}

void CGenericMonster::Precache()
{
	PrecacheModel(STRING(pev->model));
}

void CGenericMonster::PlayScriptedSentence(const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener)
{
	m_talkTime = gpGlobals->time + duration;
	PlaySentence(pszSentence, duration, volume, attenuation);

	m_hTalkTarget = pListener;
}

void CGenericMonster::MonsterThink()
{
	if ((m_afCapability & bits_CAP_TURN_HEAD) != 0)
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
				m_flCurrentYaw += std::min(20.0f, m_flIdealYaw - m_flCurrentYaw);
			}
			else
			{
				m_flCurrentYaw -= std::min(20.0f, m_flCurrentYaw - m_flIdealYaw);
			}

			SetBoneController(0, m_flCurrentYaw);
		}
	}

	CBaseMonster::MonsterThink();
}

void CGenericMonster::IdleHeadTurn(Vector& vecFriend)
{
	// turn head in desired direction only if ent has a turnable head
	if ((m_afCapability & bits_CAP_TURN_HEAD) != 0)
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
