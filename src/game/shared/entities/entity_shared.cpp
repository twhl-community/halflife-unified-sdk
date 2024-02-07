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
#include "CBaseEntity.h"

#ifndef CLIENT_DLL
#include "EntityTemplateSystem.h"
#endif

BEGIN_DATAMAP_NOBASE(entvars_t)
DEFINE_FIELD(classname, FIELD_STRING),
	DEFINE_GLOBAL_FIELD(globalname, FIELD_STRING),

	DEFINE_FIELD(origin, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(oldorigin, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(velocity, FIELD_VECTOR),
	DEFINE_FIELD(basevelocity, FIELD_VECTOR),
	DEFINE_FIELD(movedir, FIELD_VECTOR),

	DEFINE_FIELD(angles, FIELD_VECTOR),
	DEFINE_FIELD(avelocity, FIELD_VECTOR),
	DEFINE_FIELD(punchangle, FIELD_VECTOR),
	DEFINE_FIELD(v_angle, FIELD_VECTOR),
	DEFINE_FIELD(fixangle, FIELD_FLOAT),
	DEFINE_FIELD(idealpitch, FIELD_FLOAT),
	DEFINE_FIELD(pitch_speed, FIELD_FLOAT),
	DEFINE_FIELD(ideal_yaw, FIELD_FLOAT),
	DEFINE_FIELD(yaw_speed, FIELD_FLOAT),

	DEFINE_FIELD(modelindex, FIELD_INTEGER),
	DEFINE_GLOBAL_FIELD(model, FIELD_MODELNAME),

	DEFINE_FIELD(viewmodel, FIELD_MODELNAME),
	DEFINE_FIELD(weaponmodel, FIELD_MODELNAME),

	DEFINE_FIELD(absmin, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(absmax, FIELD_POSITION_VECTOR),
	DEFINE_GLOBAL_FIELD(mins, FIELD_VECTOR),
	DEFINE_GLOBAL_FIELD(maxs, FIELD_VECTOR),
	DEFINE_GLOBAL_FIELD(size, FIELD_VECTOR),

	DEFINE_FIELD(ltime, FIELD_TIME),
	DEFINE_FIELD(nextthink, FIELD_TIME),

	DEFINE_FIELD(solid, FIELD_INTEGER),
	DEFINE_FIELD(movetype, FIELD_INTEGER),

	DEFINE_FIELD(skin, FIELD_INTEGER),
	DEFINE_FIELD(body, FIELD_INTEGER),
	DEFINE_FIELD(effects, FIELD_INTEGER),

	DEFINE_FIELD(gravity, FIELD_FLOAT),
	DEFINE_FIELD(friction, FIELD_FLOAT),
	DEFINE_FIELD(light_level, FIELD_FLOAT),

	DEFINE_FIELD(frame, FIELD_FLOAT),
	DEFINE_FIELD(scale, FIELD_FLOAT),
	DEFINE_FIELD(sequence, FIELD_INTEGER),
	DEFINE_FIELD(animtime, FIELD_TIME),
	DEFINE_FIELD(framerate, FIELD_FLOAT),
	DEFINE_ARRAY(controller, FIELD_CHARACTER, NUM_ENT_CONTROLLERS),
	DEFINE_ARRAY(blending, FIELD_CHARACTER, NUM_ENT_BLENDERS),

	DEFINE_FIELD(rendermode, FIELD_INTEGER),
	DEFINE_FIELD(renderamt, FIELD_FLOAT),
	DEFINE_FIELD(rendercolor, FIELD_VECTOR),
	DEFINE_FIELD(renderfx, FIELD_INTEGER),

	DEFINE_FIELD(health, FIELD_FLOAT),
	DEFINE_FIELD(frags, FIELD_FLOAT),
	DEFINE_FIELD(weapons, FIELD_INTEGER),
	DEFINE_FIELD(takedamage, FIELD_FLOAT),

	DEFINE_FIELD(deadflag, FIELD_FLOAT),
	DEFINE_FIELD(view_ofs, FIELD_VECTOR),
	DEFINE_FIELD(button, FIELD_INTEGER),
	DEFINE_FIELD(impulse, FIELD_INTEGER),

	DEFINE_FIELD(chain, FIELD_EDICT),
	DEFINE_FIELD(dmg_inflictor, FIELD_EDICT),
	DEFINE_FIELD(enemy, FIELD_EDICT),
	DEFINE_FIELD(aiment, FIELD_EDICT),
	DEFINE_FIELD(owner, FIELD_EDICT),
	DEFINE_FIELD(groundentity, FIELD_EDICT),

	DEFINE_FIELD(spawnflags, FIELD_INTEGER),
	DEFINE_FIELD(flags, FIELD_FLOAT),

	DEFINE_FIELD(colormap, FIELD_INTEGER),
	DEFINE_FIELD(team, FIELD_INTEGER),

	DEFINE_FIELD(max_health, FIELD_FLOAT),
	DEFINE_FIELD(teleport_time, FIELD_TIME),
	DEFINE_FIELD(armortype, FIELD_FLOAT),
	DEFINE_FIELD(armorvalue, FIELD_FLOAT),
	DEFINE_FIELD(waterlevel, FIELD_INTEGER),
	DEFINE_FIELD(watertype, FIELD_INTEGER),

	// Having these fields be local to the individual levels makes it easier to test those levels individually.
	DEFINE_GLOBAL_FIELD(target, FIELD_STRING),
	DEFINE_GLOBAL_FIELD(targetname, FIELD_STRING),
	DEFINE_FIELD(netname, FIELD_STRING),
	DEFINE_FIELD(message, FIELD_STRING),

	DEFINE_FIELD(dmg_take, FIELD_FLOAT),
	DEFINE_FIELD(dmg_save, FIELD_FLOAT),
	DEFINE_FIELD(dmg, FIELD_FLOAT),
	DEFINE_FIELD(dmgtime, FIELD_TIME),

	DEFINE_FIELD(noise, FIELD_SOUNDNAME),
	DEFINE_FIELD(noise1, FIELD_SOUNDNAME),
	DEFINE_FIELD(noise2, FIELD_SOUNDNAME),
	DEFINE_FIELD(noise3, FIELD_SOUNDNAME),
	DEFINE_FIELD(speed, FIELD_FLOAT),
	DEFINE_FIELD(air_finished, FIELD_TIME),
	DEFINE_FIELD(pain_finished, FIELD_TIME),
	DEFINE_FIELD(radsuit_finished, FIELD_TIME),
	END_DATAMAP();

BEGIN_DATAMAP_NOBASE(CBaseEntity)
DEFINE_FIELD(m_pGoalEnt, FIELD_CLASSPTR),
	DEFINE_FIELD(m_EFlags, FIELD_CHARACTER),

	DEFINE_FIELD(m_ClassificationName, FIELD_STRING),
	DEFINE_FIELD(m_HasCustomClassification, FIELD_BOOLEAN),
	DEFINE_FIELD(m_ChildClassificationName, FIELD_STRING),

	DEFINE_FIELD(m_IsUnkillable, FIELD_BOOLEAN),

	DEFINE_FIELD(m_InformedOwnerOfDeath, FIELD_BOOLEAN),

	DEFINE_FIELD(m_pfnThink, FIELD_FUNCTIONPOINTER), // UNDONE: Build table of these!!!
	DEFINE_FIELD(m_pfnTouch, FIELD_FUNCTIONPOINTER),
	DEFINE_FIELD(m_pfnUse, FIELD_FUNCTIONPOINTER),
	DEFINE_FIELD(m_pfnBlocked, FIELD_FUNCTIONPOINTER),

	DEFINE_FIELD(m_sMaster, FIELD_STRING),
	DEFINE_FIELD(m_hActivator, FIELD_EHANDLE),

	DEFINE_FIELD(m_ModelReplacementFileName, FIELD_STRING),
	DEFINE_FIELD(m_SoundReplacementFileName, FIELD_STRING),
	DEFINE_FIELD(m_SentenceReplacementFileName, FIELD_STRING),

	DEFINE_FIELD(m_CustomHullMin, FIELD_VECTOR),
	DEFINE_FIELD(m_CustomHullMax, FIELD_VECTOR),
	DEFINE_FIELD(m_HasCustomHullMin, FIELD_BOOLEAN),
	DEFINE_FIELD(m_HasCustomHullMax, FIELD_BOOLEAN),

	DEFINE_FUNCTION(SUB_Remove),
	DEFINE_FUNCTION(SUB_StartFadeOut),
	DEFINE_FUNCTION(SUB_FadeOut),
	DEFINE_FUNCTION(SUB_CallUseToggle),

	DEFINE_FIELD( m_SharedKey, FIELD_STRING, SHARED_KEYVALUE_MAX ),
	DEFINE_FIELD( m_SharedValue, FIELD_STRING, SHARED_KEYVALUE_MAX ),
	DEFINE_FIELD( m_SharedKeyValues, FIELD_INTEGER ),
END_DATAMAP();

void CBaseEntity::OnCreate()
{
	// Nothing.
}

void CBaseEntity::Construct()
{
	OnCreate();

#ifndef CLIENT_DLL
	g_EntityTemplates.MaybeApplyTemplate(this);
#endif
}

void CBaseEntity::OnDestroy()
{
	// Nothing.
}

void CBaseEntity::Destruct()
{
	OnDestroy();
}

bool CBaseEntity::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_sMaster = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "classification"))
	{
		SetCustomClassification(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "child_classification"))
	{
		m_ChildClassificationName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "unkillable"))
	{
		m_IsUnkillable = atoi(pkvd->szValue) != 0;
		return true;
	}

	return false;
}

bool CBaseEntity::Save(CSave& save)
{
#ifndef CLIENT_DLL
	if (auto entvarsMap = entvars_t::GetLocalDataMap(); !save.WriteFields(pev, *entvarsMap, *entvarsMap))
	{
		return false;
	}

	auto completeDataMap = GetDataMap();

	for (auto dataMap = completeDataMap; dataMap; dataMap = dataMap->BaseMap)
	{
		if (!save.WriteFields(this, *completeDataMap, *dataMap))
		{
			return false;
		}
	}
#endif

	return true;
}

bool CBaseEntity::Restore(CRestore& restore)
{
#ifndef CLIENT_DLL
	if (!restore.ReadFields(pev, *entvars_t::GetLocalDataMap(), *entvars_t::GetLocalDataMap()))
	{
		return false;
	}

	auto completeDataMap = GetDataMap();

	for (auto dataMap = completeDataMap; dataMap; dataMap = dataMap->BaseMap)
	{
		if (!restore.ReadFields(this, *completeDataMap, *dataMap))
		{
			return false;
		}
	}
#endif

	return true;
}

void CBaseEntity::PostRestore()
{
#ifndef CLIENT_DLL
	// Reinitialize the classification
	SetClassification(STRING(m_ClassificationName));

	LoadReplacementFiles();

	if (pev->modelindex != 0 && !FStringNull(pev->model))
	{
		Vector mins, maxs;
		mins = pev->mins; // Set model is about to destroy these
		maxs = pev->maxs;

		// Don't use UTIL_PrecacheModel here because we're restoring an already-replaced name.
		UTIL_PrecacheModelDirect(STRING(pev->model));
		SetModel(STRING(pev->model));
		SetSize(mins, maxs); // Reset them
	}
#endif
}

void CBaseEntity::MaybeSetChildClassification(CBaseEntity* child)
{
	if (FStringNull(m_ChildClassificationName))
	{
		return;
	}

	const char* classification = STRING(m_ChildClassificationName);

	// Special classifications start with !
	if (classification[0] == '!')
	{
		if (FStrEq(classification, "!owner"))
		{
			// Inherit my classification.
			child->SetCustomClassification(GetClassificationName());
		}
		else if (FStrEq(classification, "!owner_or_default"))
		{
			// If my classification was overridden use that, otherwise leave child class at default.
			// Needed so Ospreys can spawn children that use their default class,
			// which differs from the Osprey's default class.

			if (HasCustomClassification())
			{
				child->SetCustomClassification(GetClassificationName());
			}
		}
		else
		{
			Logger->error("{}:{}:{}: Invalid child classification \"{}\"",
				GetClassname(), entindex(), GetTargetname(), classification);
		}

		return;
	}

	child->SetCustomClassification(classification);
}

void CBaseEntity::MaybeNotifyOwnerOfDeath()
{
	if (m_InformedOwnerOfDeath)
	{
		return;
	}

	m_InformedOwnerOfDeath = true;

	if (auto owner = GetOwner(); owner)
	{
		owner->DeathNotice(this);
	}
}
