/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   Use, distribution, and modification of this source code and/or resulting
 *   object code is restricted to non-commercial enhancements to products from
 *   Valve LLC.  All other use, distribution, or modification is prohibited
 *   without written permission from Valve LLC.
 *
 ****/

#pragma once

#include "CBaseEntity.h"

class CBaseDMStart : public CPointEntity
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	bool IsTriggered(CBaseEntity* pEntity) override;
};

inline CBaseEntity* g_pLastSpawn = nullptr;

/**
 *	@brief Returns the entity to spawn at
 *	USES AND SETS GLOBAL g_pLastSpawn
 */
CBaseEntity* EntSelectSpawnPoint(CBasePlayer* pPlayer);
