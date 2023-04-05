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

class CSatchelCharge : public CGrenade
{
	DECLARE_CLASS(CSatchelCharge, CGrenade);
	DECLARE_DATAMAP();

public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	void BounceSound() override;

	void SatchelSlide(CBaseEntity* pOther);
	void SatchelThink();

public:
	/**
	 *	@brief do whatever it is we do to an orphaned satchel when we don't want it in the world anymore.
	 */
	void Deactivate();
};

/**
 *	@brief removes all satchels owned by the provided player. Should only be used upon death.
 *	Made this global on purpose.
 */
void DeactivateSatchels(CBasePlayer* pOwner);
