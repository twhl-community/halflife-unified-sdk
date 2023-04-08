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

/**
*	@file
*	Monster-related utility code
*/

// CHECKLOCALMOVE result types
#define LOCALMOVE_INVALID 0					 //!< move is not possible
#define LOCALMOVE_INVALID_DONT_TRIANGULATE 1 //!< move is not possible, don't try to triangulate
#define LOCALMOVE_VALID 2					 //!< move is possible

// Hit Group standards
#define HITGROUP_GENERIC 0
#define HITGROUP_HEAD 1
#define HITGROUP_CHEST 2
#define HITGROUP_STOMACH 3
#define HITGROUP_LEFTARM 4
#define HITGROUP_RIGHTARM 5
#define HITGROUP_LEFTLEG 6
#define HITGROUP_RIGHTLEG 7

// Monster Spawnflags
#define SF_MONSTER_WAIT_TILL_SEEN 1 //!< spawnflag that makes monsters wait until player can see them before attacking.
#define SF_MONSTER_GAG 2			//!< no idle noises from this monster
#define SF_MONSTER_HITMONSTERCLIP 4
//										8
#define SF_MONSTER_PRISONER 16 //!< monster won't attack anyone, no one will attacke him.
//										32
//										64
#define SF_MONSTER_WAIT_FOR_SCRIPT 128 //!< spawnflag that makes monsters wait to check for attacking until the script is done or they've been attacked
#define SF_MONSTER_PREDISASTER 256	   //!< this is a predisaster scientist or barney. Influences how they speak.
#define SF_MONSTER_FADECORPSE 512	   //!< Fade out corpse after death
#define SF_MONSTER_FALL_TO_GROUND 0x80000000

// specialty spawnflags
#define SF_MONSTER_TURRET_AUTOACTIVATE 32
#define SF_MONSTER_TURRET_STARTINACTIVE 64
#define SF_MONSTER_WAIT_UNTIL_PROVOKED 64 //!< don't attack the player unless provoked

// MoveToOrigin stuff
#define MOVE_START_TURN_DIST 64 //!< when this far away from moveGoal, start turning to face next goal
#define MOVE_STUCK_DIST 32		//!< if a monster can't step this far, it is stuck.

// MoveToOrigin stuff
#define MOVE_NORMAL 0 // normal move in the direction monster is facing
#define MOVE_STRAFE 1 // moves in direction specified, no matter which way monster is facing

inline Vector g_vecAttackDir;

/**
 *	@brief Set in combat.cpp.  Used to pass the damage inflictor for death messages.
 *	Better solution:  Add as parameter to all Killed() functions.
 */
inline CBaseEntity* g_pevLastInflictor = nullptr;
inline bool g_fDrawLines = false;

// spawn flags 256 and above are already taken by the engine
void UTIL_MoveToOrigin(edict_t* pent, const Vector& vecGoal, float flDist, int iMoveType);

/**
*	@brief returns the velocity at which an object should be lobbed from vecspot1 to land near vecspot2.
*	@return g_vecZero if toss is not feasible.
*/
Vector VecCheckToss(CBaseEntity* entity, const Vector& vecSpot1, Vector vecSpot2, float flGravityAdj = 1.0);

/**
*	@brief returns the velocity vector at which an object should be thrown from vecspot1 to hit vecspot2.
*	@return g_vecZero if throw is not feasible.
*/
Vector VecCheckThrow(CBaseEntity* entity, const Vector& vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1.0);

/**
*	@brief tosses a brass shell from passed origin at passed velocity
*/
void EjectBrass(const Vector& vecOrigin, const Vector& vecVelocity, float rotation, int model, int soundtype);
void ExplodeModel(const Vector& vecOrigin, float speed, int model, int count);
bool IsFacing(CBaseEntity* pevTest, const Vector& reference);

/**
*	@brief a more accurate ( and slower ) version of FVisible.
*	!!!UNDONE - make this CBaseMonster?
*/
bool FBoxVisible(CBaseEntity* looker, CBaseEntity* target, Vector& vecTargetOrigin, float flSize = 0.0);

void DrawRoute(CBaseEntity* entity, WayPoint_t* m_Route, int m_iRouteIndex, int r, int g, int b);

// monster to monster relationship types
#define R_AL -2 //!< (ALLY) pals. Good alternative to R_NO when applicable.
#define R_FR -1 //!< (FEAR)will run
#define R_NO 0	//!< (NO RELATIONSHIP) disregard
#define R_DL 1	//!< (DISLIKE) will attack
#define R_HT 2	//!< (HATE)will attack this character instead of any visible DISLIKEd characters
#define R_NM 3	//!< (NEMESIS)  A monster Will ALWAYS attack its nemsis, no matter what

// these bits represent the monster's memory
#define MEMORY_CLEAR 0
#define bits_MEMORY_PROVOKED (1 << 0)	   //!< right now only used for houndeyes.
#define bits_MEMORY_INCOVER (1 << 1)	   //!< monster knows it is in a covered position.
#define bits_MEMORY_SUSPICIOUS (1 << 2)	   //!< Ally is suspicious of the player, and will move to provoked more easily
#define bits_MEMORY_PATH_FINISHED (1 << 3) //!< Finished monster path (just used by big momma for now)
#define bits_MEMORY_ON_PATH (1 << 4)	   //!< Moving on a path
#define bits_MEMORY_MOVE_FAILED (1 << 5)   //!< Movement has already failed
#define bits_MEMORY_FLINCHED (1 << 6)	   //!< Has already flinched
#define bits_MEMORY_KILLED (1 << 7)		   //!< HACKHACK -- remember that I've already called my Killed()
#define bits_MEMORY_CUSTOM4 (1 << 28)	   //!< Monster-specific memory
#define bits_MEMORY_CUSTOM3 (1 << 29)	   //!< Monster-specific memory
#define bits_MEMORY_CUSTOM2 (1 << 30)	   //!< Monster-specific memory
#define bits_MEMORY_CUSTOM1 (1 << 31)	   //!< Monster-specific memory

/**
*	@brief trigger conditions for scripted AI
*	these MUST match the CHOICES interface in halflife.fgd for the base monster
*/
enum
{
	AITRIGGER_NONE = 0,
	AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER,
	AITRIGGER_TAKEDAMAGE,
	AITRIGGER_HALFHEALTH,
	AITRIGGER_DEATH,
	AITRIGGER_SQUADMEMBERDIE,
	AITRIGGER_SQUADLEADERDIE,
	AITRIGGER_HEARWORLD,
	AITRIGGER_HEARPLAYER,
	AITRIGGER_HEARCOMBAT,
	AITRIGGER_SEEPLAYER_UNCONDITIONAL,
	AITRIGGER_SEEPLAYER_NOT_IN_COMBAT,
};
/*
		0 : "No Trigger"
		1 : "See Player"
		2 : "Take Damage"
		3 : "50% Health Remaining"
		4 : "Death"
		5 : "Squad Member Dead"
		6 : "Squad Leader Dead"
		7 : "Hear World"
		8 : "Hear Player"
		9 : "Hear Combat"
*/

struct GibLimit
{
	const int MaxGibs;
};

/**
 *	@brief Data used to spawn gibs
 */
struct GibData
{
	const char* const ModelName;
	const int FirstSubModel;
	const int SubModelCount;

	/**
	 *	@brief Optional list of limits to apply to each submodel
	 *	Must be SubModelCount elements large
	 *	If used, instead of randomly selecting a submodel each submodel is used until the requested number of gibs have been spawned
	 */
	const GibLimit* const Limits = nullptr;
};

/**
*	@brief A gib is a chunk of a body, or a piece of wood/metal/rocks/etc.
*/
class CGib : public CBaseEntity
{
	DECLARE_CLASS(CGib, CBaseEntity);
	DECLARE_DATAMAP();

public:
	/**
	*	@brief Throw a chunk
	*/
	void Spawn(const char* szGibModel);

	/**
	*	@brief Gib bounces on the ground or wall, sponges some blood down, too!
	*/
	void BounceGibTouch(CBaseEntity* pOther);

	/**
	*	@brief Sticky gib puts blood on the wall and stays put.
	*/
	void StickyGibTouch(CBaseEntity* pOther);

	/**
	*	@brief in order to emit their meaty scent from the proper location,
	*	gibs should wait until they stop bouncing to emit their scent.
	*	That's what this function does.
	*/
	void WaitTillLand();
	void LimitVelocity();

	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }
	static void SpawnHeadGib(CBaseEntity* victim);
	static void SpawnRandomGibs(CBaseEntity* victim, int cGibs, const GibData& gibData);
	static void SpawnRandomGibs(CBaseEntity* victim, int cGibs, bool human);
	static void SpawnStickyGibs(CBaseEntity* victim, Vector vecOrigin, int cGibs);

	int m_bloodColor;
	int m_cBloodDecals;
	int m_material;
	float m_lifeTime;
};
