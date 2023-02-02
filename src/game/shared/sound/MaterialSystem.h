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

/**
 *	@file
 *
 *	Used to detect the texture the player is standing on,
 *	map the texture name to a material type.
 *	Play footstep sound based on material type.
 */

#define CTEXTURESMAX 512 // max number of textures loaded

void PM_InitTextureTypes();

/**
 *	@brief given texture name, find texture type.
 *	If not found, return type 'concrete'.
 */
char PM_FindTextureType(const char* name);
