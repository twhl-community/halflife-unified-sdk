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
#include "military/osprey.h"

class CBlackOpsOsprey : public COsprey
{
public:
	void OnCreate() override
	{
		COsprey::OnCreate();

		pev->model = MAKE_STRING("models/blkop_osprey.mdl");
	}

	void Precache() override
	{
		PrecacheCore("models/blkop_tailgibs.mdl", "models/blkop_bodygibs.mdl", "models/blkop_enginegibs.mdl");
	}

protected:
	const char* GetMonsterClassname() const override { return "monster_male_assassin"; }
};

LINK_ENTITY_TO_CLASS(monster_blkop_osprey, CBlackOpsOsprey);
