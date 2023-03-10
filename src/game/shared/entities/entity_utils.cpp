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

#include "cbase.h"
#include "entity_utils.h"

CBasePlayer* ToBasePlayer(CBaseEntity* entity)
{
	if (!entity || !entity->IsPlayer())
	{
		return nullptr;
	}

#if DEBUG
	assert(dynamic_cast<CBasePlayer*>(entity) != nullptr);
#endif

	return static_cast<CBasePlayer*>(entity);
}

CBasePlayer* ToBasePlayer(edict_t* entity)
{
	return ToBasePlayer(GET_PRIVATE<CBaseEntity>(entity));
}
