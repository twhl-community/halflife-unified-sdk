/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   This source code contains proprietary and confidential information of
 *   Valve LLC and its suppliers.  Access to this code is restricted to
 *   persons who have executed a written SDK license with Valve.  Any access,
 *   use or distribution of this code by or to any unlicensed person is illegal.
 *
 ****/

#pragma once

#include <memory>

#include <spdlog/logger.h>

#include "scriptevent.h"

#define SF_SCRIPT_WAITTILLSEEN 1
#define SF_SCRIPT_EXITAGITATED 2
#define SF_SCRIPT_REPEATABLE 4
#define SF_SCRIPT_LEAVECORPSE 8
// #define SF_SCRIPT_INTERPOLATE		16 // don't use, old bug
#define SF_SCRIPT_NOINTERRUPT 32
#define SF_SCRIPT_OVERRIDESTATE 64
#define SF_SCRIPT_NOSCRIPTMOVEMENT 128

/**
 *	@brief Don't reset the entity's state after completing the script
 *	For chaining scripts without sequence changes
 */
constexpr auto SF_SCRIPT_NORESETENTITY = 256;

#define SCRIPT_BREAK_CONDITIONS (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE)

enum SS_INTERRUPT
{
	SS_INTERRUPT_IDLE = 0,
	SS_INTERRUPT_BY_NAME,
	SS_INTERRUPT_AI,
};

// when a monster finishes an AI scripted sequence, we can choose
// a schedule to place them in. These defines are the aliases to
// resolve worldcraft input to real schedules (sjb)
#define SCRIPT_FINISHSCHED_DEFAULT 0
#define SCRIPT_FINISHSCHED_AMBUSH 1

/**
*	@details classname "scripted_sequence"
*	targetname "me" - there can be more than one with the same name, and they act in concert
*	target "the_entity_I_want_to_start_playing" or "class entity_classname" will pick the closest inactive scientist
*	play "name_of_sequence"
*	idle "name of idle sequence to play before starting"
*	donetrigger "whatever" - can be any other triggerable entity such as another sequence, train, door, or a special case like "die" or "remove"
*	moveto - if set the monster first moves to this nodes position
*	range # - only search this far to find the target
*	spawnflags - (stop if blocked, stop if player seen)
*/
class CCineMonster : public CBaseMonster
{
	DECLARE_CLASS(CCineMonster, CBaseMonster);
	DECLARE_DATAMAP();

public:
	static inline std::shared_ptr<spdlog::logger> AIScriptLogger;

	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void Blocked(CBaseEntity* pOther) override;
	void Touch(CBaseEntity* pOther) override;
	int ObjectCaps() override { return (CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	/**
	*	@brief Find an entity that I'm interested in and precache the sounds he'll need in the sequence.
	*/
	void Activate() override;

	// void EXPORT CineSpawnThink();
	void EXPORT CineThink();
	void Pain();
	void Die();

	/**
	*	@brief find all the cinematic entities with my targetname and tell them to wait before starting
	*/
	void DelayStart(bool state);

	/**
	*	@brief find a viable entity
	*/
	bool FindEntity();

	/**
	*	@brief make the entity enter a scripted sequence
	*/
	virtual void PossessEntity();

	/**
	*	@brief find all the cinematic entities with my targetname and stop them from playing
	*/
	void CancelScript();

	/**
	*	@brief lookup a sequence name and setup the target monster to play it
	*/
	virtual bool StartSequence(CBaseMonster* pTarget, string_t iszSeq, bool completeOnEmpty);

	/**
	*	@brief returns false, scripted sequences cannot possess entities regardless of state.
	*/
	virtual bool FCanOverrideState();

	/**
	*	@brief called when a scripted sequence animation sequence is done playing
	*	(or when an AI Scripted Sequence doesn't supply an animation sequence to play).
	*	Expects the CBaseMonster pointer to the monster that the sequence possesses.
	*/
	void SequenceDone(CBaseMonster* pMonster);

	/**
	*	@brief When a monster finishes a scripted sequence,
	*	we have to fix up its state and schedule for it to return to a normal AI monster.
	*	@details Scripted sequences just dirty the Schedule and drop the monster in Idle State.
	*/
	virtual void FixScriptMonsterSchedule(CBaseMonster* pMonster);
	bool CanInterrupt();
	void AllowInterrupt(bool fAllow);
	int IgnoreConditions() override;

	string_t m_iszIdle;	  //!< string index for idle animation
	string_t m_iszPlay;	  //!< string index for scripted animation
	string_t m_iszEntity; //!< entity that is wanted for this script
	int m_fMoveTo;
	int m_iFinishSchedule;
	float m_flRadius; //!< range to search
	float m_flRepeat; //!< repeat rate

	int m_iDelay;
	float m_startTime;

	int m_saved_movetype;
	int m_saved_solid;
	int m_saved_effects;
	//	Vector m_vecOrigOrigin;
	bool m_interruptable;
};

class CCineAI : public CCineMonster
{
	/**
	*	@brief lookup a sequence name and setup the target monster to play it
	*	overridden for CCineAI because it's ok for them to not have an animation sequence for the monster to play.
	*	For a regular Scripted Sequence, that situation is an error.
	*/
	bool StartSequence(CBaseMonster* pTarget, string_t iszSeq, bool completeOnEmpty) override;

	/**
	*	@brief make the entity carry out the scripted sequence instructions, but without destroying the monster's state.
	*/
	void PossessEntity() override;

	/**
	*	@brief returns true because scripted AI can possess entities regardless of their state.
	*/
	bool FCanOverrideState() override;

	/**
	*	@brief When a monster finishes a scripted sequence,
	*	we have to fix up its state and schedule for it to return to a normal AI monster.
	*	@details AI Scripted sequences will, depending on what the level designer selects:
	*	-Dirty the monster's schedule and drop out of the  sequence in their current state.
	*	-Select a specific AMBUSH schedule, regardless of state.
	*/
	void FixScriptMonsterSchedule(CBaseMonster* pMonster) override;
};
