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
#ifndef ROPE_CROPESEGMENT_H
#define ROPE_CROPESEGMENT_H

class CRopeSample;

/**
*	Represents a visible rope segment.
*	Segments require models to define an attachment 0 that, when the segment origin is subtracted from it, represents the length of the segment.
*/
class CRopeSegment : public CBaseAnimating
{
public:
	using BaseClass = CBaseAnimating;

	CRopeSegment();

	void Precache() override;

	void Spawn() override;

	void Think() override;

	void Touch( CBaseEntity* pOther ) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	static CRopeSegment* CreateSegment( CRopeSample* pSample, string_t iszModelName );
	
	CRopeSample* GetSample() { return m_pSample; }

	/**
	*	Applies external force to the segment.
	*	@param vecForce Force.
	*/
	void ApplyExternalForce( const Vector& vecForce );

	/**
	*	Resets the mass to the default value.
	*/
	void SetMassToDefault();

	/**
	*	Sets the default mass.
	*	@param flDefaultMass Mass.
	*/
	void SetDefaultMass( const float flDefaultMass );

	/**
	*	Sets the mass.
	*	@param flMass Mass.
	*/
	void SetMass( const float flMass );

	/**
	*	Sets whether the segment should cause damage on touch.
	*	@param bCauseDamage Whether to cause damage.
	*/
	void SetCauseDamageOnTouch( const bool bCauseDamage );

	/**
	*	Sets whether the segment can be grabbed.
	*	@param bCanBeGrabbed Whether the segment can be grabbed.
	*/
	void SetCanBeGrabbed( const bool bCanBeGrabbed );

private:
	CRopeSample* m_pSample;
	string_t m_iszModelName;
	float m_flDefaultMass;
	bool m_bCauseDamage;
	bool m_bCanBeGrabbed;
};

#endif
