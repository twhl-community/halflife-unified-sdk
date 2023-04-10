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
#include "effects.h"
#include "func_break.h"
#include "shake.h"

#define SF_GIBSHOOTER_REPEATABLE 1 // allows a gibshooter to be refired

#define SF_FUNNEL_REVERSE 1 // funnel effect repels particles instead of attracting them.

/**
 *	@brief Lightning target, just alias landmark
 */
LINK_ENTITY_TO_CLASS(info_target, CPointEntity);

class CBubbling : public CBaseEntity
{
	DECLARE_CLASS(CBubbling, CBaseEntity);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	void FizzThink();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int m_density;
	int m_frequency;
	int m_bubbleModel;
	bool m_state;
};

LINK_ENTITY_TO_CLASS(env_bubbles, CBubbling);

BEGIN_DATAMAP(CBubbling)
DEFINE_FIELD(m_density, FIELD_INTEGER),
	DEFINE_FIELD(m_frequency, FIELD_INTEGER),
	DEFINE_FIELD(m_state, FIELD_BOOLEAN),
	// Let spawn restore this!
	//	DEFINE_FIELD(m_bubbleModel, FIELD_INTEGER),
	DEFINE_FUNCTION(FizzThink),
	END_DATAMAP();

#define SF_BUBBLES_STARTOFF 0x0001

void CBubbling::Spawn()
{
	Precache();
	SetModel(STRING(pev->model)); // Set size

	pev->solid = SOLID_NOT; // Remove model & collisions
	pev->renderamt = 0;		// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;
	int speed = pev->speed > 0 ? pev->speed : -pev->speed;

	// HACKHACK!!! - Speed in rendercolor
	pev->rendercolor.x = speed >> 8;
	pev->rendercolor.y = speed & 255;
	pev->rendercolor.z = (pev->speed < 0) ? 1 : 0;


	if ((pev->spawnflags & SF_BUBBLES_STARTOFF) == 0)
	{
		SetThink(&CBubbling::FizzThink);
		pev->nextthink = gpGlobals->time + 2.0;
		m_state = true;
	}
	else
		m_state = false;
}

void CBubbling::Precache()
{
	m_bubbleModel = PrecacheModel("sprites/bubble.spr"); // Precache bubble sprite
}

void CBubbling::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, m_state))
		m_state = !m_state;

	if (m_state)
	{
		SetThink(&CBubbling::FizzThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		SetThink(nullptr);
		pev->nextthink = 0;
	}
}

bool CBubbling::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "density"))
	{
		m_density = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "frequency"))
	{
		m_frequency = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "current"))
	{
		pev->speed = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CBubbling::FizzThink()
{
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, VecBModelOrigin(this));
	WRITE_BYTE(TE_FIZZ);
	WRITE_SHORT((short)entindex());
	WRITE_SHORT((short)m_bubbleModel);
	WRITE_BYTE(m_density);
	MESSAGE_END();

	if (m_frequency > 19)
		pev->nextthink = gpGlobals->time + 0.5;
	else
		pev->nextthink = gpGlobals->time + 2.5 - (0.1 * m_frequency);
}

BEGIN_DATAMAP(CBeam)
DEFINE_FUNCTION(TriggerTouch),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(beam, CBeam);

void CBeam::Spawn()
{
	pev->solid = SOLID_NOT; // Remove model & collisions
	Precache();
}

void CBeam::Precache()
{
	if (pev->owner)
		SetStartEntity(ENTINDEX(pev->owner));
	if (pev->aiment)
		SetEndEntity(ENTINDEX(pev->aiment));
}

void CBeam::SetStartEntity(int entityIndex)
{
	pev->sequence = (entityIndex & 0x0FFF) | (pev->sequence & 0xF000);
	pev->owner = g_engfuncs.pfnPEntityOfEntIndex(entityIndex);
}

void CBeam::SetEndEntity(int entityIndex)
{
	pev->skin = (entityIndex & 0x0FFF) | (pev->skin & 0xF000);
	pev->aiment = g_engfuncs.pfnPEntityOfEntIndex(entityIndex);
}

const Vector& CBeam::GetStartPos()
{
	if (GetType() == BEAM_ENTS)
	{
		edict_t* pent = g_engfuncs.pfnPEntityOfEntIndex(GetStartEntity());
		return pent->v.origin;
	}
	return pev->origin;
}

const Vector& CBeam::GetEndPos()
{
	int type = GetType();
	if (type == BEAM_POINTS || type == BEAM_HOSE)
	{
		return pev->angles;
	}

	edict_t* pent = g_engfuncs.pfnPEntityOfEntIndex(GetEndEntity());
	if (pent)
		return pent->v.origin;
	return pev->angles;
}

CBeam* CBeam::BeamCreate(const char* pSpriteName, int width)
{
	// Create a new entity with CBeam private data
	CBeam* pBeam = g_EntityDictionary->Create<CBeam>("beam");

	pBeam->BeamInit(pSpriteName, width);

	return pBeam;
}

void CBeam::BeamInit(const char* pSpriteName, int width)
{
	pev->flags |= FL_CUSTOMENTITY;
	SetColor(255, 255, 255);
	SetBrightness(255);
	SetNoise(0);
	SetFrame(0);
	SetScrollRate(0);
	pev->model = MAKE_STRING(pSpriteName);
	SetTexture(PrecacheModel(pSpriteName));
	SetWidth(width);
	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
}

void CBeam::PointsInit(const Vector& start, const Vector& end)
{
	SetType(BEAM_POINTS);
	SetStartPos(start);
	SetEndPos(end);
	SetStartAttachment(0);
	SetEndAttachment(0);
	RelinkBeam();
}

void CBeam::HoseInit(const Vector& start, const Vector& direction)
{
	SetType(BEAM_HOSE);
	SetStartPos(start);
	SetEndPos(direction);
	SetStartAttachment(0);
	SetEndAttachment(0);
	RelinkBeam();
}

void CBeam::PointEntInit(const Vector& start, int endIndex)
{
	SetType(BEAM_ENTPOINT);
	SetStartPos(start);
	SetEndEntity(endIndex);
	SetStartAttachment(0);
	SetEndAttachment(0);
	RelinkBeam();
}

void CBeam::EntsInit(int startIndex, int endIndex)
{
	SetType(BEAM_ENTS);
	SetStartEntity(startIndex);
	SetEndEntity(endIndex);
	SetStartAttachment(0);
	SetEndAttachment(0);
	RelinkBeam();
}

void CBeam::RelinkBeam()
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();

	pev->mins.x = std::min(startPos.x, endPos.x);
	pev->mins.y = std::min(startPos.y, endPos.y);
	pev->mins.z = std::min(startPos.z, endPos.z);
	pev->maxs.x = std::max(startPos.x, endPos.x);
	pev->maxs.y = std::max(startPos.y, endPos.y);
	pev->maxs.z = std::max(startPos.z, endPos.z);
	pev->mins = pev->mins - pev->origin;
	pev->maxs = pev->maxs - pev->origin;

	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);
}

#if 0
void CBeam::SetObjectCollisionBox()
{
	const Vector& startPos = GetStartPos(), & endPos = GetEndPos();

	pev->absmin.x = std::min(startPos.x, endPos.x);
	pev->absmin.y = std::min(startPos.y, endPos.y);
	pev->absmin.z = std::min(startPos.z, endPos.z);
	pev->absmax.x = std::max(startPos.x, endPos.x);
	pev->absmax.y = std::max(startPos.y, endPos.y);
	pev->absmax.z = std::max(startPos.z, endPos.z);
}
#endif

void CBeam::TriggerTouch(CBaseEntity* pOther)
{
	if ((pOther->pev->flags & (FL_CLIENT | FL_MONSTER)) != 0)
	{
		if (pev->owner)
		{
			CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
			pOwner->Use(pOther, this, USE_TOGGLE, 0);
		}
		CBaseEntity::Logger->debug("Firing targets!!!");
	}
}

CBaseEntity* CBeam::RandomTargetname(const char* szName)
{
	int total = 0;

	CBaseEntity* pEntity = nullptr;
	CBaseEntity* pNewEntity = nullptr;
	while ((pNewEntity = UTIL_FindEntityByTargetname(pNewEntity, szName)) != nullptr)
	{
		total++;
		if (RANDOM_LONG(0, total - 1) < 1)
			pEntity = pNewEntity;
	}
	return pEntity;
}

void CBeam::DoSparks(const Vector& start, const Vector& end)
{
	if ((pev->spawnflags & (SF_BEAM_SPARKSTART | SF_BEAM_SPARKEND)) != 0)
	{
		if ((pev->spawnflags & SF_BEAM_SPARKSTART) != 0)
		{
			UTIL_Sparks(start);
		}
		if ((pev->spawnflags & SF_BEAM_SPARKEND) != 0)
		{
			UTIL_Sparks(end);
		}
	}
}

class CLightning : public CBeam
{
	DECLARE_CLASS(CLightning, CBeam);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Activate() override;

	void StrikeThink();
	void DamageThink();
	void RandomArea();
	void RandomPoint(Vector& vecSrc);
	void Zap(const Vector& vecSrc, const Vector& vecDest);
	void StrikeUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	inline bool ServerSide()
	{
		if (m_life == 0 && (pev->spawnflags & SF_BEAM_RING) == 0)
			return true;
		return false;
	}

	void BeamUpdateVars();

	static CLightning* LightningCreate(const char* pSpriteName, int width)
	{
		auto lightning = g_EntityDictionary->Create<CLightning>("env_beam");

		lightning->BeamInit(pSpriteName, width);

		return lightning;
	}

	bool m_active;
	string_t m_iszStartEntity;
	string_t m_iszEndEntity;
	float m_life;
	int m_boltWidth;
	int m_noiseAmplitude;
	int m_brightness;
	int m_speed;
	float m_restrike;
	int m_spriteTexture;
	string_t m_iszSpriteName;
	int m_frameStart;

	float m_radius;
};

LINK_ENTITY_TO_CLASS(env_lightning, CLightning);
LINK_ENTITY_TO_CLASS(env_beam, CLightning);

BEGIN_DATAMAP(CLightning)
DEFINE_FIELD(m_active, FIELD_BOOLEAN),
	DEFINE_FIELD(m_iszStartEntity, FIELD_STRING),
	DEFINE_FIELD(m_iszEndEntity, FIELD_STRING),
	DEFINE_FIELD(m_life, FIELD_FLOAT),
	DEFINE_FIELD(m_boltWidth, FIELD_INTEGER),
	DEFINE_FIELD(m_noiseAmplitude, FIELD_INTEGER),
	DEFINE_FIELD(m_brightness, FIELD_INTEGER),
	DEFINE_FIELD(m_speed, FIELD_INTEGER),
	DEFINE_FIELD(m_restrike, FIELD_FLOAT),
	DEFINE_FIELD(m_spriteTexture, FIELD_INTEGER),
	DEFINE_FIELD(m_iszSpriteName, FIELD_STRING),
	DEFINE_FIELD(m_frameStart, FIELD_INTEGER),
	DEFINE_FIELD(m_radius, FIELD_FLOAT),
	DEFINE_FUNCTION(StrikeThink),
	DEFINE_FUNCTION(DamageThink),
	DEFINE_FUNCTION(StrikeUse),
	DEFINE_FUNCTION(ToggleUse),
	END_DATAMAP();

void CLightning::Spawn()
{
	if (FStringNull(m_iszSpriteName))
	{
		SetThink(&CLightning::SUB_Remove);
		return;
	}
	pev->solid = SOLID_NOT; // Remove model & collisions
	Precache();

	pev->dmgtime = gpGlobals->time;

	if (ServerSide())
	{
		SetThink(nullptr);
		if (pev->dmg > 0)
		{
			SetThink(&CLightning::DamageThink);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		if (!FStringNull(pev->targetname))
		{
			if ((pev->spawnflags & SF_BEAM_STARTON) == 0)
			{
				pev->effects = EF_NODRAW;
				m_active = false;
				pev->nextthink = 0;
			}
			else
				m_active = true;

			SetUse(&CLightning::ToggleUse);
		}
	}
	else
	{
		m_active = false;
		if (!FStringNull(pev->targetname))
		{
			SetUse(&CLightning::StrikeUse);
		}
		if (FStringNull(pev->targetname) || FBitSet(pev->spawnflags, SF_BEAM_STARTON))
		{
			SetThink(&CLightning::StrikeThink);
			pev->nextthink = gpGlobals->time + 1.0;
		}
	}
}

void CLightning::Precache()
{
	m_spriteTexture = PrecacheModel(STRING(m_iszSpriteName));
	CBeam::Precache();
}

void CLightning::Activate()
{
	if (ServerSide())
		BeamUpdateVars();
}

bool CLightning::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "LightningStart"))
	{
		m_iszStartEntity = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "LightningEnd"))
	{
		m_iszEndEntity = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "life"))
	{
		m_life = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "BoltWidth"))
	{
		m_boltWidth = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "NoiseAmplitude"))
	{
		m_noiseAmplitude = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "TextureScroll"))
	{
		m_speed = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "StrikeTime"))
	{
		m_restrike = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "texture"))
	{
		m_iszSpriteName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "framestart"))
	{
		m_frameStart = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "Radius"))
	{
		m_radius = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		return true;
	}

	return CBeam::KeyValue(pkvd);
}

void CLightning::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_active))
		return;
	if (m_active)
	{
		m_active = false;
		pev->effects |= EF_NODRAW;
		pev->nextthink = 0;
	}
	else
	{
		m_active = true;
		pev->effects &= ~EF_NODRAW;
		DoSparks(GetStartPos(), GetEndPos());
		if (pev->dmg > 0)
		{
			pev->nextthink = gpGlobals->time;
			pev->dmgtime = gpGlobals->time;
		}
	}
}

void CLightning::StrikeUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_active))
		return;

	if (m_active)
	{
		m_active = false;
		SetThink(nullptr);
	}
	else
	{
		SetThink(&CLightning::StrikeThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}

	if (!FBitSet(pev->spawnflags, SF_BEAM_TOGGLE))
		SetUse(nullptr);
}

bool IsPointEntity(CBaseEntity* pEnt)
{
	if (0 == pEnt->pev->modelindex)
		return true;
	if (pEnt->ClassnameIs("info_target") || pEnt->ClassnameIs("info_landmark") || pEnt->ClassnameIs("path_corner"))
		return true;

	return false;
}

void CLightning::StrikeThink()
{
	if (m_life != 0)
	{
		if ((pev->spawnflags & SF_BEAM_RANDOM) != 0)
			pev->nextthink = gpGlobals->time + m_life + RANDOM_FLOAT(0, m_restrike);
		else
			pev->nextthink = gpGlobals->time + m_life + m_restrike;
	}
	m_active = true;

	if (FStringNull(m_iszEndEntity))
	{
		if (FStringNull(m_iszStartEntity))
		{
			RandomArea();
		}
		else
		{
			CBaseEntity* pStart = RandomTargetname(STRING(m_iszStartEntity));
			if (pStart != nullptr)
				RandomPoint(pStart->pev->origin);
			else
				CBaseEntity::Logger->debug("env_beam: unknown entity \"{}\"", STRING(m_iszStartEntity));
		}
		return;
	}

	CBaseEntity* pStart = RandomTargetname(STRING(m_iszStartEntity));
	CBaseEntity* pEnd = RandomTargetname(STRING(m_iszEndEntity));

	if (pStart != nullptr && pEnd != nullptr)
	{
		if (IsPointEntity(pStart) || IsPointEntity(pEnd))
		{
			if ((pev->spawnflags & SF_BEAM_RING) != 0)
			{
				// don't work
				return;
			}
		}

		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		if (IsPointEntity(pStart) || IsPointEntity(pEnd))
		{
			if (!IsPointEntity(pEnd)) // One point entity must be in pEnd
			{
				CBaseEntity* pTemp;
				pTemp = pStart;
				pStart = pEnd;
				pEnd = pTemp;
			}
			if (!IsPointEntity(pStart)) // One sided
			{
				WRITE_BYTE(TE_BEAMENTPOINT);
				WRITE_SHORT(pStart->entindex());
				WRITE_COORD(pEnd->pev->origin.x);
				WRITE_COORD(pEnd->pev->origin.y);
				WRITE_COORD(pEnd->pev->origin.z);
			}
			else
			{
				WRITE_BYTE(TE_BEAMPOINTS);
				WRITE_COORD(pStart->pev->origin.x);
				WRITE_COORD(pStart->pev->origin.y);
				WRITE_COORD(pStart->pev->origin.z);
				WRITE_COORD(pEnd->pev->origin.x);
				WRITE_COORD(pEnd->pev->origin.y);
				WRITE_COORD(pEnd->pev->origin.z);
			}
		}
		else
		{
			if ((pev->spawnflags & SF_BEAM_RING) != 0)
				WRITE_BYTE(TE_BEAMRING);
			else
				WRITE_BYTE(TE_BEAMENTS);
			WRITE_SHORT(pStart->entindex());
			WRITE_SHORT(pEnd->entindex());
		}

		WRITE_SHORT(m_spriteTexture);
		WRITE_BYTE(m_frameStart);			 // framestart
		WRITE_BYTE((int)pev->framerate);	 // framerate
		WRITE_BYTE((int)(m_life * 10.0));	 // life
		WRITE_BYTE(m_boltWidth);			 // width
		WRITE_BYTE(m_noiseAmplitude);		 // noise
		WRITE_BYTE((int)pev->rendercolor.x); // r, g, b
		WRITE_BYTE((int)pev->rendercolor.y); // r, g, b
		WRITE_BYTE((int)pev->rendercolor.z); // r, g, b
		WRITE_BYTE(pev->renderamt);			 // brightness
		WRITE_BYTE(m_speed);				 // speed
		MESSAGE_END();
		DoSparks(pStart->pev->origin, pEnd->pev->origin);
		if (pev->dmg > 0)
		{
			TraceResult tr;
			UTIL_TraceLine(pStart->pev->origin, pEnd->pev->origin, dont_ignore_monsters, nullptr, &tr);
			BeamDamageInstant(&tr, pev->dmg);
		}
	}
}

void CBeam::BeamDamage(TraceResult* ptr)
{
	RelinkBeam();
	if (ptr->flFraction != 1.0 && ptr->pHit != nullptr)
	{
		CBaseEntity* pHit = CBaseEntity::Instance(ptr->pHit);
		if (pHit)
		{
			ClearMultiDamage();
			pHit->TraceAttack(this, pev->dmg * (gpGlobals->time - pev->dmgtime), (ptr->vecEndPos - pev->origin).Normalize(), ptr, DMG_ENERGYBEAM);
			ApplyMultiDamage(this, this);
			if ((pev->spawnflags & SF_BEAM_DECALS) != 0)
			{
				if (pHit->IsBSPModel())
					UTIL_DecalTrace(ptr, DECAL_BIGSHOT1 + RANDOM_LONG(0, 4));
			}
		}
	}
	pev->dmgtime = gpGlobals->time;
}

void CLightning::DamageThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	TraceResult tr;
	UTIL_TraceLine(GetStartPos(), GetEndPos(), dont_ignore_monsters, nullptr, &tr);
	BeamDamage(&tr);
}

void CLightning::Zap(const Vector& vecSrc, const Vector& vecDest)
{
#if 1
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z);
	WRITE_COORD(vecDest.x);
	WRITE_COORD(vecDest.y);
	WRITE_COORD(vecDest.z);
	WRITE_SHORT(m_spriteTexture);
	WRITE_BYTE(m_frameStart);			 // framestart
	WRITE_BYTE((int)pev->framerate);	 // framerate
	WRITE_BYTE((int)(m_life * 10.0));	 // life
	WRITE_BYTE(m_boltWidth);			 // width
	WRITE_BYTE(m_noiseAmplitude);		 // noise
	WRITE_BYTE((int)pev->rendercolor.x); // r, g, b
	WRITE_BYTE((int)pev->rendercolor.y); // r, g, b
	WRITE_BYTE((int)pev->rendercolor.z); // r, g, b
	WRITE_BYTE(pev->renderamt);			 // brightness
	WRITE_BYTE(m_speed);				 // speed
	MESSAGE_END();
#else
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_LIGHTNING);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z);
	WRITE_COORD(vecDest.x);
	WRITE_COORD(vecDest.y);
	WRITE_COORD(vecDest.z);
	WRITE_BYTE(10);
	WRITE_BYTE(50);
	WRITE_BYTE(40);
	WRITE_SHORT(m_spriteTexture);
	MESSAGE_END();
#endif
	DoSparks(vecSrc, vecDest);
}

void CLightning::RandomArea()
{
	int iLoops = 0;

	for (iLoops = 0; iLoops < 10; iLoops++)
	{
		Vector vecSrc = pev->origin;

		Vector vecDir1 = Vector(RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0));
		vecDir1 = vecDir1.Normalize();
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, edict(), &tr1);

		if (tr1.flFraction == 1.0)
			continue;

		Vector vecDir2;
		do
		{
			vecDir2 = Vector(RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0));
		} while (DotProduct(vecDir1, vecDir2) > 0);
		vecDir2 = vecDir2.Normalize();
		TraceResult tr2;
		UTIL_TraceLine(vecSrc, vecSrc + vecDir2 * m_radius, ignore_monsters, edict(), &tr2);

		if (tr2.flFraction == 1.0)
			continue;

		if ((tr1.vecEndPos - tr2.vecEndPos).Length() < m_radius * 0.1)
			continue;

		UTIL_TraceLine(tr1.vecEndPos, tr2.vecEndPos, ignore_monsters, edict(), &tr2);

		if (tr2.flFraction != 1.0)
			continue;

		Zap(tr1.vecEndPos, tr2.vecEndPos);

		break;
	}
}

void CLightning::RandomPoint(Vector& vecSrc)
{
	int iLoops = 0;

	for (iLoops = 0; iLoops < 10; iLoops++)
	{
		Vector vecDir1 = Vector(RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0), RANDOM_FLOAT(-1.0, 1.0));
		vecDir1 = vecDir1.Normalize();
		TraceResult tr1;
		UTIL_TraceLine(vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, edict(), &tr1);

		if ((tr1.vecEndPos - vecSrc).Length() < m_radius * 0.1)
			continue;

		if (tr1.flFraction == 1.0)
			continue;

		Zap(vecSrc, tr1.vecEndPos);
		break;
	}
}

void CLightning::BeamUpdateVars()
{
	auto pStart = UTIL_FindEntityByTargetname(nullptr, STRING(m_iszStartEntity));
	auto pEnd = UTIL_FindEntityByTargetname(nullptr, STRING(m_iszEndEntity));

	if (!pStart)
	{
		pStart = World;
	}

	if (!pEnd)
	{
		pEnd = World;
	}

	bool pointStart = IsPointEntity(pStart);
	bool pointEnd = IsPointEntity(pEnd);

	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
	pev->flags |= FL_CUSTOMENTITY;
	pev->model = m_iszSpriteName;
	SetTexture(m_spriteTexture);

	int beamType = BEAM_ENTS;
	if (pointStart || pointEnd)
	{
		if (!pointStart) // One point entity must be in pStart
		{
			// Swap start & end
			std::swap(pStart, pEnd);
			std::swap(pointStart, pointEnd);
		}
		if (!pointEnd)
			beamType = BEAM_ENTPOINT;
		else
			beamType = BEAM_POINTS;
	}

	SetType(beamType);
	if (beamType == BEAM_POINTS || beamType == BEAM_ENTPOINT || beamType == BEAM_HOSE)
	{
		SetStartPos(pStart->pev->origin);
		if (beamType == BEAM_POINTS || beamType == BEAM_HOSE)
			SetEndPos(pEnd->pev->origin);
		else
			SetEndEntity(pEnd->entindex());
	}
	else
	{
		SetStartEntity(pStart->entindex());
		SetEndEntity(pEnd->entindex());
	}

	RelinkBeam();

	SetWidth(m_boltWidth);
	SetNoise(m_noiseAmplitude);
	SetFrame(m_frameStart);
	SetScrollRate(m_speed);
	if ((pev->spawnflags & SF_BEAM_SHADEIN) != 0)
		SetFlags(BEAM_FSHADEIN);
	else if ((pev->spawnflags & SF_BEAM_SHADEOUT) != 0)
		SetFlags(BEAM_FSHADEOUT);
}

LINK_ENTITY_TO_CLASS(env_laser, CLaser);

BEGIN_DATAMAP(CLaser)
DEFINE_FIELD(m_pSprite, FIELD_CLASSPTR),
	DEFINE_FIELD(m_iszSpriteName, FIELD_STRING),
	DEFINE_FIELD(m_firePosition, FIELD_POSITION_VECTOR),
	DEFINE_FUNCTION(StrikeThink),
	END_DATAMAP();

void CLaser::Spawn()
{
	if (FStringNull(pev->model))
	{
		SetThink(&CLaser::SUB_Remove);
		return;
	}
	pev->solid = SOLID_NOT; // Remove model & collisions
	Precache();

	SetThink(&CLaser::StrikeThink);
	pev->flags |= FL_CUSTOMENTITY;

	PointsInit(pev->origin, pev->origin);

	if (!m_pSprite && !FStringNull(m_iszSpriteName))
		m_pSprite = CSprite::SpriteCreate(STRING(m_iszSpriteName), pev->origin, true);
	else
		m_pSprite = nullptr;

	if (m_pSprite)
		m_pSprite->SetTransparency(kRenderGlow, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, pev->renderfx);

	if (!FStringNull(pev->targetname) && (pev->spawnflags & SF_BEAM_STARTON) == 0)
		TurnOff();
	else
		TurnOn();
}

void CLaser::Precache()
{
	pev->modelindex = PrecacheModel(STRING(pev->model));
	if (!FStringNull(m_iszSpriteName))
		PrecacheModel(STRING(m_iszSpriteName));
}

bool CLaser::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "LaserTarget"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "width"))
	{
		SetWidth((int)atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "NoiseAmplitude"))
	{
		SetNoise(atoi(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "TextureScroll"))
	{
		SetScrollRate(atoi(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->model = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "EndSprite"))
	{
		m_iszSpriteName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "framestart"))
	{
		pev->frame = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		return true;
	}

	return CBeam::KeyValue(pkvd);
}

bool CLaser::IsOn()
{
	if ((pev->effects & EF_NODRAW) != 0)
		return false;
	return true;
}

void CLaser::TurnOff()
{
	pev->effects |= EF_NODRAW;
	pev->nextthink = 0;
	if (m_pSprite)
		m_pSprite->TurnOff();
}

void CLaser::TurnOn()
{
	pev->effects &= ~EF_NODRAW;
	if (m_pSprite)
		m_pSprite->TurnOn();
	pev->dmgtime = gpGlobals->time;
	pev->nextthink = gpGlobals->time;
}

void CLaser::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool active = IsOn();

	if (!ShouldToggle(useType, active))
		return;
	if (active)
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}

void CLaser::FireAtPoint(TraceResult& tr)
{
	SetEndPos(tr.vecEndPos);
	if (m_pSprite)
		m_pSprite->SetOrigin(tr.vecEndPos);

	BeamDamage(&tr);
	DoSparks(GetStartPos(), tr.vecEndPos);
}

void CLaser::StrikeThink()
{
	CBaseEntity* pEnd = RandomTargetname(STRING(pev->message));

	if (pEnd)
		m_firePosition = pEnd->pev->origin;

	TraceResult tr;

	UTIL_TraceLine(pev->origin, m_firePosition, dont_ignore_monsters, nullptr, &tr);
	FireAtPoint(tr);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CLaser::UpdateOnRemove()
{
	if (m_pSprite)
	{
		UTIL_Remove(m_pSprite);
		m_pSprite = nullptr;
	}

	CBeam::UpdateOnRemove();
}

class CGlow : public CPointEntity
{
	DECLARE_CLASS(CGlow, CPointEntity);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Think() override;
	void Animate(float frames);

	float m_lastTime;
	float m_maxFrame;
};

LINK_ENTITY_TO_CLASS(env_glow, CGlow);

BEGIN_DATAMAP(CGlow)
DEFINE_FIELD(m_lastTime, FIELD_TIME),
	DEFINE_FIELD(m_maxFrame, FIELD_FLOAT),
	END_DATAMAP();

void CGlow::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	PrecacheModel(STRING(pev->model));
	SetModel(STRING(pev->model));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
	if (m_maxFrame > 1.0 && pev->framerate != 0)
		pev->nextthink = gpGlobals->time + 0.1;

	m_lastTime = gpGlobals->time;
}

void CGlow::Think()
{
	Animate(pev->framerate * (gpGlobals->time - m_lastTime));

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CGlow::Animate(float frames)
{
	if (m_maxFrame > 0)
		pev->frame = fmod(pev->frame + frames, m_maxFrame);
}

LINK_ENTITY_TO_CLASS(env_sprite, CSprite);

BEGIN_DATAMAP(CSprite)
DEFINE_FIELD(m_lastTime, FIELD_TIME),
	DEFINE_FIELD(m_maxFrame, FIELD_FLOAT),
	DEFINE_FUNCTION(AnimateThink),
	DEFINE_FUNCTION(ExpandThink),
	DEFINE_FUNCTION(AnimateUntilDead),
	END_DATAMAP();

void CSprite::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	Precache();
	SetModel(STRING(pev->model));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
	if (!FStringNull(pev->targetname) && (pev->spawnflags & SF_SPRITE_STARTON) == 0)
		TurnOff();
	else
		TurnOn();

	// Worldcraft only sets y rotation, copy to Z
	if (pev->angles.y != 0 && pev->angles.z == 0)
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}
}

void CSprite::Precache()
{
	PrecacheModel(STRING(pev->model));

	// Reset attachment after save/restore
	if (pev->aiment)
		SetAttachment(pev->aiment, pev->body);
	else
	{
		// Clear attachment
		pev->skin = 0;
		pev->body = 0;
	}
}

void CSprite::SpriteInit(const char* pSpriteName, const Vector& origin)
{
	pev->model = MAKE_STRING(pSpriteName);
	pev->origin = origin;
	Spawn();
}

CSprite* CSprite::SpriteCreate(const char* pSpriteName, const Vector& origin, bool animate)
{
	CSprite* pSprite = g_EntityDictionary->Create<CSprite>("env_sprite");
	pSprite->SpriteInit(pSpriteName, origin);
	pSprite->pev->solid = SOLID_NOT;
	pSprite->pev->movetype = MOVETYPE_NOCLIP;
	if (animate)
		pSprite->TurnOn();

	return pSprite;
}

void CSprite::AnimateThink()
{
	Animate(pev->framerate * (gpGlobals->time - m_lastTime));

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CSprite::AnimateUntilDead()
{
	if (gpGlobals->time > pev->dmgtime)
		UTIL_Remove(this);
	else
	{
		AnimateThink();
		pev->nextthink = gpGlobals->time;
	}
}

void CSprite::Expand(float scaleSpeed, float fadeSpeed)
{
	pev->speed = scaleSpeed;
	pev->health = fadeSpeed;
	SetThink(&CSprite::ExpandThink);

	pev->nextthink = gpGlobals->time;
	m_lastTime = gpGlobals->time;
}

void CSprite::ExpandThink()
{
	float frametime = gpGlobals->time - m_lastTime;
	pev->scale += pev->speed * frametime;
	pev->renderamt -= pev->health * frametime;
	if (pev->renderamt <= 0)
	{
		pev->renderamt = 0;
		UTIL_Remove(this);
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1;
		m_lastTime = gpGlobals->time;
	}
}

void CSprite::Animate(float frames)
{
	pev->frame += frames;
	if (pev->frame > m_maxFrame)
	{
		if ((pev->spawnflags & SF_SPRITE_ONCE) != 0)
		{
			TurnOff();
		}
		else
		{
			if (m_maxFrame > 0)
				pev->frame = fmod(pev->frame, m_maxFrame);
		}
	}
}

void CSprite::TurnOff()
{
	pev->effects = EF_NODRAW;
	pev->nextthink = 0;
}

void CSprite::TurnOn()
{
	pev->effects = 0;
	if ((0 != pev->framerate && m_maxFrame > 1.0) || (pev->spawnflags & SF_SPRITE_ONCE) != 0)
	{
		SetThink(&CSprite::AnimateThink);
		pev->nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}
	pev->frame = 0;
}

void CSprite::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool on = pev->effects != EF_NODRAW;
	if (ShouldToggle(useType, on))
	{
		if (on)
		{
			TurnOff();
		}
		else
		{
			TurnOn();
		}
	}
}

class CGibShooter : public CBaseDelay
{
	DECLARE_CLASS(CGibShooter, CBaseDelay);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void ShootThink();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	virtual CGib* CreateGib();

	int m_iGibs;
	int m_iGibCapacity;
	int m_iGibMaterial;
	int m_iGibModelIndex;
	float m_flGibVelocity;
	float m_flVariance;
	float m_flGibLife;
};

BEGIN_DATAMAP(CGibShooter)
DEFINE_FIELD(m_iGibs, FIELD_INTEGER),
	DEFINE_FIELD(m_iGibCapacity, FIELD_INTEGER),
	DEFINE_FIELD(m_iGibMaterial, FIELD_INTEGER),
	DEFINE_FIELD(m_iGibModelIndex, FIELD_INTEGER),
	DEFINE_FIELD(m_flGibVelocity, FIELD_FLOAT),
	DEFINE_FIELD(m_flVariance, FIELD_FLOAT),
	DEFINE_FIELD(m_flGibLife, FIELD_FLOAT),
	DEFINE_FUNCTION(ShootThink),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(gibshooter, CGibShooter);

void CGibShooter::Precache()
{
	if (g_Language == LANGUAGE_GERMAN)
	{
		m_iGibModelIndex = PrecacheModel("models/germanygibs.mdl");
	}
	else
	{
		m_iGibModelIndex = PrecacheModel("models/hgibs.mdl");
	}
}

bool CGibShooter::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iGibs"))
	{
		m_iGibs = m_iGibCapacity = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flVelocity"))
	{
		m_flGibVelocity = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flVariance"))
	{
		m_flVariance = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flGibLife"))
	{
		m_flGibLife = atof(pkvd->szValue);
		return true;
	}

	return CBaseDelay::KeyValue(pkvd);
}

void CGibShooter::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&CGibShooter::ShootThink);
	pev->nextthink = gpGlobals->time;
}

void CGibShooter::Spawn()
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if (m_flDelay == 0)
	{
		m_flDelay = 0.1;
	}

	if (m_flGibLife == 0)
	{
		m_flGibLife = 25;
	}

	SetMovedir(this);
	pev->body = MODEL_FRAMES(m_iGibModelIndex);
}

CGib* CGibShooter::CreateGib()
{
	if (CVAR_GET_FLOAT("violence_hgibs") == 0)
		return nullptr;

	CGib* pGib = g_EntityDictionary->Create<CGib>("gib");
	pGib->Spawn("models/hgibs.mdl");
	pGib->m_bloodColor = BLOOD_COLOR_RED;

	if (pev->body <= 1)
	{
		Logger->debug("GibShooter Body is <= 1!");
	}

	pGib->pev->body = RANDOM_LONG(1, pev->body - 1); // avoid throwing random amounts of the 0th gib. (skull).

	return pGib;
}

void CGibShooter::ShootThink()
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	Vector vecShootDir;

	vecShootDir = pev->movedir;

	vecShootDir = vecShootDir + gpGlobals->v_right * RANDOM_FLOAT(-1, 1) * m_flVariance;
	vecShootDir = vecShootDir + gpGlobals->v_forward * RANDOM_FLOAT(-1, 1) * m_flVariance;
	vecShootDir = vecShootDir + gpGlobals->v_up * RANDOM_FLOAT(-1, 1) * m_flVariance;

	vecShootDir = vecShootDir.Normalize();
	CGib* pGib = CreateGib();

	if (pGib)
	{
		pGib->pev->origin = pev->origin;
		pGib->pev->velocity = vecShootDir * m_flGibVelocity;

		pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
		pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);

		float thinkTime = pGib->pev->nextthink - gpGlobals->time;

		pGib->m_lifeTime = (m_flGibLife * RANDOM_FLOAT(0.95, 1.05)); // +/- 5%
		if (pGib->m_lifeTime < thinkTime)
		{
			pGib->pev->nextthink = gpGlobals->time + pGib->m_lifeTime;
			pGib->m_lifeTime = 0;
		}
	}

	if (--m_iGibs <= 0)
	{
		if ((pev->spawnflags & SF_GIBSHOOTER_REPEATABLE) != 0)
		{
			m_iGibs = m_iGibCapacity;
			SetThink(nullptr);
			pev->nextthink = gpGlobals->time;
		}
		else
		{
			SetThink(&CGibShooter::SUB_Remove);
			pev->nextthink = gpGlobals->time;
		}
	}
}

class CEnvShooter : public CGibShooter
{
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	CGib* CreateGib() override;
};

LINK_ENTITY_TO_CLASS(env_shooter, CEnvShooter);

bool CEnvShooter::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "shootmodel"))
	{
		pev->model = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "shootsounds"))
	{
		int iNoise = atoi(pkvd->szValue);

		switch (iNoise)
		{
		case 0:
			m_iGibMaterial = matGlass;
			break;
		case 1:
			m_iGibMaterial = matWood;
			break;
		case 2:
			m_iGibMaterial = matMetal;
			break;
		case 3:
			m_iGibMaterial = matFlesh;
			break;
		case 4:
			m_iGibMaterial = matRocks;
			break;

		default:
		case -1:
			m_iGibMaterial = matNone;
			break;
		}

		return true;
	}

	return CGibShooter::KeyValue(pkvd);
}

void CEnvShooter::Precache()
{
	m_iGibModelIndex = PrecacheModel(STRING(pev->model));
	CBreakable::MaterialSoundPrecache(this, (Materials)m_iGibMaterial);
}

CGib* CEnvShooter::CreateGib()
{
	CGib* pGib = g_EntityDictionary->Create<CGib>("gib");

	pGib->Spawn(STRING(pev->model));

	int bodyPart = 0;

	if (pev->body > 1)
		bodyPart = RANDOM_LONG(0, pev->body - 1);

	pGib->pev->body = bodyPart;
	pGib->m_bloodColor = DONT_BLEED;
	pGib->m_material = m_iGibMaterial;

	pGib->pev->rendermode = pev->rendermode;
	pGib->pev->renderamt = pev->renderamt;
	pGib->pev->rendercolor = pev->rendercolor;
	pGib->pev->renderfx = pev->renderfx;
	pGib->pev->scale = pev->scale;
	pGib->pev->skin = pev->skin;

	return pGib;
}

/**
 *	@brief Blood effects
 */
class CBlood : public CPointEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline int Color() { return pev->impulse; }
	inline float BloodAmount() { return pev->dmg; }

	inline void SetColor(int color) { pev->impulse = color; }
	inline void SetBloodAmount(float amount) { pev->dmg = amount; }

	Vector Direction();
	Vector BloodPosition(CBaseEntity* pActivator);

private:
};

LINK_ENTITY_TO_CLASS(env_blood, CBlood);

#define SF_BLOOD_RANDOM 0x0001
#define SF_BLOOD_STREAM 0x0002
#define SF_BLOOD_PLAYER 0x0004
#define SF_BLOOD_DECAL 0x0008

void CBlood::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	SetMovedir(this);
}

bool CBlood::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "color"))
	{
		int color = atoi(pkvd->szValue);
		switch (color)
		{
		case 1:
			SetColor(BLOOD_COLOR_YELLOW);
			break;
		default:
			SetColor(BLOOD_COLOR_RED);
			break;
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "amount"))
	{
		SetBloodAmount(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

Vector CBlood::Direction()
{
	if ((pev->spawnflags & SF_BLOOD_RANDOM) != 0)
		return UTIL_RandomBloodVector();

	return pev->movedir;
}

Vector CBlood::BloodPosition(CBaseEntity* pActivator)
{
	if ((pev->spawnflags & SF_BLOOD_PLAYER) != 0)
	{
		CBasePlayer* pPlayer = ToBasePlayer(pActivator);

		if (!pPlayer && !g_pGameRules->IsMultiplayer())
		{
			pPlayer = UTIL_GetLocalPlayer();
		}

		if (pPlayer)
			return (pPlayer->pev->origin + pPlayer->pev->view_ofs) + Vector(RANDOM_FLOAT(-10, 10), RANDOM_FLOAT(-10, 10), RANDOM_FLOAT(-10, 10));
	}

	return pev->origin;
}

void CBlood::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if ((pev->spawnflags & SF_BLOOD_STREAM) != 0)
		UTIL_BloodStream(BloodPosition(pActivator), Direction(), (Color() == BLOOD_COLOR_RED) ? 70 : Color(), BloodAmount());
	else
		UTIL_BloodDrips(BloodPosition(pActivator), Direction(), Color(), BloodAmount());

	if ((pev->spawnflags & SF_BLOOD_DECAL) != 0)
	{
		Vector forward = Direction();
		Vector start = BloodPosition(pActivator);
		TraceResult tr;

		UTIL_TraceLine(start, start + forward * BloodAmount() * 2, ignore_monsters, nullptr, &tr);
		if (tr.flFraction != 1.0)
			UTIL_BloodDecalTrace(&tr, Color());
	}
}

/**
 *	@brief Screen shake
 */
class CShake : public CPointEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline float Amplitude() { return pev->scale; }
	inline float Frequency() { return pev->dmg_save; }
	inline float Duration() { return pev->dmg_take; }
	inline float Radius() { return pev->dmg; }

	inline void SetAmplitude(float amplitude) { pev->scale = amplitude; }
	inline void SetFrequency(float frequency) { pev->dmg_save = frequency; }
	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetRadius(float radius) { pev->dmg = radius; }

private:
};

LINK_ENTITY_TO_CLASS(env_shake, CShake);

// pev->scale is amplitude
// pev->dmg_save is frequency
// pev->dmg_take is duration
// pev->dmg is radius
// radius of 0 means all players
// NOTE: UTIL_ScreenShake() will only shake players who are on the ground

#define SF_SHAKE_EVERYONE 0x0001 // Don't check radius
// UNDONE: These don't work yet
#define SF_SHAKE_DISRUPT 0x0002 // Disrupt controls
#define SF_SHAKE_INAIR 0x0004	// Shake players in air

void CShake::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	if ((pev->spawnflags & SF_SHAKE_EVERYONE) != 0)
		pev->dmg = 0;
}

bool CShake::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "amplitude"))
	{
		SetAmplitude(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "frequency"))
	{
		SetFrequency(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))
	{
		SetRadius(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CShake::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	UTIL_ScreenShake(pev->origin, Amplitude(), Frequency(), Duration(), Radius());
}

class CFade : public CPointEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

	inline float Duration() { return pev->dmg_take; }
	inline float HoldTime() { return pev->dmg_save; }

	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }

private:
};

LINK_ENTITY_TO_CLASS(env_fade, CFade);

// pev->dmg_take is duration
// pev->dmg_save is hold duration
#define SF_FADE_IN 0x0001		// Fade in, not out
#define SF_FADE_MODULATE 0x0002 // Modulate, don't blend
#define SF_FADE_ONLYONE 0x0004

void CFade::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
}

bool CFade::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CFade::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	int fadeFlags = 0;

	if ((pev->spawnflags & SF_FADE_IN) == 0)
		fadeFlags |= FFADE_OUT;

	if ((pev->spawnflags & SF_FADE_MODULATE) != 0)
		fadeFlags |= FFADE_MODULATE;

	if ((pev->spawnflags & SF_FADE_ONLYONE) != 0)
	{
		if (pActivator->IsNetClient())
		{
			UTIL_ScreenFade(pActivator, pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags);
		}
	}
	else
	{
		UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, fadeFlags);
	}
	SUB_UseTargets(this, USE_TOGGLE, 0);
}

class CMessage : public CPointEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool KeyValue(KeyValueData* pkvd) override;

private:
};

LINK_ENTITY_TO_CLASS(env_message, CMessage);

void CMessage::Spawn()
{
	Precache();

	pev->message = ALLOC_ESCAPED_STRING(STRING(pev->message));

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	switch (pev->impulse)
	{
	case 1: // Medium radius
		pev->speed = ATTN_STATIC;
		break;

	case 2: // Large radius
		pev->speed = ATTN_NORM;
		break;

	case 3: // EVERYWHERE
		pev->speed = ATTN_NONE;
		break;

	default:
	case 0: // Small radius
		pev->speed = ATTN_IDLE;
		break;
	}
	pev->impulse = 0;

	// No volume, use normal
	if (pev->scale <= 0)
		pev->scale = 1.0;
}

void CMessage::Precache()
{
	if (!FStringNull(pev->noise))
		PrecacheSound(STRING(pev->noise));
}

bool CMessage::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "messagesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "messagevolume"))
	{
		pev->scale = atof(pkvd->szValue) * 0.1;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "messageattenuation"))
	{
		pev->impulse = atoi(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CMessage::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if ((pev->spawnflags & SF_MESSAGE_ALL) != 0)
		UTIL_ShowMessageAll(STRING(pev->message));
	else
	{
		CBasePlayer* pPlayer = ToBasePlayer(pActivator);
		if (!pPlayer  && !g_pGameRules->IsMultiplayer())
		{
			pPlayer = UTIL_GetLocalPlayer();
		}

		if (pPlayer)
			UTIL_ShowMessage(STRING(pev->message), pPlayer);
	}
	if (!FStringNull(pev->noise))
	{
		EmitSound(CHAN_BODY, STRING(pev->noise), pev->scale, pev->speed);
	}
	if ((pev->spawnflags & SF_MESSAGE_ONCE) != 0)
		UTIL_Remove(this);

	SUB_UseTargets(this, USE_TOGGLE, 0);
}

/**
 *	@brief FunnelEffect
 */
class CEnvFunnel : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int m_iSprite; // Don't save, precache
};

void CEnvFunnel::Precache()
{
	m_iSprite = PrecacheModel("sprites/flare6.spr");
}

LINK_ENTITY_TO_CLASS(env_funnel, CEnvFunnel);

void CEnvFunnel::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_LARGEFUNNEL);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT(m_iSprite);

	if ((pev->spawnflags & SF_FUNNEL_REVERSE) != 0) // funnel flows in reverse?
	{
		WRITE_SHORT(1);
	}
	else
	{
		WRITE_SHORT(0);
	}


	MESSAGE_END();

	SetThink(&CEnvFunnel::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

void CEnvFunnel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

/**
 *	@brief Beverage Dispenser
 *	overloaded pev->frags, is now a flag for whether or not a can is stuck in the dispenser.
 *	overloaded pev->health, is now how many cans remain in the machine.
 */
class CEnvBeverage : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

void CEnvBeverage::Precache()
{
	PrecacheModel("models/can.mdl");
	PrecacheSound("weapons/g_bounce3.wav");
}

LINK_ENTITY_TO_CLASS(env_beverage, CEnvBeverage);

void CEnvBeverage::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pev->frags != 0 || pev->health <= 0)
	{
		// no more cans while one is waiting in the dispenser, or if I'm out of cans.
		return;
	}

	CBaseEntity* pCan = CBaseEntity::Create("item_sodacan", pev->origin, pev->angles, this);

	if (pev->skin == 6)
	{
		// random
		pCan->pev->skin = RANDOM_LONG(0, 5);
	}
	else
	{
		pCan->pev->skin = pev->skin;
	}

	pev->frags = 1;
	pev->health--;

	// SetThink (SUB_Remove);
	// pev->nextthink = gpGlobals->time;
}

void CEnvBeverage::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->frags = 0;

	if (pev->health == 0)
	{
		pev->health = 10;
	}
}

/**
 *	@brief Soda can
 */
class CItemSoda : public CBaseEntity
{
	DECLARE_CLASS(CItemSoda, CBaseEntity);
	DECLARE_DATAMAP();

public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	void CanThink();
	void CanTouch(CBaseEntity* pOther);
};

BEGIN_DATAMAP(CItemSoda)
DEFINE_FUNCTION(CanThink),
	DEFINE_FUNCTION(CanTouch),
	END_DATAMAP();

void CItemSoda::OnCreate()
{
	CBaseEntity::OnCreate();

	pev->model = MAKE_STRING("models/can.mdl");
}

void CItemSoda::Precache()
{
	PrecacheModel(STRING(pev->model));
}

LINK_ENTITY_TO_CLASS(item_sodacan, CItemSoda);

void CItemSoda::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;

	SetModel(STRING(pev->model));
	SetSize(Vector(0, 0, 0), Vector(0, 0, 0));

	SetThink(&CItemSoda::CanThink);
	pev->nextthink = gpGlobals->time + 0.5;
}

void CItemSoda::CanThink()
{
	EmitSound(CHAN_WEAPON, "weapons/g_bounce3.wav", 1, ATTN_NORM);

	pev->solid = SOLID_TRIGGER;
	SetSize(Vector(-8, -8, 0), Vector(8, 8, 8));
	SetThink(nullptr);
	SetTouch(&CItemSoda::CanTouch);
}

void CItemSoda::CanTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
	{
		return;
	}

	// spoit sound here

	pOther->GiveHealth(1, DMG_GENERIC); // a bit of health.

	if (!FNullEnt(pev->owner))
	{
		// tell the machine the can was taken
		pev->owner->v.frags = 0;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	SetTouch(nullptr);
	SetThink(&CItemSoda::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

const int SF_WARPBALL_FIRE_ONCE = 1 << 0;
const int SF_WARPBALL_DELAYED_DAMAGE = 1 << 1;

/**
 *	@brief Alien teleportation effect
 */
class CWarpBall : public CBaseEntity
{
	DECLARE_CLASS(CWarpBall, CBaseEntity);
	DECLARE_DATAMAP();

public:
	int Classify() override { return CLASS_NONE; }

	bool KeyValue(KeyValueData* pkvd) override;

	void Precache() override;
	void Spawn() override;

	void UpdateOnRemove() override;

	void WarpBallUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void BallThink();

	static CWarpBall* CreateWarpBall(Vector vecOrigin)
	{
		auto warpBall = g_EntityDictionary->Create<CWarpBall>("env_warpball");

		warpBall->SetOrigin(vecOrigin);

		warpBall->Spawn();

		return warpBall;
	}

	CLightning* m_pBeams;
	CSprite* m_pSprite;
	int m_iBeams;
	float m_flLastTime;
	float m_flMaxFrame;
	float m_flBeamRadius;
	string_t m_iszWarpTarget;
	float m_flWarpStart;
	float m_flDamageDelay;
	float m_flTargetDelay;
	bool m_fPlaying;
	bool m_fDamageApplied;
	bool m_fBeamsCleared;
};

LINK_ENTITY_TO_CLASS(env_warpball, CWarpBall);

BEGIN_DATAMAP(CWarpBall)
DEFINE_FIELD(m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(m_flLastTime, FIELD_FLOAT),
	DEFINE_FIELD(m_flMaxFrame, FIELD_FLOAT),
	DEFINE_FIELD(m_flBeamRadius, FIELD_FLOAT),
	DEFINE_FIELD(m_iszWarpTarget, FIELD_STRING),
	DEFINE_FIELD(m_flWarpStart, FIELD_FLOAT),
	DEFINE_FIELD(m_flDamageDelay, FIELD_FLOAT),
	DEFINE_FIELD(m_flTargetDelay, FIELD_FLOAT),
	DEFINE_FIELD(m_fPlaying, FIELD_BOOLEAN),
	DEFINE_FIELD(m_fDamageApplied, FIELD_BOOLEAN),
	DEFINE_FIELD(m_fBeamsCleared, FIELD_BOOLEAN),
	DEFINE_FIELD(m_pBeams, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pSprite, FIELD_CLASSPTR),
	DEFINE_FUNCTION(WarpBallUse),
	DEFINE_FUNCTION(BallThink),
	END_DATAMAP();

bool CWarpBall::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("radius", pkvd->szKeyName))
	{
		m_flBeamRadius = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq("warp_target", pkvd->szKeyName))
	{
		m_iszWarpTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq("damage_delay", pkvd->szKeyName))
	{
		m_flDamageDelay = atof(pkvd->szValue);
		return true;
	}

	return false;
}

void CWarpBall::Precache()
{
	PrecacheModel("sprites/Fexplo1.spr");
	PrecacheModel("sprites/XFlare1.spr");
	PrecacheModel("sprites/lgtning.spr");
	PrecacheSound("debris/alien_teleport.wav");
}

void CWarpBall::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	SetOrigin(pev->origin);
	SetSize(g_vecZero, g_vecZero);

	pev->rendermode = kRenderGlow;
	pev->renderamt = 255;
	pev->renderfx = kRenderFxNoDissipation;
	pev->framerate = 10;

	m_pSprite = CSprite::SpriteCreate("sprites/Fexplo1.spr", pev->origin, true);
	m_pSprite->TurnOff();

	SetUse(&CWarpBall::WarpBallUse);
}

void CWarpBall::UpdateOnRemove()
{
	if (m_pBeams)
	{
		UTIL_Remove(m_pBeams);
		m_pBeams = nullptr;
	}

	if (m_pSprite)
	{
		UTIL_Remove(m_pSprite);
		m_pSprite = nullptr;
	}

	CBaseEntity::UpdateOnRemove();
}

void CWarpBall::WarpBallUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!m_fPlaying)
	{
		if (!FStringNull(m_iszWarpTarget))
		{
			// TODO: don't use the old engine function
			auto targetEntity = g_engfuncs.pfnFindEntityByString(nullptr, "targetname", STRING(m_iszWarpTarget));
			if (targetEntity)
				SetOrigin(targetEntity->v.origin);
		}

		SetModel("sprites/XFlare1.spr");

		m_flMaxFrame = MODEL_FRAMES(pev->modelindex) - 1;

		pev->rendercolor.x = 77;
		pev->rendercolor.y = 210;
		pev->rendercolor.z = 130;
		pev->scale = 1.2;
		pev->frame = 0;

		if (m_pSprite)
		{
			m_pSprite->SetTransparency(kRenderGlow, 77, 210, 130, 255, kRenderFxNoDissipation);
			m_pSprite->SetScale(1);
			m_pSprite->pev->framerate = 10;
			m_pSprite->TurnOn();
		}

		if (!m_pBeams)
		{
			m_pBeams = CLightning::LightningCreate("sprites/lgtning.spr", 18);

			m_pBeams->m_iszSpriteName = MAKE_STRING("sprites/lgtning.spr");

			m_pBeams->pev->origin = pev->origin;
			m_pBeams->SetOrigin(pev->origin);

			m_pBeams->m_restrike = -0.5;
			m_pBeams->m_noiseAmplitude = 65;
			m_pBeams->m_boltWidth = 18;
			m_pBeams->m_life = 0.5;

			m_pBeams->SetColor(0, 255, 0);

			m_pBeams->pev->spawnflags |= SF_BEAM_SPARKEND;
			m_pBeams->pev->spawnflags |= SF_BEAM_TOGGLE;

			m_pBeams->m_radius = m_flBeamRadius;
			m_pBeams->m_iszStartEntity = pev->targetname;

			m_pBeams->BeamUpdateVars();
		}

		if (m_pBeams)
		{
			m_pBeams->pev->solid = 0;
			m_pBeams->Precache();
			m_pBeams->SetThink(&CLightning::StrikeThink);
			m_pBeams->pev->nextthink = gpGlobals->time + 0.1;
		}

		SetThink(&CWarpBall::BallThink);
		pev->nextthink = gpGlobals->time + 0.1;

		m_flLastTime = gpGlobals->time;
		m_fBeamsCleared = false;
		m_fPlaying = true;

		if (m_flDamageDelay == 0)
		{
			::RadiusDamage(pev->origin, this, this, 300, 48, CLASS_NONE, DMG_SHOCK);
			m_fDamageApplied = true;
		}
		else
		{
			m_fDamageApplied = false;
		}

		SUB_UseTargets(this, USE_TOGGLE, 0);

		UTIL_ScreenShake(pev->origin, 4, 100, 2, 1000);

		m_flWarpStart = gpGlobals->time;

		EmitSound(CHAN_WEAPON, "debris/alien_teleport.wav", VOL_NORM, ATTN_NORM);
	}
}

void CWarpBall::BallThink()
{
	pev->frame = ((gpGlobals->time - m_flLastTime) * pev->framerate) + pev->frame;

	if (pev->frame > m_flMaxFrame)
	{
		SetModel("");

		SetThink(nullptr);

		if ((pev->spawnflags & SF_WARPBALL_FIRE_ONCE) != 0)
			UTIL_Remove(this);

		if (m_pSprite)
			m_pSprite->TurnOff();

		m_fPlaying = false;
	}
	else
	{
		// TODO: this flag is probably supposed to be a "do radius damage" flag, but it isn't used in the Use method
		if ((pev->spawnflags & SF_WARPBALL_DELAYED_DAMAGE) != 0 && !m_fDamageApplied && (gpGlobals->time - m_flWarpStart) >= m_flDamageDelay)
		{
			::RadiusDamage(pev->origin, this, this, 300, 48, CLASS_NONE, DMG_SHOCK);
			m_fDamageApplied = true;
		}

		if (m_pBeams)
		{
			if (pev->frame >= (m_flMaxFrame - 4.0))
			{
				m_pBeams->SetThink(nullptr);
				m_pBeams->pev->nextthink = gpGlobals->time;
			}
		}

		pev->nextthink = gpGlobals->time + 0.1;
		m_flLastTime = gpGlobals->time;
	}
}
