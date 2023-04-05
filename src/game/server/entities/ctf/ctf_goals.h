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

#include "CBaseAnimating.h"

class CBasePlayer;

extern float g_flFlagReturnTime;

void DumpCTFFlagInfo(CBasePlayer* pPlayer);

class CTFGoal : public CBaseAnimating
{
	DECLARE_CLASS(CTFGoal, CBaseAnimating);
	DECLARE_DATAMAP();

public:
	int Classify() override { return CLASS_NONE; }

	bool KeyValue(KeyValueData* pkvd) override;

	void Spawn() override;

	void SetObjectCollisionBox() override;

	void StartGoal();

	void PlaceGoal();

	int m_iGoalNum;
	int m_iGoalState;
	Vector m_GoalMin;
	Vector m_GoalMax;
};

class CTFGoalBase : public CTFGoal
{
	DECLARE_CLASS(CTFGoalBase, CTFGoal);
	DECLARE_DATAMAP();

public:
	int Classify() override { return CLASS_NONE; }

	void BaseThink();

	void Spawn() override;

	void TurnOnLight(CBasePlayer* pPlayer);
};

class CTFGoalFlag : public CTFGoal
{
	DECLARE_CLASS(CTFGoalFlag, CTFGoal);
	DECLARE_DATAMAP();

public:
	void Precache() override;

	void PlaceItem();

	void ReturnFlag();

	void FlagCarryThink();

	void goal_item_dropthink();

	void Spawn() override;

	void ReturnFlagThink();

	void StartItem();

	void ScoreFlagTouch(CBasePlayer* pOther);

	void TurnOnLight(CBasePlayer* pPlayer);

	void GiveFlagToPlayer(CBasePlayer* pPlayer);

	void goal_item_touch(CBaseEntity* pOther);

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
