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

#include "CRopeSample.h"

TYPEDESCRIPTION	CRopeSample::m_SaveData[] =
{
	DEFINE_FIELD( CRopeSample, m_Data.mPosition, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, m_Data.mVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, m_Data.mForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, m_Data.mExternalForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, m_Data.mApplyExternalForce, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRopeSample, m_Data.mMassReciprocal, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSample, m_pMasterRope, FIELD_CLASSPTR ),
};

LINK_ENTITY_TO_CLASS( rope_sample, CRopeSample );

IMPLEMENT_SAVERESTORE( CRopeSample, CRopeSample::BaseClass );

void CRopeSample::Spawn()
{
	//TODO: needed?
	//pev->effects |= EF_NODRAW;
}

CRopeSample* CRopeSample::CreateSample()
{
	auto pSample = GetClassPtr( reinterpret_cast<CRopeSample*>( VARS( CREATE_NAMED_ENTITY( MAKE_STRING( "rope_sample" ) ) ) ) );

	pSample->Spawn();

	return pSample;
}
