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
#include "CBaseTrigger.h"

static void UTIL_TeleportToTarget(CBaseEntity* teleportee, Vector targetPosition, const Vector& targetAngles)
{
	if (teleportee->IsPlayer())
	{
		targetPosition.z -= teleportee->pev->mins.z; // make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	targetPosition.z++;

	teleportee->pev->flags &= ~FL_ONGROUND;

	teleportee->SetOrigin(targetPosition);

	teleportee->pev->angles = targetAngles;

	if (teleportee->IsPlayer())
	{
		teleportee->pev->v_angle = targetAngles;
	}

	teleportee->pev->fixangle = FIXANGLE_ABSOLUTE;
	teleportee->pev->velocity = teleportee->pev->basevelocity = g_vecZero;
}

class CTriggerTeleport : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerTeleport, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;

	void TeleportTouch(CBaseEntity* pOther);

private:
	string_t m_FireOnTeleportTarget;
};

BEGIN_DATAMAP(CTriggerTeleport)
DEFINE_FIELD(m_FireOnTeleportTarget, FIELD_STRING),
	DEFINE_FUNCTION(TeleportTouch),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(trigger_teleport, CTriggerTeleport);
LINK_ENTITY_TO_CLASS(info_teleport_destination, CPointEntity);

bool CTriggerTeleport::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "fire_on_teleport"))
	{
		m_FireOnTeleportTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CTriggerTeleport::Spawn()
{
	InitTrigger();

	SetTouch(&CTriggerTeleport::TeleportTouch);
}

void CTriggerTeleport::TeleportTouch(CBaseEntity* pOther)
{
	// Only teleport monsters or clients
	if (!FBitSet(pOther->pev->flags, FL_CLIENT | FL_MONSTER))
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther, m_UseLocked))
		return;

	// No target to teleport to.
	if (FStrEq(GetTarget(), ""))
	{
		return;
	}

	if ((pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS) == 0)
	{ // no monsters allowed!
		if (FBitSet(pOther->pev->flags, FL_MONSTER))
		{
			return;
		}
	}

	if ((pev->spawnflags & SF_TRIGGER_NOCLIENTS) != 0)
	{ // no clients allowed
		if (pOther->IsPlayer())
		{
			return;
		}
	}

	auto target = UTIL_FindEntityByTargetname(nullptr, GetTarget());
	if (FNullEnt(target))
		return;

	UTIL_TeleportToTarget(pOther, target->pev->origin, target->pev->angles);

	if (!FStringNull(m_FireOnTeleportTarget))
	{
		FireTargets(STRING(m_FireOnTeleportTarget), pOther, this, USE_TOGGLE, 0);
	}
}

class CPointTeleport : public CPointEntity
{
	DECLARE_CLASS(CPointTeleport, CPointEntity);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;

	void TeleportUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

private:
	string_t m_FireOnTeleportTarget;
};

BEGIN_DATAMAP(CPointTeleport)
DEFINE_FIELD(m_FireOnTeleportTarget, FIELD_STRING),
	DEFINE_FUNCTION(TeleportUse),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(point_teleport, CPointTeleport);

bool CPointTeleport::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "fire_on_teleport"))
	{
		m_FireOnTeleportTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CPointTeleport::Spawn()
{
	SetUse(&CPointTeleport::TeleportUse);
}

void CPointTeleport::TeleportUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (FStringNull(pev->target))
	{
		Logger->error("{}:{}:{}: No teleport target specified", GetClassname(), GetTargetname(), entindex());
		return;
	}

	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator, m_UseLocked))
		return;

	// Figure out who to teleport
	const char* targetName = GetTarget();

	CBaseEntity* teleportee = UTIL_FindEntityByTargetname(nullptr, targetName, pActivator, this);

	if (!teleportee)
	{
		Logger->error("{}:{}:{}: Invalid target \"{}\" specified",
			GetClassname(), GetTargetname(), entindex(), targetName);
		return;
	}

	UTIL_TeleportToTarget(teleportee, pev->origin, pev->angles);

	if (!FStringNull(m_FireOnTeleportTarget))
	{
		FireTargets(STRING(m_FireOnTeleportTarget), pActivator, this, USE_TOGGLE, 0);
	}
}
