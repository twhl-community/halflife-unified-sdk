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

/**
 *	@brief Body queue class here.... It's really just CBaseEntity
 */
class CCorpse : public CBaseEntity
{
	int ObjectCaps() override { return FCAP_DONT_SAVE; }
};

inline CBaseEntity* g_pBodyQueueHead = nullptr;

void InitBodyQue();

/**
 *	@brief make a body que entry for the given ent so the ent can be respawned elsewhere
 *	@details GLOBALS ASSUMED SET:  g_pBodyQueueHead
 */
void CopyToBodyQue(CBaseEntity* entity);
