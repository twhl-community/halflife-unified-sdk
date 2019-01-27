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
#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "CTFSpawn.h"

const char* const sTeamSpawnNames[] = 
{
	"ctfs0",
	"ctfs1",
	"ctfs2",
};

LINK_ENTITY_TO_CLASS( info_ctfspawn, CTFSpawn );

void CTFSpawn::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( "team_no", pkvd->szKeyName ) )
	{
		team_no = static_cast<CTFTeam>( atoi( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "master", pkvd->szKeyName ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
	{
		CBaseEntity::KeyValue( pkvd );
	}
}

void CTFSpawn::Spawn()
{
	if( team_no < CTFTeam::None || team_no > CTFTeam::OpposingForce )
	{
		ALERT( at_console, "Teamspawnpoint with an invalid team_no of %d\n", team_no );
		return;
	}

	pev->netname = pev->classname;

	//Change the classname to the owning team's spawn name
	pev->classname = MAKE_STRING( sTeamSpawnNames[ static_cast<int>( team_no ) ] );
	m_fState = true;
}

BOOL CTFSpawn::IsTriggered( CBaseEntity* pEntity )
{
	if( !FStringNull( pev->targetname ) && STRING( pev->targetname ) )
		return m_fState;
	else
		return UTIL_IsMasterTriggered( pev->netname, pEntity );
}
