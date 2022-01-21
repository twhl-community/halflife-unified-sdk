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
	void Spawn() override
	{
		SpawnCore("models/zombie_barney.mdl", gSkillData.zombieBarneyHealth);
	}

	void Precache() override
	{
		PrecacheCore("models/zombie_barney.mdl");
	}

protected:
	float GetOneSlashDamage() override { return gSkillData.zombieBarneyDmgOneSlash; }
	float GetBothSlashDamage() override { return gSkillData.zombieBarneyDmgBothSlash; }
};

LINK_ENTITY_TO_CLASS(monster_zombie_barney, CZombieBarney);
