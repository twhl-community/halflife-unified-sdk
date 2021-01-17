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
#ifndef CITEMCTF_H
#define CITEMCTF_H

#include "CTFDefs.h"

class CItemSpawnCTF;

class CItemCTF : public CBaseAnimating
{
public:
	static CItemSpawnCTF* m_pLastSpawn;

public:
	void KeyValue( KeyValueData* pkvd ) override;
	void Precache() override;
	void Spawn() override;

	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector( -16, -16, 0 );
		pev->absmax = pev->origin + Vector( 16, 16, 48 );
	}

	void EXPORT DropPreThink();

	void EXPORT DropThink();

	void EXPORT CarryThink();

	void EXPORT ItemTouch( CBaseEntity* pOther );

	virtual void RemoveEffect( CBasePlayer* pPlayer ) {}

	virtual bool MyTouch( CBasePlayer* pPlayer ) { return false; }

	void DropItem( CBasePlayer* pPlayer, bool bForceRespawn );
	void ScatterItem( CBasePlayer* pPlayer );
	void ThrowItem( CBasePlayer* pPlayer );

	CTFTeam team_no;
	int m_iLastTouched;
	float m_flNextTouchTime;
	float m_flPickupTime;
	CTFItem::CTFItem m_iItemFlag;
	const char* m_pszItemName;
};

#endif
