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

const int MAX_CHANGE_KEYVALUES = 16;

/**
 *	@brief Changes up to ::MAX_CHANGE_KEYVALUES key values in entities with the targetname specified in this entity's target keyvalue
 *	Can also change the target entity's target by specifying a changetarget value
 */
class CTriggerChangeKeyValue : public CBaseDelay
{
	DECLARE_CLASS(CTriggerChangeKeyValue, CBaseDelay);
	DECLARE_DATAMAP();

public:
	int ObjectCaps() override { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	string_t m_changeTargetName;
	int m_cTargets;
	string_t m_iKey[MAX_CHANGE_KEYVALUES];
	string_t m_iValue[MAX_CHANGE_KEYVALUES];
};

LINK_ENTITY_TO_CLASS(trigger_changekeyvalue, CTriggerChangeKeyValue);

BEGIN_DATAMAP(CTriggerChangeKeyValue)
DEFINE_FIELD(m_changeTargetName, FIELD_STRING),
	DEFINE_FIELD(m_cTargets, FIELD_INTEGER),
	DEFINE_ARRAY(m_iKey, FIELD_STRING, MAX_CHANGE_KEYVALUES),
	DEFINE_ARRAY(m_iValue, FIELD_STRING, MAX_CHANGE_KEYVALUES),
	END_DATAMAP();

bool CTriggerChangeKeyValue::KeyValue(KeyValueData* pkvd)
{
	// Make sure base class keys are handled properly
	if (FStrEq("origin", pkvd->szKeyName) || FStrEq("target", pkvd->szKeyName) || FStrEq("targetname", pkvd->szKeyName) || FStrEq("classname", pkvd->szKeyName))
	{
		return CBaseDelay::KeyValue(pkvd);
	}
	else if (FStrEq("changetarget", pkvd->szKeyName))
	{
		m_changeTargetName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (m_cTargets < MAX_CHANGE_KEYVALUES)
	{
		char temp[256];

		UTIL_StripToken(pkvd->szKeyName, temp);
		m_iKey[m_cTargets] = ALLOC_STRING(temp);

		UTIL_StripToken(pkvd->szValue, temp);
		m_iValue[m_cTargets] = ALLOC_STRING(temp);

		++m_cTargets;

		return true;
	}

	return false;
}

void CTriggerChangeKeyValue::Spawn()
{
	// Nothing
}

void CTriggerChangeKeyValue::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	char tmpkey[128];
	char tmpvalue[128];

	for (auto target : UTIL_FindEntitiesByTargetname(STRING(pev->target)))
	{
		for (int i = 0; i < m_cTargets; ++i)
		{
			strncpy(tmpkey, STRING(m_iKey[i]), sizeof(tmpkey) - 1);
			tmpkey[sizeof(tmpkey) - 1] = '\0';

			strncpy(tmpvalue, STRING(m_iValue[i]), sizeof(tmpvalue) - 1);
			tmpvalue[sizeof(tmpvalue) - 1] = '\0';

			KeyValueData kvd{.szClassName = STRING(target->pev->classname), .szKeyName = tmpkey, .szValue = tmpvalue, .fHandled = 0};

			DispatchKeyValue(target->pev->pContainingEntity, &kvd);
		}

		if (!FStringNull(m_changeTargetName))
		{
			target->pev->target = m_changeTargetName;
		}
	}
}
