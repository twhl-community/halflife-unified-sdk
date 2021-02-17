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
#include "weapons.h"
#include "player.h"
#include "effects.h"
#include "skill.h"
#include "decals.h"
#include "gamerules.h"
#include "ctf/CTFDefs.h"
#include "ctf/CTFGoal.h"
#include "ctf/CTFGoalFlag.h"
#include "UserMessages.h"

#include "CDisplacerBall.h"

extern CBaseEntity* g_pLastSpawn;

namespace
{
//TODO: can probably be smarter - Solokiller
const char* const displace[] = 
{
	"monster_bloater",
	"monster_snark",
	"monster_shockroach",
	"monster_rat",
	"monster_alien_babyvoltigore",
	"monster_babycrab",
	"monster_cockroach",
	"monster_flyer_flock",
	"monster_headcrab",
	"monster_leech",
	"monster_alien_controller",
	"monster_alien_slave",
	"monster_barney",
	"monster_bullchicken",
	"monster_cleansuit_scientist",
	"monster_houndeye",
	"monster_human_assassin",
	"monster_human_grunt",
	"monster_human_grunt_ally",
	"monster_human_medic_ally",
	"monster_human_torch_ally",
	"monster_male_assassin",
	"monster_otis",
	"monster_pitdrone",
	"monster_scientist",
	"monster_zombie",
	"monster_zombie_barney",
	"monster_zombie_soldier",
	"monster_alien_grunt",
	"monster_alien_voltigore",
	"monster_assassin_repel",
	"monster_grunt_ally_repel",
	"monster_gonome",
	"monster_grunt_repel",
	"monster_ichthyosaur",
	"monster_shocktrooper"
};
}

LINK_ENTITY_TO_CLASS( displacer_ball, CDisplacerBall );

void CDisplacerBall::Precache()
{
	PRECACHE_MODEL( "sprites/exit1.spr" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	m_iTrail = PRECACHE_MODEL( "sprites/disp_ring.spr" );

	PRECACHE_SOUND( "weapons/displacer_impact.wav" );
	PRECACHE_SOUND( "weapons/displacer_teleport.wav" );
}

void CDisplacerBall::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL( edict(), "sprites/exit1.spr" );

	UTIL_SetOrigin( pev, pev->origin );

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	pev->rendermode = kRenderTransAdd;

	pev->renderamt = 255;

	pev->scale = 0.75;

	SetTouch( &CDisplacerBall::BallTouch );
	SetThink( &CDisplacerBall::FlyThink );

	pev->nextthink = gpGlobals->time + 0.2;

	InitBeams();
}

int CDisplacerBall::Classify()
{
	return CLASS_NONE;
}

void CDisplacerBall::BallTouch( CBaseEntity* pOther )
{
	pev->velocity = g_vecZero;

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD_VECTOR( pev->origin );			// coord coord coord (center position) 
		WRITE_COORD( pev->origin.x );				// coord coord coord (axis and radius) 
		WRITE_COORD( pev->origin.y );			
		WRITE_COORD( pev->origin.z + 800.0 );	
		WRITE_SHORT( m_iTrail );					 // short (sprite index) 
		WRITE_BYTE( 0 );							 // byte (starting frame) 
		WRITE_BYTE( 0 );							 // byte (frame rate in 0.1's) 
		WRITE_BYTE( 3 );							 // byte (life in 0.1's) 
		WRITE_BYTE( 16 );							 // byte (line width in 0.1's) 
		WRITE_BYTE( 0 );							 // byte (noise amplitude in 0.01's) 
		WRITE_BYTE( 255 );							 // byte,byte,byte (color)
		WRITE_BYTE( 255 );							 
		WRITE_BYTE( 255 );							 
		WRITE_BYTE( 255 );							 // byte (brightness)
		WRITE_BYTE( 0 );							 // byte (scroll speed in 0.1's)
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD_VECTOR( pev->origin );			// coord, coord, coord (pos) 
		WRITE_BYTE( 16 );							// byte (radius in 10's) 
		WRITE_BYTE( 255 );							// byte byte byte (color)
		WRITE_BYTE( 180 );							
		WRITE_BYTE( 96 );							
		WRITE_BYTE( 10 );							// byte (brightness)
		WRITE_BYTE( 10 );							// byte (life in 10's)
	MESSAGE_END();

	m_hDisplacedTarget = nullptr;

	SetTouch( nullptr );
	SetThink( nullptr );

	TraceResult tr;

	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, edict(), &tr );

	UTIL_DecalTrace( &tr, DECAL_SCORCH1 + RANDOM_LONG( 0, 1 ) );

	if( pOther->IsPlayer() )
	{
		CBasePlayer* pPlayer = static_cast<CBasePlayer*>( pOther );

		//TODO: what is this for? - Solokiller
		pPlayer->pev->flags = FL_CLIENT;
		pPlayer->m_flFallVelocity = 0;

		if( g_pGameRules->IsCTF() && pPlayer->m_pFlag )
		{
			auto pFlag = static_cast<CTFGoalFlag*>(static_cast<CBaseEntity*>(pPlayer->m_pFlag));

			pFlag->DropFlag(pPlayer);

			auto pOwner = CBaseEntity::Instance(pev->owner);

			if(pOwner->IsPlayer())
			{
				auto pOwnerPlayer = static_cast<CBasePlayer*>(pOwner);

				if (pOwnerPlayer->m_iTeamNum != pPlayer->m_iTeamNum)
				{
					MESSAGE_BEGIN(MSG_ALL, gmsgCTFScore);
					WRITE_BYTE(pPlayer->entindex());
					WRITE_BYTE(pPlayer->m_iCTFScore);
					MESSAGE_END();

					ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#CTFScorePoint");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s", STRING(pPlayer->pev->netname)));

					if (pPlayer->m_iTeamNum == CTFTeam::BlackMesa)
					{
						UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagDisplacedBM");
					}
					else if (pPlayer->m_iTeamNum == CTFTeam::OpposingForce)
					{
						UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagDisplacedOF");
					}
				}
			}
		}

		auto pSpawnSpot = VARS( g_pGameRules->GetPlayerSpawnSpot( pPlayer ) );

		Vector vecEnd = pSpawnSpot->origin;

		vecEnd.z -= 100;

		UTIL_TraceLine( pSpawnSpot->origin, pSpawnSpot->origin - Vector( 0, 0, 100 ), ignore_monsters, edict(), &tr );
	
		UTIL_SetOrigin( pPlayer->pev, tr.vecEndPos + Vector( 0, 0, 37 ) );

		pPlayer->pev->sequence = pPlayer->LookupActivity( ACT_IDLE );

		pPlayer->SetIsClimbing( false );

		pPlayer->m_lastx = 0;
		pPlayer->m_lasty = 0;

		pPlayer->m_fNoPlayerSound = false;
	}

	if( ClassifyTarget( pOther ) )
	{
		tr = UTIL_GetGlobalTrace();

		ClearMultiDamage();

		m_hDisplacedTarget = pOther;

		pOther->Killed( pev, GIB_NEVER );
	}

	pev->basevelocity = g_vecZero;

	pev->velocity = g_vecZero;

	pev->solid = SOLID_NOT;

	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CDisplacerBall::KillThink );

	pev->nextthink = gpGlobals->time + ( g_pGameRules->IsMultiplayer() ? 0.2 : 0.5 );
}

void CDisplacerBall::FlyThink()
{
	ArmBeam( -1 );
	ArmBeam( 1 );
	pev->nextthink = gpGlobals->time + 0.05;
}

void CDisplacerBall::FlyThink2()
{
	UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );

	ArmBeam( -1 );
	ArmBeam( 1 );

	pev->nextthink = gpGlobals->time + 0.05;
}

void CDisplacerBall::FizzleThink()
{
	ClearBeams();

	pev->dmg = gSkillData.plrDmgDisplacerOther;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD_VECTOR( pev->origin );
		WRITE_BYTE( 16 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 180 );
		WRITE_BYTE( 96 );
		WRITE_BYTE( 10 );
		WRITE_BYTE( 10 );
	MESSAGE_END();

	auto pOwner = VARS( pev->owner );

	pev->owner = nullptr;
	
	RadiusDamage( pev->origin, pev, pOwner, pev->dmg, 128.0, CLASS_NONE, DMG_ALWAYSGIB | DMG_BLAST );

	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/displacer_impact.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NORM );

	UTIL_Remove( this );
}

void CDisplacerBall::ExplodeThink()
{
	ClearBeams();

	pev->dmg = gSkillData.plrDmgDisplacerOther;

	auto pOwner = VARS( pev->owner );

	pev->owner = nullptr;

	RadiusDamage( pev->origin, pev, pOwner, pev->dmg, gSkillData.plrRadiusDisplacer, CLASS_NONE, DMG_ALWAYSGIB | DMG_BLAST );

	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/displacer_teleport.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NORM );

	UTIL_Remove( this );
}

void CDisplacerBall::KillThink()
{
	if( CBaseEntity* pTarget = m_hDisplacedTarget )
	{
		pTarget->SetThink( &CBaseEntity::SUB_Remove );

		//TODO: no next think? - Solokiller
	}

	SetThink( &CDisplacerBall::ExplodeThink );
	pev->nextthink = gpGlobals->time + 0.2;
}

void CDisplacerBall::InitBeams()
{
	memset( m_pBeam, 0, sizeof( m_pBeam ) );

	m_uiBeams = 0;

	pev->skin = 0;
}

void CDisplacerBall::ClearBeams()
{
	for( auto& pBeam : m_pBeam )
	{
		if( pBeam )
		{
			UTIL_Remove( pBeam );
			pBeam = nullptr;
		}
	}

	m_uiBeams = 0;

	pev->skin = 0;
}

void CDisplacerBall::ArmBeam( int iSide )
{
	//This method is identical to the Alien Slave's ArmBeam, except it treats m_pBeam as a circular buffer.
	if( m_uiBeams >= NUM_BEAMS )
		m_uiBeams = 0;

	TraceResult tr;
	float flDist = 1.0;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * iSide * 16 + gpGlobals->v_forward * 32;

	for( int i = 0; i < 3; i++ )
	{
		Vector vecAim = gpGlobals->v_right * iSide * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1 );
		if( flDist > tr1.flFraction )
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if( flDist == 1.0 )
		return;

	//The beam might already exist if we've created all beams before. - Solokiller
	if( !m_pBeam[ m_uiBeams ] )
		m_pBeam[ m_uiBeams ] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if( !m_pBeam[ m_uiBeams ] )
		return;

	auto pHit = Instance( tr.pHit );

	if( pHit && pHit->pev->takedamage != DAMAGE_NO )
	{
		//Beam hit something, deal radius damage to it. - Solokiller
		m_pBeam[ m_uiBeams ]->EntsInit( pHit->entindex(), entindex() );

		m_pBeam[ m_uiBeams ]->SetColor( 255, 255, 255 );

		m_pBeam[ m_uiBeams ]->SetBrightness( 255 );

		RadiusDamage( tr.vecEndPos, pev, VARS( pev->owner ), 25, 15, CLASS_NONE, DMG_ENERGYBEAM );
	}
	else
	{
		m_pBeam[ m_uiBeams ]->PointEntInit( tr.vecEndPos, entindex() );
		m_pBeam[ m_uiBeams ]->SetEndAttachment( iSide < 0 ? 2 : 1 );
		// m_pBeam[ m_uiBeams ]->SetColor( 180, 255, 96 );
		m_pBeam[ m_uiBeams ]->SetColor( 96, 128, 16 );
		m_pBeam[ m_uiBeams ]->SetBrightness( 255 );
		m_pBeam[ m_uiBeams ]->SetNoise( 80 );
	}

	++m_uiBeams;
}

bool CDisplacerBall::ClassifyTarget( CBaseEntity* pTarget )
{
	if( !pTarget || pTarget->IsPlayer() )
		return false;

	for( size_t uiIndex = 0; uiIndex < ARRAYSIZE( displace ); ++uiIndex )
	{
		if( strcmp( STRING( pTarget->pev->classname ), displace[ uiIndex ] ) == 0 )
			return true;
	}

	return false;
}

CDisplacerBall* CDisplacerBall::CreateDisplacerBall( const Vector& vecOrigin, const Vector& vecAngles, CBaseEntity* pOwner )
{
	auto pBall = GetClassPtr<CDisplacerBall>( nullptr );

	pBall->pev->classname = MAKE_STRING( "displacer_ball" );

	UTIL_SetOrigin( pBall->pev, vecOrigin );

	Vector vecNewAngles = vecAngles;

	vecNewAngles.x = -vecNewAngles.x;

	pBall->pev->angles = vecNewAngles;

	UTIL_MakeVectors( vecAngles );

	pBall->pev->velocity = gpGlobals->v_forward * 500;

	pBall->pev->owner = pOwner->edict();

	pBall->Spawn();

	return pBall;
}
