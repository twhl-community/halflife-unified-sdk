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

/**
 *	@file
 *	frequently used global functions
 */

#include "cbase.h"
#include "nodes.h"
#include "doors.h"

void CPointEntity::Spawn()
{
	pev->solid = SOLID_NOT;
	//	SetSize(g_vecZero, g_vecZero);
}

/**
 *	@brief Null Entity, remove on startup
 */
class CNullEntity : public CBaseEntity
{
public:
	void Spawn() override;
};

void CNullEntity::Spawn()
{
	REMOVE_ENTITY(edict());
}

LINK_ENTITY_TO_CLASS(info_null, CNullEntity);

void CBaseEntity::UpdateOnRemove()
{
	// tell owner (if any) that we're dead.This is mostly for MonsterMaker functionality.
	MaybeNotifyOwnerOfDeath();

	if (FBitSet(pev->flags, FL_GRAPHED))
	{
		// this entity was a LinkEnt in the world node graph, so we must remove it from
		// the graph since we are removing it from the world.
		for (int i = 0; i < WorldGraph.m_cLinks; i++)
		{
			if (WorldGraph.m_pLinkPool[i].m_pLinkEnt == pev)
			{
				// if this link has a link ent which is the same ent that is removing itself, remove it!
				WorldGraph.m_pLinkPool[i].m_pLinkEnt = nullptr;
			}
		}
	}
	if (!FStringNull(pev->globalname))
		gGlobalState.EntitySetState(pev->globalname, GLOBAL_DEAD);
}

void CBaseEntity::SUB_Remove()
{
	UpdateOnRemove();
	if (pev->health > 0)
	{
		// this situation can screw up monsters who can't tell their entity pointers are invalid.
		pev->health = 0;
		Logger->debug("SUB_Remove called on entity \"{}\" ({}) with health > 0", STRING(pev->targetname), STRING(pev->classname));
	}

	REMOVE_ENTITY(edict());
}

BEGIN_DATAMAP(CBaseDelay)
DEFINE_FIELD(m_flDelay, FIELD_FLOAT),
	DEFINE_FIELD(m_iszKillTarget, FIELD_STRING),
	DEFINE_FUNCTION(DelayThink),
	END_DATAMAP();

bool CBaseDelay::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "delay"))
	{
		m_flDelay = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "killtarget"))
	{
		m_iszKillTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CBaseEntity::SUB_UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value)
{
	if (!FStringNull(pev->target))
	{
		FireTargets(STRING(pev->target), pActivator, this, useType, value);
	}
}

namespace subs
{
	const char* UseType( USE_TYPE UseType )
	{
		switch( UseType )
		{
			case USE_OFF:		return "USE_OFF (0)";
			case USE_ON:		return "USE_ON (1)";
			case USE_SET:		return "USE_SET (2)";
			case USE_TOGGLE:	return "USE_TOGGLE (3)";
			case USE_KILL:		return "USE_KILL (4)";
			case USE_SAME:		return "USE_SAME (5)";
			case USE_OPPOSITE:	return "USE_OPPOSITE (6)";
			case USE_TOUCH:		return "USE_TOUCH (7)";
			case USE_LOCK:		return "USE_LOCK (8)";
			case USE_UNLOCK:	return "USE_UNLOCK (9)";
		}
		return "USE_UNKNOWN";
	}

	const char* UseName( CBaseEntity* pEnt )
	{
		return ( pEnt->IsPlayer() ? STRING( pEnt->pev->netname ) :
					!FStringNull( pEnt->pev->targetname ) ? STRING( pEnt->pev->targetname ) :
						pEnt ? STRING( pEnt->pev->classname ) : "NULL" );
	}

	const char* UseLock( USE_VALUE value )
	{
		switch( value )
		{
			case USE_VALUE_LOCK_MASTER:	return "/Any/ (Master)";
			case USE_VALUE_LOCK_TOUCH:	return "Touch";
			case USE_VALUE_LOCK_USE:	return "Use";
		}
		return "USE_VALUE_UNKNOWN";
	}
}

void FireTargets(const char* targetName, CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!targetName)
		return;

	CBaseEntity::IOLogger->debug("Firing: ({})", targetName);

	const char* s2 = targetName;
	const char* s3 = subs::UseName( pActivator );
	const char* s4 = subs::UseName( pCaller );

	CBaseEntity* target = nullptr;

	while ((target = UTIL_FindEntityByTargetname(target, targetName, pActivator, pCaller)) != nullptr)
	{
		if (target && (target->pev->flags & FL_KILLME) == 0) // Don't use dying ents
		{
			const char* s1 = STRING( target->pev->classname );

			if( FBitSet( target->m_UseLocked, USE_VALUE_LOCK_USE ) && pCaller->m_UseType != USE_UNLOCK )
				continue; // Do this check in here or we won't be able to unlock :aaagaben:

			if( pCaller && pCaller->m_UseType > USE_UNSET && pCaller->m_UseType < USE_UNKNOWN )
			{
				target->m_UseTypeLast = pCaller->m_UseType; // Get the USE_TYPE that caller received.

				if( pCaller->m_UseType == USE_LOCK || pCaller->m_UseType == USE_UNLOCK )
				{
					USE_VALUE UseValue = static_cast<USE_VALUE>( (int)pCaller->m_UseValue );

					if( UseValue > 0 )
					{
						if( pCaller->m_UseType == USE_UNLOCK )
						{
							CBaseEntity::IOLogger->debug( "{} {}->{} Un-Locked. will receive calls now", s1, s2, subs::UseLock( UseValue ) );
							ClearBits( target->m_UseLocked, UseValue );
						}
						else
						{
							CBaseEntity::IOLogger->debug( "{} {}->{} Locked. won't receive any calls until get {}", s1, s2, subs::UseLock( UseValue ), subs::UseType( USE_UNLOCK ) );
							SetBits( target->m_UseLocked, UseValue );
						}
					}
				}
				else if( pCaller->m_UseType == USE_TOUCH && !FBitSet( target->m_UseLocked, USE_VALUE_LOCK_TOUCH ) )
				{
					CBaseEntity::IOLogger->debug( "{} {}->Touch( {} )", s1, s2, s3 );
					target->Touch( pActivator );
				}
				else if( pCaller->m_UseType == USE_KILL )
				{
					CBaseEntity::IOLogger->debug( "{} {}->UpdateOnRemove()", s1, s2 );
					UTIL_Remove( target );
				}
				else
				{
					// Get the USE_TYPE that caller received.
					if( pCaller->m_UseType == USE_SAME )
					{
						target->m_UseTypeLast = ( pCaller->m_UseTypeLast != USE_SAME && pCaller->m_UseTypeLast != USE_OPPOSITE ? pCaller->m_UseTypeLast : USE_TOGGLE );
						CBaseEntity::IOLogger->debug( "{} {}->Use( {}, {}, {} > {}, 0 )", s1, s2, s3, s4, subs::UseType( pCaller->m_UseType ), subs::UseType( target->m_UseTypeLast ) );
					}
					// Switch between USE_OFF and USE_ON, else just send USE_TOGGLE
					else if( pCaller->m_UseType == USE_OPPOSITE )
					{
						target->m_UseTypeLast = ( pCaller->m_UseTypeLast == USE_ON ? USE_OFF : pCaller->m_UseTypeLast == USE_OFF ? USE_ON : USE_TOGGLE );
						CBaseEntity::IOLogger->debug( "{} {}->Use( {}, {}, {} > {}, 0 )", s1, s2, s3, s4, subs::UseType( pCaller->m_UseType ), subs::UseType( target->m_UseTypeLast ) );
					}
					// Send the set float value.
					else if( pCaller->m_UseType == USE_SET )
					{
						if( pCaller->m_UseValue )
						{
							value = pCaller->m_UseValue;
						}
						// pass value to debug. -Mikk
						CBaseEntity::IOLogger->debug( "{} {}->Use( {}, {}, {}, {} )", s1, s2, s3, s4, subs::UseType( target->m_UseTypeLast ), value );
					}
					else
					{
						CBaseEntity::IOLogger->debug( "{} {}->Use( {}, {}, {}, 0 )", s1, s2, s3, s4, subs::UseType( target->m_UseTypeLast ) );
					}

					target->Use( pActivator, pCaller, target->m_UseTypeLast, value );
				}
			}
			else
			{
				CBaseEntity::IOLogger->debug( "{} {}->Use( {}, {}, {}, 0 )", s1, s2, s3, s4, subs::UseType( useType ) );
				target->Use( pActivator, pCaller, useType, value );
			}
		}
	}
}

LINK_ENTITY_TO_CLASS(delayed_use, CBaseDelay);

void CBaseDelay::SUB_UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value)
{
	//
	// exit immediatly if we don't have a target or kill target
	//
	if (FStringNull(pev->target) && FStringNull(m_iszKillTarget))
		return;

	//
	// check for a delay
	//
	if (m_flDelay != 0)
	{
		// create a temp object to fire at a later time
		CBaseDelay* pTemp = g_EntityDictionary->Create<CBaseDelay>("delayed_use");

		pTemp->pev->nextthink = gpGlobals->time + m_flDelay;

		pTemp->SetThink(&CBaseDelay::DelayThink);

		// Save the useType
		pTemp->m_UseType = m_UseType;
		pTemp->m_UseTypeLast = m_UseTypeLast;
		pTemp->m_UseValue = m_UseValue;
		pTemp->pev->button = (int)useType;
		pTemp->m_iszKillTarget = m_iszKillTarget;
		pTemp->m_flDelay = 0; // prevent "recursion"
		pTemp->pev->target = pev->target;
		pTemp->m_hActivator = pActivator;

		return;
	}

	//
	// kill the killtargets
	//

	if (!FStringNull(m_iszKillTarget))
	{
		CBaseEntity::IOLogger->debug("KillTarget: {}", STRING(m_iszKillTarget));

		CBaseEntity* killTarget = nullptr;

		while ((killTarget = UTIL_FindEntityByTargetname(killTarget, STRING(m_iszKillTarget), pActivator, this)) != nullptr)
		{
			if (UTIL_IsRemovableEntity(killTarget))
			{
				UTIL_Remove(killTarget);
				CBaseEntity::IOLogger->debug("killing {}", STRING(killTarget->pev->classname));
			}
			else
			{
				CBaseEntity::IOLogger->debug("Can't kill \"{}\": not allowed to remove entities of this type",
					STRING(killTarget->pev->classname));
			}
		}
	}

	//
	// fire targets
	//
	if (!FStringNull(pev->target))
	{
		FireTargets(GetTarget(), pActivator, this, useType, value);
	}
}

void CBaseDelay::DelayThink()
{
	// If a player activated this on delay
	CBaseEntity* pActivator = m_hActivator;

	// The use type is cached (and stashed) in pev->button
	SUB_UseTargets(pActivator, (USE_TYPE)pev->button, 0);
	REMOVE_ENTITY(edict());
}

BEGIN_DATAMAP(CBaseToggle)
DEFINE_FIELD(m_toggle_state, FIELD_INTEGER),
	DEFINE_FIELD(m_flActivateFinished, FIELD_TIME),
	DEFINE_FIELD(m_flMoveDistance, FIELD_FLOAT),
	DEFINE_FIELD(m_flWait, FIELD_FLOAT),
	DEFINE_FIELD(m_flLip, FIELD_FLOAT),
	DEFINE_FIELD(m_vecPosition1, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_vecPosition2, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_vecAngle1, FIELD_VECTOR), // UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD(m_vecAngle2, FIELD_VECTOR), // UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD(m_pfnCallWhenMoveDone, FIELD_FUNCTIONPOINTER),
	DEFINE_FIELD(m_vecFinalDest, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_vecFinalAngle, FIELD_VECTOR),
	DEFINE_FUNCTION(LinearMoveDone),
	DEFINE_FUNCTION(AngularMoveDone),
	END_DATAMAP();

bool CBaseToggle::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "lip"))
	{
		m_flLip = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "distance"))
	{
		m_flMoveDistance = atof(pkvd->szValue);
		return true;
	}

	return CBaseDelay::KeyValue(pkvd);
}

void CBaseToggle::LinearMove(Vector vecDest, float flSpeed)
{
	ASSERTSZ(flSpeed != 0, "LinearMove:  no speed is defined!");
	//	ASSERTSZ(m_pfnCallWhenMoveDone != nullptr, "LinearMove: no post-move function defined");

	m_vecFinalDest = vecDest;

	// Already there?
	if (vecDest == pev->origin)
	{
		LinearMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - pev->origin;

	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to LinearMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink(&CBaseToggle::LinearMoveDone);

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->velocity = vecDestDelta / flTravelTime;
}

void CBaseToggle::LinearMoveDone()
{
	Vector delta = m_vecFinalDest - pev->origin;
	float error = delta.Length();
	if (error > 0.03125)
	{
		LinearMove(m_vecFinalDest, 100);
		return;
	}

	SetOrigin(m_vecFinalDest);
	pev->velocity = g_vecZero;
	pev->nextthink = -1;
	if (m_pfnCallWhenMoveDone)
		(this->*m_pfnCallWhenMoveDone)();
}

void CBaseToggle::AngularMove(Vector vecDestAngle, float flSpeed)
{
	ASSERTSZ(flSpeed != 0, "AngularMove:  no speed is defined!");
	//	ASSERTSZ(m_pfnCallWhenMoveDone != nullptr, "AngularMove: no post-move function defined");

	m_vecFinalAngle = vecDestAngle;

	// Already there?
	if (vecDestAngle == pev->angles)
	{
		AngularMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDestAngle - pev->angles;

	// divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to AngularMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink(&CBaseToggle::AngularMoveDone);

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->avelocity = vecDestDelta / flTravelTime;
}

void CBaseToggle::AngularMoveDone()
{
	pev->angles = m_vecFinalAngle;
	pev->avelocity = g_vecZero;
	pev->nextthink = -1;
	if (m_pfnCallWhenMoveDone)
		(this->*m_pfnCallWhenMoveDone)();
}

float CBaseToggle::AxisValue(int flags, const Vector& angles)
{
	if (FBitSet(flags, SF_DOOR_ROTATE_Z))
		return angles.z;
	if (FBitSet(flags, SF_DOOR_ROTATE_X))
		return angles.x;

	return angles.y;
}

void CBaseToggle::AxisDir(CBaseEntity* entity)
{
	if (FBitSet(entity->pev->spawnflags, SF_DOOR_ROTATE_Z))
		entity->pev->movedir = Vector(0, 0, 1); // around z-axis
	else if (FBitSet(entity->pev->spawnflags, SF_DOOR_ROTATE_X))
		entity->pev->movedir = Vector(1, 0, 0); // around x-axis
	else
		entity->pev->movedir = Vector(0, 1, 0); // around y-axis
}

float CBaseToggle::AxisDelta(int flags, const Vector& angle1, const Vector& angle2)
{
	if (FBitSet(flags, SF_DOOR_ROTATE_Z))
		return angle1.z - angle2.z;

	if (FBitSet(flags, SF_DOOR_ROTATE_X))
		return angle1.x - angle2.x;

	return angle1.y - angle2.y;
}

/**
 *	@brief returns true if the passed entity is visible to caller, even if not infront ()
 */
bool FEntIsVisible(CBaseEntity* entity, CBaseEntity* target)
{
	Vector vecSpot1 = entity->pev->origin + entity->pev->view_ofs;
	Vector vecSpot2 = target->pev->origin + target->pev->view_ofs;
	TraceResult tr;

	UTIL_TraceLine(vecSpot1, vecSpot2, ignore_monsters, entity->edict(), &tr);

	if (0 != tr.fInOpen && 0 != tr.fInWater)
		return false; // sight line crossed contents

	if (tr.flFraction == 1)
		return true;

	return false;
}
