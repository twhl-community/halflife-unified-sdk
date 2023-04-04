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
#include "CGlobalState.h"

CGlobalState gGlobalState;

CGlobalState::CGlobalState()
{
	Reset();
}

void CGlobalState::Reset()
{
	m_pList = nullptr;
	m_listCount = 0;
}

globalentity_t* CGlobalState::Find(string_t globalname)
{
	if (FStringNull(globalname))
		return nullptr;

	globalentity_t* pTest;
	const char* pEntityName = STRING(globalname);


	pTest = m_pList;
	while (pTest)
	{
		if (FStrEq(pEntityName, pTest->name))
			break;

		pTest = pTest->pNext;
	}

	return pTest;
}

// This is available all the time now on impulse 104, remove later
// #ifdef _DEBUG
void CGlobalState::DumpGlobals()
{
	static const char* estates[] = {"Off", "On", "Dead"};
	globalentity_t* pTest;

	CBaseEntity::Logger->debug("-- Globals --");
	pTest = m_pList;
	while (pTest)
	{
		CBaseEntity::Logger->debug("{}: {} ({})", pTest->name, pTest->levelName, estates[pTest->state]);
		pTest = pTest->pNext;
	}
}
// #endif

void CGlobalState::EntityAdd(string_t globalname, string_t mapName, GLOBALESTATE state)
{
	ASSERT(!Find(globalname));

	globalentity_t* pNewEntity = (globalentity_t*)calloc(sizeof(globalentity_t), 1);
	ASSERT(pNewEntity != nullptr);
	pNewEntity->pNext = m_pList;
	m_pList = pNewEntity;
	strcpy(pNewEntity->name, STRING(globalname));
	strcpy(pNewEntity->levelName, STRING(mapName));
	pNewEntity->state = state;
	m_listCount++;
}

void CGlobalState::EntitySetState(string_t globalname, GLOBALESTATE state)
{
	globalentity_t* pEnt = Find(globalname);

	if (pEnt)
		pEnt->state = state;
}

const globalentity_t* CGlobalState::EntityFromTable(string_t globalname)
{
	globalentity_t* pEnt = Find(globalname);

	return pEnt;
}

GLOBALESTATE CGlobalState::EntityGetState(string_t globalname)
{
	globalentity_t* pEnt = Find(globalname);
	if (pEnt)
		return pEnt->state;

	return GLOBAL_OFF;
}

BEGIN_DATAMAP_NOBASE(CGlobalState)
DEFINE_FIELD(m_listCount, FIELD_INTEGER),
	END_DATAMAP();

BEGIN_DATAMAP_NOBASE(globalentity_t)
DEFINE_ARRAY(name, FIELD_CHARACTER, 64),
	DEFINE_ARRAY(levelName, FIELD_CHARACTER, 32),
	DEFINE_FIELD(state, FIELD_INTEGER),
	END_DATAMAP();

bool CGlobalState::Save(CSave& save)
{
	if (!save.WriteFields(this, *GetDataMap()))
		return false;

	globalentity_t* pEntity = m_pList;
	for (int i = 0; i < m_listCount && pEntity; i++)
	{
		if (!save.WriteFields(pEntity, *pEntity->GetDataMap()))
			return false;

		pEntity = pEntity->pNext;
	}

	return true;
}

bool CGlobalState::Restore(CRestore& restore)
{
	globalentity_t tmpEntity;

	ClearStates();
	if (!restore.ReadFields(this, *GetDataMap()))
		return false;

	const int listCount = m_listCount; // Get new list count
	m_listCount = 0;				   // Clear loaded data

	for (int i = 0; i < listCount; i++)
	{
		if (!restore.ReadFields(&tmpEntity, *tmpEntity.GetDataMap()))
			return false;
		EntityAdd(MAKE_STRING(tmpEntity.name), MAKE_STRING(tmpEntity.levelName), tmpEntity.state);
	}
	return true;
}

void CGlobalState::EntityUpdate(string_t globalname, string_t mapname)
{
	globalentity_t* pEnt = Find(globalname);

	if (pEnt)
		strcpy(pEnt->levelName, STRING(mapname));
}

void CGlobalState::ClearStates()
{
	globalentity_t* pFree = m_pList;
	while (pFree)
	{
		globalentity_t* pNext = pFree->pNext;
		free(pFree);
		pFree = pNext;
	}
	Reset();
}

void SaveGlobalState(SAVERESTOREDATA* pSaveData)
{
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	CSave saveHelper(*pSaveData);
	gGlobalState.Save(saveHelper);
}

void RestoreGlobalState(SAVERESTOREDATA* pSaveData)
{
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	CRestore restoreHelper(*pSaveData);
	gGlobalState.Restore(restoreHelper);
}

void ResetGlobalState()
{
	gGlobalState.ClearStates();
	gInitHUD = true; // Init the HUD on a new game / load game
}
