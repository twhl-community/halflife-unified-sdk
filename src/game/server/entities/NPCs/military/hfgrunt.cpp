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
#include "hgrunt.h"

int g_fFGruntQuestion; //!< true if an idle grunt asked a question. Cleared when someone answers.

/**
*	@brief friendly hgrunt
*/
class CHFGrunt : public CHGrunt
{
public:
	int IRelationship(CBaseEntity* pTarget) override
	{
		// Players are allies
		if (pTarget->IsPlayer())
			return R_AL;

		return CHGrunt::IRelationship(pTarget);
	}

protected:
	int& GetGruntQuestion() override { return g_fFGruntQuestion; }
};

LINK_ENTITY_TO_CLASS(monster_human_friendly_grunt, CHFGrunt);

/**
 *	@brief when triggered, spawns a monster_human_friendly_grunt repelling down a line.
 */
class CHFGruntRepel : public CHGruntRepel
{
	DECLARE_CLASS(CHFGruntRepel, CHGruntRepel);
	DECLARE_DATAMAP();

public:
	void Precache() override
	{
		PrecacheCore("monster_human_friendly_grunt");
	}

	void Spawn() override
	{
		CHGruntRepel::Spawn();
		SetUse(&CHFGruntRepel::RepelUse);
	}

	void RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
	{
		CreateMonster("monster_human_friendly_grunt");
	}
};

BEGIN_DATAMAP(CHFGruntRepel)
DEFINE_FUNCTION(RepelUse),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(monster_fgrunt_repel, CHFGruntRepel);

/**
 *	@brief DEAD HGRUNT PROP
 *	@details Same as regular grunt
 */
LINK_ENTITY_TO_CLASS(monster_fhgrunt_dead, CDeadHGrunt);
