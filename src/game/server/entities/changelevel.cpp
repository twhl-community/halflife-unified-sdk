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

/**
 *	@file
 *	Entities relating to level changes and save games.
 */

#include "cbase.h"
#include "changelevel.h"
#include "ServerLibrary.h"
#include "shake.h"

#include "gamerules/PersistentInventorySystem.h"

LINK_ENTITY_TO_CLASS(info_landmark, CPointEntity);

/**
 *	@brief Define space that travels across a level transition
 *	@details Derive from point entity so this doesn't move across levels
 */
class CTriggerVolume : public CPointEntity
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS(trigger_transition, CTriggerVolume);

void CTriggerVolume::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetModel(STRING(pev->model)); // set size and link into world
	pev->model = string_t::Null;
	pev->modelindex = 0;
}

/**
 *	@brief Fires a target after level transition and then dies
 */
class CFireAndDie : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	void Think() override;
	int ObjectCaps() override { return CBaseDelay::ObjectCaps() | FCAP_FORCE_TRANSITION; } // Always go across transitions
};

LINK_ENTITY_TO_CLASS(fireanddie, CFireAndDie);

void CFireAndDie::Spawn()
{
	// Don't call Precache() - it should be called on restore
}

void CFireAndDie::Precache()
{
	// This gets called on restore
	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CFireAndDie::Think()
{
	SUB_UseTargets(this, USE_TOGGLE, 0);
	UTIL_Remove(this);
}

LINK_ENTITY_TO_CLASS(trigger_changelevel, CChangeLevel);

BEGIN_DATAMAP(CChangeLevel)
DEFINE_ARRAY(m_szMapName, FIELD_CHARACTER, cchMapNameMost),
	DEFINE_ARRAY(m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost),
	DEFINE_FIELD(m_changeTarget, FIELD_STRING),
	DEFINE_FIELD(m_changeTargetDelay, FIELD_FLOAT),
	DEFINE_FIELD(m_UsePersistentLevelChange, FIELD_BOOLEAN),
	DEFINE_FUNCTION(UseChangeLevel),
	DEFINE_FUNCTION(TouchChangeLevel),
	END_DATAMAP();

bool CChangeLevel::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "map"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			Logger->error("Map name '{}' too long (32 chars)", pkvd->szValue);
		strcpy(m_szMapName, pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "landmark"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			Logger->error("Landmark name '{}' too long (32 chars)", pkvd->szValue);
		strcpy(m_szLandmarkName, pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "changetarget"))
	{
		m_changeTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "changedelay"))
	{
		m_changeTargetDelay = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "use_persistent_level_change"))
	{
		m_UsePersistentLevelChange = atoi(pkvd->szValue) != 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "use_persistent_inventory"))
	{
		m_UsePersistentInventory = atoi(pkvd->szValue) != 0;
		return true;
	}

	return CBaseTrigger::KeyValue(pkvd);
}

void CChangeLevel::Spawn()
{
	if (FStrEq(m_szMapName, ""))
		Logger->warn("a trigger_changelevel doesn't have a map");

	if (m_UsePersistentLevelChange && FStrEq(m_szLandmarkName, ""))
		Logger->warn("Persistent trigger_changelevel to {} doesn't have a landmark", m_szMapName);

	if (g_pGameRules->IsMultiplayer() && !FStrEq(m_szLandmarkName, ""))
	{
		Logger->warn("trigger_changelevel doesn't support landmarks in multiplayer");
	}

	if (0 == stricmp(m_szMapName, STRING(gpGlobals->mapname)))
	{
		Logger->error("trigger_changelevel points to the current map ({}), which does not work", STRING(gpGlobals->mapname));
	}

	if (!FStringNull(pev->targetname))
	{
		SetUse(&CChangeLevel::UseChangeLevel);
	}
	InitTrigger();
	if ((pev->spawnflags & SF_CHANGELEVEL_USEONLY) == 0)
		SetTouch(&CChangeLevel::TouchChangeLevel);
	//	Logger->debug("TRANSITION: {} ({})", m_szMapName, m_szLandmarkName);
}

CBaseEntity* CChangeLevel::FindLandmark(const char* pLandmarkName)
{
	for (auto landmark : UTIL_FindEntitiesByTargetname(pLandmarkName))
	{
		// Found the landmark
		if (landmark->ClassnameIs("info_landmark"))
			return landmark;
	}

	// Only an error if a name was specified.
	if (!FStrEq(pLandmarkName, ""))
	{
		Logger->error("Can't find landmark {}", pLandmarkName);
	}

	return nullptr;
}

void CChangeLevel::UseChangeLevel(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	ChangeLevelNow(pActivator);
}

// This differs from the engine's version in that it allows non-persistent level changes to use landmarks.
static void QueueChangelevel(const char* mapName, const char* landmarkName, bool usePersistentLevelChange)
{
	static int lastSpawnCount = 0;

	// Only queue a single level change at a time.
	if (g_Server.GetSpawnCount() == lastSpawnCount)
	{
		return;
	}

	lastSpawnCount = g_Server.GetSpawnCount();

	// Persistent level changes only work in singleplayer.
	if (usePersistentLevelChange && !g_pGameRules->IsMultiplayer())
	{
		SERVER_COMMAND(fmt::format("changelevel2 {} {}\n", mapName, landmarkName).c_str());
	}
	else
	{
		SERVER_COMMAND(fmt::format("changelevel {} {}\n", mapName, landmarkName).c_str());
	}
}

void CChangeLevel::ChangeLevelNow(CBaseEntity* pActivator)
{
	ASSERT(!FStrEq(m_szMapName, ""));

	// Some people are firing these multiple times in a frame, disable
	if (gpGlobals->time == pev->dmgtime)
		return;

	pev->dmgtime = gpGlobals->time;

	// This only works as intended in singleplayer.
	if (!g_pGameRules->IsMultiplayer())
	{
		CBaseEntity* pPlayer = UTIL_GetLocalPlayer();
		if (!InTransitionVolume(pPlayer, m_szLandmarkName))
		{
			Logger->debug("Player isn't in the transition volume {}, aborting", m_szLandmarkName);
			return;
		}

		// Create an entity to fire the changetarget
		if (!FStringNull(m_changeTarget))
		{
			CFireAndDie* pFireAndDie = g_EntityDictionary->Create<CFireAndDie>("fireanddie");
			if (pFireAndDie)
			{
				// Set target and delay
				pFireAndDie->pev->target = m_changeTarget;
				pFireAndDie->m_flDelay = m_changeTargetDelay;
				pFireAndDie->pev->origin = pPlayer->pev->origin;
				// Call spawn
				DispatchSpawn(pFireAndDie->edict());
			}
		}
	}

	m_hActivator = pActivator;
	SUB_UseTargets(pActivator, USE_TOGGLE, 0);

	// Init landmark to empty string
	const char* landmarkNameInNextMap = "";

	if (!g_pGameRules->IsMultiplayer())
	{
		// look for a landmark entity
		auto landmark = FindLandmark(m_szLandmarkName);
		if (!FNullEnt(landmark))
		{
			landmarkNameInNextMap = m_szLandmarkName;
			gpGlobals->vecLandmarkOffset = landmark->pev->origin;
		}
	}

	// Logger->debug("Level touches {} levels", ChangeList(levels, std::size(levels)));

	Logger->debug("CHANGE LEVEL: {} {}", m_szMapName, landmarkNameInNextMap);

	if (m_UsePersistentInventory)
	{
		g_PersistentInventory.InitializeFromPlayers();
	}

	QueueChangelevel(m_szMapName, landmarkNameInNextMap, m_UsePersistentLevelChange);
}

void CChangeLevel::TouchChangeLevel(CBaseEntity* pOther)
{
	auto player = ToBasePlayer(pOther);

	if (!player)
		return;

	// This only happens if we're spawning/loading into a map and touching a level change right away.
	if (!g_Server.HasFinishedLoading())
	{
		m_Enabled = false;
	}

	if (!m_Enabled)
	{
		if (m_LastTooCloseMessageTime < gpGlobals->time)
		{
			m_LastTooCloseMessageTime = gpGlobals->time + TooCloseMessageInterval;

			// Every time the player tries to use a level change that they were touching on load
			// we tell them why it's disabled.
			// This needs to be broken up into lines because centerprint only allows up to 39 characters to be drawn per line.
			const auto message = fmt::format(
				"The level change to\n\n\"{}\"\n\nnear {} is too close\n\nto the player when they enter the map",
				m_szMapName, Center());

			ClientPrint(player, HUD_PRINTCENTER, message.c_str());
			Con_Printf("%s\n", message.c_str());
		}

		return;
	}

	ChangeLevelNow(pOther);
}

bool CChangeLevel::AddTransitionToList(LEVELLIST* pLevelList, int listCount, const char* pMapName, const char* pLandmarkName, CBaseEntity* landmark)
{
	if (!pLevelList || !pMapName || !pLandmarkName || !landmark)
		return false;

	auto landmarkEdict = landmark->edict();

	for (int i = 0; i < listCount; i++)
	{
		if (pLevelList[i].pentLandmark == landmarkEdict && strcmp(pLevelList[i].mapName, pMapName) == 0)
			return false;
	}
	strcpy(pLevelList[listCount].mapName, pMapName);
	strcpy(pLevelList[listCount].landmarkName, pLandmarkName);
	pLevelList[listCount].pentLandmark = landmarkEdict;
	pLevelList[listCount].vecLandmarkOrigin = landmark->pev->origin;

	return true;
}

bool CChangeLevel::InTransitionVolume(CBaseEntity* pEntity, char* pVolumeName)
{
	if ((pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION) != 0)
		return true;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if (pEntity->pev->movetype == MOVETYPE_FOLLOW)
	{
		if (pEntity->pev->aiment != nullptr)
			pEntity = CBaseEntity::Instance(pEntity->pev->aiment);
	}

	bool inVolume = true; // Unless we find a trigger_transition, everything is in the volume

	for (auto volume : UTIL_FindEntitiesByTargetname(pVolumeName))
	{
		if (volume && volume->ClassnameIs("trigger_transition"))
		{
			if (volume->Intersects(pEntity)) // It touches one, it's in the volume
				return true;
			else
				inVolume = false; // Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
	}

	return inVolume;
}

// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// This has grown into a complicated beast
// Can we make this more elegant?
int CChangeLevel::ChangeList(LEVELLIST* pLevelList, int maxList)
{
	int count = 0;

	// Find all of the possible level changes on this BSP
	for (auto changelevel : UTIL_FindEntitiesByClassname<CChangeLevel>("trigger_changelevel"))
	{
		// Find the corresponding landmark
		auto landmark = FindLandmark(changelevel->m_szLandmarkName);
		if (landmark)
		{
			// Build a list of unique transitions
			if (AddTransitionToList(pLevelList, count, changelevel->m_szMapName, changelevel->m_szLandmarkName, landmark))
			{
				count++;
				if (count >= maxList) // FULL!!
					break;
			}
		}
	}

	if (count > 0)
	{
		// Token table is null at this point, so don't use CSaveRestoreBuffer::IsValidSaveRestoreData here.
		if (auto pSaveData = reinterpret_cast<SAVERESTOREDATA*>(gpGlobals->pSaveData);
			nullptr != pSaveData && pSaveData->pTable)
		{
			CSave saveHelper(*pSaveData);

			for (int i = 0; i < count; i++)
			{
				int j, entityCount = 0;
				CBaseEntity* pEntList[MAX_ENTITY];
				int entityFlags[MAX_ENTITY];

				// Follow the linked list of entities in the PVS of the transition landmark
				edict_t* pent = UTIL_EntitiesInPVS(pLevelList[i].pentLandmark);

				// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
				while (!FNullEnt(pent))
				{
					CBaseEntity* pEntity = CBaseEntity::Instance(pent);
					if (pEntity)
					{
						// Logger->debug("Trying {}", STRING(pEntity->pev->classname));
						int caps = pEntity->ObjectCaps();
						if ((caps & FCAP_DONT_SAVE) == 0)
						{
							int flags = 0;

							// If this entity can be moved or is global, mark it
							if ((caps & FCAP_ACROSS_TRANSITION) != 0)
								flags |= FENTTABLE_MOVEABLE;
							if (!FStringNull(pEntity->pev->globalname) && !pEntity->IsDormant())
								flags |= FENTTABLE_GLOBAL;
							if (0 != flags)
							{
								pEntList[entityCount] = pEntity;
								entityFlags[entityCount] = flags;
								entityCount++;
								if (entityCount > MAX_ENTITY)
									Logger->error("Too many entities across a transition!");
							}
							// else
							//	Logger->debug("Failed {}", STRING(pEntity->pev->classname));
						}
						// else
						//	Logger->debug("DON'T SAVE {}", STRING(pEntity->pev->classname));
					}
					pent = pent->v.chain;
				}

				for (j = 0; j < entityCount; j++)
				{
					// Check to make sure the entity isn't screened out by a trigger_transition
					if (0 != entityFlags[j] && InTransitionVolume(pEntList[j], pLevelList[i].landmarkName))
					{
						// Mark entity table with 1<<i
						int index = saveHelper.EntityIndex(pEntList[j]);
						// Flag it with the level number
						saveHelper.EntityFlagsSet(index, entityFlags[j] | (1 << i));
					}
					// else
					//	Logger->debug("Screened out {}", STRING(pEntList[j]->pev->classname));
				}
			}
		}
	}

	return count;
}

class CTriggerSave : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerSave, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void SaveTouch(CBaseEntity* pOther);
};

BEGIN_DATAMAP(CTriggerSave)
DEFINE_FUNCTION(SaveTouch),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(trigger_autosave, CTriggerSave);

void CTriggerSave::Spawn()
{
	if (g_pGameRules->IsMultiplayer())
	{
		REMOVE_ENTITY(edict());
		return;
	}

	InitTrigger();
	SetTouch(&CTriggerSave::SaveTouch);
}

void CTriggerSave::SaveTouch(CBaseEntity* pOther)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pOther, m_UseLocked))
		return;

	// Only save on clients
	if (!pOther->IsPlayer())
		return;

	SetTouch(nullptr);
	UTIL_Remove(this);
	SERVER_COMMAND("autosave\n");
}

#define SF_ENDSECTION_USEONLY 0x0001

class CTriggerEndSection : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerEndSection, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void EndSectionTouch(CBaseEntity* pOther);
	bool KeyValue(KeyValueData* pkvd) override;
	void EndSectionUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
};

BEGIN_DATAMAP(CTriggerEndSection)
DEFINE_FUNCTION(EndSectionTouch),
	DEFINE_FUNCTION(EndSectionUse),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(trigger_endsection, CTriggerEndSection);

void CTriggerEndSection::EndSectionUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Only save on clients
	if (pActivator && !pActivator->IsNetClient())
		return;

	SetUse(nullptr);

	if (!FStringNull(pev->message))
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	UTIL_Remove(this);
}

void CTriggerEndSection::Spawn()
{
	if (g_pGameRules->IsMultiplayer())
	{
		REMOVE_ENTITY(edict());
		return;
	}

	InitTrigger();

	SetUse(&CTriggerEndSection::EndSectionUse);
	// If it is a "use only" trigger, then don't set the touch function.
	if ((pev->spawnflags & SF_ENDSECTION_USEONLY) == 0)
		SetTouch(&CTriggerEndSection::EndSectionTouch);
}

void CTriggerEndSection::EndSectionTouch(CBaseEntity* pOther)
{
	// Only save on clients
	if (!pOther->IsNetClient())
		return;

	SetTouch(nullptr);

	if (!FStringNull(pev->message))
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	UTIL_Remove(this);
}

bool CTriggerEndSection::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "section"))
	{
		//		m_iszSectionName = ALLOC_STRING( pkvd->szValue );
		// Store this in message so we don't have to write save/restore for this ent
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseTrigger::KeyValue(pkvd);
}

class CRevertSaved : public CPointEntity
{
	DECLARE_CLASS(CRevertSaved, CPointEntity);
	DECLARE_DATAMAP();

public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void MessageThink();
	void LoadThink();
	bool KeyValue(KeyValueData* pkvd) override;

	inline float Duration() { return pev->dmg_take; }
	inline float HoldTime() { return pev->dmg_save; }
	inline float MessageTime() { return m_messageTime; }
	inline float LoadTime() { return m_loadTime; }

	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }
	inline void SetMessageTime(float time) { m_messageTime = time; }
	inline void SetLoadTime(float time) { m_loadTime = time; }

private:
	float m_messageTime;
	float m_loadTime;
};

LINK_ENTITY_TO_CLASS(player_loadsaved, CRevertSaved);

BEGIN_DATAMAP(CRevertSaved)
DEFINE_FIELD(m_messageTime, FIELD_FLOAT), // These are not actual times, but durations, so save as floats
	DEFINE_FIELD(m_loadTime, FIELD_FLOAT),
	DEFINE_FUNCTION(MessageThink),
	DEFINE_FUNCTION(LoadThink),
	END_DATAMAP();

bool CRevertSaved::KeyValue(KeyValueData* pkvd)
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
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime(atof(pkvd->szValue));
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime(atof(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CRevertSaved::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT);
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink(&CRevertSaved::MessageThink);
}

void CRevertSaved::MessageThink()
{
	UTIL_ShowMessageAll(GetMessage());
	float nextThink = LoadTime() - MessageTime();
	if (nextThink > 0)
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink(&CRevertSaved::LoadThink);
	}
	else
		LoadThink();
}

void CRevertSaved::LoadThink()
{
	if (!g_pGameRules->IsMultiplayer())
	{
		SERVER_COMMAND("reload\n");
	}
}
