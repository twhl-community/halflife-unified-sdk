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

class CGunmanCycler : public CBaseMonster
{
public:
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE); }
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void OnCreate() override;
	void Spawn() override;
	void Think() override;
	//void Pain( float flDamage );
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	// Don't treat as a live target
	bool IsAlive() override { return false; }

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool m_animate;
};

LINK_ENTITY_TO_CLASS(gunman_cycler, CGunmanCycler);

TYPEDESCRIPTION CGunmanCycler::m_SaveData[] =
	{
		DEFINE_FIELD(CGunmanCycler, m_animate, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CGunmanCycler, CBaseMonster);

void CGunmanCycler::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = 80000; // no cycler should die
}

void CGunmanCycler::Spawn()
{
	const char* szModel = STRING(pev->model);

	const Vector vecMin(-16, -16, 0);
	const Vector vecMax(16, 16, 72);

	if (!szModel || '\0' == *szModel)
	{
		ALERT(at_error, "gunman cycler at %.0f %.0f %0.f missing modelname", pev->origin.x, pev->origin.y, pev->origin.z);
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	PrecacheModel(szModel);
	SetModel(szModel);

	InitBoneControllers();
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;
	pev->yaw_speed = 5;
	pev->ideal_yaw = pev->angles.y;
	ChangeYaw(360);

	m_flFrameRate = 75;
	m_flGroundSpeed = 0;

	pev->nextthink += 1.0;

	ResetSequenceInfo();

	if (pev->sequence != 0 || pev->frame != 0)
	{
		m_animate = false;
		pev->framerate = 0;
	}
	else
	{
		m_animate = true;
	}

	UTIL_SetSize(pev, vecMin, vecMax);
}

void CGunmanCycler::Think()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_animate)
	{
		StudioFrameAdvance();
	}

	UpdateShockEffect();

	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		// ResetSequenceInfo();
		// hack to avoid reloading model every frame
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0;
		m_fSequenceFinished = false;
		m_flLastEventCheck = gpGlobals->time;
		pev->frame = 0;
		if (!m_animate)
			pev->framerate = 0.0; // FIX: don't reset framerate
	}
}

void CGunmanCycler::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_animate = !m_animate;
	if (m_animate)
		pev->framerate = 1.0;
	else
		pev->framerate = 0.0;
}

bool CGunmanCycler::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (m_animate)
	{
		pev->sequence++;

		ResetSequenceInfo();

		if (m_flFrameRate == 0.0)
		{
			pev->sequence = 0;
			ResetSequenceInfo();
		}
		pev->frame = 0;
	}
	else
	{
		pev->framerate = 1.0;
		StudioFrameAdvance(0.1);
		pev->framerate = 0;
		ALERT(at_console, "sequence: %d, frame %.0f\n", pev->sequence, pev->frame);
	}

	return false;
}