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
#include "ehandle.h"

edict_t* BaseEntityHandle::GetEdict() const
{
	if (!m_Edict)
		return nullptr;

	if (m_Edict->serialnumber != m_SerialNumber)
		return nullptr;

	return m_Edict;
}

CBaseEntity* BaseEntityHandle::InternalGetEntity() const
{
	return (CBaseEntity*)GET_PRIVATE(GetEdict());
}

void BaseEntityHandle::InternalSetEntity(CBaseEntity* entity)
{
	if (!entity)
	{
		m_Edict = nullptr;
		m_SerialNumber = 0;
		return;
	}

	m_Edict = entity->edict();
	m_SerialNumber = m_Edict->serialnumber;
}
