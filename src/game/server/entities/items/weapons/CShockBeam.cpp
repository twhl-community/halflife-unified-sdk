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
#include "customentity.h"

#include "CShockBeam.h"

BEGIN_DATAMAP(CShockBeam)
DEFINE_FIELD(m_pBeam1, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pBeam2, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pSprite, FIELD_CLASSPTR),
	DEFINE_FUNCTION(FlyThink),
	DEFINE_FUNCTION(ExplodeThink),
	DEFINE_FUNCTION(WaterExplodeThink),
	DEFINE_FUNCTION(BallTouch),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(shock_beam, CShockBeam);

void CShockBeam::Precache()
{
	PrecacheModel("sprites/flare3.spr");
	PrecacheModel("sprites/lgtning.spr");
	PrecacheModel("sprites/glow01.spr");
	PrecacheModel("models/shock_effect.mdl");
	PrecacheSound("weapons/shock_impact.wav");
}

void CShockBeam::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SetModel("models/shock_effect.mdl");

	SetOrigin(pev->origin);

	SetSize(Vector(-4, -4, -4), Vector(4, 4, 4));

	SetTouch(&CShockBeam::BallTouch);
	SetThink(&CShockBeam::FlyThink);

	m_pSprite = CSprite::SpriteCreate("sprites/flare3.spr", pev->origin, false);

	m_pSprite->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxDistort);

	m_pSprite->SetScale(0.35);

	m_pSprite->SetAttachment(edict(), 0);

	m_pBeam1 = CBeam::BeamCreate("sprites/lgtning.spr", 60);

	if (m_pBeam1)
	{
		m_pBeam1->SetOrigin(pev->origin);

		m_pBeam1->EntsInit(entindex(), entindex());

		m_pBeam1->SetStartAttachment(1);
		m_pBeam1->SetEndAttachment(2);

		m_pBeam1->SetColor(0, 253, 253);

		m_pBeam1->SetFlags(BEAM_FSHADEOUT);
		m_pBeam1->SetBrightness(180);
		m_pBeam1->SetNoise(0);

		m_pBeam1->SetScrollRate(10);

		if (g_pGameRules->IsMultiplayer())
		{
			pev->nextthink = gpGlobals->time + 0.01;
			return;
		}

		m_pBeam2 = CBeam::BeamCreate("sprites/lgtning.spr", 20);

		if (m_pBeam2)
		{
			m_pBeam2->SetOrigin(pev->origin);

			m_pBeam2->EntsInit(entindex(), entindex());

			m_pBeam2->SetStartAttachment(1);
			m_pBeam2->SetEndAttachment(2);

			m_pBeam2->SetColor(255, 255, 157);

			m_pBeam2->SetFlags(BEAM_FSHADEOUT);
			m_pBeam2->SetBrightness(180);
			m_pBeam2->SetNoise(30);

			m_pBeam2->SetScrollRate(30);

			pev->nextthink = gpGlobals->time + 0.01;
		}
	}
}

void CShockBeam::FlyThink()
{
	if (pev->waterlevel == WaterLevel::Head)
	{
		SetThink(&CShockBeam::WaterExplodeThink);
	}

	pev->nextthink = gpGlobals->time + 0.01;
}

void CShockBeam::ExplodeThink()
{
	Explode();
	UTIL_Remove(this);
}

void CShockBeam::WaterExplodeThink()
{
	auto pOwner = GetOwner();

	Explode();

	::RadiusDamage(pev->origin, this, pOwner, 100.0, 150.0, DMG_ALWAYSGIB | DMG_BLAST);

	UTIL_Remove(this);
}

void CShockBeam::BallTouch(CBaseEntity* pOther)
{
	SetTouch(nullptr);
	SetThink(nullptr);

	if (pOther->pev->takedamage != DAMAGE_NO)
	{
		TraceResult tr = UTIL_GetGlobalTrace();

		ClearMultiDamage();

		const auto damage = g_pGameRules->IsMultiplayer() ? GetSkillFloat("plr_shockroachm"sv) : GetSkillFloat("plr_shockroachs"sv);

		auto bitsDamageTypes = DMG_ALWAYSGIB | DMG_SHOCK;

		auto pMonster = pOther->MyMonsterPointer();

		if (pMonster)
		{
			bitsDamageTypes = DMG_BLAST;

			if (pMonster->m_flShockDuration > 1)
			{
				bitsDamageTypes = DMG_ALWAYSGIB;
			}
		}

		pOther->TraceAttack(GetOwner(), damage, pev->velocity.Normalize(), &tr, bitsDamageTypes);

		if (pMonster)
		{
			pMonster->AddShockEffect(63.0, 152.0, 208.0, 16.0, 0.5);
		}

		ApplyMultiDamage(this, GetOwner());

		pev->velocity = g_vecZero;
	}

	SetThink(&CShockBeam::ExplodeThink);
	pev->nextthink = gpGlobals->time + 0.01;

	if (pOther->pev->takedamage == DAMAGE_NO)
	{
		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, edict(), &tr);

		UTIL_DecalTrace(&tr, DECAL_OFSCORCH1 + RANDOM_LONG(0, 2));

		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		g_engfuncs.pfnWriteByte(TE_SPARKS);
		g_engfuncs.pfnWriteCoord(pev->origin.x);
		g_engfuncs.pfnWriteCoord(pev->origin.y);
		g_engfuncs.pfnWriteCoord(pev->origin.z);
		MESSAGE_END();
	}
}

void CShockBeam::Explode()
{
	if (m_pSprite)
	{
		UTIL_Remove(m_pSprite);
		m_pSprite = nullptr;
	}

	if (m_pBeam1)
	{
		UTIL_Remove(m_pBeam1);
		m_pBeam1 = nullptr;
	}

	if (m_pBeam2)
	{
		UTIL_Remove(m_pBeam2);
		m_pBeam2 = nullptr;
	}

	pev->dmg = 40;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_BYTE(8);
	WRITE_BYTE(0);
	WRITE_BYTE(253);
	WRITE_BYTE(253);
	WRITE_BYTE(5);
	WRITE_BYTE(10);
	MESSAGE_END();

	pev->owner = nullptr;

	EmitSound(CHAN_WEAPON, "weapons/shock_impact.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
}

CShockBeam* CShockBeam::CreateShockBeam(const Vector& vecOrigin, const Vector& vecAngles, CBaseEntity* pOwner)
{
	auto pBeam = g_EntityDictionary->Create<CShockBeam>("shock_beam");

	UTIL_InitializeKeyValues( static_cast<CBaseEntity*>( pBeam ), pOwner->m_SharedKey, pOwner->m_SharedValue, pOwner->m_SharedKeyValues );

	pBeam->pev->angles = vecAngles;
	pBeam->pev->angles.x = -pBeam->pev->angles.x;

	pBeam->SetOrigin(vecOrigin);

	UTIL_MakeVectors(pBeam->pev->angles);

	pBeam->pev->velocity = gpGlobals->v_forward * 2000.0;
	pBeam->pev->velocity.z = -pBeam->pev->velocity.z;

	pBeam->Spawn();

	pBeam->pev->owner = pOwner->edict();

	return pBeam;
}
