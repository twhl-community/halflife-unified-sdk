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

#include "Platform.h"
#include "DataMap.h"

enum GLOBALESTATE
{
	GLOBAL_OFF = 0,
	GLOBAL_ON = 1,
	GLOBAL_DEAD = 2
};

struct globalentity_t
{
	DECLARE_CLASS_NOBASE(globalentity_t);
	DECLARE_SIMPLE_DATAMAP();

public:
	char name[64];
	char levelName[32];
	GLOBALESTATE state;
	globalentity_t* pNext;
};

class CGlobalState
{
	DECLARE_CLASS_NOBASE(CGlobalState);
	DECLARE_SIMPLE_DATAMAP();

public:
	CGlobalState();
	void Reset();
	void ClearStates();
	void EntityAdd(string_t globalname, string_t mapName, GLOBALESTATE state);
	void EntitySetState(string_t globalname, GLOBALESTATE state);
	void EntityUpdate(string_t globalname, string_t mapname);
	const globalentity_t* EntityFromTable(string_t globalname);
	GLOBALESTATE EntityGetState(string_t globalname);
	bool EntityInTable(string_t globalname) { return Find(globalname) != nullptr; }
	bool Save(CSave& save);
	bool Restore(CRestore& restore);

	// #ifdef _DEBUG
	void DumpGlobals();
	// #endif

private:
	globalentity_t* Find(string_t globalname);
	globalentity_t* m_pList;
	int m_listCount;
};

extern CGlobalState gGlobalState;
