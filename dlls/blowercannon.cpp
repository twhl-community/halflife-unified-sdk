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
#include "weapons.h"

#include "weapons/CDisplacerBall.h"
#include "weapons/CShockBeam.h"
#include "weapons/CSpore.h"

class CBlowerCannon : public CBaseToggle
{
private:
	enum class WeaponType
	{
		SporeRocket = 1,
		SporeGrenade,
		ShockBeam,
		DisplacerBall
	};

	enum class FireType
	{
		Toggle = 1,
		FireOnTrigger,
	};

public:
	using BaseClass = CBaseToggle;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void KeyValue( KeyValueData* pkvd ) override;

	void Precache() override;
	void Spawn() override;

	void EXPORT BlowerCannonStart( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT BlowerCannonStop( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT BlowerCannonThink();

	//TODO: probably shadowing CBaseDelay
	float m_flDelay;
	int m_iZOffset;
	WeaponType m_iWeaponType;
	FireType m_iFireType;
};

TYPEDESCRIPTION	CBlowerCannon::m_SaveData[] =
{
	DEFINE_FIELD( CBlowerCannon, m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CBlowerCannon, m_iZOffset, FIELD_INTEGER ),
	DEFINE_FIELD( CBlowerCannon, m_iWeaponType, FIELD_INTEGER ),
	DEFINE_FIELD( CBlowerCannon, m_iFireType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBlowerCannon, CBlowerCannon::BaseClass );

LINK_ENTITY_TO_CLASS( env_blowercannon, CBlowerCannon );

void CBlowerCannon::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( pkvd->szKeyName, "delay" ) )
	{
		m_flDelay = atof( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "weaptype" ) )
	{
		m_iWeaponType = static_cast<WeaponType>( atoi( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "firetype" ) )
	{
		m_iFireType = static_cast<FireType>( atoi( pkvd->szValue ) );
		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "zoffset" ) )
	{
		m_iZOffset = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	else
		//TODO: should call base
		pkvd->fHandled = false;
}

void CBlowerCannon::Precache()
{
	UTIL_PrecacheOther( "displacer_ball" );
	UTIL_PrecacheOther( "spore" );
	UTIL_PrecacheOther( "shock_beam" );
}

void CBlowerCannon::Spawn()
{
	SetThink( nullptr );
	SetUse( &CBlowerCannon::BlowerCannonStart );

	pev->nextthink = gpGlobals->time + 0.1;

	if( m_flDelay < 0 )
	{
		m_flDelay = 1;
	}

	Precache();
}

void CBlowerCannon::BlowerCannonStart( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	SetUse( &CBlowerCannon::BlowerCannonStop );
	SetThink( &CBlowerCannon::BlowerCannonThink );

	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CBlowerCannon::BlowerCannonStop( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	SetUse( &CBlowerCannon::BlowerCannonStart );
	SetThink( nullptr );
}

void CBlowerCannon::BlowerCannonThink()
{
	auto pTarget = GetNextTarget()->pev;

	auto distance = pTarget->origin - pev->origin;
	distance.z += m_iZOffset;

	auto angles = UTIL_VecToAngles( distance );
	angles.z = -angles.z;

	switch( m_iWeaponType )
	{
	case WeaponType::SporeRocket:
	case WeaponType::SporeGrenade:
		CSpore::CreateSpore( pev->origin, angles, this, static_cast< CSpore::SporeType >( ( m_iWeaponType != WeaponType::SporeRocket ? 1 : 0 ) + static_cast< int >( CSpore::SporeType::ROCKET ) ), 0, 0 );
		break;

	case WeaponType::ShockBeam:
		CShockBeam::CreateShockBeam( pev->origin, angles, this );
		break;

	case WeaponType::DisplacerBall:
		CDisplacerBall::CreateDisplacerBall( pev->origin, angles, this );
		break;

	default: break;
	}

	if( m_iFireType == FireType::FireOnTrigger )
	{
		SetUse( &CBlowerCannon::BlowerCannonStart );
		SetThink( nullptr );
	}

	pev->nextthink = gpGlobals->time + m_flDelay;
}
