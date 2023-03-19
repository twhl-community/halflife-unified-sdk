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

#include "CBaseDelay.h"

class CBaseAnimating : public CBaseDelay
{
public:
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	// Basic Monster Animation functions
	/**
	*	@brief advance the animation frame from last time called up to the current time
	*	if an flInterval is passed in, only advance animation that number of seconds
	*/
	float StudioFrameAdvance(float flInterval = 0.0);
	int GetSequenceFlags();
	int LookupActivity(int activity);

	/**
	*	@brief Get activity with highest 'weight'
	*/
	int LookupActivityHeaviest(int activity);
	int LookupSequence(const char* label);
	void ResetSequenceInfo();
	void DispatchAnimEvents(float flFutureInterval = 0.1); // Handle events that have happend since last time called up until X seconds into the future

	/**
	*	@brief catches the messages that occur when tagged animation frames are played.
	*/
	virtual void HandleAnimEvent(MonsterEvent_t* pEvent) {}
	float SetBoneController(int iController, float flValue);
	void InitBoneControllers();
	float SetBlending(int iBlender, float flValue);
	void GetBonePosition(int iBone, Vector& origin, Vector& angles);
	void GetAutomovement(Vector& origin, Vector& angles, float flInterval = 0.1);
	int FindTransition(int iEndingSequence, int iGoalSequence, int* piDir);
	void GetAttachment(int iAttachment, Vector& origin, Vector& angles);
	void SetBodygroup(int iGroup, int iValue);
	int GetBodygroup(int iGroup) const;
	int GetBodygroupSubmodelCount(int group);
	bool ExtractBbox(int sequence, Vector& mins, Vector& maxs);
	void SetSequenceBox();

	// animation needs
	float m_flFrameRate;	  //!< computed FPS for current sequence
	float m_flGroundSpeed;	  //!< computed linear movement rate for current sequence
	float m_flLastEventCheck; //!< last time the event list was checked
	bool m_fSequenceFinished; //!< flag set when StudioAdvanceFrame moves across a frame boundry
	bool m_fSequenceLoops;	  //!< true if the sequence loops
};
