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
#ifndef ROPE_CROPESAMPLE_H
#define CROPESAMPLE_H

class CRope;

/**
*	Data for a single rope joint.
*/
struct RopeSampleData
{
	Vector mPosition;
	Vector mVelocity;
	Vector mForce;
	Vector mExternalForce;

	bool mApplyExternalForce;

	float mMassReciprocal;
};

/**
*	Represents a single joint in a rope. There are numSegments + 1 samples in a rope.
*/
class CRopeSample : public CBaseEntity
{
public:
	using BaseClass = CBaseEntity;

	void Spawn() override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	static CRopeSample* CreateSample();

	const RopeSampleData& GetData() const { return m_Data; }

	RopeSampleData& GetData() { return m_Data; }

	CRope* GetMasterRope() { return m_pMasterRope; }

	void SetMasterRope( CRope* pRope )
	{
		m_pMasterRope = pRope;
	}

private:
	RopeSampleData m_Data;
	CRope* m_pMasterRope;
};

#endif
