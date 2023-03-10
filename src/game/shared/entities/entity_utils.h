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

class CBaseEntity;
class CBasePlayer;

/**
 *	@brief Converts an entity pointer to a player pointer, returning nullptr if it isn't a player.
 *	In debug builds the type is checked to make sure it's a player.
 */
CBasePlayer* ToBasePlayer(CBaseEntity* entity);

/**
 *	@brief Converts an edict to a player pointer, returning nullptr if it isn't a player.
 *	In debug builds the type is checked to make sure it's a player.
 */
CBasePlayer* ToBasePlayer(edict_t* entity);
