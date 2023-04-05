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
#include "CRpg.h"
#include "CRpgRocket.h"

BEGIN_DATAMAP(CRpgRocket)
DEFINE_FIELD(m_flIgniteTime, FIELD_TIME),
	DEFINE_FIELD(m_pLauncher, FIELD_EHANDLE),
	DEFINE_FUNCTION(FollowThink),
	DEFINE_FUNCTION(IgniteThink),
	DEFINE_FUNCTION(RocketTouch),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(rpg_rocket, CRpgRocket);

CRpgRocket::~CRpgRocket()
{
	if (m_pLauncher)
	{
		// TODO: this probably needs to be put in UpdateOnRemove
		//  my launcher is still around, tell it I'm dead.
		m_pLauncher->m_cActiveRockets--;
	}
}

CRpgRocket* CRpgRocket::CreateRpgRocket(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, CRpg* pLauncher)
{
	CRpgRocket* pRocket = g_EntityDictionary->Create<CRpgRocket>("rpg_rocket");

	pRocket->SetOrigin(vecOrigin);
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch(&CRpgRocket::RocketTouch);
	pRocket->m_pLauncher = pLauncher; // remember what RPG fired me.
	pLauncher->m_cActiveRockets++;	  // register this missile as active for the launcher
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

void CRpgRocket::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SetModel("models/rpgrocket.mdl");
	SetSize(Vector(0, 0, 0), Vector(0, 0, 0));
	SetOrigin(pev->origin);

	SetThink(&CRpgRocket::IgniteThink);
	SetTouch(&CRpgRocket::ExplodeTouch);

	pev->angles.x -= 30;
	UTIL_MakeVectors(pev->angles);
	pev->angles.x = -(pev->angles.x + 30);

	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.4;

	pev->dmg = GetSkillFloat("plr_rpg"sv);
}

void CRpgRocket::RocketTouch(CBaseEntity* pOther)
{
	StopSound(CHAN_VOICE, "weapons/rocket1.wav");
	ExplodeTouch(pOther);
}

void CRpgRocket::Precache()
{
	PrecacheModel("models/rpgrocket.mdl");
	m_iTrail = PrecacheModel("sprites/smoke.spr");
	PrecacheSound("weapons/rocket1.wav");
}

void CRpgRocket::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EmitSound(CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5);

	// rocket trail
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex()); // entity
	WRITE_SHORT(m_iTrail);	 // model
	WRITE_BYTE(40);			 // life
	WRITE_BYTE(5);			 // width
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(255);		 // r, g, b
	WRITE_BYTE(255);		 // brightness

	MESSAGE_END(); // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink(&CRpgRocket::FollowThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CRpgRocket::FollowThink()
{
	CBaseEntity* pOther = nullptr;
	Vector vecTarget;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;

	UTIL_MakeAimVectors(pev->angles);

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;

	// Examine all entities within a reasonable radius
	while ((pOther = UTIL_FindEntityByClassname(pOther, "laser_spot")) != nullptr)
	{
		UTIL_TraceLine(pev->origin, pOther->pev->origin, dont_ignore_monsters, ENT(pev), &tr);
		// WeaponsLogger->debug("{}", tr.flFraction);
		if (tr.flFraction >= 0.90)
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length();
			vecDir = vecDir.Normalize();
			flDot = DotProduct(gpGlobals->v_forward, vecDir);
			if ((flDot > 0) && (flDist * (1 - flDot) < flMax))
			{
				flMax = flDist * (1 - flDot);
				vecTarget = vecDir;
			}
		}
	}

	pev->angles = UTIL_VecToAngles(vecTarget);

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if (gpGlobals->time - m_flIgniteTime < 1.0)
	{
		pev->velocity = pev->velocity * 0.2 + vecTarget * (flSpeed * 0.8 + 400);
		if (pev->waterlevel == WaterLevel::Head)
		{
			// go slow underwater
			if (pev->velocity.Length() > 300)
			{
				pev->velocity = pev->velocity.Normalize() * 300;
			}
			UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 4);
		}
		else
		{
			if (pev->velocity.Length() > 2000)
			{
				pev->velocity = pev->velocity.Normalize() * 2000;
			}
		}
	}
	else
	{
		if ((pev->effects & EF_LIGHT) != 0)
		{
			pev->effects = 0;
			StopSound(CHAN_VOICE, "weapons/rocket1.wav");
		}
		pev->velocity = pev->velocity * 0.2 + vecTarget * flSpeed * 0.798;
		if (pev->waterlevel == WaterLevel::Dry && pev->velocity.Length() < 1500)
		{
			Detonate();
		}
	}
	// WeaponsLogger->debug("{:.0f}", flSpeed);

	pev->nextthink = gpGlobals->time + 0.1;
}
