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

class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT UseChangeLevel(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT TouchChangeLevel(CBaseEntity* pOther);
	void ChangeLevelNow(CBaseEntity* pActivator);

	static CBaseEntity* FindLandmark(const char* pLandmarkName);
	static int ChangeList(LEVELLIST* pLevelList, int maxList);
	static bool AddTransitionToList(LEVELLIST* pLevelList, int listCount, const char* pMapName, const char* pLandmarkName, CBaseEntity* pentLandmark);
	static bool InTransitionVolume(CBaseEntity* pEntity, char* pVolumeName);

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	char m_szMapName[cchMapNameMost];	   // trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost]; // trigger_changelevel only:  landmark on next map
	string_t m_changeTarget;
	float m_changeTargetDelay;
};
