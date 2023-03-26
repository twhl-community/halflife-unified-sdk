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
#include "cbase.h"

const auto SF_ITEMGENERIC_DROP_TO_FLOOR = 1 << 0;
const auto SF_ITEMGENERIC_SOLID = 1 << 1;

class CGenericItem : public CBaseAnimating
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Precache() override;
	void Spawn() override;

	void EXPORT StartItem();
	void EXPORT AnimateThink();

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	string_t m_iSequence;
};

LINK_ENTITY_TO_CLASS(item_generic, CGenericItem);

bool CGenericItem::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("sequencename", pkvd->szKeyName))
	{
		m_iSequence = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq("skin", pkvd->szKeyName))
	{
		pev->skin = static_cast<int>(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq("body", pkvd->szKeyName))
	{
		pev->body = atoi(pkvd->szValue);
		return true;
	}

	return CBaseDelay::KeyValue(pkvd);
}

void CGenericItem::Precache()
{
	PrecacheModel(STRING(pev->model));
}

void CGenericItem::Spawn()
{
	pev->solid = 0;
	pev->movetype = 0;
	pev->effects = 0;
	pev->frame = 0;

	Precache();

	SetModel(STRING(pev->model));

	int sequence = 0;

	if (!FStringNull(m_iSequence))
	{
		SetThink(&CGenericItem::StartItem);
		pev->nextthink = gpGlobals->time + 0.1;

		sequence = LookupSequence(STRING(m_iSequence));

		if (sequence == -1)
		{
			CBaseEntity::Logger->debug("ERROR! FIX ME: item generic: {}, model: {}, does not have animation: {}",
				STRING(pev->targetname), STRING(pev->model), STRING(m_iSequence));

			sequence = 0;
		}
	}

	// Set sequence now. If none was specified then this will use the first sequence.
	pev->sequence = sequence;

	if ((pev->spawnflags & SF_ITEMGENERIC_DROP_TO_FLOOR) != 0)
	{
		if (0 == g_engfuncs.pfnDropToFloor(pev->pContainingEntity))
		{
			CBaseEntity::Logger->error("Item {} fell out of level at {}", STRING(pev->classname), pev->origin);
			UTIL_Remove(this);
		}
	}

	if ((pev->spawnflags & SF_ITEMGENERIC_SOLID) != 0)
	{
		Vector mins, maxs;

		pev->solid = SOLID_SLIDEBOX;
		
		ExtractBbox(sequence, mins, maxs);

		SetSize(mins, maxs);
		SetOrigin(pev->origin);
	}
}

void CGenericItem::StartItem()
{
	pev->effects = 0;

	pev->frame = 0;
	ResetSequenceInfo();

	SetThink(&CGenericItem::AnimateThink);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->frame = 0;
}

void CGenericItem::AnimateThink()
{
	DispatchAnimEvents();
	StudioFrameAdvance();

	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		pev->frame = 0;
		ResetSequenceInfo();
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CGenericItem::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&CGenericItem::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}
