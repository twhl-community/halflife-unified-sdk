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
#ifndef WEAPONS_CDISPLACERBALL_H
#define WEAPONS_CDISPLACERBALL_H

class CDisplacerBall : public CBaseEntity
{
private:
	static const size_t NUM_BEAMS = 8;

public:
	using BaseClass = CBaseEntity;

	void Precache() override;

	void Spawn() override;

	int Classify() override;

	void EXPORT BallTouch( CBaseEntity* pOther );

	void EXPORT FlyThink();

	void EXPORT FlyThink2();

	void EXPORT FizzleThink();

	void EXPORT ExplodeThink();

	void EXPORT KillThink();

	void InitBeams();

	void ClearBeams();

	void ArmBeam( int iSide );

	bool ClassifyTarget( CBaseEntity* pTarget );

	static CDisplacerBall* CreateDisplacerBall( const Vector& vecOrigin, const Vector& vecAngles, CBaseEntity* pOwner );

private:
	int m_iTrail;

	CBeam* m_pBeam[ NUM_BEAMS ];

	size_t m_uiBeams;

	EHANDLE m_hDisplacedTarget;
};

#endif