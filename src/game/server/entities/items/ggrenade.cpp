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

LINK_ENTITY_TO_CLASS(grenade, CGrenade);

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE 0x0001

BEGIN_DATAMAP(CGrenade)
DEFINE_FUNCTION(Smoke),
	DEFINE_FUNCTION(BounceTouch),
	DEFINE_FUNCTION(SlideTouch),
	DEFINE_FUNCTION(ExplodeTouch),
	DEFINE_FUNCTION(DangerSoundThink),
	DEFINE_FUNCTION(PreDetonate),
	DEFINE_FUNCTION(Detonate),
	DEFINE_FUNCTION(DetonateUse),
	DEFINE_FUNCTION(TumbleThink),
	END_DATAMAP();

void CGrenade::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->model = MAKE_STRING("models/grenade.mdl");
}

void CGrenade::Precache()
{
	// Grenade exploded after save, but hasn't been removed yet. Don't precache.
	if (!FStringNull(pev->model))
	{
		PrecacheModel(STRING(pev->model));
	}
}

void CGrenade::Explode(Vector vecSrc, Vector vecAim)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -32), ignore_monsters, edict(), &tr);

	Explode(&tr, DMG_BLAST);
}

void CGrenade::Explode(TraceResult* pTrace, int bitsDamageType)
{
	pev->model = string_t::Null; // invisible
	pev->solid = SOLID_NOT;		 // intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 0.6);
	}

	int iContents = UTIL_PointContents(pev->origin);

	// This makes a dynamic light and the explosion sprites/sound
	// Send to PAS because of the sound
	UTIL_ExplosionEffect(pev->origin, iContents != CONTENTS_WATER ? g_sModelIndexFireball : g_sModelIndexWExplosion,
		(pev->dmg - 50) * .60, 15, TE_EXPLFLAG_NONE,
		MSG_PAS, pev->origin);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);
	CBaseEntity* owner = GetOwner();

	pev->owner = nullptr; // can't traceline attack owner if this is set

	// Counteract the + 1 in RadiusDamage.
	Vector origin = pev->origin;
	origin.z -= 1;

	RadiusDamage(origin, this, owner, pev->dmg, bitsDamageType);

	if (RANDOM_FLOAT(0, 1) < 0.5)
	{
		UTIL_DecalTrace(pTrace, DECAL_SCORCH1);
	}
	else
	{
		UTIL_DecalTrace(pTrace, DECAL_SCORCH2);
	}

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EmitSound(CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);
		break;
	case 1:
		EmitSound(CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);
		break;
	case 2:
		EmitSound(CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);
		break;
	}

	pev->effects |= EF_NODRAW;
	SetThink(&CGrenade::Smoke);
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0, 3);
		for (int i = 0; i < sparkCount; i++)
			Create("spark_shower", pev->origin, pTrace->vecPlaneNormal, nullptr);
	}
}

void CGrenade::Smoke()
{
	if (UTIL_PointContents(pev->origin) == CONTENTS_WATER)
	{
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);
	}
	else
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE((pev->dmg - 50) * 0.80); // scale * 10
		WRITE_BYTE(12);						// framerate
		MESSAGE_END();
	}
	UTIL_Remove(this);
}

void CGrenade::Killed(CBaseEntity* attacker, int iGib)
{
	Detonate();
}

void CGrenade::DetonateUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&CGrenade::Detonate);
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate()
{
	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 400, 0.3);

	SetThink(&CGrenade::Detonate);
	pev->nextthink = gpGlobals->time + 1;
}

void CGrenade::Detonate()
{
	TraceResult tr;
	Vector vecSpot; // trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, edict(), &tr);

	Explode(&tr, DMG_BLAST);
}

void CGrenade::ExplodeTouch(CBaseEntity* pOther)
{
	TraceResult tr;
	Vector vecSpot; // trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine(vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, edict(), &tr);

	Explode(&tr, DMG_BLAST);
}

void CGrenade::DangerSoundThink()
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length(), 0.2);
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->waterlevel != WaterLevel::Dry)
	{
		pev->velocity = pev->velocity * 0.5;
	}
}

void CGrenade::BounceTouch(CBaseEntity* pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther == GetOwner())
		return;

	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100)
	{
		CBaseEntity* owner = GetOwner();
		if (owner)
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack(owner, 1, gpGlobals->v_forward, &tr, DMG_CLUB);
			ApplyMultiDamage(this, owner);
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity.
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity;
	vecTestVelocity.z *= 0.45;

	if (!m_fRegisteredSound && vecTestVelocity.Length() <= 60)
	{
		// CBaseEntity::Logger->debug("Grenade Registered!: {}", vecTestVelocity.Length());

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving.
		// go ahead and emit the danger sound.

		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, pev->dmg / 0.4, 0.3);
		m_fRegisteredSound = true;
	}

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG(1, 1);
		ResetSequenceInfo();
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
	{
		pev->framerate = 0;
		pev->frame = 0;
	}
}

void CGrenade::SlideTouch(CBaseEntity* pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	// pev->avelocity = Vector (300, 300, 300);

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;

		if (pev->velocity.x != 0 || pev->velocity.y != 0)
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGrenade::BounceSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EmitSound(CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM);
		break;
	case 1:
		EmitSound(CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM);
		break;
	case 2:
		EmitSound(CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM);
		break;
	}
}

void CGrenade::TumbleThink()
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink(&CGrenade::Detonate);
	}
	if (pev->waterlevel != WaterLevel::Dry)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}

void CGrenade::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;

	SetModel(STRING(pev->model));
	SetSize(Vector(0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
	m_fRegisteredSound = false;
}

CGrenade* CGrenade::ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity)
{
	CGrenade* pGrenade = g_EntityDictionary->Create<CGrenade>("grenade");
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5; // lower gravity since grenade is aerodynamic and engine doesn't know it.
	pGrenade->SetOrigin(vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = owner->edict();

	// make monsters afaid of it while in the air
	pGrenade->SetThink(&CGrenade::DangerSoundThink);
	pGrenade->pev->nextthink = gpGlobals->time;

	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT(-100, -500);

	// Explode on contact
	pGrenade->SetTouch(&CGrenade::ExplodeTouch);

	pGrenade->pev->dmg = GetSkillFloat("plr_9mmAR_grenade"sv);

	return pGrenade;
}

CGrenade* CGrenade::ShootTimed(CBaseEntity* owner, Vector vecStart, Vector vecVelocity, float time)
{
	CGrenade* pGrenade = g_EntityDictionary->Create<CGrenade>("grenade");
	pGrenade->Spawn();
	pGrenade->SetOrigin(vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = owner->edict();

	pGrenade->SetTouch(&CGrenade::BounceTouch); // Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed().

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink(&CGrenade::TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	pGrenade->SetModel("models/w_grenade.mdl");
	pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->framerate = 1.0;
	pGrenade->ResetSequenceInfo();

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;

	pGrenade->pev->dmg = 100;

	return pGrenade;
}

CGrenade* CGrenade::ShootSatchelCharge(CBaseEntity* owner, Vector vecStart, Vector vecVelocity)
{
	CGrenade* pGrenade = g_EntityDictionary->Create<CGrenade>("grenade");
	pGrenade->pev->movetype = MOVETYPE_BOUNCE;

	pGrenade->pev->solid = SOLID_BBOX;

	pGrenade->SetModel("models/grenade.mdl"); // Change this to satchel charge model

	pGrenade->SetSize(Vector(0, 0, 0), Vector(0, 0, 0));

	pGrenade->pev->dmg = 200;
	pGrenade->SetOrigin(vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = g_vecZero;
	pGrenade->pev->owner = owner->edict();

	// Detonate in "time" seconds
	pGrenade->SetThink(nullptr);
	pGrenade->SetUse(&CGrenade::DetonateUse);
	pGrenade->SetTouch(&CGrenade::SlideTouch);
	pGrenade->pev->spawnflags = SF_DETONATE;

	pGrenade->pev->friction = 0.9;

	return pGrenade;
}

void CGrenade::UseSatchelCharges(CBaseEntity* owner, SATCHELCODE code)
{
	if (!owner)
		return;

	for (auto pEnt : UTIL_FindEntitiesByClassname("grenade"))
	{
		if (FBitSet(pEnt->pev->spawnflags, SF_DETONATE) && pEnt->GetOwner() == owner)
		{
			if (code == SATCHEL_DETONATE)
				pEnt->Use(owner, owner, USE_ON, 0);
			else // SATCHEL_RELEASE
				pEnt->pev->owner = nullptr;
		}
	}
}
