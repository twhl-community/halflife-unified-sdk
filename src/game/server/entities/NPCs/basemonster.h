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

#include <span>

#include "CBaseToggle.h"
#include "monsters.h"

/**
 *	@brief Enum namespace
 */
namespace NPCWeaponState
{
/**
 *	@brief Values for NPC weapon equip state.
 */
enum NPCWeaponState
{
	Blank = 0, //!< Not visible at all.
	Holstered, //!< In holster.
	Drawn	   //!< In hand.
};
}

enum class PlayerAllyRelationship : int
{
	Default = 0,
	No,
	Yes
};

#define ROUTE_SIZE 8	  //!< how many waypoints a monster can store at one time
#define MAX_OLD_ENEMIES 4 //!< how many old enemies to remember

struct ScheduleList
{
	const ScheduleList* BaseList{};
	std::span<const Schedule_t*> Schedules;
};

#define DECLARE_CUSTOM_SCHEDULES_COMMON()              \
private:                                               \
	static const ScheduleList m_ScheduleList;          \
													   \
public:                                                \
	static const ScheduleList* GetLocalScheduleList(); \
	static const ScheduleList* GetBaseScheduleList()

#define DECLARE_CUSTOM_SCHEDULES_NOBASE() \
	DECLARE_CUSTOM_SCHEDULES_COMMON();    \
	virtual const ScheduleList* GetScheduleList() const

#define DECLARE_CUSTOM_SCHEDULES()     \
	DECLARE_CUSTOM_SCHEDULES_COMMON(); \
	const ScheduleList* GetScheduleList() const override

/**
 *	@brief Creates a schedule list for type @c ThisClass containing @c schedules
 */
template <typename ThisClass, typename... Args>
ScheduleList CreateScheduleList(Args&&... schedules)
{
	// The cast ensures that only schedule pointers (and arrays decaying to pointers) are accepted.
	static std::array Schedules{static_cast<const Schedule_t*>(schedules)...};

	return ScheduleList{
		.BaseList = ThisClass::GetBaseScheduleList(),
		.Schedules = Schedules};
}

// Overload for creating empty schedule lists.
template <typename ThisClass>
ScheduleList CreateScheduleList()
{
	return ScheduleList{
		.BaseList = ThisClass::GetBaseScheduleList()};
}

#define BEGIN_CUSTOM_SCHEDULES_COMMON(thisClass)           \
	const ScheduleList* thisClass::GetLocalScheduleList()  \
	{                                                      \
		return &m_ScheduleList;                            \
	}                                                      \
	const ScheduleList* thisClass::GetScheduleList() const \
	{                                                      \
		return &m_ScheduleList;                            \
	}                                                      \
const ScheduleList thisClass::m_ScheduleList = CreateScheduleList<thisClass>(

#define BEGIN_CUSTOM_SCHEDULES_NOBASE(thisClass)         \
	const ScheduleList* thisClass::GetBaseScheduleList() \
	{                                                    \
		return nullptr;                                  \
	}                                                    \
	BEGIN_CUSTOM_SCHEDULES_COMMON(thisClass)

#define BEGIN_CUSTOM_SCHEDULES(thisClass)                    \
	const ScheduleList* thisClass::GetBaseScheduleList()     \
	{                                                        \
		return thisClass::BaseClass::GetLocalScheduleList(); \
	}                                                        \
	BEGIN_CUSTOM_SCHEDULES_COMMON(thisClass)

#define END_CUSTOM_SCHEDULES() \
	)

/**
 *	@brief generic Monster
 */
class CBaseMonster : public CBaseToggle
{
	DECLARE_CLASS(CBaseMonster, CBaseToggle);
	DECLARE_DATAMAP();
	DECLARE_CUSTOM_SCHEDULES_NOBASE();

private:
	int m_afConditions;

public:
	static inline std::shared_ptr<spdlog::logger> AILogger;

	enum SCRIPTSTATE
	{
		SCRIPT_PLAYING = 0, //!< Playing the sequence
		SCRIPT_WAIT,		//!< Waiting on everyone in the script to be ready
		SCRIPT_CLEANUP,		//!< Cancelling the script / cleaning up
		SCRIPT_WALK_TO_MARK,
		SCRIPT_RUN_TO_MARK,
	};



	// these fields have been added in the process of reworking the state machine. (sjb)
	EHANDLE m_hEnemy;	  //!< the entity that the monster is fighting.
	EHANDLE m_hTargetEnt; //!< the entity that the monster is trying to reach
	EHANDLE m_hOldEnemy[MAX_OLD_ENEMIES];
	Vector m_vecOldEnemy[MAX_OLD_ENEMIES];

	float m_flFieldOfView;	//!< width of monster's field of view ( dot product )
	float m_flWaitFinished; //!< if we're told to wait, this is the time that the wait will be over.
	float m_flMoveWaitFinished;

	Activity m_Activity;	  //!< what the monster is doing (animation)
	Activity m_IdealActivity; //!< monster should switch to this activity

	int m_LastHitGroup; //!< the last body region that took damage

	MONSTERSTATE m_MonsterState;	  //!< monster's current state
	MONSTERSTATE m_IdealMonsterState; //!< monster should change to this state

	int m_iTaskStatus;
	const Schedule_t* m_pSchedule;
	int m_iScheduleIndex;

	WayPoint_t m_Route[ROUTE_SIZE]; //!< Positions of movement
	int m_movementGoal;				//!< Goal that defines route
	int m_iRouteIndex;				//!< index into m_Route[]
	float m_moveWaitTime;			//!< How long I should wait for something to move

	Vector m_vecMoveGoal;		 //!< kept around for node graph moves, so we know our ultimate goal
	Activity m_movementActivity; //!< When moving, set this activity

	int m_iAudibleList; //!< first index of a linked list of sounds that the monster can hear.
	int m_afSoundTypes;

	Vector m_vecLastPosition; //!< monster sometimes wants to return to where it started after an operation.

	int m_iHintNode; //!< this is the hint node that the monster is moving towards or performing active idle on.

	int m_afMemory;

	int m_iMaxHealth; //!< keeps track of monster's maximum health value (for re-healing, etc)

	Vector m_vecEnemyLKP; //!< last known position of enemy. (enemy's origin)

	int m_cAmmoLoaded; //!< how much ammo is in the weapon (used to trigger reload anim sequences)

	int m_afCapability; //!< tells us what a monster can/can't do.

	float m_flNextAttack; //!< cannot attack again until this time

	int m_bitsDamageType; //!< what types of damage has monster (player) taken
	byte m_rgbTimeBasedDamage[CDMG_TIMEBASED];

	/**
	 *	@brief how much damage did monster (player) last take time based damage counters, decr. 1 per 2 seconds
	 */
	int m_lastDamageAmount;
	int m_bloodColor; //!< color of blood particless

	int m_failSchedule; //!< Schedule type to choose if current schedule fails

	float m_flHungryTime; //!< set this is a future time to stop the monster from eating for a while.

	float m_flDistTooFar; //!< if enemy farther away than this, bits_COND_ENEMY_TOOFAR set in CheckEnemy
	float m_flDistLook;	  //!< distance monster sees (Default 2048)

	int m_iTriggerCondition;	 //!< for scripted AI, this is the condition that will cause the activation of the monster's TriggerTarget
	string_t m_iszTriggerTarget; //!< name of target that should be fired.

	PlayerAllyRelationship m_PlayerAllyRelationship = PlayerAllyRelationship::Default;

	Vector m_HackedGunPos; //!< HACK until we can query end of gun

	// Scripted sequence Info
	SCRIPTSTATE m_scriptState; //!< internal cinematic state
	CCineMonster* m_pCine;

	float m_flLastYawTime;

	bool m_AllowItemDropping = true;

	int ObjectCaps() override
	{
		int caps = BaseClass::ObjectCaps();

		if (m_AllowFollow)
		{
			caps |= FCAP_IMPULSE_USE;
		}

		return caps;
	}

	void PostRestore() override;

	/**
	 *	@brief !!! netname entvar field is used in squadmonster for groupname!!!
	 */
	bool KeyValue(KeyValueData* pkvd) override;

	/**
	 *	@brief will make a monster angry at whomever activated it.
	 */
	void MonsterUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	// overrideable Monster member functions

	int BloodColor() override { return m_bloodColor; }

	CBaseMonster* MyMonsterPointer() override { return this; }

	/**
	 *	@brief Base class monster function to find enemies or food by sight.
	 *	@details Sets the sight bits of the m_afConditions mask to indicate which types of entities were sighted.
	 *	Function also sets the Looker's m_pLink to the head of a link list that contains all visible ents.
	 *	(linked via each ent's m_pLink field)
	 *	@param iDistance distance ( in units ) that the monster can see.
	 */
	virtual void Look(int iDistance); //!< basic sight function for monsters
	virtual void RunAI();			  //!< core ai function!

	/**
	 *	@brief monsters dig through the active sound list for any sounds that may interest them. (smells, too!)
	 */
	void Listen();

	bool IsAlive() override { return (pev->deadflag != DEAD_DEAD); }
	virtual bool ShouldFadeOnDeath();

	// Basic Monster AI functions
	/**
	 *	@brief turns a monster towards its ideal_yaw
	 */
	virtual float ChangeYaw(int speed);

	/**
	 *	@brief turns a directional vector into a yaw value that points down that vector.
	 */
	float VecToYaw(Vector vecDir);

	/**
	 *	@brief returns the difference (in degrees) between monster's current yaw and ideal_yaw
	 *	Positive result is left turn, negative is right turn
	 */
	float FlYawDiff();

	float DamageForce(float damage);

	// stuff written for new state machine
	/**
	 *	@brief calls out to core AI functions and handles this monster's specific animation events
	 */
	virtual void MonsterThink();
	void CallMonsterThink() { this->MonsterThink(); }

	/**
	 *	@brief returns an integer that describes the relationship between two types of monster.
	 */
	virtual Relationship IRelationship(CBaseEntity* pTarget);

	/**
	 *	@brief after a monster is spawned, it needs to be dropped into the world, checked for mobility problems,
	 *	and put on the proper path, if any. This function does all of those things after the monster spawns.
	 *	Any initialization that should take place for all monsters goes here.
	 */
	virtual void MonsterInit();

	/**
	 *	@brief Call after animation/pose is set up
	 */
	virtual void MonsterInitDead();

	virtual void BecomeDead();
	void CorpseFallThink();

	/**
	 *	@brief Calls StartMonster. Startmonster is virtual, but this function cannot be
	 */
	void MonsterInitThink();

	/**
	 *	@brief final bit of initization before a monster is turned over to the AI.
	 */
	virtual void StartMonster();

	/**
	 *	@brief finds best visible enemy for attack
	 *	@details this functions searches the link list whose head is the caller's m_pLink field,
	 *	and returns a pointer to the enemy entity in that list that is nearest the caller.
	 */
	virtual CBaseEntity* BestVisibleEnemy();

	/**
	 *	@brief returns true is the passed ent is in the caller's forward view cone.
	 *	The dot product is performed in 2d, making the view cone infinitely tall.
	 */
	virtual bool FInViewCone(CBaseEntity* pEntity);

	/**
	 *	@brief returns true is the passed vector is in the caller's forward view cone.
	 *	The dot product is performed in 2d, making the view cone infinitely tall.
	 */
	virtual bool FInViewCone(Vector* pOrigin);

	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	/**
	 *	@brief check validity of a straight move through space
	 *	returns true if the caller can walk a straight line from its current origin to the given location.
	 *	If so, don't use the node graph!
	 *	@details if a valid pointer to a int is passed,
	 *	the function will fill that int with the distance that the check reached before hitting something.
	 *	THIS ONLY HAPPENS IF THE LOCAL MOVE CHECK FAILS!
	 */
	virtual int CheckLocalMove(const Vector& vecStart, const Vector& vecEnd, CBaseEntity* pTarget, float* pflDist);

	/**
	 *	@brief take a single step towards the next ROUTE location
	 */
	virtual void Move(float flInterval = 0.1);
	virtual void MoveExecute(CBaseEntity* pTargetEnt, const Vector& vecDir, float flInterval);
	virtual bool ShouldAdvanceRoute(float flWaypointDist);

	virtual Activity GetStoppedActivity() { return ACT_IDLE; }
	virtual void Stop() { m_IdealActivity = GetStoppedActivity(); }

	/**
	 *	@brief This will stop animation until you call ResetSequenceInfo() at some point in the future
	 */
	inline void StopAnimation() { pev->framerate = 0; }

	// these functions will survey conditions and set appropriate conditions bits for attack types.
	// flDot is the cos of the angle of the cone within which the attack can occur.
	virtual bool CheckRangeAttack1(float flDot, float flDist);
	virtual bool CheckRangeAttack2(float flDot, float flDist);
	virtual bool CheckMeleeAttack1(float flDot, float flDist);
	virtual bool CheckMeleeAttack2(float flDot, float flDist);

	/**
	 *	@brief Returns true if monster's m_pSchedule is anything other than nullptr.
	 */
	bool FHaveSchedule();

	/**
	 *	@brief returns true as long as the current schedule is still the proper schedule to be executing,
	 *	taking into account all conditions
	 */
	bool FScheduleValid();

	/**
	 *	@brief blanks out the caller's schedule pointer and index.
	 */
	void ClearSchedule();

	/**
	 *	@brief Returns true if the caller is on the last task in the schedule
	 */
	bool FScheduleDone();

	/**
	 *	@brief replaces the monster's schedule pointer with the passed pointer, and sets the ScheduleIndex back to 0
	 */
	void ChangeSchedule(const Schedule_t* pNewSchedule);

	/**
	 *	@brief increments the ScheduleIndex
	 */
	void NextScheduledTask();

	const Schedule_t* ScheduleFromName(const char* pName) const;

	/**
	 *	@brief does all the per-think schedule maintenance.
	 *	ensures that the monster leaves this function with a valid schedule!
	 */
	void MaintainSchedule();

	/**
	 *	@brief selects the correct activity and performs any necessary calculations to start the next task on the schedule.
	 */
	virtual void StartTask(const Task_t* pTask);
	virtual void RunTask(const Task_t* pTask);

	/**
	 *	@brief returns a pointer to one of the monster's available schedules of the indicated type.
	 */
	virtual const Schedule_t* GetScheduleOfType(int Type);

	/**
	 *	@brief Decides which type of schedule best suits the monster's current state and conditions.
	 *	Then calls monster's member function to get a pointer to a schedule of the proper type.
	 */
	virtual const Schedule_t* GetSchedule();
	virtual void ScheduleChange() {}

	/*
	virtual bool CanPlaySequence()
	{
		return ((m_pCine == nullptr) &&
			(m_MonsterState == MONSTERSTATE_NONE ||
				m_MonsterState == MONSTERSTATE_IDLE ||
				m_IdealMonsterState == MONSTERSTATE_IDLE));
	}
	*/

	/**
	 *	@brief determines whether or not the monster can play the scripted sequence or AI sequence
	 *	that is trying to possess it.
	 *	@param fDisregardState If set, the monster will be sucked into the script no matter what state it is in.
	 *		ONLY Scripted AI ents should allow this.
	 *	@param interruptLevel Interrupt level of the current sequence play request
	 *		@see SS_INTERRUPT
	 */
	virtual bool CanPlaySequence(bool fDisregardState, int interruptLevel);

	virtual bool CanPlaySentence(bool fDisregardState)
	{
		return IsAlive() && (m_MonsterState == MONSTERSTATE_SCRIPT || pev->deadflag == DEAD_NO);
	}

	void PlaySentence(const char* pszSentence, float duration, float volume, float attenuation);

protected:
	virtual void PlaySentenceCore(const char* pszSentence, float duration, float volume, float attenuation);

public:
	virtual void PlayScriptedSentence(
		const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener);

	virtual void SentenceStop();

	/**
	 *	@brief returns a pointer to the current scheduled task. nullptr if there's a problem.
	 */
	const Task_t* GetTask();

	/**
	 *	@brief surveys the Conditions information available and finds the best new state for a monster.
	 */
	virtual MONSTERSTATE GetIdealState();

	virtual void SetActivity(Activity NewActivity);
	void SetSequenceByName(const char* szSequence);
	void SetState(MONSTERSTATE State);
	virtual void ReportAIState();

	/**
	 *	@brief sets all of the bits for attacks that the monster is capable of carrying out on the passed entity.
	 */
	void CheckAttacks(CBaseEntity* pTarget, float flDist);

	/**
	 *	@brief part of the Condition collection process,
	 *	gets and stores data and conditions pertaining to a monster's enemy.
	 *	Returns true if Enemy LKP was updated.
	 */
	virtual bool CheckEnemy(CBaseEntity* pEnemy);

	/**
	 *	@brief remember the last few enemies, always remember the player
	 */
	void PushEnemy(CBaseEntity* pEnemy, Vector& vecLastKnownPos);

	/**
	 *	@brief try remembering the last few enemies
	 */
	bool PopEnemy();

	/**
	 *	@brief tries to build an entire node path from the callers origin to the passed vector.
	 *	If this is possible, ROUTE_SIZE waypoints will be copied into the callers m_Route.
	 *	@return true if the operation succeeds (path is valid) or false if failed (no path exists)
	 */
	bool FGetNodeRoute(Vector vecDest);

	inline void TaskComplete()
	{
		if (!HasConditions(bits_COND_TASK_FAILED))
			m_iTaskStatus = TASKSTATUS_COMPLETE;
	}
	void MovementComplete();
	inline void TaskFail() { SetConditions(bits_COND_TASK_FAILED); }
	inline void TaskBegin() { m_iTaskStatus = TASKSTATUS_RUNNING; }
	bool TaskIsRunning();
	inline bool TaskIsComplete() { return (m_iTaskStatus == TASKSTATUS_COMPLETE); }
	inline bool MovementIsComplete() { return (m_movementGoal == MOVEGOAL_NONE); }

	/**
	 *	@brief returns an integer with all Conditions bits that are currently set and
	 *	also set in the current schedule's Interrupt mask.
	 */
	int IScheduleFlags();

	/**
	 *	@brief after calculating a path to the monster's target,
	 *	this function copies as many waypoints as possible from that path to the monster's Route array
	 */
	bool FRefreshRoute();

	/**
	 *	@brief returns true is the Route is cleared out (invalid)
	 */
	bool FRouteClear();

	/**
	 *	@brief Attempts to make the route more direct by cutting out unnecessary nodes & cutting corners.
	 */
	void RouteSimplify(CBaseEntity* pTargetEnt);

	/**
	 *	@brief poorly named function that advances the m_iRouteIndex.
	 *	If it goes beyond ROUTE_SIZE, the route is refreshed.
	 */
	void AdvanceRoute(float distance);

	/**
	 *	@brief tries to overcome local obstacles by triangulating a path around them.
	 *	@param vecStart Path start point.
	 *	@param vecEnd Path end point.
	 *	@param flDist Distance to move in current movement operation.
	 *	@param pTargetEnt Optional. Entity we've moving towards.
	 *	@param pApex How far the obstruction that we are trying to triangulate around is from the monster.
	 */
	virtual bool FTriangulate(const Vector& vecStart, const Vector& vecEnd, float flDist, CBaseEntity* pTargetEnt, Vector* pApex);

	/**
	 *	@brief gets a yaw value for the caller that would face the supplied vector.
	 *	Value is stuffed into the monster's ideal_yaw
	 */
	void MakeIdealYaw(Vector vecTarget);

	/**
	 *	@brief allows different yaw_speeds for each activity
	 */
	virtual void SetYawSpeed() {}

	bool BuildRoute(const Vector& vecGoal, int iMoveFlag, CBaseEntity* pTarget);

	/**
	 *	@brief tries to build a route as close to the target as possible, even if there isn't a path to the final point.
	 *	@details If supplied, search will return a node at least as far away as MinDist from vecThreat,
	 *	but no farther than MaxDist.
	 *	if MaxDist isn't supplied, it defaults to a reasonable value
	 */
	virtual bool BuildNearestRoute(Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist);
	int RouteClassify(int iMoveFlag);

	/**
	 *	@brief Rebuilds the existing route so that the supplied vector and moveflags are the first waypoint in the route,
	 *	and fills the rest of the route with as much of the pre-existing route as possible
	 */
	void InsertWaypoint(Vector vecLocation, int afMoveFlags);

	/**
	 *	@brief attempts to locate a spot in the world directly to the left or right of the caller
	 *	that will conceal them from view of pSightEnt
	 */
	bool FindLateralCover(const Vector& vecThreat, const Vector& vecViewOffset);

	/**
	 *	@brief tries to find a nearby node that will hide the caller from its enemy.
	 *	@details If supplied, search will return a node at least as far away as MinDist, but no farther than MaxDist.
	 *	if MaxDist isn't supplied, it defaults to a reasonable value
	 */
	virtual bool FindCover(Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist);
	virtual bool FValidateCover(const Vector& vecCoverLocation) { return true; }
	virtual float CoverRadius() { return 784; } //!< Default cover radius

	/**
	 *	@brief prequalifies a monster to do more fine checking of potential attacks.
	 */
	virtual bool FCanCheckAttacks();
	virtual void CheckAmmo() {}

	/**
	 *	@brief before a set of conditions is allowed to interrupt a monster's schedule,
	 *	this function removes conditions that we have flagged to interrupt the current schedule,
	 *	but may not want to interrupt the schedule every time. (Pain, for instance)
	 */
	virtual int IgnoreConditions();

	inline void SetConditions(int iConditions) { m_afConditions |= iConditions; }
	inline void ClearConditions(int iConditions) { m_afConditions &= ~iConditions; }
	inline bool HasConditions(int iConditions)
	{
		if (m_afConditions & iConditions)
			return true;
		return false;
	}
	inline bool HasAllConditions(int iConditions)
	{
		if ((m_afConditions & iConditions) == iConditions)
			return true;
		return false;
	}

	/**
	 *	@brief tells use whether or not the monster cares about the type of Hint Node given
	 */
	virtual bool FValidateHintType(short sHint);
	int FindHintNode();
	virtual bool FCanActiveIdle();

	/**
	 *	@brief measures the difference between the way the monster is facing and determines
	 *	whether or not to select one of the 180 turn animations.
	 */
	void SetTurnActivity();

	/**
	 *	@brief subtracts the volume of the given sound from the distance the sound source is from the caller,
	 *	and returns that value, which is considered to be the 'local' volume of the sound.
	 */
	float FLSoundVolume(CSound* pSound);

	bool MoveToNode(Activity movementAct, float waitTime, const Vector& goal);
	bool MoveToTarget(Activity movementAct, float waitTime);
	bool MoveToLocation(Activity movementAct, float waitTime, const Vector& goal);
	bool MoveToEnemy(Activity movementAct, float waitTime);

	/**
	 *	@brief Returns the time when the door will be open
	 */
	float OpenDoorAndWait(CBaseEntity* door);

	/**
	 *	@brief returns a bit mask indicating which types of sounds this monster regards.
	 *	In the base class implementation, monsters care about all sounds, but no scents.
	 */
	virtual int ISoundMask();

	/**
	 *	@brief returns a pointer to the sound the monster should react to. Right now responds only to nearest sound.
	 */
	virtual CSound* PBestSound();

	/**
	 *	@brief returns a pointer to the scent the monster should react to. Right now responds only to nearest scent.
	 */
	virtual CSound* PBestScent();

	virtual float HearingSensitivity() { return 1.0; }

	/**
	 *	@brief tries to send a monster into PRONE state.
	 *	right now only used when a barnacle snatches someone, so may have some special case stuff for that.
	 */
	bool FBecomeProne() override;

	/**
	 *	@brief called by Barnacle victims when the barnacle pulls their head into its mouth
	 */
	virtual void BarnacleVictimBitten(CBaseEntity* pevBarnacle);

	/**
	 *	@brief called by barnacle victims when the host barnacle is killed.
	 */
	virtual void BarnacleVictimReleased();

	/**
	 *	@brief queries the monster's model for $eyeposition and copies that vector to the monster's view_ofs
	 */
	void SetEyePosition();

	bool FShouldEat();				//!< see if a monster is 'hungry'
	void Eat(float flFullDuration); //!< make the monster 'full' for a while.

	/**
	 *	@brief expects a length to trace, amount of damage to do, and damage type.
	 *	Returns a pointer to the damaged entity in case the monster wishes to do other stuff to the victim (punchangle, etc)
	 *	Used for many contact-range melee attacks. Bites, claws, etc.
	 */
	CBaseEntity* CheckTraceHullAttack(float flDist, int iDamage, int iDmgType);

	/**
	 *	@brief tells us if a monster is facing its ideal yaw.
	 *	Created this function because many spots in the code were checking the yawdiff against this magic number.
	 *	Nicer to have it in one place if we're gonna be stuck with it.
	 */
	bool FacingIdeal();

	/**
	 *	@brief checks the monster's AI Trigger Conditions, if there is a condition, then checks to see if condition is met.
	 *	If yes, the monster's TriggerTarget is fired.
	 *	@return true if the target is fired.
	 */
	bool FCheckAITrigger();

	/**
	 *	@brief check to see if the monster's bounding box is lying flat on a surface
	 *	(traces from all four corners are same length.)
	 */
	bool BBoxFlat();

	/**
	 *	@brief this function runs after conditions are collected and before scheduling code is run.
	 */
	virtual void PrescheduleThink() {}

	/**
	 *	@brief tries to find the best suitable enemy for the monster.
	 */
	bool GetEnemy();

	void MakeDamageBloodDecal(int cCount, float flNoise, TraceResult* ptr, const Vector& vecDir);
	void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	// combat functions
	/**
	 *	@brief determines the best type of death anim to play.
	 */
	virtual Activity GetDeathActivity();

	/**
	 *	@brief determines the best type of flinch anim to play.
	 */
	Activity GetSmallFlinchActivity();
	void Killed(CBaseEntity* attacker, int iGib) override;

	/**
	 *	@brief create some gore and get rid of a monster's model.
	 */
	virtual void GibMonster();

	bool ShouldGibMonster(int iGib);
	void CallGibMonster();
	virtual bool HasHumanGibs() { return false; }
	virtual bool HasAlienGibs() { return false; }
	virtual void FadeMonster(); //!< Called instead of GibMonster() when gibs are disabled

	/**
	 *	@brief Is this some kind of biological weapon?
	 */
	virtual bool IsBioWeapon() const { return false; }

	Vector ShootAtEnemy(const Vector& shootOrigin);

	/**
	 *	@brief position to shoot at
	 */
	Vector BodyTarget(const Vector& posSrc) override { return Center() * 0.75 + EyePosition() * 0.25; }

	virtual Vector GetGunPosition();

	bool GiveHealth(float flHealth, int bitsDamageType) override;

	/**
	 *	@copydoc CBaseEntity::TakeDamage()
	 *	@details Time-based damage: only occurs while the monster is within the trigger_hurt.
	 *	When a monster is poisoned via an arrow etc it takes all the poison damage at once.
	 */
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	/**
	 *	@brief takedamage function called when a monster's corpse is damaged.
	 */
	bool DeadTakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType);

	void RadiusDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage,
		int bitsDamageType, EntityClassification iClassIgnore = ENTCLASS_NONE);
	void RadiusDamage(Vector vecSrc, CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage,
		int bitsDamageType, EntityClassification iClassIgnore = ENTCLASS_NONE);
	bool IsMoving() override { return m_movementGoal != MOVEGOAL_NONE; }

	/**
	 *	@brief zeroes out the monster's route array and goal
	 */
	void RouteClear();

	/**
	 *	@brief clears out a route to be changed, but keeps goal intact.
	 */
	void RouteNew();

	virtual void DeathSound() {}
	virtual void AlertSound() {}
	virtual void IdleSound() {}
	virtual void PainSound() {}

	inline void Remember(int iMemory) { m_afMemory |= iMemory; }
	inline void Forget(int iMemory) { m_afMemory &= ~iMemory; }
	inline bool HasMemory(int iMemory)
	{
		if (m_afMemory & iMemory)
			return true;
		return false;
	}
	inline bool HasAllMemories(int iMemory)
	{
		if ((m_afMemory & iMemory) == iMemory)
			return true;
		return false;
	}

	bool ExitScriptedSequence();
	bool CineCleanup();

	/**
	 *	@brief Drop an item.
	 *	Will return @c nullptr if item dropping is disabled for this NPC.
	 */
	CBaseEntity* DropItem(const char* pszItemName, const Vector& vecPos, const Vector& vecAng);

	bool JumpToTarget(Activity movementAct, float waitTime);

	// Shock rifle shock effect
	float m_flShockDuration = 0;
	float m_flShockTime = 0;
	int m_iOldRenderMode = 0;
	int m_iOldRenderFX = 0;
	Vector m_OldRenderColor;
	float m_flOldRenderAmt = 0;
	bool m_fShockEffect = false;

	void AddShockEffect(float r, float g, float b, float size, float flShockDuration);
	void UpdateShockEffect();
	void ClearShockEffect();

	/**
	 *	@brief Invokes @c callback on each friend
	 *	@details Return false to stop iteration
	 */
	template <typename Callback>
	void ForEachFriend(Callback callback);

	template <typename Callback>
	void EnumFriends(Callback callback, bool trace);

	// For following
	bool m_AllowFollow = true;
	float m_useTime;	 //!< Don't allow +USE until this time
	string_t m_iszUse;	 //!< +USE sentence group (follow)
	string_t m_iszUnUse; //!< +USE sentence group (stop following)

	bool CanFollow();
	bool IsFollowing() { return m_hTargetEnt != nullptr && m_hTargetEnt->IsPlayer(); }
	virtual int GetMaxFollowers() const { return 1; } // TODO: inconsistent
	virtual void StopFollowing(bool clearSchedule);
	virtual void StartFollowing(CBaseEntity* pLeader);
	virtual void DeclineFollowing() {}
	void LimitFollowers(CBaseEntity* pPlayer, int maxFollowers);

	void FollowerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
};

template <typename Callback>
void CBaseMonster::ForEachFriend(Callback callback)
{
	// First pass: check for other NPCs of the same classification.
	// This typically includes NPCs with the same entity class, but a modder may implement custom classifications.
	const auto myClass = Classify();

	// Can't have friends if you identify as a prop.
	if (myClass == ENTCLASS_NONE)
	{
		return;
	}

	for (auto friendEntity : UTIL_FindEntities())
	{
		if (friendEntity->IsPlayer())
		{
			// No players
			continue;
		}

		auto monster = friendEntity->MyMonsterPointer();

		if (!monster)
		{
			// Not a monster, can't be a friend.
			continue;
		}

		if (myClass != monster->Classify())
		{
			continue;
		}

		callback(monster);
	}

	// Second pass: check for other NPCs that are friendly to us
	for (auto friendEntity : UTIL_FindEntities())
	{
		if (friendEntity->IsPlayer())
		{
			// No players
			continue;
		}

		auto monster = friendEntity->MyMonsterPointer();

		if (!monster)
		{
			// Not a monster, can't be a friend.
			continue;
		}

		const auto otherClass = monster->Classify();

		if (otherClass == ENTCLASS_NONE)
		{
			// Don't consider props to be friends.
			continue;
		}

		if (myClass == otherClass)
		{
			// Already checked these. Includes other NPCs of my type
			continue;
		}

		const auto relationship = IRelationship(monster);

		if (relationship != Relationship::Ally && relationship != Relationship::None)
		{
			// Not a friend
			continue;
		}

		callback(monster);
	}
}

template <typename Callback>
void CBaseMonster::EnumFriends(Callback callback, bool trace)
{
	auto wrapper = [&](CBaseMonster* pFriend)
	{
		if (pFriend == this || !pFriend->IsAlive() || !(pFriend->pev->flags & FL_MONSTER))
		{
			// don't talk to self or dead people or non-monster entities
			return;
		}

		if (trace)
		{
			Vector vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;

			TraceResult tr;
			UTIL_TraceLine(pev->origin, vecCheck, ignore_monsters, edict(), &tr);

			if (tr.flFraction != 1.0)
			{
				return;
			}
		}

		callback(pFriend);
	};

	ForEachFriend(wrapper);
}
