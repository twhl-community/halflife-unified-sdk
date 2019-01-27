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
#ifndef CTFSPAWN_H
#define CTFSPAWN_H

#include "CTFDefs.h"

class CTFSpawn : public CBaseEntity
{
public:
	int Classify() override { return CLASS_NONE; }

	void KeyValue( KeyValueData* pkvd ) override;

	void Spawn() override;

	BOOL IsTriggered( CBaseEntity* pEntity ) override;

	CTFTeam team_no;
	bool m_fState;
};


#endif
