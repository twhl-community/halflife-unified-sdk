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
#ifndef CTFGOAL_H
#define CTFGOAL_H

class CTFGoal : public CBaseAnimating
{
public:
	int Classify() override { return CLASS_NONE; }

	void KeyValue(KeyValueData* pkvd) override;

	void Spawn() override;

	void SetObjectCollisionBox() override;

	void StartGoal();

	void EXPORT PlaceGoal();

	int m_iGoalNum;
	int m_iGoalState;
	Vector m_GoalMin;
	Vector m_GoalMax;
};

#endif