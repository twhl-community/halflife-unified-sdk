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

#include "CTFGoal.h"

extern float g_flFlagReturnTime;

void DumpCTFFlagInfo(CBasePlayer* pPlayer);

class CTFGoalFlag : public CTFGoal
{
public:
	void Precache() override;

	void EXPORT PlaceItem();

	void EXPORT ReturnFlag();

	void EXPORT FlagCarryThink();

	void EXPORT goal_item_dropthink();

	void Spawn() override;

	void EXPORT ReturnFlagThink();

	void StartItem();

	void EXPORT ScoreFlagTouch(CBasePlayer* pOther);

	void TurnOnLight(CBasePlayer* pPlayer);

	void GiveFlagToPlayer(CBasePlayer* pPlayer);

	void EXPORT goal_item_touch(CBaseEntity* pOther);

	void SetDropTouch();

	void DoDrop(const Vector& vecOrigin);

	void DropFlag(CBasePlayer* pPlayer);

	void DisplayFlagStatus(CBasePlayer* pPlayer);

	int Classify() override { return CLASS_NONE; }

	void SetObjectCollisionBox() override;

	unsigned int drop_time;
	Vector m_OriginalAngles;
	int m_nReturnPlayer;
	float m_flReturnTime;
};
