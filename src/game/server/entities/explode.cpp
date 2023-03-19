/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

/**
*	@file
*	Explosion-related code
*/

#include "cbase.h"
#include "explode.h"

/**
*	@brief Spark Shower
*/
class CShower : public CBaseEntity
{
public:
	void OnCreate() override;
	void Precache() override;
	void Spawn() override;
	void Think() override;
	void Touch(CBaseEntity* pOther) override;
	int ObjectCaps() override { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS(spark_shower, CShower);

void CShower::OnCreate()
{
	CBaseEntity::OnCreate();

	// Need a model, just use the grenade, we don't draw it anyway
	pev->model = MAKE_STRING("models/grenade.mdl");
}

void CShower::Precache()
{
	PrecacheModel(STRING(pev->model));
}

void CShower::Spawn()
{
	Precache();

	pev->velocity = RANDOM_FLOAT(200, 300) * pev->angles;
	pev->velocity.x += RANDOM_FLOAT(-100.f, 100.f);
	pev->velocity.y += RANDOM_FLOAT(-100.f, 100.f);
	if (pev->velocity.z >= 0)
		pev->velocity.z += 200;
	else
		pev->velocity.z -= 200;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;
	SetModel(STRING(pev->model));
	SetSize(g_vecZero, g_vecZero);
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT(0.5, 1.5);

	pev->angles = g_vecZero;
}

void CShower::Think()
{
	UTIL_Sparks(pev->origin);

	pev->speed -= 0.1;
	if (pev->speed > 0)
		pev->nextthink = gpGlobals->time + 0.1;
	else
		UTIL_Remove(this);
	pev->flags &= ~FL_ONGROUND;
}

void CShower::Touch(CBaseEntity* pOther)
{
	if ((pev->flags & FL_ONGROUND) != 0)
		pev->velocity = pev->velocity * 0.1;
	else
		pev->velocity = pev->velocity * 0.6;

	if ((pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y) < 10.0)
		pev->speed = 0;
}

class CEnvExplosion : public CBaseMonster
{
public:
	void Spawn() override;
	void EXPORT Smoke();
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iMagnitude;  // how large is the fireball? how much damage?
	int m_spriteScale; // what's the exact fireball sprite scale?
};

TYPEDESCRIPTION CEnvExplosion::m_SaveData[] =
	{
		DEFINE_FIELD(CEnvExplosion, m_iMagnitude, FIELD_INTEGER),
		DEFINE_FIELD(CEnvExplosion, m_spriteScale, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CEnvExplosion, CBaseMonster);
LINK_ENTITY_TO_CLASS(env_explosion, CEnvExplosion);

bool CEnvExplosion::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "iMagnitude"))
	{
		m_iMagnitude = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CEnvExplosion::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	pev->movetype = MOVETYPE_NONE;
	/*
	if ( m_iMagnitude > 250 )
	{
		m_iMagnitude = 250;
	}
	*/

	float flSpriteScale;
	flSpriteScale = (m_iMagnitude - 50) * 0.6;

	/*
	if ( flSpriteScale > 50 )
	{
		flSpriteScale = 50;
	}
	*/
	if (flSpriteScale < 10)
	{
		flSpriteScale = 10;
	}

	m_spriteScale = (int)flSpriteScale;
}

void CEnvExplosion::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	TraceResult tr;

	pev->model = string_t::Null; // invisible
	pev->solid = SOLID_NOT;		 // intangible

	Vector vecSpot; // trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);

	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	// Pull out of the wall a bit
	if (tr.flFraction != 1.0)
	{
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (m_iMagnitude - 24) * 0.6);
	}
	else
	{
		pev->origin = pev->origin;
	}

	// draw decal
	if ((pev->spawnflags & SF_ENVEXPLOSION_NODECAL) == 0)
	{
		if (RANDOM_FLOAT(0, 1) < 0.5)
		{
			UTIL_DecalTrace(&tr, DECAL_SCORCH1);
		}
		else
		{
			UTIL_DecalTrace(&tr, DECAL_SCORCH2);
		}
	}

	// draw fireball
	UTIL_ExplosionEffect(pev->origin, g_sModelIndexFireball,
		(pev->spawnflags & SF_ENVEXPLOSION_NOFIREBALL) == 0 ? static_cast<byte>(m_spriteScale) : 0,
		15, TE_EXPLFLAG_NONE,
		MSG_PAS, pev->origin);

	// do damage
	if ((pev->spawnflags & SF_ENVEXPLOSION_NODAMAGE) == 0)
	{
		RadiusDamage(this, this, m_iMagnitude, CLASS_NONE, DMG_BLAST);
	}

	SetThink(&CEnvExplosion::Smoke);
	pev->nextthink = gpGlobals->time + 0.3;

	// draw sparks
	if ((pev->spawnflags & SF_ENVEXPLOSION_NOSPARKS) == 0)
	{
		int sparkCount = RANDOM_LONG(0, 3);

		for (int i = 0; i < sparkCount; i++)
		{
			Create("spark_shower", pev->origin, tr.vecPlaneNormal, nullptr);
		}
	}
}

void CEnvExplosion::Smoke()
{
	if ((pev->spawnflags & SF_ENVEXPLOSION_NOSMOKE) == 0)
	{
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE((byte)m_spriteScale); // scale * 10
		WRITE_BYTE(12);					 // framerate
		MESSAGE_END();
	}

	if ((pev->spawnflags & SF_ENVEXPLOSION_REPEATABLE) == 0)
	{
		UTIL_Remove(this);
	}
}

// HACKHACK -- create one of these and fake a keyvalue to get the right explosion setup
void ExplosionCreate(const Vector& center, const Vector& angles, edict_t* pOwner, int magnitude, bool doDamage)
{
	KeyValueData kvd;
	char buf[128];

	CBaseEntity* pExplosion = CBaseEntity::Create("env_explosion", center, angles, pOwner);
	sprintf(buf, "%3d", magnitude);
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue(&kvd);
	if (!doDamage)
		pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();
	pExplosion->Use(nullptr, nullptr, USE_TOGGLE, 0);
}
