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
#include "client.h"
#include "ServerLibrary.h"

void EntvarsKeyvalue(entvars_t* pev, KeyValueData* pkvd);

void PM_Move(struct playermove_s* ppmove, int server);
void PM_Init(struct playermove_s* ppmove);
char PM_FindTextureType(char* name);

void OnFreeEntPrivateData(edict_s* pEdict);

void DummySpectatorFunction(edict_t*)
{
	//Nothing
}

Vector VecBModelOrigin(entvars_t* pevBModel);

static DLL_FUNCTIONS gFunctionTable =
	{
		GameDLLInit,			   //pfnGameInit
		DispatchSpawn,			   //pfnSpawn
		DispatchThink,			   //pfnThink
		DispatchUse,			   //pfnUse
		DispatchTouch,			   //pfnTouch
		DispatchBlocked,		   //pfnBlocked
		DispatchKeyValue,		   //pfnKeyValue
		DispatchSave,			   //pfnSave
		DispatchRestore,		   //pfnRestore
		DispatchObjectCollsionBox, //pfnAbsBox

		SaveWriteFields, //pfnSaveWriteFields
		SaveReadFields,	 //pfnSaveReadFields

		SaveGlobalState,	//pfnSaveGlobalState
		RestoreGlobalState, //pfnRestoreGlobalState
		ResetGlobalState,	//pfnResetGlobalState

		ClientConnect,		   //pfnClientConnect
		ClientDisconnect,	   //pfnClientDisconnect
		ClientKill,			   //pfnClientKill
		ClientPutInServer,	   //pfnClientPutInServer
		ExecuteClientCommand,  // pfnClientCommand
		ClientUserInfoChanged, //pfnClientUserInfoChanged
		ServerActivate,		   //pfnServerActivate
		ServerDeactivate,	   //pfnServerDeactivate

		PlayerPreThink,	 //pfnPlayerPreThink
		PlayerPostThink, //pfnPlayerPostThink

		StartFrame,		  //pfnStartFrame
		ParmsNewLevel,	  //pfnParmsNewLevel
		ParmsChangeLevel, //pfnParmsChangeLevel

		GetGameDescription,	 //pfnGetGameDescription    Returns string describing current .dll game.
		PlayerCustomization, //pfnPlayerCustomization   Notifies .dll of new customization for player.

		DummySpectatorFunction, //pfnSpectatorConnect      Called when spectator joins server
		DummySpectatorFunction, //pfnSpectatorDisconnect   Called when spectator leaves the server
		DummySpectatorFunction, //pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

		Sys_Error, //pfnSys_Error				Called when engine has encountered an error

		PM_Move,			//pfnPM_Move
		PM_Init,			//pfnPM_Init				Server version of player movement initialization
		PM_FindTextureType, //pfnPM_FindTextureType

		SetupVisibility,		  //pfnSetupVisibility        Set up PVS and PAS for networking for this client
		UpdateClientData,		  //pfnUpdateClientData       Set up data sent only to specific client
		AddToFullPack,			  //pfnAddToFullPack
		CreateBaseline,			  //pfnCreateBaseline			Tweak entity baseline for network encoding, allows setup of player baselines, too.
		RegisterEncoders,		  //pfnRegisterEncoders		Callbacks for network encoding
		GetWeaponData,			  //pfnGetWeaponData
		CmdStart,				  //pfnCmdStart
		CmdEnd,					  //pfnCmdEnd
		ConnectionlessPacket,	  //pfnConnectionlessPacket
		GetHullBounds,			  //pfnGetHullBounds
		CreateInstancedBaselines, //pfnCreateInstancedBaselines
		InconsistentFile,		  //pfnInconsistentFile
		AllowLagCompensation,	  //pfnAllowLagCompensation
};

NEW_DLL_FUNCTIONS gNewDLLFunctions =
	{
		OnFreeEntPrivateData, //pfnOnFreeEntPrivateData
		GameDLLShutdown,
};

static void SetObjectCollisionBox(entvars_t* pev);

extern "C" {

int GetEntityAPI(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion)
{
	if (!pFunctionTable || interfaceVersion != INTERFACE_VERSION)
	{
		return 0;
	}

	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));
	return 1;
}

int GetEntityAPI2(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != INTERFACE_VERSION)
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return 0;
	}

	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));
	return 1;
}

int GetNewDLLFunctions(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return 0;
	}

	memcpy(pFunctionTable, &gNewDLLFunctions, sizeof(gNewDLLFunctions));
	return 1;
}
}


int DispatchSpawn(edict_t* pent)
{
	CBaseEntity* pEntity = (CBaseEntity*)GET_PRIVATE(pent);

	if (pEntity)
	{
		// Initialize these or entities who don't link to the world won't have anything in here
		pEntity->pev->absmin = pEntity->pev->origin - Vector(1, 1, 1);
		pEntity->pev->absmax = pEntity->pev->origin + Vector(1, 1, 1);

		pEntity->Spawn();

		// Try to get the pointer again, in case the spawn function deleted the entity.
		// UNDONE: Spawn() should really return a code to ask that the entity be deleted, but
		// that would touch too much code for me to do that right now.
		pEntity = (CBaseEntity*)GET_PRIVATE(pent);

		if (pEntity)
		{
			if (g_pGameRules && !g_pGameRules->IsAllowedToSpawn(pEntity))
				return -1; // return that this entity should be deleted
			if ((pEntity->pev->flags & FL_KILLME) != 0)
				return -1;
		}


		// Handle global stuff here
		if (pEntity && !FStringNull(pEntity->pev->globalname))
		{
			const globalentity_t* pGlobal = gGlobalState.EntityFromTable(pEntity->pev->globalname);
			if (pGlobal)
			{
				// Already dead? delete
				if (pGlobal->state == GLOBAL_DEAD)
					return -1;
				else if (!FStrEq(STRING(gpGlobals->mapname), pGlobal->levelName))
					pEntity->MakeDormant(); // Hasn't been moved to this level yet, wait but stay alive
											// In this level & not dead, continue on as normal
			}
			else
			{
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd(pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON);
				//				ALERT( at_console, "Added global entity %s (%s)\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->globalname) );
			}
		}
	}

	return 0;
}

void DispatchKeyValue(edict_t* pentKeyvalue, KeyValueData* pkvd)
{
	if (!pkvd || !pentKeyvalue)
		return;

	g_Server.CheckForNewMapStart(false);

	EntvarsKeyvalue(VARS(pentKeyvalue), pkvd);

	// If the key was an entity variable, or there's no class set yet, don't look for the object, it may
	// not exist yet.
	if (0 != pkvd->fHandled || pkvd->szClassName == nullptr)
		return;

	// Get the actualy entity object
	CBaseEntity* pEntity = (CBaseEntity*)GET_PRIVATE(pentKeyvalue);

	if (!pEntity)
		return;

	pkvd->fHandled = static_cast<int32>(pEntity->KeyValue(pkvd));
}

void DispatchTouch(edict_t* pentTouched, edict_t* pentOther)
{
	if (gTouchDisabled)
		return;

	CBaseEntity* pEntity = (CBaseEntity*)GET_PRIVATE(pentTouched);
	CBaseEntity* pOther = (CBaseEntity*)GET_PRIVATE(pentOther);

	if (pEntity && pOther && ((pEntity->pev->flags | pOther->pev->flags) & FL_KILLME) == 0)
		pEntity->Touch(pOther);
}


void DispatchUse(edict_t* pentUsed, edict_t* pentOther)
{
	CBaseEntity* pEntity = (CBaseEntity*)GET_PRIVATE(pentUsed);
	CBaseEntity* pOther = (CBaseEntity*)GET_PRIVATE(pentOther);

	if (pEntity && (pEntity->pev->flags & FL_KILLME) == 0)
		pEntity->Use(pOther, pOther, USE_TOGGLE, 0);
}

void DispatchThink(edict_t* pent)
{
	CBaseEntity* pEntity = (CBaseEntity*)GET_PRIVATE(pent);
	if (pEntity)
	{
		if (FBitSet(pEntity->pev->flags, FL_DORMANT))
			ALERT(at_error, "Dormant entity %s is thinking!!\n", STRING(pEntity->pev->classname));

		pEntity->Think();
	}
}

void DispatchBlocked(edict_t* pentBlocked, edict_t* pentOther)
{
	CBaseEntity* pEntity = (CBaseEntity*)GET_PRIVATE(pentBlocked);
	CBaseEntity* pOther = (CBaseEntity*)GET_PRIVATE(pentOther);

	if (pEntity)
		pEntity->Blocked(pOther);
}

void DispatchSave(edict_t* pent, SAVERESTOREDATA* pSaveData)
{
	gpGlobals->time = pSaveData->time;

	CBaseEntity* pEntity = (CBaseEntity*)GET_PRIVATE(pent);

	if (pEntity && CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		ENTITYTABLE* pTable = &pSaveData->pTable[pSaveData->currentIndex];

		if (pTable->pent != pent)
			ALERT(at_error, "ENTITY TABLE OR INDEX IS WRONG!!!!\n");

		if ((pEntity->ObjectCaps() & FCAP_DONT_SAVE) != 0)
			return;

		// These don't use ltime & nextthink as times really, but we'll fudge around it.
		if (pEntity->pev->movetype == MOVETYPE_PUSH)
		{
			float delta = pEntity->pev->nextthink - pEntity->pev->ltime;
			pEntity->pev->ltime = gpGlobals->time;
			pEntity->pev->nextthink = pEntity->pev->ltime + delta;
		}

		pTable->location = pSaveData->size;			 // Remember entity position for file I/O
		pTable->classname = pEntity->pev->classname; // Remember entity class for respawn

		CSave saveHelper(*pSaveData);
		pEntity->Save(saveHelper);

		pTable->size = pSaveData->size - pTable->location; // Size of entity block is data size written to block
	}
}

void OnFreeEntPrivateData(edict_s* pEdict)
{
	if (pEdict && pEdict->pvPrivateData)
	{
		auto entity = reinterpret_cast<CBaseEntity*>(pEdict->pvPrivateData);

		entity->OnDestroy();

		delete entity;

		//Zero this out so the engine doesn't try to free it again.
		pEdict->pvPrivateData = nullptr;
	}
}

// Find the matching global entity.  Spit out an error if the designer made entities of
// different classes with the same global name
CBaseEntity* FindGlobalEntity(string_t classname, string_t globalname)
{
	edict_t* pent = FIND_ENTITY_BY_STRING(nullptr, "globalname", STRING(globalname));
	CBaseEntity* pReturn = CBaseEntity::Instance(pent);
	if (pReturn)
	{
		if (!FClassnameIs(pReturn->pev, STRING(classname)))
		{
			ALERT(at_console, "Global entity found %s, wrong class %s\n", STRING(globalname), STRING(pReturn->pev->classname));
			pReturn = nullptr;
		}
	}

	return pReturn;
}

int DispatchRestore(edict_t* pent, SAVERESTOREDATA* pSaveData, int globalEntity)
{
	gpGlobals->time = pSaveData->time;

	CBaseEntity* pEntity = (CBaseEntity*)GET_PRIVATE(pent);

	if (pEntity && CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		entvars_t tmpVars;
		Vector oldOffset;

		CRestore restoreHelper(*pSaveData);
		if (0 != globalEntity)
		{
			CRestore tmpRestore(*pSaveData);
			tmpRestore.PrecacheMode(false);
			tmpRestore.ReadEntVars("ENTVARS", &tmpVars);

			// HACKHACK - reset the save pointers, we're going to restore for real this time
			pSaveData->size = pSaveData->pTable[pSaveData->currentIndex].location;
			pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;
			// -------------------


			const globalentity_t* pGlobal = gGlobalState.EntityFromTable(tmpVars.globalname);

			// Don't overlay any instance of the global that isn't the latest
			// pSaveData->szCurrentMapName is the level this entity is coming from
			// pGlobla->levelName is the last level the global entity was active in.
			// If they aren't the same, then this global update is out of date.
			if (!FStrEq(pSaveData->szCurrentMapName, pGlobal->levelName))
				return 0;

			// Compute the new global offset
			oldOffset = pSaveData->vecLandmarkOffset;
			CBaseEntity* pNewEntity = FindGlobalEntity(tmpVars.classname, tmpVars.globalname);
			if (pNewEntity)
			{
				//				ALERT( at_console, "Overlay %s with %s\n", STRING(pNewEntity->pev->classname), STRING(tmpVars.classname) );
				// Tell the restore code we're overlaying a global entity from another level
				restoreHelper.SetGlobalMode(true); // Don't overwrite global fields
				pSaveData->vecLandmarkOffset = (pSaveData->vecLandmarkOffset - pNewEntity->pev->mins) + tmpVars.mins;
				pEntity = pNewEntity; // we're going to restore this data OVER the old entity
				pent = ENT(pEntity->pev);
				// Update the global table to say that the global definition of this entity should come from this level
				gGlobalState.EntityUpdate(pEntity->pev->globalname, gpGlobals->mapname);
			}
			else
			{
				// This entity will be freed automatically by the engine.  If we don't do a restore on a matching entity (below)
				// or call EntityUpdate() to move it to this level, we haven't changed global state at all.
				return 0;
			}
		}

		if ((pEntity->ObjectCaps() & FCAP_MUST_SPAWN) != 0)
		{
			pEntity->Restore(restoreHelper);
			pEntity->Spawn();
		}
		else
		{
			pEntity->Restore(restoreHelper);
			pEntity->Precache();
		}

		// Again, could be deleted, get the pointer again.
		pEntity = (CBaseEntity*)GET_PRIVATE(pent);

#if 0
		if (pEntity && pEntity->pev->globalname && globalEntity)
		{
			ALERT(at_console, "Global %s is %s\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->model));
		}
#endif

		// Is this an overriding global entity (coming over the transition), or one restoring in a level
		if (0 != globalEntity)
		{
			//			ALERT( at_console, "After: %f %f %f %s\n", pEntity->pev->origin.x, pEntity->pev->origin.y, pEntity->pev->origin.z, STRING(pEntity->pev->model) );
			pSaveData->vecLandmarkOffset = oldOffset;
			if (pEntity)
			{
				UTIL_SetOrigin(pEntity->pev, pEntity->pev->origin);
				pEntity->OverrideReset();
			}
		}
		else if (pEntity && !FStringNull(pEntity->pev->globalname))
		{
			const globalentity_t* pGlobal = gGlobalState.EntityFromTable(pEntity->pev->globalname);
			if (pGlobal)
			{
				// Already dead? delete
				if (pGlobal->state == GLOBAL_DEAD)
					return -1;
				else if (!FStrEq(STRING(gpGlobals->mapname), pGlobal->levelName))
				{
					pEntity->MakeDormant(); // Hasn't been moved to this level yet, wait but stay alive
				}
				// In this level & not dead, continue on as normal
			}
			else
			{
				ALERT(at_error, "Global Entity %s (%s) not in table!!!\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->classname));
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd(pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON);
			}
		}
	}
	return 0;
}


void DispatchObjectCollsionBox(edict_t* pent)
{
	CBaseEntity* pEntity = (CBaseEntity*)GET_PRIVATE(pent);
	if (pEntity)
	{
		pEntity->SetObjectCollisionBox();
	}
	else
		SetObjectCollisionBox(&pent->v);
}


void SaveWriteFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	CSave saveHelper(*pSaveData);
	saveHelper.WriteFields(pname, pBaseData, pFields, fieldCount);
}


void SaveReadFields(SAVERESTOREDATA* pSaveData, const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	if (!CSaveRestoreBuffer::IsValidSaveRestoreData(pSaveData))
	{
		return;
	}

	//Will happen here if we're loading a saved game
	//ETABLE is the first chunk of data read after the engine has set up some global variables that we need
	if (0 == strcmp(pname, "ETABLE"))
	{
		g_Server.CheckForNewMapStart(true);
	}

	CRestore restoreHelper(*pSaveData);
	restoreHelper.ReadFields(pname, pBaseData, pFields, fieldCount);
}

int CBaseEntity::PrecacheModel(const char* s)
{
	return UTIL_PrecacheModel(s);
}

void CBaseEntity::SetModel(const char* s)
{
	s = UTIL_CheckForGlobalModelReplacement(s);

	g_engfuncs.pfnSetModel(edict(), s);
}

int CBaseEntity::PrecacheSound(const char* s)
{
	return UTIL_PrecacheSound(s);
}

void CBaseEntity::OnCreate()
{
	//Nothing.
}

void CBaseEntity::OnDestroy()
{
	//Nothing.
}

// give health
bool CBaseEntity::TakeHealth(float flHealth, int bitsDamageType)
{
	if (0 == pev->takedamage)
		return false;

	// heal
	if (pev->health >= pev->max_health)
		return false;

	pev->health += flHealth;

	if (pev->health > pev->max_health)
		pev->health = pev->max_health;

	return true;
}

// inflict damage on this entity.  bitsDamageType indicates type of damage inflicted, ie: DMG_CRUSH

bool CBaseEntity::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Vector vecTemp;

	if (0 == pev->takedamage)
		return false;

	// UNDONE: some entity types may be immune or resistant to some bitsDamageType

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin).
	if (pevAttacker == pevInflictor)
	{
		vecTemp = pevInflictor->origin - (VecBModelOrigin(pev));
	}
	else
	// an actual missile was involved.
	{
		vecTemp = pevInflictor->origin - (VecBModelOrigin(pev));
	}

	// this global is still used for glass and other non-monster killables, along with decals.
	g_vecAttackDir = vecTemp.Normalize();

	// save damage based on the target's armor level

	// figure momentum add (don't let hurt brushes or other triggers move player)
	if ((!FNullEnt(pevInflictor)) && (pev->movetype == MOVETYPE_WALK || pev->movetype == MOVETYPE_STEP) && (pevAttacker->solid != SOLID_TRIGGER))
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();

		float flForce = flDamage * ((32 * 32 * 72.0) / (pev->size.x * pev->size.y * pev->size.z)) * 5;

		if (flForce > 1000.0)
			flForce = 1000.0;
		pev->velocity = pev->velocity + vecDir * flForce;
	}

	// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		Killed(pevAttacker, GIB_NORMAL);
		return false;
	}

	return true;
}


void CBaseEntity::Killed(entvars_t* pevAttacker, int iGib)
{
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	UTIL_Remove(this);
}


CBaseEntity* CBaseEntity::GetNextTarget()
{
	if (FStringNull(pev->target))
		return nullptr;
	edict_t* pTarget = FIND_ENTITY_BY_TARGETNAME(nullptr, STRING(pev->target));
	if (FNullEnt(pTarget))
		return nullptr;

	return Instance(pTarget);
}

// Global Savedata for Delay
TYPEDESCRIPTION CBaseEntity::m_SaveData[] =
	{
		DEFINE_FIELD(CBaseEntity, m_pGoalEnt, FIELD_CLASSPTR),
		DEFINE_FIELD(CBaseEntity, m_EFlags, FIELD_CHARACTER),

		DEFINE_FIELD(CBaseEntity, m_pfnThink, FIELD_FUNCTION), // UNDONE: Build table of these!!!
		DEFINE_FIELD(CBaseEntity, m_pfnTouch, FIELD_FUNCTION),
		DEFINE_FIELD(CBaseEntity, m_pfnUse, FIELD_FUNCTION),
		DEFINE_FIELD(CBaseEntity, m_pfnBlocked, FIELD_FUNCTION),
};


bool CBaseEntity::Save(CSave& save)
{
	if (save.WriteEntVars("ENTVARS", pev))
		return save.WriteFields("BASE", this, m_SaveData, std::size(m_SaveData));

	return false;
}

bool CBaseEntity::Restore(CRestore& restore)
{
	bool status;

	status = restore.ReadEntVars("ENTVARS", pev);
	if (status)
		status = restore.ReadFields("BASE", this, m_SaveData, std::size(m_SaveData));

	if (pev->modelindex != 0 && !FStringNull(pev->model))
	{
		Vector mins, maxs;
		mins = pev->mins; // Set model is about to destroy these
		maxs = pev->maxs;

		//Don't use UTIL_PrecacheModel here because we're restoring an already-replaced name.
		g_engfuncs.pfnPrecacheModel(STRING(pev->model));
		SetModel(STRING(pev->model));
		UTIL_SetSize(pev, mins, maxs); // Reset them
	}

	return status;
}


// Initialize absmin & absmax to the appropriate box
void SetObjectCollisionBox(entvars_t* pev)
{
	if ((pev->solid == SOLID_BSP) && pev->angles != g_vecZero)
	{ // expand for rotation
		float max, v;
		int i;

		max = 0;
		for (i = 0; i < 3; i++)
		{
			v = fabs(pev->mins[i]);
			if (v > max)
				max = v;
			v = fabs(pev->maxs[i]);
			if (v > max)
				max = v;
		}
		for (i = 0; i < 3; i++)
		{
			pev->absmin[i] = pev->origin[i] - max;
			pev->absmax[i] = pev->origin[i] + max;
		}
	}
	else
	{
		pev->absmin = pev->origin + pev->mins;
		pev->absmax = pev->origin + pev->maxs;
	}

	pev->absmin.x -= 1;
	pev->absmin.y -= 1;
	pev->absmin.z -= 1;
	pev->absmax.x += 1;
	pev->absmax.y += 1;
	pev->absmax.z += 1;
}


void CBaseEntity::SetObjectCollisionBox()
{
	::SetObjectCollisionBox(pev);
}


bool CBaseEntity::Intersects(CBaseEntity* pOther)
{
	if (pOther->pev->absmin.x > pev->absmax.x ||
		pOther->pev->absmin.y > pev->absmax.y ||
		pOther->pev->absmin.z > pev->absmax.z ||
		pOther->pev->absmax.x < pev->absmin.x ||
		pOther->pev->absmax.y < pev->absmin.y ||
		pOther->pev->absmax.z < pev->absmin.z)
		return false;
	return true;
}

void CBaseEntity::MakeDormant()
{
	SetBits(pev->flags, FL_DORMANT);

	// Don't touch
	pev->solid = SOLID_NOT;
	// Don't move
	pev->movetype = MOVETYPE_NONE;
	// Don't draw
	SetBits(pev->effects, EF_NODRAW);
	// Don't think
	pev->nextthink = 0;
	// Relink
	UTIL_SetOrigin(pev, pev->origin);
}

bool CBaseEntity::IsDormant()
{
	return FBitSet(pev->flags, FL_DORMANT);
}

bool CBaseEntity::IsInWorld()
{
	// position
	if (pev->origin.x >= 4096)
		return false;
	if (pev->origin.y >= 4096)
		return false;
	if (pev->origin.z >= 4096)
		return false;
	if (pev->origin.x <= -4096)
		return false;
	if (pev->origin.y <= -4096)
		return false;
	if (pev->origin.z <= -4096)
		return false;
	// speed
	if (pev->velocity.x >= 2000)
		return false;
	if (pev->velocity.y >= 2000)
		return false;
	if (pev->velocity.z >= 2000)
		return false;
	if (pev->velocity.x <= -2000)
		return false;
	if (pev->velocity.y <= -2000)
		return false;
	if (pev->velocity.z <= -2000)
		return false;

	return true;
}

bool CBaseEntity::ShouldToggle(USE_TYPE useType, bool currentState)
{
	if (useType != USE_TOGGLE && useType != USE_SET)
	{
		if ((currentState && useType == USE_ON) || (!currentState && useType == USE_OFF))
			return false;
	}
	return true;
}


int CBaseEntity::DamageDecal(int bitsDamageType)
{
	if (pev->rendermode == kRenderTransAlpha)
		return -1;

	if (pev->rendermode != kRenderNormal)
		return DECAL_BPROOF1;

	return DECAL_GUNSHOT1 + RANDOM_LONG(0, 4);
}



// NOTE: szName must be a pointer to constant memory, e.g. "monster_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity* CBaseEntity::Create(const char* szName, const Vector& vecOrigin, const Vector& vecAngles, edict_t* pentOwner)
{
	edict_t* pent;
	CBaseEntity* pEntity;

	pent = CREATE_NAMED_ENTITY(MAKE_STRING(szName));
	if (FNullEnt(pent))
	{
		ALERT(at_console, "nullptr Ent in Create!\n");
		return nullptr;
	}
	pEntity = Instance(pent);
	pEntity->pev->owner = pentOwner;
	pEntity->pev->origin = vecOrigin;
	pEntity->pev->angles = vecAngles;
	DispatchSpawn(pEntity->edict());
	return pEntity;
}
