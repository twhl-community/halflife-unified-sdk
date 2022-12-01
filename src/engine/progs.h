/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#include "progdefs.h"
#include "event_flags.h"
#include "event_args.h"
#include "entity_state.h"
#include "edict.h"

#define STRUCT_FROM_LINK(l, t, m) ((t*)((byte*)l - (int)&(((t*)0)->m)))
#define EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l, edict_t, area)
