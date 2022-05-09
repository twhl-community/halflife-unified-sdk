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
// Zombie Barney
//=========================================================

#include "cbase.h"
#include "zombie.h"

class CZombieBarney : public CZombie
{
public:
	void OnCreate() override
	{
		CZombie::OnCreate();

		pev->health = GetSkillFloat("zombie_barney_health"sv);
	}

	void Spawn() override
	{
		SpawnCore("models/zombie_barney.mdl");
	}

	void Precache() override
	{
		PrecacheCore("models/zombie_barney.mdl");
	}

protected:
	float GetOneSlashDamage() override { return GetSkillFloat("zombie_barney_dmg_one_slash"sv); }
	float GetBothSlashDamage() override { return GetSkillFloat("zombie_barney_dmg_both_slash"sv); }
};

LINK_ENTITY_TO_CLASS(monster_zombie_barney, CZombieBarney);
