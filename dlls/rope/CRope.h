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
#ifndef GAME_SERVER_ENTITIES_ROPE_CROPE_H
#define GAME_SERVER_ENTITIES_ROPE_CROPE_H

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

	void KeyValue( KeyValueData* pkvd ) override;

	void Precache() override;

	void Spawn() override;

	void Think() override;

	void Touch( CBaseEntity* pOther ) override;

	int Save( CSave &save ) override;
	int Restore( CRestore &restore ) override;

	static TYPEDESCRIPTION m_SaveData[];

	/**
	*	Initializes the rope simulation data.
	*/
	void InitializeRopeSim();

	/**
	*	Initializes the springs.
	*	@param uiNumSprings Number of springs to create.
	*/
	void InitializeSprings( const size_t uiNumSprings );

	/**
	*	Runs simulation on the samples.
	*/
	void RunSimOnSamples();

	/**
	*	Computes forces on the given sample list.
	*	@param pSystem List of sample data. m_NumSamples Elements large.
	*/
	void ComputeForces( RopeSampleData* pSystem );

	/**
	*	Computes forces on the given sample list.
	*	@param pSystem List of samples. m_NumSamples Elements large.
	*/
	void ComputeForces( CRopeSample** ppSystem );

	/**
	*	Computes forces for the given sample data.
	*	@param data Sample data.
	*/
	void ComputeSampleForce( RopeSampleData& data );

	/**
	*	Computes spring force for the given sample datas.
	*	@param first First sample.
	*	@param second Second sample.
	*	@param spring Spring.
	*/
	void ComputeSpringForce( RopeSampleData& first, RopeSampleData& second, const Spring& spring );

	/**
	*	Runs RK4 integration.
	*	@param flDeltaTime Delta between previous and current time.
	*	@param ppSampleSource Previous sample state.
	*	@param ppSampleTarget Next sample state.
	*/
	void RK4Integrate( const float flDeltaTime, CRopeSample** ppSampleSource, CRopeSample** ppSampleTarget );

	/**
	*	Traces model positions and angles and corrects them.
	*	@param ppPrimarySegs Visible segments.
	*	@param ppHiddenSegs hidden segments.
	*/
	void TraceModels( CRopeSegment** ppPrimarySegs, CRopeSegment** ppHiddenSegs );

	/**
	*	Traces model positions and angles, makes visible segments visible and hidden segments hidden.
	*/
	void SetRopeSegments( const size_t uiNumSegments, 
						  CRopeSegment** ppPrimarySegs, CRopeSegment** ppHiddenSegs );

	/**
	*	Moves the attached object up.
	*	@param flDeltaTime Time between previous and current movement.
	*	@return true if the object is still on the rope, false otherwise.
	*/
	bool MoveUp( const float flDeltaTime );

	/**
	*	Moves the attached object down.
	*	@param flDeltaTime Time between previous and current movement.
	*	@return true if the object is still on the rope, false otherwise.
	*/
	bool MoveDown( const float flDeltaTime );

	/**
	*	@return The attached object's velocity.
	*/
	Vector GetAttachedObjectsVelocity() const;

	/**
	*	Applies force from the player. Only applies if there is currently an object attached to the rope.
	*	@param vecForce Force.
	*/
	void ApplyForceFromPlayer( const Vector& vecForce );

	/**
	*	Applies force to a specific segment.
	*	@param vecForce Force.
	*	@param uiSegment Segment index.
	*/
	void ApplyForceToSegment( const Vector& vecForce, const size_t uiSegment );

	/**
	*	Attached an object to the given segment.
	*/
	void AttachObjectToSegment( CRopeSegment* pSegment );

	/**
	*	Detaches an attached object.
	*/
	void DetachObject();

	/**
	*	@return Whether an object is attached.
	*/
	bool IsObjectAttached() const { return m_bObjectAttached; }

	/**
	*	@return Whether this rope allows attachments.
	*/
	bool IsAcceptingAttachment() const;

	/**
	*	@return The number of segments.
	*/
	size_t GetNumSegments() const { return m_uiSegments; }

	/**
	*	@return The segments.
	*/
	CRopeSegment** GetSegments() { return seg; }

	/**
	*	@return The alternative segments.
	*/
	CRopeSegment** GetAltSegments() { return altseg; }

	/**
	*	@return The toggle value.
	*/
	bool GetToggleValue() const { return m_bToggle; }

	/**
	*	@return Whether this rope is allowed to make sounds.
	*/
	bool IsSoundAllowed() const { return m_bMakeSound; }

	/**
	*	Sets whether this rope is allowed to make sounds.
	*/
	void SetSoundAllowed( const bool bAllowed )
	{
		m_bMakeSound = bAllowed;
	}

	/**
	*	@return Whether this rope should creak.
	*/
	bool ShouldCreak() const;

	/**
	*	Plays a creak sound.
	*/
	void Creak();

	/**
	*	@return The body model name.
	*/
	string_t GetBodyModel() const { return m_iszBodyModel; }

	/**
	*	@return The ending model name.
	*/
	string_t GetEndingModel() const { return m_iszEndingModel; }

	/**
	*	@return Segment length for the given segment.
	*/
	float GetSegmentLength( size_t uiSegmentIndex ) const;

	/**
	*	@return Total rope length.
	*/
	float GetRopeLength() const;

	/**
	*	@return The rope's origin.
	*/
	Vector GetRopeOrigin() const;

	/**
	*	@return Whether the given segment index is valid.
	*/
	bool IsValidSegmentIndex( const size_t uiSegment ) const;

	/**
	*	@return The origin of the given segment.
	*/
	Vector GetSegmentOrigin( const size_t uiSegment ) const;

	/**
	*	@return The attachment point of the given segment.
	*/
	Vector GetSegmentAttachmentPoint( const size_t uiSegment ) const;

	/**
	*	@param pSegment Segment.
	*	Sets the attached object segment.
	*/
	void SetAttachedObjectsSegment( CRopeSegment* pSegment );

	/**
	*	@param uiSegmentIndex Segment index.
	*	@return The segment direction normal from its origin.
	*/
	Vector GetSegmentDirFromOrigin( const size_t uiSegmentIndex ) const;

	/**
	*	@return The attached object position.
	*/
	Vector GetAttachedObjectsPosition() const;

private:
	size_t m_uiSegments;

	CRopeSegment* seg[ MAX_SEGMENTS ];
	CRopeSegment* altseg[ MAX_SEGMENTS ];

	bool m_bToggle;

	bool m_bInitialDeltaTime;

	float m_flLastTime;

	Vector m_vecLastEndPos;
	Vector m_vecGravity;
	float m_flHookConstant;
	float m_flSpringDampning;

	CRopeSample* m_CurrentSys[ MAX_SAMPLES ];
	CRopeSample* m_TargetSys[ MAX_SAMPLES ];
	RopeSampleData* m_TempSys[ MAX_TEMP_SAMPLES ];

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

#endif
