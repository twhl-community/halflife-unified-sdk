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

#include "CBaseTrigger.h"

#define SF_CHANGELEVEL_USEONLY 0x0002

/**
*	@brief When the player touches this, he gets sent to the map listed in the "map" variable.
*/
class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;

	/**
	*	@brief allows level transitions to be triggered by buttons, etc.
	*/
	void EXPORT UseChangeLevel(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void EXPORT TouchChangeLevel(CBaseEntity* pOther);
	void ChangeLevelNow(CBaseEntity* pActivator);

	static CBaseEntity* FindLandmark(const char* pLandmarkName);

	/**
	*	@brief This builds the list of all transitions on this level and
	*	which entities are in their PVS's and can/should be moved across.
	*/
	static int ChangeList(LEVELLIST* pLevelList, int maxList);

	/**
	*	@brief Add a transition to the list, but ignore duplicates
	*	(a designer may have placed multiple trigger_changelevels with the same landmark)
	*/
	static bool AddTransitionToList(LEVELLIST* pLevelList, int listCount, const char* pMapName, const char* pLandmarkName, CBaseEntity* pentLandmark);

	static bool InTransitionVolume(CBaseEntity* pEntity, char* pVolumeName);

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	char m_szMapName[cchMapNameMost];	   // trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost]; // trigger_changelevel only:  landmark on next map
	string_t m_changeTarget;
	float m_changeTargetDelay;

	bool m_UsePersistentLevelChange = true;
};
