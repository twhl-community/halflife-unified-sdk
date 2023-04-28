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
#include "basemonster.h"

/**
 *	@brief Opposing Force loader
 */
class COFLoader : public CBaseMonster
{
public:
	int ISoundMask() override { return bits_SOUND_NONE; }

	void OnCreate() override;
	void Precache() override;

	void Spawn() override;

	void SetYawSpeed() override;

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void StartTask(const Task_t* pTask) override;

	void SetTurnActivity();
};

LINK_ENTITY_TO_CLASS(monster_op4loader, COFLoader);

void COFLoader::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = 8;
	pev->model = MAKE_STRING("models/loader.mdl");

	SetClassification("player_ally");
}

void COFLoader::Precache()
{
	PrecacheModel(STRING(pev->model));
	PrecacheSound("ambience/loader_step1.wav");
	PrecacheSound("ambience/loader_hydra1.wav");
}

void COFLoader::Spawn()
{
	Precache();

	SetModel(STRING(pev->model));

	if (FStrEq(STRING(pev->model), "models/player.mdl") || FStrEq(STRING(pev->model), "models/holo.mdl"))
	{
		SetSize(VEC_HULL_MIN, VEC_HULL_MAX);
	}
	else
	{
		SetSize(VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	}

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = DONT_BLEED;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = 0.5f;
	pev->takedamage = DAMAGE_NO;

	MonsterInit();
}

void COFLoader::SetYawSpeed()
{
	pev->yaw_speed = 90;
}

bool COFLoader::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	// Don't take damage
	return true;
}

void COFLoader::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	UTIL_Ricochet(ptr->vecEndPos, g_engfuncs.pfnRandomFloat(1.0, 2.0));
}

void COFLoader::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	CBaseMonster::HandleAnimEvent(pEvent);
}

void COFLoader::StartTask(const Task_t* pTask)
{
	float newYawAngle;

	switch (pTask->iTask)
	{
	case TASK_TURN_LEFT:
		newYawAngle = UTIL_AngleMod(pev->angles.y) + pTask->flData;
		break;

	case TASK_TURN_RIGHT:
		newYawAngle = UTIL_AngleMod(pev->angles.y) - pTask->flData;
		break;

	default:
		CBaseMonster::StartTask(pTask);
		return;
	}

	pev->ideal_yaw = UTIL_AngleMod(newYawAngle);

	SetTurnActivity();
}

void COFLoader::SetTurnActivity()
{
	const auto difference = FlYawDiff();

	if (difference <= -45 && LookupActivity(ACT_TURN_RIGHT) != -1)
	{
		m_IdealActivity = ACT_TURN_RIGHT;
	}
	else if (difference > 45.0 && LookupActivity(ACT_TURN_LEFT) != -1)
	{
		m_IdealActivity = ACT_TURN_LEFT;
	}
}
