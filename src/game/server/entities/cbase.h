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

#pragma once

/*

Class Hierachy

CBaseEntity
	CBaseDelay
		CBaseToggle
			CBaseItem
			CBaseMonster
				CBaseCycler
				CBasePlayer
				CBaseGroup
*/

// Include common headers here.
#include <memory>
#include <string_view>

#include <spdlog/logger.h>

#include "Platform.h"

#include "extdll.h"
#include "util.h"

#include "saverestore.h"
#include "schedule.h"
#include "monsterevent.h"
#include "ehandle.h"
#include "EntityDictionary.h"
#include "animation.h"
#include "decals.h"
#include "skill.h"
#include "game.h"
#include "gamerules.h"

#include "basemonster.h"
#include "CBaseAnimating.h"
#include "CBaseButton.h"
#include "CBaseDelay.h"
#include "CBaseEntity.h"
#include "CBaseToggle.h"
#include "CMultiSource.h"
#include "CPointEntity.h"
#include "effects.h"
#include "player.h"
#include "soundent.h"
#include "weapons.h"
#include "world.h"

// C functions for external declarations that call the appropriate C++ methods

extern "C" DLLEXPORT int GetEntityAPI(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion);
extern "C" DLLEXPORT int GetEntityAPI2(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);
extern "C" DLLEXPORT int GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);

/**
 *	@brief HACKHACK -- this is a hack to keep the node graph entity from "touching" things (like triggers)
 *	while it builds the graph
 */
inline bool gTouchDisabled = false;

int DispatchSpawn(edict_t* pent);
void DispatchKeyValue(edict_t* pentKeyvalue, KeyValueData* pkvd);
void DispatchTouch(edict_t* pentTouched, edict_t* pentOther);
void DispatchUse(edict_t* pentUsed, edict_t* pentOther);
void DispatchThink(edict_t* pent);
void DispatchBlocked(edict_t* pentBlocked, edict_t* pentOther);
void DispatchSave(edict_t* pent, SAVERESTOREDATA* pSaveData);
int DispatchRestore(edict_t* pent, SAVERESTOREDATA* pSaveData, int globalEntity);
void DispatchObjectCollsionBox(edict_t* pent);
void SaveWriteFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount);
void SaveReadFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount);
void SaveGlobalState(SAVERESTOREDATA* pSaveData);
void RestoreGlobalState(SAVERESTOREDATA* pSaveData);
void ResetGlobalState();
