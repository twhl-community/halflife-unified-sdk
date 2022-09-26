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

#include <cassert>

#include "cbase.h"
#include "ScriptingConstants.h"

namespace scripting
{
std::string_view ModuleTypeToString(ModuleType type)
{
	switch (type)
	{
	case ModuleType::Map: return "Map";
	case ModuleType::Plugin: return "Plugin";
	default:
		// TODO: could use C++23 std::unreachable here someday.
		assert(!"Invalid module type");
		return "Invalid";
	}
}
}
