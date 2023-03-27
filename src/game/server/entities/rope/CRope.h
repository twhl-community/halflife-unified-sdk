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

class CRopeSegment;
class CRopeSample;

struct RopeSampleData;

/**
 *	Represents a spring that keeps samples a given distance apart.
 */
struct Spring
{
	size_t p1;
	size_t p2;
	float restLength;
	float hookConstant;
	float springDampning;
};

/**
 *	A rope with a number of segments.
 *	Uses an RK4 integrator with dampened springs to simulate rope physics.
 */
class CRope : public CBaseDelay
{
public:
	static const size_t MAX_SEGMENTS = 63;

	static const size_t MAX_SAMPLES = 64;

	static const size_t MAX_TEMP_SAMPLES = 5;

	using BaseClass = CBaseDelay;

public:
	CRope();
	~CRope();

	bool KeyValue(KeyValueData* pkvd) override;

	void Precache() override;

	void Spawn() override;

	void UpdateOnRemove() override;

	void Think() override;

	void Touch(CBaseEntity* pOther) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	void InitializeRopeSim();

	void InitializeSprings(const size_t uiNumSprings);

	void RunSimOnSamples();

	/**
	 *	Computes forces on the given sample list.
	 *	@param pSystem List of sample data. m_NumSamples Elements large.
	 */
	void ComputeForces(RopeSampleData* pSystem);

	/**
	 *	Computes forces on the given sample list.
	 *	@param pSystem List of samples. m_NumSamples Elements large.
	 */
	void ComputeForces(CRopeSample** ppSystem);

	void ComputeSampleForce(RopeSampleData& data);

	void ComputeSpringForce(RopeSampleData& first, RopeSampleData& second, const Spring& spring);

	/**
	 *	Runs RK4 integration.
	 *	@param flDeltaTime Delta between previous and current time.
	 *	@param ppSampleSource Previous sample state.
	 *	@param ppSampleTarget Next sample state.
	 */
	void RK4Integrate(const float flDeltaTime, CRopeSample** ppSampleSource, CRopeSample** ppSampleTarget);

	/**
	 *	Traces model positions and angles and corrects them.
	 *	@param ppPrimarySegs Visible segments.
	 *	@param ppHiddenSegs hidden segments.
	 */
	void TraceModels(CRopeSegment** ppPrimarySegs, CRopeSegment** ppHiddenSegs);

	/**
	 *	Traces model positions and angles, makes visible segments visible and hidden segments hidden.
	 */
	void SetRopeSegments(const size_t uiNumSegments,
		CRopeSegment** ppPrimarySegs, CRopeSegment** ppHiddenSegs);

	/**
	 *	Moves the attached object up.
	 *	@param flDeltaTime Time between previous and current movement.
	 *	@return true if the object is still on the rope, false otherwise.
	 */
	bool MoveUp(const float flDeltaTime);

	/**
	 *	Moves the attached object down.
	 *	@param flDeltaTime Time between previous and current movement.
	 *	@return true if the object is still on the rope, false otherwise.
	 */
	bool MoveDown(const float flDeltaTime);

	Vector GetAttachedObjectsVelocity() const;

	/**
	 *	Applies force from the player. Only applies if there is currently an object attached to the rope.
	 */
	void ApplyForceFromPlayer(const Vector& vecForce);

	void ApplyForceToSegment(const Vector& vecForce, const size_t uiSegment);

	void AttachObjectToSegment(CRopeSegment* pSegment);

	void DetachObject();

	bool IsObjectAttached() const { return m_bObjectAttached != false; }

	bool IsAcceptingAttachment() const;

	size_t GetNumSegments() const { return m_uiSegments; }

	CRopeSegment** GetSegments() { return seg; }

	CRopeSegment** GetAltSegments() { return altseg; }

	bool GetToggleValue() const { return m_bToggle != false; }

	bool IsSoundAllowed() const { return m_bMakeSound != false; }

	void SetSoundAllowed(const bool bAllowed)
	{
		m_bMakeSound = bAllowed;
	}

	bool ShouldCreak() const;

	/**
	 *	Plays a creak sound.
	 */
	void Creak();

	string_t GetBodyModel() const { return m_iszBodyModel; }

	string_t GetEndingModel() const { return m_iszEndingModel; }

	float GetSegmentLength(size_t uiSegmentIndex) const;

	float GetRopeLength() const;

	Vector GetRopeOrigin() const;

	bool IsValidSegmentIndex(const size_t uiSegment) const;

	Vector GetSegmentOrigin(const size_t uiSegment) const;

	Vector GetSegmentAttachmentPoint(const size_t uiSegment) const;

	void SetAttachedObjectsSegment(CRopeSegment* pSegment);

	/**
	 *	@param uiSegmentIndex Segment index.
	 *	@return The segment direction normal from its origin.
	 */
	Vector GetSegmentDirFromOrigin(const size_t uiSegmentIndex) const;

	Vector GetAttachedObjectsPosition() const;

private:
	size_t m_uiSegments;

	CRopeSegment* seg[MAX_SEGMENTS];
	CRopeSegment* altseg[MAX_SEGMENTS];

	bool m_bToggle;

	bool m_bInitialDeltaTime;

	float m_flLastTime;

	Vector m_vecLastEndPos;
	Vector m_vecGravity;
	float m_flHookConstant;
	float m_flSpringDampning;

	CRopeSample* m_CurrentSys[MAX_SAMPLES];
	CRopeSample* m_TargetSys[MAX_SAMPLES];
	RopeSampleData* m_TempSys[MAX_TEMP_SAMPLES];

	size_t m_uiNumSamples;

	Spring* m_pSprings;

	size_t m_SpringCnt;

	bool m_bSpringsInitialized;

	bool m_bObjectAttached;

	size_t m_uiAttachedObjectsSegment;
	float m_flAttachedObjectsOffset;
	float m_flDetachTime;

	string_t m_iszBodyModel;
	string_t m_iszEndingModel;

	bool m_bDisallowPlayerAttachment;

	bool m_bMakeSound;
};
