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
#include "CBaseDelayAPI.h"
#include "CBaseEntityAPI.h"
#include "EntityAPIs.h"

namespace scripting
{
static void RegisterUseType(asIScriptEngine& engine)
{
	const char* const name = "USE_TYPE";

	engine.RegisterEnum(name);
	engine.RegisterEnumValue(name, "USE_OFF", USE_OFF);
	engine.RegisterEnumValue(name, "USE_ON", USE_ON);
	engine.RegisterEnumValue(name, "USE_SET", USE_SET);
	engine.RegisterEnumValue(name, "USE_TOGGLE", USE_TOGGLE);
}

static void RegisterSolid(asIScriptEngine& engine)
{
	const char* const name = "SOLID";

	engine.RegisterEnum(name);
	engine.RegisterEnumValue(name, "SOLID_NOT", SOLID_NOT);
	engine.RegisterEnumValue(name, "SOLID_TRIGGER", SOLID_TRIGGER);
	engine.RegisterEnumValue(name, "SOLID_BBOX", SOLID_BBOX);
	engine.RegisterEnumValue(name, "SOLID_SLIDEBOX", SOLID_SLIDEBOX);
	engine.RegisterEnumValue(name, "SOLID_BSP", SOLID_BSP);
}

static void RegisterMovetype(asIScriptEngine& engine)
{
	const char* const name = "MOVETYPE";

	engine.RegisterEnum(name);
	engine.RegisterEnumValue(name, "MOVETYPE_NONE", MOVETYPE_NONE);
	engine.RegisterEnumValue(name, "MOVETYPE_WALK", MOVETYPE_WALK);
	engine.RegisterEnumValue(name, "MOVETYPE_STEP", MOVETYPE_STEP);
	engine.RegisterEnumValue(name, "MOVETYPE_FLY", MOVETYPE_FLY);
	engine.RegisterEnumValue(name, "MOVETYPE_TOSS", MOVETYPE_TOSS);
	engine.RegisterEnumValue(name, "MOVETYPE_PUSH", MOVETYPE_PUSH);
	engine.RegisterEnumValue(name, "MOVETYPE_NOCLIP", MOVETYPE_NOCLIP);
	engine.RegisterEnumValue(name, "MOVETYPE_FLYMISSILE", MOVETYPE_FLYMISSILE);
	engine.RegisterEnumValue(name, "MOVETYPE_BOUNCE", MOVETYPE_BOUNCE);
	engine.RegisterEnumValue(name, "MOVETYPE_BOUNCEMISSILE", MOVETYPE_BOUNCEMISSILE);
	engine.RegisterEnumValue(name, "MOVETYPE_FOLLOW", MOVETYPE_FOLLOW);
	engine.RegisterEnumValue(name, "MOVETYPE_PUSHSTEP", MOVETYPE_PUSHSTEP);
}

static void RegisterMiscConstants(asIScriptEngine& engine)
{
	engine.RegisterGlobalProperty("const Vector VEC_HUMAN_HULL_MIN", const_cast<Vector*>(&VEC_HUMAN_HULL_MIN));
	engine.RegisterGlobalProperty("const Vector VEC_HUMAN_HULL_MAX", const_cast<Vector*>(&VEC_HUMAN_HULL_MAX));
}

void RegisterGlobalvars(asIScriptEngine& engine)
{
	const char* const name = "Globalvars";

	engine.RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOCOUNT);

	engine.RegisterObjectProperty(name, "const float time", asOFFSET(globalvars_t, time));

	engine.RegisterGlobalProperty("Globalvars Globals", gpGlobals);
}

static CBaseEntity* FindEntityByClassname(CBaseEntity* startEntity, const std::string& className)
{
	return UTIL_FindEntityByClassname(startEntity, className.c_str());
}

static void RegisterEntityUtilityFunctions(asIScriptEngine& engine)
{
	const std::string previousNamespace = engine.GetDefaultNamespace();

	engine.SetDefaultNamespace("Entities");

	engine.RegisterGlobalFunction(
		"CBaseEntity@ FindEntityByClassname(CBaseEntity@ startEntity, const string& in className)",
		asFUNCTION(FindEntityByClassname), asCALL_CDECL);

	engine.SetDefaultNamespace(previousNamespace.c_str());
}

void RegisterEntityAPIs(asIScriptEngine& engine)
{
	RegisterUseType(engine);
	RegisterSolid(engine);
	RegisterMovetype(engine);
	RegisterMiscConstants(engine);

	RegisterGlobalvars(engine);

	RegisterCBaseEntity(engine);
	RegisterCBaseDelay(engine);

	RegisterEntityUtilityFunctions(engine);
}
}
