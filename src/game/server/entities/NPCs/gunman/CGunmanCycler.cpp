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
	void OnCreate() override;
	void Spawn() override;
	void Think() override;

	// Don't treat as a live target
	bool IsAlive() override { return false; }

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool KeyValue(KeyValueData* pkvd) override;

	bool m_animate;
	int m_iSubmodel1;
	int m_iSubmodel2;
	int m_iSubmodel3;
};

LINK_ENTITY_TO_CLASS(gunman_cycler, CGunmanCycler);

TYPEDESCRIPTION CGunmanCycler::m_SaveData[] = {
    DEFINE_FIELD(CGunmanCycler, m_animate, FIELD_BOOLEAN),
    DEFINE_FIELD(CGunmanCycler, m_iSubmodel1, FIELD_INTEGER),
    DEFINE_FIELD(CGunmanCycler, m_iSubmodel2, FIELD_INTEGER),
    DEFINE_FIELD(CGunmanCycler, m_iSubmodel3, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CGunmanCycler, CBaseMonster);

bool CGunmanCycler::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "cyc_submodel1"))
	{
		m_iSubmodel1 = atoi(pkvd->szValue);
		return true;
	}
	else if(FStrEq(pkvd->szKeyName, "cyc_submodel2"))
	{
		m_iSubmodel2 = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "cyc_submodel3"))
	{
		m_iSubmodel3 = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

void CGunmanCycler::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = 80000; // no cycler should die
}

void CGunmanCycler::Spawn()
{
	const char* szModel = STRING(pev->model);
    
	if (!szModel || '\0' == *szModel)
	{
		ALERT(at_error, "gunman cycler at %.0f %.0f %0.f missing modelname", pev->origin.x, pev->origin.y, pev->origin.z);
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	PrecacheModel(szModel);
	SetModel(szModel);

    SetBodygroup(1, m_iSubmodel1);
	SetBodygroup(2, m_iSubmodel2);
	SetBodygroup(3, m_iSubmodel3);

	InitBoneControllers();
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;
	pev->effects = 0;
	pev->yaw_speed = 5;
	pev->ideal_yaw = pev->angles.y;
	ChangeYaw(360);

	m_flFrameRate = 75;
	m_flGroundSpeed = 0;

	pev->nextthink += 1.0f;

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
}

void CGunmanCycler::Think()
{
	pev->nextthink = gpGlobals->time + 0.001f;

	if (m_animate)
	{
		StudioFrameAdvance();
	}

	UpdateShockEffect();

	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0;
		m_fSequenceFinished = false;
		m_flLastEventCheck = gpGlobals->time;
		pev->frame = 0;
		if (!m_animate)
			pev->framerate = 0.0; // FIX: don't reset framerate
	}
}

/**************************************************************************/

enum decore_asteroid_size
{
	ASTEROID_SIZE_BIG = 0,
	ASTEROID_SIZE_MEDIUM,
	ASTEROID_SIZE_SMALL
};

class CDecoreAsteroid : public CGunmanCycler
{
public:
	void OnCreate() override;
	void Spawn() override;
	void Think() override;

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iAsteroidSize = ASTEROID_SIZE_SMALL;
	float m_flMaxRotation = 0.5f;
	float m_flMinRotation = 0.1f;
};

LINK_ENTITY_TO_CLASS(decore_asteroid, CDecoreAsteroid);

bool CDecoreAsteroid::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "asteroidsize"))
	{
		m_iAsteroidSize = static_cast<int>(std::clamp(atoi(pkvd->szValue),
            static_cast<int>(ASTEROID_SIZE_BIG),
            static_cast<int>(ASTEROID_SIZE_SMALL)));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "maxrotation"))
	{
		m_flMaxRotation = std::clamp(atof(pkvd->szValue), 0.0, 360.0);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "minrotation"))
	{
		m_flMinRotation = std::clamp(atof(pkvd->szValue), 0.0, 360.0);
		return true;
	}
	
	return CGunmanCycler::KeyValue(pkvd);
}

void CDecoreAsteroid::OnCreate()
{
	pev->model = MAKE_STRING("models/asteroid.mdl");

	CGunmanCycler::OnCreate();
}

void CDecoreAsteroid::Spawn()
{
	const char* szModel = STRING(pev->model);

	if (!szModel || '\0' == *szModel)
	{
		ALERT(at_error, "decore asteroid at %.0f %.0f %0.f missing modelname", pev->origin.x, pev->origin.y, pev->origin.z);
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	PrecacheModel(szModel);
	SetModel(szModel);

	pev->movetype = MOVETYPE_FLY;
    pev->scale = 3.0f;

    SetBodygroup(0, 0);

    pev->nextthink += 1.0f;

    switch (m_iAsteroidSize)
    {
	    case ASTEROID_SIZE_MEDIUM:
	    case ASTEROID_SIZE_BIG:
	    {
		    SetBodygroup(0, 1);
		    pev->scale = 5.0f;
		    break;
	    }
	    case ASTEROID_SIZE_SMALL:
	    {
		    SetBodygroup(0, 0);
		    pev->scale = 5.0f;
		    break;
	    }
	}
}

void CDecoreAsteroid::Think()
{
	CGunmanCycler::Think();

    float flRotate = RANDOM_FLOAT(m_flMinRotation, m_flMaxRotation);
	pev->angles.x += flRotate;
	pev->angles.y += flRotate;
	pev->angles.z += flRotate;

	if (pev->angles.x >= 360.0f)
		pev->angles.x = 0.0f;

	if (pev->angles.y >= 360.0f)
		pev->angles.y = 0.0f;

	if (pev->angles.z >= 360.0f)
		pev->angles.z = 0.0f;
}