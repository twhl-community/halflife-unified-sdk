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

class CDisplacerBall : public CBaseEntity
{
	DECLARE_CLASS(CDisplacerBall, CBaseEntity);
	DECLARE_DATAMAP();

private:
	static const size_t NUM_BEAMS = 8;

public:
	void Precache() override;
	void Spawn() override;

	void BallTouch(CBaseEntity* pOther);

	void FlyThink();
	void FlyThink2();
	void FizzleThink();
	void ExplodeThink();
	void KillThink();

	void InitBeams();

	void ClearBeams();

	void ArmBeam(int iSide);

	bool ClassifyTarget(CBaseEntity* pTarget);

	static CDisplacerBall* CreateDisplacerBall(const Vector& vecOrigin, const Vector& vecAngles, CBaseEntity* pOwner);

private:
	int m_iTrail;

	CBeam* m_pBeam[NUM_BEAMS];

	size_t m_uiBeams;

	EHANDLE m_hDisplacedTarget;
};
