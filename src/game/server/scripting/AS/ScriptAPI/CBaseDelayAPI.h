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

#include "CBaseEntityAPI.h"

namespace scripting
{
inline std::string CBaseDelay_GetKillTarget(const CBaseDelay* entity)
{
	return STRING(entity->m_iszKillTarget);
}

inline void CBaseDelay_SetKillTarget(CBaseDelay* entity, const std::string& value)
{
	entity->m_iszKillTarget = ALLOC_STRING(value.c_str());
}

/**
 *	@brief Registers the @c CBaseEntity members for direct access.
 */
template <std::derived_from<CBaseEntity> T>
void RegisterCBaseDelayMembers(asIScriptEngine& engine, const char* name)
{
	RegisterCBaseEntityMembers<T>(engine, name);

	as::RegisterCasts<T, CBaseEntity>(engine, name, "CBaseEntity");

	// Properties
	engine.RegisterObjectProperty(name, "float m_flDelay", asOFFSET(T, m_flDelay));

	engine.RegisterObjectMethod(name, "string get_killtarget() const property",
		asFUNCTION(CBaseDelay_GetKillTarget), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "void set_killtarget(const string& in value) property",
		asFUNCTION(CBaseDelay_SetKillTarget), asCALL_CDECL_OBJFIRST);
}

inline void RegisterCBaseDelay(asIScriptEngine& engine)
{
	RegisterCBaseDelayMembers<CBaseDelay>(engine, "CBaseDelay");
}
}
