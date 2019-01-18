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
#include "player.h"

#include "CRopeSample.h"
#include "CRope.h"

#include "CRopeSegment.h"

TYPEDESCRIPTION	CRopeSegment::m_SaveData[] =
{
	DEFINE_FIELD( CRopeSegment, m_pSample, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRopeSegment, m_iszModelName, FIELD_STRING ),
	DEFINE_FIELD( CRopeSegment, m_flDefaultMass, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSegment, m_bCauseDamage, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRopeSegment, m_bCanBeGrabbed, FIELD_BOOLEAN ),
};

LINK_ENTITY_TO_CLASS( rope_segment, CRopeSegment );

IMPLEMENT_SAVERESTORE( CRopeSegment, CRopeSegment::BaseClass );

CRopeSegment::CRopeSegment()
{
	m_iszModelName = MAKE_STRING( "models/rope16.mdl" );
}

void CRopeSegment::Precache()
{
	BaseClass::Precache();

	PRECACHE_MODEL( const_cast<char*>( STRING( m_iszModelName ) ) );
	PRECACHE_SOUND( "items/grab_rope.wav" );
}

void CRopeSegment::Spawn()
{
	Precache();

	SET_MODEL( edict(), STRING( m_iszModelName ) );

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_TRIGGER;
	pev->flags |= FL_ALWAYSTHINK;
	pev->effects = EF_NODRAW;
	UTIL_SetOrigin( pev, pev->origin );

	UTIL_SetSize( pev, Vector( -30, -30, -30 ), Vector( 30, 30, 30 ) );

	pev->nextthink = gpGlobals->time + 0.5;
}

void CRopeSegment::Think()
{
	//Do nothing.
}

void CRopeSegment::Touch( CBaseEntity* pOther )
{
	if( pOther->IsPlayer() )
	{
		auto pPlayer = static_cast<CBasePlayer*>( pOther );

		//Electrified wires deal damage. - Solokiller
		if( m_bCauseDamage )
		{
			pOther->TakeDamage( pev, pev, 1, DMG_SHOCK );
		}

		if( m_pSample->GetMasterRope()->IsAcceptingAttachment() && !pPlayer->IsOnRope() )
		{
			if( m_bCanBeGrabbed )
			{
				auto& data = m_pSample->GetData();

				UTIL_SetOrigin( pOther->pev, data.mPosition );

				pPlayer->SetOnRopeState( true );
				pPlayer->SetRope( m_pSample->GetMasterRope() );
				m_pSample->GetMasterRope()->AttachObjectToSegment( this );

				const Vector& vecVelocity = pOther->pev->velocity;

				if( vecVelocity.Length() > 0.5 )
				{
					//Apply some external force to move the rope. - Solokiller
					data.mApplyExternalForce = true;

					data.mExternalForce = data.mExternalForce + vecVelocity * 750;
				}

				if( m_pSample->GetMasterRope()->IsSoundAllowed() )
				{
					EMIT_SOUND( edict(), CHAN_BODY, "items/grab_rope.wav", 1.0, ATTN_NORM );
				}
			}
			else
			{
				//This segment cannot be grabbed, so grab the highest one if possible. - Solokiller
				auto pRope = m_pSample->GetMasterRope();

				CRopeSegment* pSegment;

				if( pRope->GetNumSegments() <= 4 )
				{
					//Fewer than 5 segments exist, so allow grabbing the last one. - Solokiller
					pSegment = pRope->GetSegments()[ pRope->GetNumSegments() - 1 ];
					pSegment->SetCanBeGrabbed( true );
				}
				else
				{
					pSegment = pRope->GetSegments()[ 4 ];
				}

				pSegment->Touch( pOther );
			}
		}
	}
}

CRopeSegment* CRopeSegment::CreateSegment( CRopeSample* pSample, string_t iszModelName )
{
	auto pSegment = GetClassPtr( reinterpret_cast<CRopeSegment*>( VARS( CREATE_NAMED_ENTITY( MAKE_STRING( "rope_segment" ) ) ) ) );

	pSegment->m_iszModelName = iszModelName;

	pSegment->Spawn();

	pSegment->m_pSample = pSample;

	pSegment->m_bCauseDamage = false;
	pSegment->m_bCanBeGrabbed = true;
	pSegment->m_flDefaultMass = pSample->GetData().mMassReciprocal;

	return pSegment;
}

void CRopeSegment::ApplyExternalForce( const Vector& vecForce )
{
	m_pSample->GetData().mApplyExternalForce = true;

	m_pSample->GetData().mExternalForce = m_pSample->GetData().mExternalForce + vecForce;
}

void CRopeSegment::SetMassToDefault()
{
	m_pSample->GetData().mMassReciprocal = m_flDefaultMass;
}

void CRopeSegment::SetDefaultMass( const float flDefaultMass )
{
	m_flDefaultMass = flDefaultMass;
}

void CRopeSegment::SetMass( const float flMass )
{
	m_pSample->GetData().mMassReciprocal = flMass;
}

void CRopeSegment::SetCauseDamageOnTouch( const bool bCauseDamage )
{
	m_bCauseDamage = bCauseDamage;
}

void CRopeSegment::SetCanBeGrabbed( const bool bCanBeGrabbed )
{
	m_bCanBeGrabbed = bCanBeGrabbed;
}
