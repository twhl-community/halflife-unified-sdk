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
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "decals.h"

class CMortarShell : public CGrenade
{
public:
	int	Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	void Precache() override;
	void Spawn() override;

	void EXPORT BurnThink();

	void EXPORT FlyThink();

	void EXPORT MortarExplodeTouch( CBaseEntity* pOther );

	static CMortarShell* CreateMortarShell( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, int velocity );

	int m_iTrail;
	float m_flIgniteTime;
	int m_velocity;
	int m_iSoundedOff;
};

TYPEDESCRIPTION	CMortarShell::m_SaveData[] =
{
	DEFINE_FIELD( CMortarShell, m_velocity, FIELD_INTEGER ),
	DEFINE_FIELD( CMortarShell, m_flIgniteTime, FIELD_TIME ),
	DEFINE_FIELD( CMortarShell, m_iSoundedOff, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMortarShell, CGrenade );

LINK_ENTITY_TO_CLASS( mortar_shell, CMortarShell );

void CMortarShell::Precache()
{
	PRECACHE_MODEL( "models/mortarshell.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/wep_smoke_01.spr" );
	PRECACHE_SOUND( "weapons/gauss2.wav" );
	PRECACHE_SOUND( "weapons/ofmortar.wav" );
}

void CMortarShell::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( edict(), "models/mortarshell.mdl" );

	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "mortar_shell" );

	SetThink( &CMortarShell::BurnThink );
	SetTouch( &CMortarShell::MortarExplodeTouch );

	UTIL_MakeVectors( pev->angles );

	pev->velocity = -( gpGlobals->v_forward * m_velocity );
	pev->gravity = 1;

	//Deal twice the damage that the RPG does
	pev->dmg = 2 * gSkillData.plrDmgRPG;

	pev->nextthink = gpGlobals->time + 0.01;
	m_flIgniteTime  = gpGlobals->time;
	m_iSoundedOff = false;
}

void CMortarShell::BurnThink()
{
	pev->angles = UTIL_VecToAngles( pev->velocity );

	pev->angles.x -= 90;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_SPRITE_SPRAY );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_COORD( 0.0 );
	WRITE_COORD( 0.0 );
	WRITE_COORD( 1.0 );
	WRITE_SHORT( m_iTrail );
	WRITE_BYTE( 1 );
	WRITE_BYTE( 12 );
	WRITE_BYTE( 120 );
	MESSAGE_END();

	if( gpGlobals->time > m_flIgniteTime + 0.2 )
	{
		SetThink( &CMortarShell::FlyThink );
	}

	pev->nextthink = gpGlobals->time + 0.01;
}

void CMortarShell::FlyThink()
{
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.x -= 90.;

	if( pev->velocity.z < 20.0 && !m_iSoundedOff )
	{
		m_iSoundedOff = true;
		EMIT_SOUND( edict(), CHAN_VOICE, "weapons/ofmortar.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NONE );
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CMortarShell::MortarExplodeTouch( CBaseEntity* pOther )
{
	pev->enemy = pOther->edict();

	const auto direction = pev->velocity.Normalize();

	const auto vecSpot = pev->origin - direction * 32;

	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + direction * 64, ignore_monsters, edict(), &tr );

	pev->model = 0;

	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	if( tr.flFraction != 1.0 )
	{
		pev->origin = 0.6 * ( ( pev->dmg - 24 ) * tr.vecPlaneNormal ) + tr.vecEndPos;
	}

	const auto contents = UTIL_PointContents( pev->origin );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLOSION );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );

	if( contents == CONTENTS_WATER )
		g_engfuncs.pfnWriteShort( g_sModelIndexWExplosion );
	else
		g_engfuncs.pfnWriteShort( g_sModelIndexFireball );

	WRITE_BYTE( static_cast<int>( ( pev->dmg - 50.0 ) * 5.0 ) );
	WRITE_BYTE( 10 );
	WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 3.0 );

	auto pOwner = VARS( pev->owner );
	pev->owner = nullptr;

	RadiusDamage( pev, pOwner, pev->dmg, CLASS_NONE, 64 );

	if( RANDOM_FLOAT( 0, 1 ) >= 0.5 )
		UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
	else
		UTIL_DecalTrace( &tr, DECAL_SCORCH1 );

	//TODO: never used?
	//g_engfuncs.pfnRandomFloat( 0.0, 1.0 );

	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0: EMIT_SOUND( edict(), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM ); break;
	case 1: EMIT_SOUND( edict(), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM ); break;
	case 2: EMIT_SOUND( edict(), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM ); break;
	}

	pev->effects |= EF_NODRAW;

	SetThink( &CMortarShell::Smoke );

	pev->velocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.3;

	if( contents != CONTENTS_WATER )
	{
		const auto sparkCount = g_engfuncs.pfnRandomLong( 0, 3 );

		for( auto i = 0; i < sparkCount; ++i )
		{
			CBaseEntity::Create( "spark_shower", pev->origin, tr.vecPlaneNormal );
		}
	}
}

CMortarShell* CMortarShell::CreateMortarShell( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, int velocity )
{
	auto pShell = GetClassPtr<CMortarShell>( nullptr );

	UTIL_SetOrigin( pShell->pev, vecOrigin );

	pShell->pev->angles = vecAngles;
	pShell->m_velocity = velocity;

	pShell->Spawn();

	pShell->pev->owner = pOwner->edict();

	return pShell;
}

const auto SF_MORTAR_ACTIVE = 1 << 0;
const auto SF_MORTAR_LINE_OF_SIGHT = 1 << 4;
const auto SF_MORTAR_CONTROLLABLE = 1 << 5;

class COp4Mortar : public CBaseMonster
{
public:
	int	Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	void KeyValue( KeyValueData* pkvd ) override;

	void Precache() override;

	void Spawn() override;

	int ObjectCaps() override { return 0; }

	void EXPORT MortarThink();

	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) override;

	void PlaySound();

	void Off();

	void AIUpdatePosition();

	CBaseEntity* FindTarget();

	void UpdatePosition( int direction, int controller );

	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;

	int d_x;
	int d_y;
	float m_lastupdate;
	int m_playsound;
	int m_updated;
	int m_direction;
	Vector m_start;
	Vector m_end;
	int m_velocity;
	int m_hmin;
	int m_hmax;
	float m_fireLast;
	float m_maxRange;
	float m_minRange;
	int m_iEnemyType;
	float m_fireDelay;
	float m_trackDelay;
	BOOL m_tracking;
	float m_zeroYaw;
	Vector m_vGunAngle;
	Vector m_vIdealGunVector;
	Vector m_vIdealGunAngle;
};

TYPEDESCRIPTION	COp4Mortar::m_SaveData[] =
{
	DEFINE_FIELD( COp4Mortar, d_x, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, d_y, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_lastupdate, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_playsound, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_updated, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_direction, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_start, FIELD_VECTOR ),
	DEFINE_FIELD( COp4Mortar, m_end, FIELD_VECTOR ),
	DEFINE_FIELD( COp4Mortar, m_velocity, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_hmin, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_hmax, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_fireLast, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_maxRange, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_minRange, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_iEnemyType, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_fireDelay, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_trackDelay, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_tracking, FIELD_BOOLEAN ),
	DEFINE_FIELD( COp4Mortar, m_zeroYaw, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_vGunAngle, FIELD_VECTOR ),
	DEFINE_FIELD( COp4Mortar, m_vIdealGunVector, FIELD_VECTOR ),
	DEFINE_FIELD( COp4Mortar, m_vIdealGunAngle, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( COp4Mortar, CBaseMonster );

LINK_ENTITY_TO_CLASS( op4mortar, COp4Mortar );

void COp4Mortar::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( "h_max", pkvd->szKeyName ) )
	{
		m_hmax = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "h_min", pkvd->szKeyName ) )
	{
		m_hmin = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "mortar_velocity", pkvd->szKeyName ) )
	{
		m_velocity = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "mindist", pkvd->szKeyName ) )
	{
		m_minRange = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "maxdist", pkvd->szKeyName ) )
	{
		m_maxRange = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "enemytype", pkvd->szKeyName ) )
	{
		m_iEnemyType = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( "firedelay", pkvd->szKeyName ) )
	{
		m_fireDelay = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
	{
		CBaseToggle::KeyValue( pkvd );
	}
}

void COp4Mortar::Precache()
{
	PRECACHE_MODEL( "models/mortar.mdl" );
	UTIL_PrecacheOther( "mortar_shell" );
	PRECACHE_SOUND( "weapons/mortarhit.wav" );
	PRECACHE_SOUND( "player/pl_grate1.wav" );
}

void COp4Mortar::Spawn()
{
	Precache();

	UTIL_SetOrigin( pev, pev->origin );

	SET_MODEL( edict(), "models/mortar.mdl" );

	pev->health = 1;
	pev->sequence = LookupSequence( "idle" );

	ResetSequenceInfo();

	pev->frame = 0;
	pev->framerate = 1;

	m_tracking = false;

	if( m_fireDelay < 0.5 )
		m_fireDelay = 5;

	if( m_minRange == 0 )
		m_minRange = 128;

	if( m_maxRange == 0 )
		m_maxRange = 2048;

	InitBoneControllers();

	m_vGunAngle = g_vecZero;

	m_lastupdate = gpGlobals->time;

	m_zeroYaw = UTIL_AngleMod( pev->angles.y + 180.0 );

	m_fireLast = gpGlobals->time;
	m_trackDelay = gpGlobals->time;

	m_hEnemy = nullptr;

	pev->nextthink = gpGlobals->time + 0.01;
	SetThink( &COp4Mortar::MortarThink );
}

void COp4Mortar::MortarThink()
{
	const auto flInterval = StudioFrameAdvance();

	if( m_fSequenceFinished )
	{
		if( pev->sequence != LookupSequence( "idle" ) )
		{
			pev->frame = 0;
			pev->sequence = LookupSequence( "idle" );
			ResetSequenceInfo();
		}
	}

	DispatchAnimEvents( flInterval );

	UpdateShockEffect();

	pev->nextthink = gpGlobals->time + 0.1;

	if( pev->spawnflags & SF_MORTAR_ACTIVE )
	{
		if( !m_hEnemy )
		{
			m_hEnemy = FindTarget();
		}

		auto pEnemy = m_hEnemy.Entity<CBaseEntity>();

		if( pEnemy )
		{
			const auto distance = ( pEnemy->pev->origin - pev->origin ).Length();

			if( pEnemy->IsAlive() && m_minRange <= distance && distance <= m_maxRange )
			{
				if( gpGlobals->time - m_trackDelay > 0.5 )
				{
					Vector vecPos, vecAngle;
					GetAttachment( 0, vecPos, vecAngle );

					m_vIdealGunVector = VecCheckThrow( pev, vecPos, pEnemy->pev->origin, m_velocity / 2 );

					m_vIdealGunAngle = UTIL_VecToAngles( m_vIdealGunVector );

					m_trackDelay = gpGlobals->time;
				}

				AIUpdatePosition();

				const auto idealDistance = m_vIdealGunVector.Length();

				if( idealDistance > 1.0 )
				{
					if( gpGlobals->time - m_fireLast > m_fireDelay )
					{
						EMIT_SOUND( edict(), CHAN_VOICE, "weapons/mortarhit.wav", VOL_NORM, ATTN_NORM );
						UTIL_ScreenShake( pev->origin, 12.0, 100.0, 2.0, 1000.0 );

						Vector vecPos, vecAngle;
						GetAttachment( 0, vecPos, vecAngle );

						vecAngle = m_vGunAngle;

						vecAngle.y = UTIL_AngleMod( pev->angles.y + m_vGunAngle.y );

						if( CMortarShell::CreateMortarShell( vecPos, vecAngle, this, idealDistance ) )
						{
							pev->sequence = LookupSequence( "fire" );
							pev->frame = 0;
							ResetSequenceInfo();
						}

						m_fireLast = gpGlobals->time;
					}
				}
				else
				{
					m_fireLast = gpGlobals->time;
				}
			}
			else
			{
				m_hEnemy = nullptr;
			}
		}
	}
}

int COp4Mortar::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	//Ignore all damage
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, 0, bitsDamageType );
}

void COp4Mortar::PlaySound()
{
	EMIT_SOUND( edict(), CHAN_VOICE, "player/pl_grate1.wav", VOL_NORM, ATTN_NORM );
}

void COp4Mortar::Off()
{
	m_playsound = true;
}

void COp4Mortar::AIUpdatePosition()
{
	if( fabs( m_vGunAngle.x - m_vIdealGunAngle.x ) >= 3.0 )
	{
		const auto angle = UTIL_AngleDiff( m_vGunAngle.x, m_vIdealGunAngle.x );

		if( angle != 0 )
		{
			const auto absolute = fabs( angle );
			if( absolute <= 3.0 )
				d_x = static_cast<int>( -absolute );
			else
				d_x = angle > 0 ? -3 : 3;
		}
	}

	const auto yawAngle = UTIL_AngleMod( m_zeroYaw + m_vGunAngle.y );

	if( fabs( yawAngle - m_vIdealGunAngle.y ) >= 3.0 )
	{
		const auto angle = UTIL_AngleDiff( yawAngle, m_vIdealGunAngle.y );

		if( angle != 0 )
		{
			const auto absolute = fabs( angle );
			if( absolute <= 3.0 )
				d_y = static_cast<int>( -absolute );
			else
				d_y = angle > 0 ? -3 : 3;
		}
	}

	m_vGunAngle.x += d_x;
	m_vGunAngle.y += d_y;

	if( m_hmin > m_vGunAngle.y )
	{
		m_vGunAngle.y = m_hmin;
		d_y = 0;
	}

	if( m_vGunAngle.y > m_hmax )
	{
		m_vGunAngle.y = m_hmax;
		d_y = 0;
	}

	if( m_vGunAngle.x < 10.0 )
	{
		m_vGunAngle.x = 10.0;
		d_x = 0;
	}
	else if( m_vGunAngle.x > 90.0 )
	{
		m_vGunAngle.x = 90.0;
		d_x = 0;
	}

	if( d_x || d_y )
	{
		PlaySound();
	}

	SetBoneController( 0, m_vGunAngle.x );
	SetBoneController( 1, m_vGunAngle.y );

	d_y = 0;
	d_x = 0;
}

CBaseEntity* COp4Mortar::FindTarget()
{
	auto pPlayerTarget = UTIL_FindEntityByClassname( nullptr, "player" );

	if( !pPlayerTarget )
		return pPlayerTarget;

	m_pLink = nullptr;

	CBaseEntity *pIdealTarget = nullptr;
	auto flIdealDist = m_maxRange;

	Vector barrelEnd, barrelAngle;
	GetAttachment( 0, barrelEnd, barrelAngle );

	if( pPlayerTarget->IsAlive() )
	{
		const auto distance = ( pPlayerTarget->pev->origin - pev->origin ).Length();

		if( distance >= m_minRange && m_maxRange >= distance )
		{
			TraceResult tr;
			UTIL_TraceLine( barrelEnd, pPlayerTarget->pev->origin + pPlayerTarget->pev->view_ofs, dont_ignore_monsters, edict(), &tr );

			if( !( pev->spawnflags & SF_MORTAR_LINE_OF_SIGHT ) || tr.pHit == pPlayerTarget->pev->pContainingEntity )
			{
				if( !m_iEnemyType )
					return pPlayerTarget;

				flIdealDist = distance;
				pIdealTarget = pPlayerTarget;
			}
		}
	}

	const Vector maxRange{ m_maxRange, m_maxRange, m_maxRange };

	CBaseEntity *pList[ 100 ];
	const auto count = UTIL_EntitiesInBox( pList, ARRAYSIZE( pList ), pev->origin - maxRange, pev->origin + maxRange, FL_MONSTER | FL_CLIENT );

	for( auto i = 0; i < count; ++i )
	{
		auto pEntity = pList[ i ];

		if( this == pEntity )
			continue;

		if( pEntity->pev->spawnflags & SF_MONSTER_PRISONER )
			continue;

		if( pEntity->pev->health <= 0 )
			continue;

		auto pMonster = pEntity->MyMonsterPointer();

		if( !pMonster )
			continue;

		if( pMonster->IRelationship( pPlayerTarget ) != R_AL )
			continue;

		if( pEntity->pev->flags & FL_NOTARGET )
			continue;

		if( !FVisible( pEntity ) )
			continue;

		if( pEntity->IsPlayer() && ( pev->spawnflags & SF_MORTAR_ACTIVE ) )
		{
			if( pMonster->FInViewCone( this ) )
			{
				pev->spawnflags &= ~SF_MORTAR_ACTIVE;
			}
		}
	}

	for( auto pEntity = m_pLink; pEntity; pEntity = pEntity->m_pLink )
	{
		const auto distance = ( pEntity->pev->origin - pev->origin ).Length();

		if( distance >= m_minRange && m_maxRange >= distance && ( !pIdealTarget || flIdealDist > distance ) )
		{
			TraceResult tr;
			UTIL_TraceLine( barrelEnd, pEntity->pev->origin + pEntity->pev->view_ofs, dont_ignore_monsters, edict(), &tr );

			if( pev->spawnflags & SF_MORTAR_LINE_OF_SIGHT )
			{
				if( tr.pHit == pEntity->edict() )
				{
					flIdealDist = distance;
				}
				if( tr.pHit == pEntity->edict() )
					pIdealTarget = pEntity;
			}
			else
			{
				flIdealDist = distance;
				pIdealTarget = pEntity;
			}
		}
	}

	return pIdealTarget;
}

void COp4Mortar::UpdatePosition( int direction, int controller )
{
	if( gpGlobals->time - m_lastupdate >= 0.09 )
	{
		switch( controller )
		{
		case 0:
			d_x = 3 * direction;
			break;

		case 1:
			d_y = 3 * direction;
			break;
		}

		m_vGunAngle.x = d_x + m_vGunAngle.x;
		m_vGunAngle.y = d_y + m_vGunAngle.y;

		if( m_hmin > m_vGunAngle.y )
		{
			m_vGunAngle.y = m_hmin;
			d_y = 0;
		}

		if( m_vGunAngle.y > m_hmax )
		{
			m_vGunAngle.y = m_hmax;
			d_y = 0;
		}

		if( m_vGunAngle.x < 10 )
		{
			m_vGunAngle.x = 10;
			d_x = 0;
		}
		else if( m_vGunAngle.x > 90 )
		{
			m_vGunAngle.x = 90;
			d_x = 0;
		}

		if( d_x || d_y )
		{
			PlaySound();
		}

		SetBoneController( 0, m_vGunAngle.x );
		SetBoneController( 1, m_vGunAngle.y );

		d_x = 0;
		d_y = 0;
			
		m_lastupdate = gpGlobals->time;
	}
}

void COp4Mortar::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if( useType == USE_TOGGLE && ( !pActivator || pActivator->Classify() == CLASS_PLAYER ) )
	{
		if( !( pev->spawnflags & SF_MORTAR_ACTIVE ) && ( pev->spawnflags & SF_MORTAR_CONTROLLABLE ) )
		{
			//Player fired a mortar
			EMIT_SOUND( edict(), CHAN_VOICE, "weapons/mortarhit.wav", VOL_NORM, ATTN_NONE );
			UTIL_ScreenShake( pev->origin, 12.0, 100.0, 2.0, 1000.0 );

			Vector pos, angle;
			GetAttachment( 0, pos, angle );

			angle = m_vGunAngle;

			angle.y = UTIL_AngleMod( pev->angles.y + m_vGunAngle.y );

			if( CMortarShell::CreateMortarShell( pos, angle, pActivator ? pActivator : this, m_velocity ) )
			{
				pev->sequence = LookupSequence( "fire" );
				pev->frame = 0;
				ResetSequenceInfo();
			}
			return;
		}
	}

	//Toggle AI active state
	if( ShouldToggle( useType, ( pev->spawnflags & SF_MORTAR_ACTIVE ) != 0 ) )
	{
		pev->spawnflags ^= SF_MORTAR_ACTIVE;

		m_fireLast = 0;
		m_hEnemy = nullptr;
	}
}

class COp4MortarController : public CBaseToggle
{
public:
	int	Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	int ObjectCaps() override { return FCAP_CONTINUOUS_USE; }

	void KeyValue( KeyValueData* pkvd ) override;

	void Spawn() override;

	void EXPORT Reverse();

	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;

	int m_direction;
	int m_controller;
	float m_lastpush;
};

TYPEDESCRIPTION	COp4MortarController::m_SaveData[] =
{
	DEFINE_FIELD( COp4MortarController, m_direction, FIELD_INTEGER ),
	DEFINE_FIELD( COp4MortarController, m_controller, FIELD_INTEGER ),
	DEFINE_FIELD( COp4MortarController, m_lastpush, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( COp4MortarController, CBaseToggle );

LINK_ENTITY_TO_CLASS( func_op4mortarcontroller, COp4MortarController );

void COp4MortarController::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( "mortar_axis", pkvd->szKeyName ) )
	{
		m_controller = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
	{
		CBaseToggle::KeyValue( pkvd );
	}
}

void COp4MortarController::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin( pev, pev->origin );

	SET_MODEL( edict(), STRING( pev->model ) );

	m_direction = 1;
	m_lastpush = gpGlobals->time;
}

void COp4MortarController::Reverse()
{
	m_direction = -m_direction;
	SetThink( nullptr );
}

void COp4MortarController::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if( gpGlobals->time - m_lastpush > 0.5 )
		m_direction = -m_direction;

	for( auto pEntity : UTIL_FindEntitiesByTargetname<COp4Mortar>( STRING( pev->target ) ) )
	{
		if( FClassnameIs( pEntity->pev, "op4mortar" ) )
		{
			pEntity->UpdatePosition( m_direction, m_controller );
		}
	}

	m_lastpush = gpGlobals->time;
}
