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

#include "weapons.h"

class CRpg;

class CRpgRocket : public CGrenade
{
	DECLARE_CLASS(CRpgRocket, CGrenade);
	DECLARE_DATAMAP();

public:
	~CRpgRocket() override;

	void Spawn() override;
	void Precache() override;
	void FollowThink();
	void IgniteThink();
	void RocketTouch(CBaseEntity* pOther);
	static CRpgRocket* CreateRpgRocket(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, CRpg* pLauncher);

	int m_iTrail;
	float m_flIgniteTime;
	EntityHandle<CRpg> m_pLauncher; // handle back to the launcher that fired me.
};
