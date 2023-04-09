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

#define SF_ITEM_USE_ONLY 256 //  ITEM_USE_ONLY = BUTTON_USE_ONLY = DOOR_USE_ONLY!!!

class CBaseToggle;

using MoveDonePtr = TBASEPTR<CBaseToggle>;

/**
 *	@brief generic Toggle entity.
 */
class CBaseToggle : public CBaseAnimating
{
	DECLARE_CLASS(CBaseToggle, CBaseAnimating);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;

	TOGGLE_STATE m_toggle_state;
	float m_flActivateFinished; // like attack_finished, but for doors
	float m_flMoveDistance;		// how far a door should slide or rotate
	float m_flWait;
	float m_flLip;

	Vector m_vecPosition1;
	Vector m_vecPosition2;
	Vector m_vecAngle1;
	Vector m_vecAngle2;

	MoveDonePtr m_pfnCallWhenMoveDone;
	Vector m_vecFinalDest;
	Vector m_vecFinalAngle;

	float GetDelay() override { return m_flWait; }

	// common member functions
	/**
	 *	@brief calculate pev->velocity and pev->nextthink to reach vecDest from pev->origin traveling at flSpeed
	 */
	void LinearMove(Vector vecDest, float flSpeed);

	/**
	 *	@brief After moving, set origin to exact final destination, call "move done" function
	 */
	void LinearMoveDone();

	/**
	 *	@brief calculate pev->velocity and pev->nextthink to reach vecDest from pev->origin traveling at flSpeed
	 *	Just like LinearMove, but rotational.
	 */
	void AngularMove(Vector vecDestAngle, float flSpeed);

	/**
	 *	@brief After rotating, set angle to exact final angle, call "move done" function
	 */
	void AngularMoveDone();

	static float AxisValue(int flags, const Vector& angles);
	static void AxisDir(CBaseEntity* entity);
	static float AxisDelta(int flags, const Vector& angle1, const Vector& angle2);

	template <typename T>
	MoveDonePtr MoveDoneSet(TBASEPTR<T> func, const char* name)
	{
		return FunctionSet<T>(m_pfnCallWhenMoveDone, func, name);
	}
};

#define SetMoveDone(a) MoveDoneSet(a, #a)
