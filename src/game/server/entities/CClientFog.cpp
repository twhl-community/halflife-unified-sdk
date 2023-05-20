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
#include "CClientFog.h"

LINK_ENTITY_TO_CLASS(env_fog, CClientFog);

BEGIN_DATAMAP(CClientFog)
DEFINE_FIELD(m_Density, FIELD_FLOAT),
	DEFINE_FIELD(m_StartDistance, FIELD_FLOAT),
	DEFINE_FIELD(m_StopDistance, FIELD_FLOAT),
	END_DATAMAP();

bool CClientFog::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "density"))
	{
		m_Density = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "fogStartDistance"))
	{
		m_StartDistance = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "fogStopDistance"))
	{
		m_StopDistance = atof(pkvd->szValue);
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CClientFog::Spawn()
{
	// Not really needed, perhaps fog was once a brush entity?
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;
}
