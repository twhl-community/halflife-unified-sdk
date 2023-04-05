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
#include "config/CommandWhitelist.h"

class CLogicSetCVar : public CPointEntity
{
	DECLARE_CLASS(CLogicSetCVar, CPointEntity);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	string_t m_CVarName;
	string_t m_CVarValue;

	// Not saved, used to speed up repeated calls and log diagnostics.
	cvar_t* m_CVar{};
};

BEGIN_DATAMAP(CLogicSetCVar)
DEFINE_FIELD(m_CVarName, FIELD_STRING),
	DEFINE_FIELD(m_CVarValue, FIELD_STRING),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(logic_setcvar, CLogicSetCVar);

bool CLogicSetCVar::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "cvar_name"))
	{
		m_CVarName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "cvar_value"))
	{
		m_CVarValue = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CLogicSetCVar::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator))
	{
		return;
	}

	const auto cvarName = STRING(m_CVarName);
	const auto cvarValue = STRING(m_CVarValue);

	Logger->trace("{}:{}:{}: Attempting to set cvar", GetClassname(), entindex(), GetTargetname());

	// Commands should be evaluated on execution in case we've saved and restored and the whitelist has changed.
	if (!g_CommandWhitelist.contains(cvarName))
	{
		Logger->error(
			"The console variable \"{} {}\" cannot be changed because it is not listed in the command whitelist",
			cvarName, cvarValue);
		return;
	}

	if (!m_CVar)
	{
		m_CVar = CVAR_GET_POINTER(cvarName);

		if (!m_CVar)
		{
			Logger->error("The console variable \"{}\" does not exist");
			return;
		}
	}

	Logger->debug("Changing cvar \"{}\" from \"{}\" to \"{}\"", cvarName, m_CVar->string, cvarValue);
	g_engfuncs.pfnCvar_DirectSet(m_CVar, cvarValue);
}
