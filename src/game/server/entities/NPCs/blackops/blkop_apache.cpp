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
#include "military/apache.h"

class COFBlackOpsApache : public CApache
{
	void Spawn() override;
	void Precache() override;
};

LINK_ENTITY_TO_CLASS(monster_blkop_apache, COFBlackOpsApache);

void COFBlackOpsApache::Spawn()
{
	SpawnCore("models/blkop_apache.mdl");
}

void COFBlackOpsApache::Precache()
{
	PrecacheCore("models/blkop_apache.mdl");
}
