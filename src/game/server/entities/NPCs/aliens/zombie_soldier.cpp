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
// Zombie Soldier
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include "zombie.h"

class CZombieSoldier : public CZombie
{
public:
	void Spawn() override
	{
		SpawnCore("models/zombie_soldier.mdl", gSkillData.zombieSoldierHealth);
	}

	void Precache() override
	{
		PrecacheCore("models/zombie_soldier.mdl");
	}

protected:
	float GetOneSlashDamage() override { return gSkillData.zombieSoldierDmgOneSlash; }
	float GetBothSlashDamage() override { return gSkillData.zombieSoldierDmgBothSlash; }
};

LINK_ENTITY_TO_CLASS(monster_zombie_soldier, CZombieSoldier);

//=========================================================
// DEAD HGRUNT ZOMBIE PROP
//=========================================================
class CDeadZombieSoldier : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify() override { return	CLASS_ALIEN_MONSTER; }

	void KeyValue(KeyValueData* pkvd) override;

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char* m_szPoses[2];
};

char* CDeadZombieSoldier::m_szPoses[] = {"dead_on_stomach", "dead_on_back"};

void CDeadZombieSoldier::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_zombie_soldier_dead, CDeadZombieSoldier);

//=========================================================
// ********** DeadZombieSoldier SPAWN **********
//=========================================================
void CDeadZombieSoldier::Spawn()
{
	PRECACHE_MODEL("models/zombie_soldier.mdl");
	SET_MODEL(ENT(pev), "models/zombie_soldier.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hgrunt with bad pose\n");
	}

	// Corpses have less health
	pev->health = 8;

	MonsterInitDead();
}
