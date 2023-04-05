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
#include "CSatchelCharge.h"

BEGIN_DATAMAP(CSatchelCharge)
DEFINE_FUNCTION(SatchelSlide),
	DEFINE_FUNCTION(SatchelThink),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(monster_satchel, CSatchelCharge);

void CSatchelCharge::OnCreate()
{
	CGrenade::OnCreate();

	pev->model = MAKE_STRING("models/w_satchel.mdl");
}

void CSatchelCharge::Deactivate()
{
	pev->solid = SOLID_NOT;
	UTIL_Remove(this);
}

void CSatchelCharge::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SetModel(STRING(pev->model));
	// SetSize(Vector( -16, -16, -4), Vector(16, 16, 32));	// Old box -- size of headcrab monsters/players get blocked by this
	SetSize(Vector(-4, -4, -4), Vector(4, 4, 4)); // Uses point-sized, and can be stepped over
	SetOrigin(pev->origin);

	SetTouch(&CSatchelCharge::SatchelSlide);
	SetUse(&CSatchelCharge::DetonateUse);
	SetThink(&CSatchelCharge::SatchelThink);
	pev->nextthink = gpGlobals->time + 0.1;

	pev->gravity = 0.5;
	pev->friction = 0.8;

	pev->dmg = GetSkillFloat("plr_satchel"sv);
	// ResetSequenceInfo( );
	pev->sequence = 1;
}

void CSatchelCharge::SatchelSlide(CBaseEntity* pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	// pev->avelocity = Vector (300, 300, 300);
	pev->gravity = 1; // normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 10), ignore_monsters, edict(), &tr);

	if (tr.flFraction < 1.0)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if ((pev->flags & FL_ONGROUND) == 0 && pev->velocity.Length2D() > 10)
	{
		BounceSound();
	}
	StudioFrameAdvance();
}

void CSatchelCharge::SatchelThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->waterlevel == WaterLevel::Head)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel == WaterLevel::Dry)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8;
	}
}

void CSatchelCharge::Precache()
{
	PrecacheModel(STRING(pev->model));
	PrecacheSound("weapons/g_bounce1.wav");
	PrecacheSound("weapons/g_bounce2.wav");
	PrecacheSound("weapons/g_bounce3.wav");
}

void CSatchelCharge::BounceSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EmitSound(CHAN_VOICE, "weapons/g_bounce1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EmitSound(CHAN_VOICE, "weapons/g_bounce2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EmitSound(CHAN_VOICE, "weapons/g_bounce3.wav", 1, ATTN_NORM);
		break;
	}
}

void DeactivateSatchels(CBasePlayer* pOwner)
{
	for (auto satchel : UTIL_FindEntitiesByClassname<CSatchelCharge>("monster_satchel"))
	{
		if (satchel->pev->owner == pOwner->edict())
		{
			satchel->Deactivate();
		}
	}
}
