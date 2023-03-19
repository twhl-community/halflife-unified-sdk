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

#include "CBaseToggle.h"

class CBaseTrigger : public CBaseToggle
{
public:
	void EXPORT TeleportTouch(CBaseEntity* pOther);
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT MultiTouch(CBaseEntity* pOther);

	/**
	*	@brief When touched, a hurt trigger does DMG points of damage each half-second
	*/
	void EXPORT HurtTouch(CBaseEntity* pOther);

	void ActivateMultiTrigger(CBaseEntity* pActivator);

	/**
	*	@brief the wait time has passed, so set back up for another activation
	*/
	void EXPORT MultiWaitOver();

	void EXPORT CounterUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	/**
	*	@brief If this is the USE function for a trigger, its state will toggle every time it's fired
	*/
	void EXPORT ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void InitTrigger();

	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};
