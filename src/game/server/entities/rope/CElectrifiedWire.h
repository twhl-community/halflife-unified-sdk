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

#include "CRope.h"

/**
*	An electrified wire.
*	Can be toggled on and off. Starts on.
*/
class CElectrifiedWire : public CRope
{
public:
	using BaseClass = CRope;

	bool KeyValue(KeyValueData* pkvd) override;

	void Precache() override;

	void Spawn() override;

	void Think() override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float flValue) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	bool IsActive() const { return m_bIsActive != false; }

	/**
	*	@param iFrequency Frequency.
	*	@return Whether the spark effect should be performed.
	*/
	bool ShouldDoEffect(const int iFrequency);

	void DoSpark(const size_t uiSegment, const bool bExertForce);

	void DoLightning();

public:
	bool m_bIsActive = true;

	int m_iTipSparkFrequency = 3;
	int m_iBodySparkFrequency = 100;
	int m_iLightningFrequency = 150;

	int m_iXJoltForce = 0;
	int m_iYJoltForce = 0;
	int m_iZJoltForce = 0;

	size_t m_uiNumUninsulatedSegments = 0;
	size_t m_uiUninsulatedSegments[MAX_SEGMENTS];

	int m_iLightningSprite = 0;

	float m_flLastSparkTime = 0;
};
