/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"

const auto SF_ITEMGENERIC_DROP_TO_FLOOR = 1 << 0;

class CGenericItem : public CBaseAnimating
{
public:
	void KeyValue( KeyValueData* pkvd ) override;
	void Precache() override;
	void Spawn() override;

	void EXPORT StartItem();
	void EXPORT AnimateThink();

	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;

	float m_lastTime;
	int m_iSequence;
};

LINK_ENTITY_TO_CLASS( item_generic, CGenericItem );

void CGenericItem::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( "sequencename", pkvd->szKeyName ) )
	{
		m_iSequence = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "skin", pkvd->szKeyName ) )
	{
		pev->skin = static_cast<int>( atof( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "body", pkvd->szKeyName ) )
	{
		pev->body = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

void CGenericItem::Precache()
{
	PRECACHE_MODEL( const_cast<char*>( STRING( pev->model ) ) );
}

void CGenericItem::Spawn()
{
	pev->solid = 0;
	pev->movetype = 0;
	pev->effects = 0;
	pev->frame = 0;

	Precache();

	SET_MODEL( edict(), STRING( pev->model ) );

	if( m_iSequence )
	{
		SetThink( &CGenericItem::StartItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	if( pev->spawnflags & SF_ITEMGENERIC_DROP_TO_FLOOR )
	{
		if( !g_engfuncs.pfnDropToFloor( pev->pContainingEntity ) )
		{
			ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z );
			UTIL_Remove( this );
		}
	}
}

void CGenericItem::StartItem()
{
	pev->effects = 0;

	pev->sequence = LookupSequence( STRING( m_iSequence ) );
	pev->frame = 0;
	ResetSequenceInfo();

	SetThink( &CGenericItem::AnimateThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->frame = 0;
}

void CGenericItem::AnimateThink()
{
	DispatchAnimEvents();
	StudioFrameAdvance();

	if( m_fSequenceFinished && !m_fSequenceLoops )
	{
		pev->frame = 0;
		ResetSequenceInfo();
	}

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CGenericItem::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGenericItem::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}
