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

/**
 *	@file
 *	Monster-related utility code
 */

#include <EASTL/fixed_string.h>

#include "cbase.h"
#include "nodes.h"
#include "scripted.h"
#include "squadmonster.h"

#define MONSTER_CUT_CORNER_DIST 8 //!< 8 means the monster's bounding box is contained without the box of the node in WC

// UNDONE: Save schedule data?  Can this be done?  We may
// lose our enemy pointer or other data (goal ent, target, etc)
// that make the current schedule invalid, perhaps it's best
// to just pick a new one when we start up again.
BEGIN_DATAMAP(CBaseMonster)
DEFINE_FIELD(m_hEnemy, FIELD_EHANDLE),
	DEFINE_FIELD(m_hTargetEnt, FIELD_EHANDLE),
	DEFINE_ARRAY(m_hOldEnemy, FIELD_EHANDLE, MAX_OLD_ENEMIES),
	DEFINE_ARRAY(m_vecOldEnemy, FIELD_POSITION_VECTOR, MAX_OLD_ENEMIES),
	DEFINE_FIELD(m_flFieldOfView, FIELD_FLOAT),
	DEFINE_FIELD(m_flWaitFinished, FIELD_TIME),
	DEFINE_FIELD(m_flMoveWaitFinished, FIELD_TIME),

	DEFINE_FIELD(m_Activity, FIELD_INTEGER),
	DEFINE_FIELD(m_IdealActivity, FIELD_INTEGER),
	DEFINE_FIELD(m_LastHitGroup, FIELD_INTEGER),
	DEFINE_FIELD(m_MonsterState, FIELD_INTEGER),
	DEFINE_FIELD(m_IdealMonsterState, FIELD_INTEGER),
	DEFINE_FIELD(m_iTaskStatus, FIELD_INTEGER),

	// Schedule_t			*m_pSchedule;

	DEFINE_FIELD(m_iScheduleIndex, FIELD_INTEGER),
	DEFINE_FIELD(m_afConditions, FIELD_INTEGER),
	// WayPoint_t			m_Route[ ROUTE_SIZE ];
	//	DEFINE_FIELD(m_movementGoal, FIELD_INTEGER),
	//	DEFINE_FIELD(m_iRouteIndex, FIELD_INTEGER),
	//	DEFINE_FIELD(m_moveWaitTime, FIELD_FLOAT),

	DEFINE_FIELD(m_vecMoveGoal, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_movementActivity, FIELD_INTEGER),

	//		int					m_iAudibleList; // first index of a linked list of sounds that the monster can hear.
	//	DEFINE_FIELD(m_afSoundTypes, FIELD_INTEGER),
	DEFINE_FIELD(m_vecLastPosition, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_iHintNode, FIELD_INTEGER),
	DEFINE_FIELD(m_afMemory, FIELD_INTEGER),
	DEFINE_FIELD(m_iMaxHealth, FIELD_INTEGER),

	DEFINE_FIELD(m_vecEnemyLKP, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_cAmmoLoaded, FIELD_INTEGER),
	DEFINE_FIELD(m_afCapability, FIELD_INTEGER),

	DEFINE_FIELD(m_flNextAttack, FIELD_TIME),
	DEFINE_FIELD(m_bitsDamageType, FIELD_INTEGER),
	DEFINE_ARRAY(m_rgbTimeBasedDamage, FIELD_CHARACTER, CDMG_TIMEBASED),
	DEFINE_FIELD(m_bloodColor, FIELD_INTEGER),
	DEFINE_FIELD(m_failSchedule, FIELD_INTEGER),

	DEFINE_FIELD(m_flHungryTime, FIELD_TIME),
	DEFINE_FIELD(m_flDistTooFar, FIELD_FLOAT),
	DEFINE_FIELD(m_flDistLook, FIELD_FLOAT),
	DEFINE_FIELD(m_iTriggerCondition, FIELD_INTEGER),
	DEFINE_FIELD(m_iszTriggerTarget, FIELD_STRING),
	DEFINE_FIELD(m_PlayerAllyRelationship, FIELD_INTEGER),

	DEFINE_FIELD(m_HackedGunPos, FIELD_VECTOR),

	DEFINE_FIELD(m_scriptState, FIELD_INTEGER),
	DEFINE_FIELD(m_pCine, FIELD_CLASSPTR),
	DEFINE_FIELD(m_AllowItemDropping, FIELD_BOOLEAN),
	DEFINE_FIELD(m_AllowFollow, FIELD_BOOLEAN),

	DEFINE_FUNCTION(MonsterUse),
	DEFINE_FUNCTION(CallMonsterThink),
	DEFINE_FUNCTION(CorpseFallThink),
	DEFINE_FUNCTION(MonsterInitThink),
	DEFINE_FUNCTION(FollowerUse),
	END_DATAMAP();

void CBaseMonster::PostRestore()
{
	BaseClass::PostRestore();

	// We don't save/restore routes yet
	RouteClear();

	// We don't save/restore schedules yet
	m_pSchedule = nullptr;
	m_iTaskStatus = TASKSTATUS_NEW;

	// Reset animation
	m_Activity = ACT_RESET;

	// If we don't have an enemy, clear conditions like see enemy, etc.
	if (m_hEnemy == nullptr)
		m_afConditions = 0;
}

void CBaseMonster::Eat(float flFullDuration)
{
	m_flHungryTime = gpGlobals->time + flFullDuration;
}

bool CBaseMonster::FShouldEat()
{
	if (m_flHungryTime > gpGlobals->time)
	{
		return false;
	}

	return true;
}

void CBaseMonster::BarnacleVictimBitten(CBaseEntity* pevBarnacle)
{
	const Schedule_t* pNewSchedule = GetScheduleOfType(SCHED_BARNACLE_VICTIM_CHOMP);

	if (pNewSchedule)
	{
		ChangeSchedule(pNewSchedule);
	}
}

void CBaseMonster::BarnacleVictimReleased()
{
	m_IdealMonsterState = MONSTERSTATE_IDLE;
	m_IdealActivity = ACT_RESET; // Force a new activity to be selected so monsters don't remain in the grab state.

	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_STEP;
}

void CBaseMonster::Listen()
{
	int iSound;
	int iMySounds;
	float hearingSensitivity;
	CSound* pCurrentSound;

	m_iAudibleList = SOUNDLIST_EMPTY;
	ClearConditions(bits_COND_HEAR_SOUND | bits_COND_SMELL | bits_COND_SMELL_FOOD);
	m_afSoundTypes = 0;

	iMySounds = ISoundMask();

	if (m_pSchedule)
	{
		//!!!WATCH THIS SPOT IF YOU ARE HAVING SOUND RELATED BUGS!
		// Make sure your schedule AND personal sound masks agree!
		iMySounds &= m_pSchedule->iSoundMask;
	}

	iSound = CSoundEnt::ActiveList();

	// UNDONE: Clear these here?
	ClearConditions(bits_COND_HEAR_SOUND | bits_COND_SMELL_FOOD | bits_COND_SMELL);
	hearingSensitivity = HearingSensitivity();

	while (iSound != SOUNDLIST_EMPTY)
	{
		pCurrentSound = CSoundEnt::SoundPointerForIndex(iSound);

		if (nullptr != pCurrentSound &&
			(pCurrentSound->m_iType & iMySounds) != 0 &&
			(pCurrentSound->m_vecOrigin - EarPosition()).Length() <= pCurrentSound->m_iVolume * hearingSensitivity)

		// if ( ( g_pSoundEnt->m_SoundPool[ iSound ].m_iType & iMySounds ) && ( g_pSoundEnt->m_SoundPool[ iSound ].m_vecOrigin - EarPosition()).Length () <= g_pSoundEnt->m_SoundPool[ iSound ].m_iVolume * hearingSensitivity )
		{
			// the monster cares about this sound, and it's close enough to hear.
			// g_pSoundEnt->m_SoundPool[ iSound ].m_iNextAudible = m_iAudibleList;
			pCurrentSound->m_iNextAudible = m_iAudibleList;

			if (pCurrentSound->FIsSound())
			{
				// this is an audible sound.
				SetConditions(bits_COND_HEAR_SOUND);
			}
			else
			{
				// if not a sound, must be a smell - determine if it's just a scent, or if it's a food scent
				//				if ( g_pSoundEnt->m_SoundPool[ iSound ].m_iType & ( bits_SOUND_MEAT | bits_SOUND_CARCASS ) )
				if ((pCurrentSound->m_iType & (bits_SOUND_MEAT | bits_SOUND_CARCASS)) != 0)
				{
					// the detected scent is a food item, so set both conditions.
					// !!!BUGBUG - maybe a virtual function to determine whether or not the scent is food?
					SetConditions(bits_COND_SMELL_FOOD);
					SetConditions(bits_COND_SMELL);
				}
				else
				{
					// just a normal scent.
					SetConditions(bits_COND_SMELL);
				}
			}

			//			m_afSoundTypes |= g_pSoundEnt->m_SoundPool[ iSound ].m_iType;
			m_afSoundTypes |= pCurrentSound->m_iType;

			m_iAudibleList = iSound;
		}

		//		iSound = g_pSoundEnt->m_SoundPool[ iSound ].m_iNext;
		iSound = pCurrentSound->m_iNext;
	}
}

float CBaseMonster::FLSoundVolume(CSound* pSound)
{
	return (pSound->m_iVolume - ((pSound->m_vecOrigin - pev->origin).Length()));
}

bool CBaseMonster::FValidateHintType(short sHint)
{
	return false;
}

void CBaseMonster::Look(int iDistance)
{
	int iSighted = 0;

	// DON'T let visibility information from last frame sit around!
	ClearConditions(bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR | bits_COND_SEE_NEMESIS | bits_COND_SEE_CLIENT);

	m_pLink = nullptr;

	CBaseEntity* pSightEnt = nullptr; // the current visible entity that we're dealing with

	// See no evil if prisoner is set
	if (!FBitSet(pev->spawnflags, SF_MONSTER_PRISONER))
	{
		CBaseEntity* pList[100];

		Vector delta = Vector(iDistance, iDistance, iDistance);

		// Find only monsters/clients in box, NOT limited to PVS
		int count = UTIL_EntitiesInBox(pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT | FL_MONSTER);
		for (int i = 0; i < count; i++)
		{
			pSightEnt = pList[i];
			// !!!temporarily only considering other monsters and clients, don't see prisoners
			if (pSightEnt != this &&
				!FBitSet(pSightEnt->pev->spawnflags, SF_MONSTER_PRISONER) &&
				pSightEnt->pev->health > 0)
			{
				// the looker will want to consider this entity
				// don't check anything else about an entity that can't be seen, or an entity that you don't care about.
				if (IRelationship(pSightEnt) != Relationship::None &&
					FInViewCone(pSightEnt) &&
					!FBitSet(pSightEnt->pev->flags, FL_NOTARGET) &&
					FVisible(pSightEnt))
				{
					if (pSightEnt->IsPlayer())
					{
						if ((pev->spawnflags & SF_MONSTER_WAIT_TILL_SEEN) != 0)
						{
							CBaseMonster* pClient;

							pClient = pSightEnt->MyMonsterPointer();
							// don't link this client in the list if the monster is wait till seen and the player isn't facing the monster
							if (pSightEnt && !pClient->FInViewCone(this))
							{
								// we're not in the player's view cone.
								continue;
							}
							else
							{
								// player sees us, become normal now.
								pev->spawnflags &= ~SF_MONSTER_WAIT_TILL_SEEN;
							}
						}

						// if we see a client, remember that (mostly for scripted AI)
						iSighted |= bits_COND_SEE_CLIENT;
					}

					pSightEnt->m_pLink = m_pLink;
					m_pLink = pSightEnt;

					if (pSightEnt == m_hEnemy)
					{
						// we know this ent is visible, so if it also happens to be our enemy, store that now.
						iSighted |= bits_COND_SEE_ENEMY;
					}

					// don't add the Enemy's relationship to the conditions. We only want to worry about conditions when
					// we see monsters other than the Enemy.
					switch (IRelationship(pSightEnt))
					{
					case Relationship::Nemesis:
						iSighted |= bits_COND_SEE_NEMESIS;
						break;
					case Relationship::Hate:
						iSighted |= bits_COND_SEE_HATE;
						break;
					case Relationship::Dislike:
						iSighted |= bits_COND_SEE_DISLIKE;
						break;
					case Relationship::Fear:
						iSighted |= bits_COND_SEE_FEAR;
						break;
					case Relationship::Ally:
						break;
					default:
						AILogger->debug("{} can't assess {}", STRING(pev->classname), STRING(pSightEnt->pev->classname));
						break;
					}
				}
			}
		}
	}

	SetConditions(iSighted);
}

int CBaseMonster::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_PLAYER;
}

CSound* CBaseMonster::PBestSound()
{
	int iThisSound;
	int iBestSound = -1;
	float flBestDist = 8192; // so first nearby sound will become best so far.
	float flDist;
	CSound* pSound;

	iThisSound = m_iAudibleList;

	if (iThisSound == SOUNDLIST_EMPTY)
	{
		AILogger->debug("ERROR! monster {} has no audible sounds!", STRING(pev->classname));
#if _DEBUG
		AILogger->error("NULL Return from PBestSound");
#endif
		return nullptr;
	}

	while (iThisSound != SOUNDLIST_EMPTY)
	{
		pSound = CSoundEnt::SoundPointerForIndex(iThisSound);

		if (pSound && pSound->FIsSound())
		{
			flDist = (pSound->m_vecOrigin - EarPosition()).Length();

			if (flDist < flBestDist)
			{
				iBestSound = iThisSound;
				flBestDist = flDist;
			}
		}

		iThisSound = pSound->m_iNextAudible;
	}
	if (iBestSound >= 0)
	{
		pSound = CSoundEnt::SoundPointerForIndex(iBestSound);
		return pSound;
	}
#if _DEBUG
	AILogger->error("NULL Return from PBestSound");
#endif
	return nullptr;
}

CSound* CBaseMonster::PBestScent()
{
	int iThisScent;
	int iBestScent = -1;
	float flBestDist = 8192; // so first nearby smell will become best so far.
	float flDist;
	CSound* pSound;

	iThisScent = m_iAudibleList; // smells are in the sound list.

	if (iThisScent == SOUNDLIST_EMPTY)
	{
		AILogger->debug("ERROR! PBestScent() has empty soundlist!");
#if _DEBUG
		AILogger->error("NULL Return from PBestSound");
#endif
		return nullptr;
	}

	while (iThisScent != SOUNDLIST_EMPTY)
	{
		pSound = CSoundEnt::SoundPointerForIndex(iThisScent);

		if (pSound->FIsScent())
		{
			flDist = (pSound->m_vecOrigin - pev->origin).Length();

			if (flDist < flBestDist)
			{
				iBestScent = iThisScent;
				flBestDist = flDist;
			}
		}

		iThisScent = pSound->m_iNextAudible;
	}
	if (iBestScent >= 0)
	{
		pSound = CSoundEnt::SoundPointerForIndex(iBestScent);

		return pSound;
	}
#if _DEBUG
	AILogger->error("NULL Return from PBestScent");
#endif
	return nullptr;
}

void CBaseMonster::MonsterThink()
{
	pev->nextthink = gpGlobals->time + 0.1; // keep monster thinking.


	RunAI();

	UpdateShockEffect();

	float flInterval = StudioFrameAdvance(); // animate
											 // start or end a fidget
											 // This needs a better home -- switching animations over time should be encapsulated on a per-activity basis
											 // perhaps MaintainActivity() or a ShiftAnimationOverTime() or something.
	if (m_MonsterState != MONSTERSTATE_SCRIPT && m_MonsterState != MONSTERSTATE_DEAD && m_Activity == ACT_IDLE && m_fSequenceFinished)
	{
		int iSequence;

		if (m_fSequenceLoops)
		{
			// animation does loop, which means we're playing subtle idle. Might need to
			// fidget.
			iSequence = LookupActivity(m_Activity);
		}
		else
		{
			// animation that just ended doesn't loop! That means we just finished a fidget
			// and should return to our heaviest weighted idle (the subtle one)
			iSequence = LookupActivityHeaviest(m_Activity);
		}
		if (iSequence != ACTIVITY_NOT_AVAILABLE)
		{
			pev->sequence = iSequence; // Set to new anim (if it's there)
			ResetSequenceInfo();
		}
	}

	DispatchAnimEvents(flInterval);

	if (!MovementIsComplete())
	{
		Move(flInterval);
	}
#if _DEBUG
	else
	{
		if (!TaskIsRunning() && !TaskIsComplete())
			AILogger->error("Schedule stalled!!");
	}
#endif
}

void CBaseMonster::MonsterUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Don't do this because it can resurrect dying monsters
	// m_IdealMonsterState = MONSTERSTATE_ALERT;
}

int CBaseMonster::IgnoreConditions()
{
	int iIgnoreConditions = 0;

	if (!FShouldEat())
	{
		// not hungry? Ignore food smell.
		iIgnoreConditions |= bits_COND_SMELL_FOOD;
	}

	if (m_MonsterState == MONSTERSTATE_SCRIPT && m_pCine)
		iIgnoreConditions |= m_pCine->IgnoreConditions();

	return iIgnoreConditions;
}

void CBaseMonster::RouteClear()
{
	RouteNew();
	m_movementGoal = MOVEGOAL_NONE;
	m_movementActivity = ACT_IDLE;
	Forget(bits_MEMORY_MOVE_FAILED);
}

void CBaseMonster::RouteNew()
{
	m_Route[0].iType = 0;
	m_iRouteIndex = 0;
}

bool CBaseMonster::FRouteClear()
{
	if (m_Route[m_iRouteIndex].iType == 0 || m_movementGoal == MOVEGOAL_NONE)
		return true;

	return false;
}

bool CBaseMonster::FRefreshRoute()
{
	CBaseEntity* pPathCorner;
	int i;
	bool returnCode;

	RouteNew();

	returnCode = false;

	switch (m_movementGoal)
	{
	case MOVEGOAL_PATHCORNER:
	{
		// monster is on a path_corner loop
		pPathCorner = m_pGoalEnt;
		i = 0;

		while (pPathCorner && i < ROUTE_SIZE)
		{
			m_Route[i].iType = bits_MF_TO_PATHCORNER;
			m_Route[i].vecLocation = pPathCorner->pev->origin;

			pPathCorner = pPathCorner->GetNextTarget();

			// Last path_corner in list?
			if (!pPathCorner)
				m_Route[i].iType |= bits_MF_IS_GOAL;

			i++;
		}
	}
		returnCode = true;
		break;

	case MOVEGOAL_ENEMY:
		returnCode = BuildRoute(m_vecEnemyLKP, bits_MF_TO_ENEMY, m_hEnemy);
		break;

	case MOVEGOAL_LOCATION:
		returnCode = BuildRoute(m_vecMoveGoal, bits_MF_TO_LOCATION, nullptr);
		break;

	case MOVEGOAL_TARGETENT:
		if (m_hTargetEnt != nullptr)
		{
			returnCode = BuildRoute(m_hTargetEnt->pev->origin, bits_MF_TO_TARGETENT, m_hTargetEnt);
		}
		break;

	case MOVEGOAL_NODE:
		returnCode = FGetNodeRoute(m_vecMoveGoal);
		//			if ( returnCode )
		//				RouteSimplify( nullptr );
		break;
	}

	return returnCode;
}

bool CBaseMonster::MoveToEnemy(Activity movementAct, float waitTime)
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_ENEMY;
	return FRefreshRoute();
}

bool CBaseMonster::MoveToLocation(Activity movementAct, float waitTime, const Vector& goal)
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_LOCATION;
	m_vecMoveGoal = goal;
	return FRefreshRoute();
}

bool CBaseMonster::MoveToTarget(Activity movementAct, float waitTime)
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_TARGETENT;
	return FRefreshRoute();
}

bool CBaseMonster::MoveToNode(Activity movementAct, float waitTime, const Vector& goal)
{
	m_movementActivity = movementAct;
	m_moveWaitTime = waitTime;

	m_movementGoal = MOVEGOAL_NODE;
	m_vecMoveGoal = goal;
	return FRefreshRoute();
}

#ifdef _DEBUG
void DrawRoute(CBaseEntity* entity, WayPoint_t* m_Route, int m_iRouteIndex, int r, int g, int b)
{
	int i;

	if (m_Route[m_iRouteIndex].iType == 0)
	{
		CBaseMonster::AILogger->debug("Can't draw route!");
		return;
	}

	//	UTIL_ParticleEffect ( m_Route[ m_iRouteIndex ].vecLocation, g_vecZero, 255, 25 );

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(entity->pev->origin.x);
	WRITE_COORD(entity->pev->origin.y);
	WRITE_COORD(entity->pev->origin.z);
	WRITE_COORD(m_Route[m_iRouteIndex].vecLocation.x);
	WRITE_COORD(m_Route[m_iRouteIndex].vecLocation.y);
	WRITE_COORD(m_Route[m_iRouteIndex].vecLocation.z);

	WRITE_SHORT(g_sModelIndexLaser);
	WRITE_BYTE(0);	 // frame start
	WRITE_BYTE(10);	 // framerate
	WRITE_BYTE(1);	 // life
	WRITE_BYTE(16);	 // width
	WRITE_BYTE(0);	 // noise
	WRITE_BYTE(r);	 // r, g, b
	WRITE_BYTE(g);	 // r, g, b
	WRITE_BYTE(b);	 // r, g, b
	WRITE_BYTE(255); // brightness
	WRITE_BYTE(10);	 // speed
	MESSAGE_END();

	for (i = m_iRouteIndex; i < ROUTE_SIZE - 1; i++)
	{
		if ((m_Route[i].iType & bits_MF_IS_GOAL) != 0 || (m_Route[i + 1].iType == 0))
			break;


		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_COORD(m_Route[i].vecLocation.x);
		WRITE_COORD(m_Route[i].vecLocation.y);
		WRITE_COORD(m_Route[i].vecLocation.z);
		WRITE_COORD(m_Route[i + 1].vecLocation.x);
		WRITE_COORD(m_Route[i + 1].vecLocation.y);
		WRITE_COORD(m_Route[i + 1].vecLocation.z);
		WRITE_SHORT(g_sModelIndexLaser);
		WRITE_BYTE(0);	 // frame start
		WRITE_BYTE(10);	 // framerate
		WRITE_BYTE(1);	 // life
		WRITE_BYTE(8);	 // width
		WRITE_BYTE(0);	 // noise
		WRITE_BYTE(r);	 // r, g, b
		WRITE_BYTE(g);	 // r, g, b
		WRITE_BYTE(b);	 // r, g, b
		WRITE_BYTE(255); // brightness
		WRITE_BYTE(10);	 // speed
		MESSAGE_END();

		//		UTIL_ParticleEffect ( m_Route[ i ].vecLocation, g_vecZero, 255, 25 );
	}
}
#endif

bool ShouldSimplify(int routeType)
{
	routeType &= ~bits_MF_IS_GOAL;

	// TODO: verify this this needs to be a comparison and not a bit check
	if ((routeType == bits_MF_TO_PATHCORNER) || (routeType & bits_MF_DONT_SIMPLIFY) != 0)
		return false;
	return true;
}

void CBaseMonster::RouteSimplify(CBaseEntity* pTargetEnt)
{
	// BUGBUG: this doesn't work 100% yet
	int i, count, outCount;
	Vector vecStart;
	WayPoint_t outRoute[ROUTE_SIZE * 2]; // Any points except the ends can turn into 2 points in the simplified route

	count = 0;

	for (i = m_iRouteIndex; i < ROUTE_SIZE; i++)
	{
		if (0 == m_Route[i].iType)
			break;
		else
			count++;
		if ((m_Route[i].iType & bits_MF_IS_GOAL) != 0)
			break;
	}
	// Can't simplify a direct route!
	if (count < 2)
	{
		//		DrawRoute( pev, m_Route, m_iRouteIndex, 0, 0, 255 );
		return;
	}

	outCount = 0;
	vecStart = pev->origin;
	for (i = 0; i < count - 1; i++)
	{
		// Don't eliminate path_corners
		if (!ShouldSimplify(m_Route[m_iRouteIndex + i].iType))
		{
			outRoute[outCount] = m_Route[m_iRouteIndex + i];
			outCount++;
		}
		else if (CheckLocalMove(vecStart, m_Route[m_iRouteIndex + i + 1].vecLocation, pTargetEnt, nullptr) == LOCALMOVE_VALID)
		{
			// Skip vert
			continue;
		}
		else
		{
			Vector vecTest, vecSplit;

			// Halfway between this and next
			vecTest = (m_Route[m_iRouteIndex + i + 1].vecLocation + m_Route[m_iRouteIndex + i].vecLocation) * 0.5;

			// Halfway between this and previous
			vecSplit = (m_Route[m_iRouteIndex + i].vecLocation + vecStart) * 0.5;

			int iType = (m_Route[m_iRouteIndex + i].iType | bits_MF_TO_DETOUR) & ~bits_MF_NOT_TO_MASK;
			if (CheckLocalMove(vecStart, vecTest, pTargetEnt, nullptr) == LOCALMOVE_VALID)
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecTest;
			}
			else if (CheckLocalMove(vecSplit, vecTest, pTargetEnt, nullptr) == LOCALMOVE_VALID)
			{
				outRoute[outCount].iType = iType;
				outRoute[outCount].vecLocation = vecSplit;
				outRoute[outCount + 1].iType = iType;
				outRoute[outCount + 1].vecLocation = vecTest;
				outCount++; // Adding an extra point
			}
			else
			{
				outRoute[outCount] = m_Route[m_iRouteIndex + i];
			}
		}
		// Get last point
		vecStart = outRoute[outCount].vecLocation;
		outCount++;
	}
	ASSERT(i < count);
	outRoute[outCount] = m_Route[m_iRouteIndex + i];
	outCount++;

	// Terminate
	outRoute[outCount].iType = 0;
	ASSERT(outCount < (ROUTE_SIZE * 2));

	// Copy the simplified route, disable for testing
	m_iRouteIndex = 0;
	for (i = 0; i < ROUTE_SIZE && i < outCount; i++)
	{
		m_Route[i] = outRoute[i];
	}

	// Terminate route
	if (i < ROUTE_SIZE)
		m_Route[i].iType = 0;

		// Debug, test movement code
#if 0
//	if ( CVAR_GET_FLOAT( "simplify" ) != 0 )
	DrawRoute(pev, outRoute, 0, 255, 0, 0);
	//	else
	DrawRoute(pev, m_Route, m_iRouteIndex, 0, 255, 0);
#endif
}

bool CBaseMonster::FBecomeProne()
{
	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		pev->flags -= FL_ONGROUND;
	}

	m_IdealMonsterState = MONSTERSTATE_PRONE;
	return true;
}

bool CBaseMonster::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist > 64 && flDist <= 784 && flDot >= 0.5)
	{
		return true;
	}
	return false;
}

bool CBaseMonster::CheckRangeAttack2(float flDot, float flDist)
{
	if (flDist > 64 && flDist <= 512 && flDot >= 0.5)
	{
		return true;
	}
	return false;
}

bool CBaseMonster::CheckMeleeAttack1(float flDot, float flDist)
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if (flDist <= 64 && flDot >= 0.7 && m_hEnemy != nullptr && FBitSet(m_hEnemy->pev->flags, FL_ONGROUND))
	{
		return true;
	}
	return false;
}

bool CBaseMonster::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= 64 && flDot >= 0.7)
	{
		return true;
	}
	return false;
}

void CBaseMonster::CheckAttacks(CBaseEntity* pTarget, float flDist)
{
	Vector2D vec2LOS;
	float flDot;

	UTIL_MakeVectors(pev->angles);

	vec2LOS = (pTarget->pev->origin - pev->origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	// we know the enemy is in front now. We'll find which attacks the monster is capable of by
	// checking for corresponding Activities in the model file, then do the simple checks to validate
	// those attack types.

	// Clear all attack conditions
	ClearConditions(bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2);

	if ((m_afCapability & bits_CAP_RANGE_ATTACK1) != 0)
	{
		if (CheckRangeAttack1(flDot, flDist))
			SetConditions(bits_COND_CAN_RANGE_ATTACK1);
	}
	if ((m_afCapability & bits_CAP_RANGE_ATTACK2) != 0)
	{
		if (CheckRangeAttack2(flDot, flDist))
			SetConditions(bits_COND_CAN_RANGE_ATTACK2);
	}
	if ((m_afCapability & bits_CAP_MELEE_ATTACK1) != 0)
	{
		if (CheckMeleeAttack1(flDot, flDist))
			SetConditions(bits_COND_CAN_MELEE_ATTACK1);
	}
	if ((m_afCapability & bits_CAP_MELEE_ATTACK2) != 0)
	{
		if (CheckMeleeAttack2(flDot, flDist))
			SetConditions(bits_COND_CAN_MELEE_ATTACK2);
	}
}

bool CBaseMonster::FCanCheckAttacks()
{
	if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions(bits_COND_ENEMY_TOOFAR))
	{
		return true;
	}

	return false;
}

bool CBaseMonster::CheckEnemy(CBaseEntity* pEnemy)
{
	float flDistToEnemy;
	bool iUpdatedLKP; // set this to true if you update the EnemyLKP in this function.

	iUpdatedLKP = false;
	ClearConditions(bits_COND_ENEMY_FACING_ME);

	if (!FVisible(pEnemy))
	{
		ASSERT(!HasConditions(bits_COND_SEE_ENEMY));
		SetConditions(bits_COND_ENEMY_OCCLUDED);
	}
	else
		ClearConditions(bits_COND_ENEMY_OCCLUDED);

	if (!pEnemy->IsAlive())
	{
		SetConditions(bits_COND_ENEMY_DEAD);
		ClearConditions(bits_COND_SEE_ENEMY | bits_COND_ENEMY_OCCLUDED);
		return false;
	}

	Vector vecEnemyPos = pEnemy->pev->origin;
	// distance to enemy's origin
	flDistToEnemy = (vecEnemyPos - pev->origin).Length();
	vecEnemyPos.z += pEnemy->pev->size.z * 0.5;
	// distance to enemy's head
	if (float flDistToEnemy2 = (vecEnemyPos - pev->origin).Length(); flDistToEnemy2 < flDistToEnemy)
		flDistToEnemy = flDistToEnemy2;
	else
	{
		// distance to enemy's feet
		vecEnemyPos.z -= pEnemy->pev->size.z;
		flDistToEnemy2 = (vecEnemyPos - pev->origin).Length();
		if (flDistToEnemy2 < flDistToEnemy)
			flDistToEnemy = flDistToEnemy2;
	}

	if (HasConditions(bits_COND_SEE_ENEMY))
	{
		CBaseMonster* pEnemyMonster;

		iUpdatedLKP = true;
		m_vecEnemyLKP = pEnemy->pev->origin;

		pEnemyMonster = pEnemy->MyMonsterPointer();

		if (pEnemyMonster)
		{
			if (pEnemyMonster->FInViewCone(this))
			{
				SetConditions(bits_COND_ENEMY_FACING_ME);
			}
			else
				ClearConditions(bits_COND_ENEMY_FACING_ME);
		}

		if (pEnemy->pev->velocity != Vector(0, 0, 0))
		{
			// trail the enemy a bit
			m_vecEnemyLKP = m_vecEnemyLKP - pEnemy->pev->velocity * RANDOM_FLOAT(-0.05, 0);
		}
		else
		{
			// UNDONE: use pev->oldorigin?
		}
	}
	else if (!HasConditions(bits_COND_ENEMY_OCCLUDED | bits_COND_SEE_ENEMY) && (flDistToEnemy <= 256))
	{
		// if the enemy is not occluded, and unseen, that means it is behind or beside the monster.
		// if the enemy is near enough the monster, we go ahead and let the monster know where the
		// enemy is.
		iUpdatedLKP = true;
		m_vecEnemyLKP = pEnemy->pev->origin;
	}

	if (flDistToEnemy >= m_flDistTooFar)
	{
		// enemy is very far away from monster
		SetConditions(bits_COND_ENEMY_TOOFAR);
	}
	else
		ClearConditions(bits_COND_ENEMY_TOOFAR);

	if (FCanCheckAttacks())
	{
		CheckAttacks(m_hEnemy, flDistToEnemy);
	}

	if (m_movementGoal == MOVEGOAL_ENEMY)
	{
		for (int i = m_iRouteIndex; i < ROUTE_SIZE; i++)
		{
			if (m_Route[i].iType == (bits_MF_IS_GOAL | bits_MF_TO_ENEMY))
			{
				// UNDONE: Should we allow monsters to override this distance (80?)
				if ((m_Route[i].vecLocation - m_vecEnemyLKP).Length() > 80)
				{
					// Refresh
					FRefreshRoute();
					return iUpdatedLKP;
				}
			}
		}
	}

	return iUpdatedLKP;
}

void CBaseMonster::PushEnemy(CBaseEntity* pEnemy, Vector& vecLastKnownPos)
{
	int i;

	if (pEnemy == nullptr)
		return;

	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for (i = 0; i < MAX_OLD_ENEMIES; i++)
	{
		if (m_hOldEnemy[i] == pEnemy)
			return;
		if (m_hOldEnemy[i] == nullptr) // someone died, reuse their slot
			break;
	}
	if (i >= MAX_OLD_ENEMIES)
		return;

	m_hOldEnemy[i] = pEnemy;
	m_vecOldEnemy[i] = vecLastKnownPos;
}

bool CBaseMonster::PopEnemy()
{
	// UNDONE: blah, this is bad, we should use a stack but I'm too lazy to code one.
	for (int i = MAX_OLD_ENEMIES - 1; i >= 0; i--)
	{
		if (m_hOldEnemy[i] != nullptr)
		{
			if (m_hOldEnemy[i]->IsAlive()) // cheat and know when they die
			{
				m_hEnemy = m_hOldEnemy[i];
				m_vecEnemyLKP = m_vecOldEnemy[i];
				// AILogger->debug("remembering");
				return true;
			}
			else
			{
				m_hOldEnemy[i] = nullptr;
			}
		}
	}
	return false;
}

void CBaseMonster::SetActivity(Activity NewActivity)
{
	const Activity oldActivity = NewActivity;

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;

	const int iSequence = LookupActivity(NewActivity);

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			// don't reset frame between walk and run
			if (!(oldActivity == ACT_WALK || oldActivity == ACT_RUN) || !(NewActivity == ACT_WALK || NewActivity == ACT_RUN))
				pev->frame = 0;
		}

		pev->sequence = iSequence; // Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		AILogger->debug("{} has no sequence for act:{}", STRING(pev->classname), NewActivity);
		pev->sequence = 0; // Set to the reset anim (if it's there)
	}
}

void CBaseMonster::SetSequenceByName(const char* szSequence)
{
	int iSequence;

	iSequence = LookupSequence(szSequence);

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence; // Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		AILogger->debug("{} has no sequence named:{}", STRING(pev->classname), szSequence);
		pev->sequence = 0; // Set to the reset anim (if it's there)
	}
}

// !!!PERFORMANCE - should we try to load balance this?
// DON'T USE SETORIGIN!
#define LOCAL_STEP_SIZE 16
int CBaseMonster::CheckLocalMove(const Vector& vecStart, const Vector& vecEnd, CBaseEntity* pTarget, float* pflDist)
{
	Vector vecStartPos; // record monster's position before trying the move
	float flYaw;
	float flDist;
	float flStep, stepSize;
	int iReturn;

	vecStartPos = pev->origin;


	flYaw = VectorToYaw(vecEnd - vecStart);	 // build a yaw that points to the goal.
	flDist = (vecEnd - vecStart).Length2D(); // get the distance.
	iReturn = LOCALMOVE_VALID;				 // assume everything will be ok.

	// move the monster to the start of the local move that's to be checked.
	SetOrigin(vecStart); // !!!BUGBUG - won't this fire triggers? - nope, SetOrigin doesn't fire

	if ((pev->flags & (FL_FLY | FL_SWIM)) == 0)
	{
		DROP_TO_FLOOR(edict()); // make sure monster is on the floor!
	}

	// pev->origin.z = vecStartPos.z;//!!!HACKHACK

	//	pev->origin = vecStart;

	/*
	if ( flDist > 1024 )
	{
		// !!!PERFORMANCE - this operation may be too CPU intensive to try checks this large.
		// We don't lose much here, because a distance this great is very likely
		// to have something in the way.

		// since we've actually moved the monster during the check, undo the move.
		pev->origin = vecStartPos;
		return false;
	}
*/
	// this loop takes single steps to the goal.
	for (flStep = 0; flStep < flDist; flStep += LOCAL_STEP_SIZE)
	{
		stepSize = LOCAL_STEP_SIZE;

		if ((flStep + LOCAL_STEP_SIZE) >= (flDist - 1))
			stepSize = (flDist - flStep) - 1;

		//		UTIL_ParticleEffect ( pev->origin, g_vecZero, 255, 25 );

		if (!WALK_MOVE(edict(), flYaw, stepSize, WALKMOVE_CHECKONLY))
		{ // can't take the next step, fail!

			if (pflDist != nullptr)
			{
				*pflDist = flStep;
			}
			if (pTarget && pTarget->edict() == gpGlobals->trace_ent)
			{
				// if this step hits target ent, the move is legal.
				iReturn = LOCALMOVE_VALID;
				break;
			}
			else
			{
				// If we're going toward an entity, and we're almost getting there, it's OK.
				//				if ( pTarget && fabs( flDist - iStep ) < LOCAL_STEP_SIZE )
				//					fReturn = true;
				//				else
				iReturn = LOCALMOVE_INVALID;
				break;
			}
		}
	}

	if (iReturn == LOCALMOVE_VALID && (pev->flags & (FL_FLY | FL_SWIM)) == 0 && (!pTarget || (pTarget->pev->flags & FL_ONGROUND) != 0))
	{
		// The monster can move to a spot UNDER the target, but not to it. Don't try to triangulate, go directly to the node graph.
		// UNDONE: Magic # 64 -- this used to be pev->size.z but that won't work for small creatures like the headcrab
		if (fabs(vecEnd.z - pev->origin.z) > 64)
		{
			iReturn = LOCALMOVE_INVALID_DONT_TRIANGULATE;
		}
	}
	/*
	// uncommenting this block will draw a line representing the nearest legal move.
	WRITE_BYTE(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(MSG_BROADCAST, TE_SHOWLINE);
	WRITE_COORD(MSG_BROADCAST, pev->origin.x);
	WRITE_COORD(MSG_BROADCAST, pev->origin.y);
	WRITE_COORD(MSG_BROADCAST, pev->origin.z);
	WRITE_COORD(MSG_BROADCAST, vecStart.x);
	WRITE_COORD(MSG_BROADCAST, vecStart.y);
	WRITE_COORD(MSG_BROADCAST, vecStart.z);
	*/

	// since we've actually moved the monster during the check, undo the move.
	SetOrigin(vecStartPos);

	return iReturn;
}

float CBaseMonster::OpenDoorAndWait(CBaseEntity* door)
{
	float flTravelTime = 0;

	// AILogger->trace("A door.");
	if (door && !door->IsLockedByMaster())
	{
		// AILogger->trace("unlocked!");
		door->Use(this, this, USE_ON, 0.0);
		// AILogger->trace("door->pev->nextthink = {} ms", (int)(1000*door->pev->nextthink));
		// AILogger->trace("door->pev->ltime = {} ms", (int)(1000*door->pev->ltime));
		// AILogger->trace("pev->nextthink = {} ms", (int)(1000*pev->nextthink));
		// AILogger->trace("pev->ltime = {} ms", (int)(1000*pev->ltime));
		flTravelTime = door->pev->nextthink - door->pev->ltime;
		// AILogger->trace("Waiting {} ms", (int)(1000*flTravelTime));
		if (!FStringNull(door->pev->targetname))
		{
			for (auto target : UTIL_FindEntitiesByTargetname(STRING(door->pev->targetname)))
			{
				if (target != door)
				{
					if (target->ClassnameIs(STRING(door->pev->classname)))
					{
						target->Use(this, this, USE_ON, 0.0);
					}
				}
			}
		}
	}

	return gpGlobals->time + flTravelTime;
}

void CBaseMonster::AdvanceRoute(float distance)
{

	if (m_iRouteIndex == ROUTE_SIZE - 1)
	{
		// time to refresh the route.
		if (!FRefreshRoute())
		{
			AILogger->debug("Can't Refresh Route!!");
		}
	}
	else
	{
		if ((m_Route[m_iRouteIndex].iType & bits_MF_IS_GOAL) == 0)
		{
			// If we've just passed a path_corner, advance m_pGoalEnt
			if ((m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_PATHCORNER)
				m_pGoalEnt = m_pGoalEnt->GetNextTarget();

			// IF both waypoints are nodes, then check for a link for a door and operate it.
			//
			if ((m_Route[m_iRouteIndex].iType & bits_MF_TO_NODE) == bits_MF_TO_NODE && (m_Route[m_iRouteIndex + 1].iType & bits_MF_TO_NODE) == bits_MF_TO_NODE)
			{
				// AILogger->trace("SVD: Two nodes.");

				int iSrcNode = WorldGraph.FindNearestNode(m_Route[m_iRouteIndex].vecLocation, this);
				int iDestNode = WorldGraph.FindNearestNode(m_Route[m_iRouteIndex + 1].vecLocation, this);

				int iLink;
				WorldGraph.HashSearch(iSrcNode, iDestNode, iLink);

				if (iLink >= 0 && WorldGraph.m_pLinkPool[iLink].m_pLinkEnt != nullptr)
				{
					// AILogger->trace("A link.");
					if (WorldGraph.HandleLinkEnt(iSrcNode, WorldGraph.m_pLinkPool[iLink].m_pLinkEnt, m_afCapability, CGraph::NODEGRAPH_DYNAMIC))
					{
						// AILogger->trace("usable.");
						entvars_t* pevDoor = WorldGraph.m_pLinkPool[iLink].m_pLinkEnt;
						if (pevDoor)
						{
							auto door = CBaseEntity::Instance(pevDoor);
							m_flMoveWaitFinished = OpenDoorAndWait(door);
							//							AILogger->trace("Waiting for door {:.2f}", m_flMoveWaitFinished-gpGlobals->time);
						}
					}
				}
			}
			m_iRouteIndex++;
		}
		else // At goal!!!
		{
			if (distance < m_flGroundSpeed * 0.2 /* FIX */)
			{
				MovementComplete();
			}
		}
	}
}

int CBaseMonster::RouteClassify(int iMoveFlag)
{
	int movementGoal;

	movementGoal = MOVEGOAL_NONE;

	if ((iMoveFlag & bits_MF_TO_TARGETENT) != 0)
		movementGoal = MOVEGOAL_TARGETENT;
	else if ((iMoveFlag & bits_MF_TO_ENEMY) != 0)
		movementGoal = MOVEGOAL_ENEMY;
	else if ((iMoveFlag & bits_MF_TO_PATHCORNER) != 0)
		movementGoal = MOVEGOAL_PATHCORNER;
	else if ((iMoveFlag & bits_MF_TO_NODE) != 0)
		movementGoal = MOVEGOAL_NODE;
	else if ((iMoveFlag & bits_MF_TO_LOCATION) != 0)
		movementGoal = MOVEGOAL_LOCATION;

	return movementGoal;
}

bool CBaseMonster::BuildRoute(const Vector& vecGoal, int iMoveFlag, CBaseEntity* pTarget)
{
	float flDist;
	Vector vecApex;
	int iLocalMove;

	RouteNew();
	m_movementGoal = RouteClassify(iMoveFlag);

	// so we don't end up with no moveflags
	m_Route[0].vecLocation = vecGoal;
	m_Route[0].iType = iMoveFlag | bits_MF_IS_GOAL;

	// check simple local move
	iLocalMove = CheckLocalMove(pev->origin, vecGoal, pTarget, &flDist);

	if (iLocalMove == LOCALMOVE_VALID)
	{
		// monster can walk straight there!
		return true;
	}
	// try to triangulate around any obstacles.
	else if (iLocalMove != LOCALMOVE_INVALID_DONT_TRIANGULATE && FTriangulate(pev->origin, vecGoal, flDist, pTarget, &vecApex))
	{
		// there is a slightly more complicated path that allows the monster to reach vecGoal
		m_Route[0].vecLocation = vecApex;
		m_Route[0].iType = (iMoveFlag | bits_MF_TO_DETOUR);

		m_Route[1].vecLocation = vecGoal;
		m_Route[1].iType = iMoveFlag | bits_MF_IS_GOAL;

		/*
		WRITE_BYTE(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(MSG_BROADCAST, TE_SHOWLINE);
		WRITE_COORD(MSG_BROADCAST, vecApex.x );
		WRITE_COORD(MSG_BROADCAST, vecApex.y );
		WRITE_COORD(MSG_BROADCAST, vecApex.z );
		WRITE_COORD(MSG_BROADCAST, vecApex.x );
		WRITE_COORD(MSG_BROADCAST, vecApex.y );
		WRITE_COORD(MSG_BROADCAST, vecApex.z + 128 );
		*/

		RouteSimplify(pTarget);
		return true;
	}

	// last ditch, try nodes
	if (FGetNodeRoute(vecGoal))
	{
		// AILogger->debug("Can get there on nodes");
		m_vecMoveGoal = vecGoal;
		RouteSimplify(pTarget);
		return true;
	}

	// b0rk
	return false;
}

void CBaseMonster::InsertWaypoint(Vector vecLocation, int afMoveFlags)
{
	int i, type;


	// we have to save some Index and Type information from the real
	// path_corner or node waypoint that the monster was trying to reach. This makes sure that data necessary
	// to refresh the original path exists even in the new waypoints that don't correspond directy to a path_corner
	// or node.
	type = afMoveFlags | (m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK);

	for (i = ROUTE_SIZE - 1; i > 0; i--)
		m_Route[i] = m_Route[i - 1];

	m_Route[m_iRouteIndex].vecLocation = vecLocation;
	m_Route[m_iRouteIndex].iType = type;
}

bool CBaseMonster::FTriangulate(const Vector& vecStart, const Vector& vecEnd, float flDist, CBaseEntity* pTargetEnt, Vector* pApex)
{
	Vector vecDir;
	Vector vecForward;
	Vector vecLeft;	   // the spot we'll try to triangulate to on the left
	Vector vecRight;   // the spot we'll try to triangulate to on the right
	Vector vecTop;	   // the spot we'll try to triangulate to on the top
	Vector vecBottom;  // the spot we'll try to triangulate to on the bottom
	Vector vecFarSide; // the spot that we'll move to after hitting the triangulated point, before moving on to our normal goal.
	int i;
	float sizeX, sizeZ;

	// If the hull width is less than 24, use 24 because CheckLocalMove uses a min of
	// 24.
	sizeX = pev->size.x;
	if (sizeX < 24.0)
		sizeX = 24.0;
	else if (sizeX > 48.0)
		sizeX = 48.0;
	sizeZ = pev->size.z;
	// if (sizeZ < 24.0)
	//	sizeZ = 24.0;

	vecForward = (vecEnd - vecStart).Normalize();

	Vector vecDirUp(0, 0, 1);
	vecDir = CrossProduct(vecForward, vecDirUp);

	// start checking right about where the object is, picking two equidistant starting points, one on
	// the left, one on the right. As we progress through the loop, we'll push these away from the obstacle,
	// hoping to find a way around on either side. pev->size.x is added to the ApexDist in order to help select
	// an apex point that insures that the monster is sufficiently past the obstacle before trying to turn back
	// onto its original course.

	vecLeft = pev->origin + (vecForward * (flDist + sizeX)) - vecDir * (sizeX * 3);
	vecRight = pev->origin + (vecForward * (flDist + sizeX)) + vecDir * (sizeX * 3);
	if (pev->movetype == MOVETYPE_FLY)
	{
		vecTop = pev->origin + (vecForward * flDist) + (vecDirUp * sizeZ * 3);
		vecBottom = pev->origin + (vecForward * flDist) - (vecDirUp * sizeZ * 3);
	}

	vecFarSide = m_Route[m_iRouteIndex].vecLocation;

	vecDir = vecDir * sizeX * 2;
	if (pev->movetype == MOVETYPE_FLY)
		vecDirUp = vecDirUp * sizeZ * 2;

	for (i = 0; i < 8; i++)
	{
		// Debug, Draw the triangulation
#if 0
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_SHOWLINE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(vecRight.x);
		WRITE_COORD(vecRight.y);
		WRITE_COORD(vecRight.z);
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_SHOWLINE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(vecLeft.x);
		WRITE_COORD(vecLeft.y);
		WRITE_COORD(vecLeft.z);
		MESSAGE_END();
#endif

#if 0
		if (pev->movetype == MOVETYPE_FLY)
		{
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SHOWLINE);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(vecTop.x);
			WRITE_COORD(vecTop.y);
			WRITE_COORD(vecTop.z);
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SHOWLINE);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_COORD(vecBottom.x);
			WRITE_COORD(vecBottom.y);
			WRITE_COORD(vecBottom.z);
			MESSAGE_END();
}
#endif

		if (CheckLocalMove(pev->origin, vecRight, pTargetEnt, nullptr) == LOCALMOVE_VALID)
		{
			if (CheckLocalMove(vecRight, vecFarSide, pTargetEnt, nullptr) == LOCALMOVE_VALID)
			{
				if (pApex)
				{
					*pApex = vecRight;
				}

				return true;
			}
		}
		if (CheckLocalMove(pev->origin, vecLeft, pTargetEnt, nullptr) == LOCALMOVE_VALID)
		{
			if (CheckLocalMove(vecLeft, vecFarSide, pTargetEnt, nullptr) == LOCALMOVE_VALID)
			{
				if (pApex)
				{
					*pApex = vecLeft;
				}

				return true;
			}
		}

		if (pev->movetype == MOVETYPE_FLY)
		{
			if (CheckLocalMove(pev->origin, vecTop, pTargetEnt, nullptr) == LOCALMOVE_VALID)
			{
				if (CheckLocalMove(vecTop, vecFarSide, pTargetEnt, nullptr) == LOCALMOVE_VALID)
				{
					if (pApex)
					{
						*pApex = vecTop;
						// AILogger->trace("triangulate over");
					}

					return true;
				}
			}
#if 1
			if (CheckLocalMove(pev->origin, vecBottom, pTargetEnt, nullptr) == LOCALMOVE_VALID)
			{
				if (CheckLocalMove(vecBottom, vecFarSide, pTargetEnt, nullptr) == LOCALMOVE_VALID)
				{
					if (pApex)
					{
						*pApex = vecBottom;
						// AILogger->trace("triangulate under");
					}

					return true;
				}
			}
#endif
		}

		vecRight = vecRight + vecDir;
		vecLeft = vecLeft - vecDir;
		if (pev->movetype == MOVETYPE_FLY)
		{
			vecTop = vecTop + vecDirUp;
			vecBottom = vecBottom - vecDirUp;
		}
	}

	return false;
}

#define DIST_TO_CHECK 200

void CBaseMonster::Move(float flInterval)
{
	float flWaypointDist;
	float flCheckDist;
	float flDist; // how far the lookahead check got before hitting an object.
	Vector vecDir;
	Vector vecApex;
	CBaseEntity* pTargetEnt;

	// Don't move if no valid route
	if (FRouteClear())
	{
		// If we still have a movement goal, then this is probably a route truncated by SimplifyRoute()
		// so refresh it.
		if (m_movementGoal == MOVEGOAL_NONE || !FRefreshRoute())
		{
			AILogger->debug("Tried to move with no route!");
			TaskFail();
			return;
		}
	}

	if (m_flMoveWaitFinished > gpGlobals->time)
		return;

		// Debug, test movement code
#if 0
//	if ( CVAR_GET_FLOAT("stopmove" ) != 0 )
	{
		if (m_movementGoal == MOVEGOAL_ENEMY)
			RouteSimplify(m_hEnemy);
		else
			RouteSimplify(m_hTargetEnt);
		FRefreshRoute();
		return;
}
#else
// Debug, draw the route
//	DrawRoute( pev, m_Route, m_iRouteIndex, 0, 200, 0 );
#endif

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = nullptr;

	// local move to waypoint.
	vecDir = (m_Route[m_iRouteIndex].vecLocation - pev->origin).Normalize();
	flWaypointDist = (m_Route[m_iRouteIndex].vecLocation - pev->origin).Length2D();

	MakeIdealYaw(m_Route[m_iRouteIndex].vecLocation);
	ChangeYaw(pev->yaw_speed);

	// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
	if (flWaypointDist < DIST_TO_CHECK)
	{
		flCheckDist = flWaypointDist;
	}
	else
	{
		flCheckDist = DIST_TO_CHECK;
	}

	if ((m_Route[m_iRouteIndex].iType & (~bits_MF_NOT_TO_MASK)) == bits_MF_TO_ENEMY)
	{
		// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
		pTargetEnt = m_hEnemy;
	}
	else if ((m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_TARGETENT)
	{
		pTargetEnt = m_hTargetEnt;
	}

	// !!!BUGBUG - CheckDist should be derived from ground speed.
	// If this fails, it should be because of some dynamic entity blocking this guy.
	// We've already checked this path, so we should wait and time out if the entity doesn't move
	flDist = 0;
	if (CheckLocalMove(pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist) != LOCALMOVE_VALID)
	{
		CBaseEntity* pBlocker;

		// Can't move, stop
		Stop();
		// Blocking entity is in global trace_ent
		pBlocker = CBaseEntity::Instance(gpGlobals->trace_ent);
		if (pBlocker)
		{
			DispatchBlocked(edict(), pBlocker->edict());
		}

		if (pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time - m_flMoveWaitFinished) > 3.0)
		{
			// Can we still move toward our target?
			if (flDist < m_flGroundSpeed)
			{
				// No, Wait for a second
				m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
				return;
			}
			// Ok, still enough room to take a step
		}
		else
		{
			// try to triangulate around whatever is in the way.
			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist, pTargetEnt, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR);
				RouteSimplify(pTargetEnt);
			}
			else
			{
				//				AILogger->debug("Couldn't Triangulate");
				Stop();
				// Only do this once until your route is cleared
				if (m_moveWaitTime > 0 && (m_afMemory & bits_MEMORY_MOVE_FAILED) == 0)
				{
					FRefreshRoute();
					if (FRouteClear())
					{
						TaskFail();
					}
					else
					{
						// Don't get stuck
						if ((gpGlobals->time - m_flMoveWaitFinished) < 0.2)
							Remember(bits_MEMORY_MOVE_FAILED);

						m_flMoveWaitFinished = gpGlobals->time + 0.1;
					}
				}
				else
				{
					TaskFail();
					AILogger->debug("{} Failed to move ({})!", STRING(pev->classname), static_cast<int>(HasMemory(bits_MEMORY_MOVE_FAILED)));
					// AILogger->debug("{}, {}, {}", pev->origin.z, (pev->origin + (vecDir * flCheckDist)).z, m_Route[m_iRouteIndex].vecLocation.z);
				}
				return;
			}
		}
	}

	// close enough to the target, now advance to the next target. This is done before actually reaching
	// the target so that we get a nice natural turn while moving.
	if (ShouldAdvanceRoute(flWaypointDist)) ///!!!BUGBUG- magic number
	{
		AdvanceRoute(flWaypointDist);
	}

	// Might be waiting for a door
	if (m_flMoveWaitFinished > gpGlobals->time)
	{
		Stop();
		return;
	}

	// UNDONE: this is a hack to quit moving farther than it has looked ahead.
	if (flCheckDist < m_flGroundSpeed * flInterval)
	{
		flInterval = flCheckDist / m_flGroundSpeed;
		// AILogger->debug("{:.02f}", flInterval);
	}
	MoveExecute(pTargetEnt, vecDir, flInterval);

	if (MovementIsComplete())
	{
		Stop();
		RouteClear();
	}
}

bool CBaseMonster::ShouldAdvanceRoute(float flWaypointDist)
{
	if (flWaypointDist <= MONSTER_CUT_CORNER_DIST)
	{
		// AILogger->debug("cut {}", flWaypointDist);
		return true;
	}

	return false;
}

void CBaseMonster::MoveExecute(CBaseEntity* pTargetEnt, const Vector& vecDir, float flInterval)
{
	//	float flYaw = VectorToYaw(m_Route[ m_iRouteIndex ].vecLocation - pev->origin);// build a yaw that points to the goal.
	//	WALK_MOVE( edict(), flYaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );
	if (m_IdealActivity != m_movementActivity)
		m_IdealActivity = m_movementActivity;

	float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	float flStep;
	while (flTotal > 0.001)
	{
		// don't walk more than 16 units or stairs stop working
		flStep = std::min(16.0f, flTotal);
		UTIL_MoveToOrigin(edict(), m_Route[m_iRouteIndex].vecLocation, flStep, MOVE_NORMAL);
		flTotal -= flStep;
	}
	// AILogger->debug("dist {}", m_flGroundSpeed * pev->framerate * flInterval);
}

void CBaseMonster::MonsterInit()
{
	if (!g_pGameRules->FAllowMonsters())
	{
		pev->flags |= FL_KILLME; // Post this because some monster code modifies class data after calling this function
								 //		REMOVE_ENTITY(edict());
		return;
	}

	// Set fields common to all monsters
	pev->effects = 0;
	pev->takedamage = DAMAGE_AIM;
	pev->ideal_yaw = pev->angles.y;
	pev->max_health = pev->health;
	pev->deadflag = DEAD_NO;
	m_IdealMonsterState = MONSTERSTATE_IDLE; // Assume monster will be idle, until proven otherwise

	m_IdealActivity = ACT_IDLE;

	SetBits(pev->flags, FL_MONSTER);
	if ((pev->spawnflags & SF_MONSTER_HITMONSTERCLIP) != 0)
		pev->flags |= FL_MONSTERCLIP;

	ClearSchedule();
	RouteClear();
	InitBoneControllers(); // FIX: should be done in Spawn

	m_iHintNode = NO_NODE;

	m_afMemory = MEMORY_CLEAR;

	m_hEnemy = nullptr;

	m_flDistTooFar = 1024.0;
	m_flDistLook = 2048.0;

	// set eye position
	SetEyePosition();

	SetThink(&CBaseMonster::MonsterInitThink);
	pev->nextthink = gpGlobals->time + 0.1;
	// SetUse(&CBaseMonster::MonsterUse);

	if (m_AllowFollow)
	{
		SetUse(&CBaseMonster::FollowerUse);
	}
}

void CBaseMonster::MonsterInitThink()
{
	StartMonster();
}

void CBaseMonster::StartMonster()
{
	// update capabilities
	if (LookupActivity(ACT_RANGE_ATTACK1) != ACTIVITY_NOT_AVAILABLE)
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK1;
	}
	if (LookupActivity(ACT_RANGE_ATTACK2) != ACTIVITY_NOT_AVAILABLE)
	{
		m_afCapability |= bits_CAP_RANGE_ATTACK2;
	}
	if (LookupActivity(ACT_MELEE_ATTACK1) != ACTIVITY_NOT_AVAILABLE)
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK1;
	}
	if (LookupActivity(ACT_MELEE_ATTACK2) != ACTIVITY_NOT_AVAILABLE)
	{
		m_afCapability |= bits_CAP_MELEE_ATTACK2;
	}

	// Raise monster off the floor one unit, then drop to floor
	if (pev->movetype != MOVETYPE_FLY && !FBitSet(pev->spawnflags, SF_MONSTER_FALL_TO_GROUND))
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR(edict());
		// Try to move the monster to make sure it's not stuck in a brush.
		if (!WALK_MOVE(edict(), 0, 0, WALKMOVE_NORMAL))
		{
			AILogger->error("Monster {} stuck in wall--level design error", STRING(pev->classname));
			pev->effects = EF_BRIGHTFIELD;
		}
	}
	else
	{
		pev->flags &= ~FL_ONGROUND;
	}

	if (!FStringNull(pev->target)) // this monster has a target
	{
		// Find the monster's initial target entity, stash it
		m_pGoalEnt = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));

		// TODO: this was probably unintended
		if (!m_pGoalEnt)
		{
			m_pGoalEnt = World;
		}

		if (!m_pGoalEnt)
		{
			AILogger->error("ReadyMonster()--{} couldn't find target {}", STRING(pev->classname), STRING(pev->target));
		}
		else
		{
			// Monster will start turning towards his destination
			MakeIdealYaw(m_pGoalEnt->pev->origin);

			// JAY: How important is this error message?  Big Momma doesn't obey this rule, so I took it out.
#if 0
			// At this point, we expect only a path_corner as initial goal
			if (!m_pGoalEnt->ClassnameIs("path_corner"))
			{
				AILogger->warning("ReadyMonster--monster's initial goal '{}' is not a path_corner", STRING(pev->target));
		}
#endif

			// set the monster up to walk a path corner path.
			// !!!BUGBUG - this is a minor bit of a hack.
			// JAYJAY
			m_movementGoal = MOVEGOAL_PATHCORNER;

			if (pev->movetype == MOVETYPE_FLY)
				m_movementActivity = ACT_FLY;
			else
				m_movementActivity = ACT_WALK;

			if (!FRefreshRoute())
			{
				AILogger->debug("Can't Create Route!");
			}
			SetState(MONSTERSTATE_IDLE);
			ChangeSchedule(GetScheduleOfType(SCHED_IDLE_WALK));
		}
	}

	// SetState ( m_IdealMonsterState );
	// SetActivity ( m_IdealActivity );

	// Delay drop to floor to make sure each door in the level has had its chance to spawn
	// Spread think times so that they don't all happen at the same time (Carmack)
	SetThink(&CBaseMonster::CallMonsterThink);
	pev->nextthink += RANDOM_FLOAT(0.1, 0.4); // spread think times.

	if (!FStringNull(pev->targetname)) // wait until triggered
	{
		SetState(MONSTERSTATE_IDLE);
		// UNDONE: Some scripted sequence monsters don't have an idle?
		SetActivity(ACT_IDLE);
		ChangeSchedule(GetScheduleOfType(SCHED_WAIT_TRIGGER));
	}
}

void CBaseMonster::MovementComplete()
{
	switch (m_iTaskStatus)
	{
	case TASKSTATUS_NEW:
	case TASKSTATUS_RUNNING:
		m_iTaskStatus = TASKSTATUS_RUNNING_TASK;
		break;

	case TASKSTATUS_RUNNING_MOVEMENT:
		TaskComplete();
		break;

	case TASKSTATUS_RUNNING_TASK:
		AILogger->error("Movement completed twice!");
		break;

	case TASKSTATUS_COMPLETE:
		break;
	}
	m_movementGoal = MOVEGOAL_NONE;
}

bool CBaseMonster::TaskIsRunning()
{
	if (m_iTaskStatus != TASKSTATUS_COMPLETE &&
		m_iTaskStatus != TASKSTATUS_RUNNING_MOVEMENT)
		return true;

	return false;
}

Relationship CBaseMonster::IRelationship(CBaseEntity* pTarget)
{
	if (pTarget->IsPlayer())
	{
		// Hate player if they shoot you, even if otherwise friendly.
		// Only do this if we have a classification, otherwise we're a prop of some sort.
		if (Classify() != ENTCLASS_NONE)
		{
			if ((m_afMemory & bits_MEMORY_PROVOKED) != 0)
			{
				return Relationship::Hate;
			}
		}

		switch (m_PlayerAllyRelationship)
		{
		case PlayerAllyRelationship::No: return Relationship::Dislike;
		case PlayerAllyRelationship::Yes: return Relationship::Ally;
		default: break;
		}
	}

	return g_EntityClassifications.GetRelationship(Classify(), pTarget->Classify());
}

// UNDONE: Should this find the nearest node?

// float CGraph::PathLength( int iStart, int iDest, int iHull, int afCapMask )

bool CBaseMonster::FindCover(Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist)
{
	int i;
	int iMyHullIndex;
	int iMyNode;
	int iThreatNode;
	float flDist;
	Vector vecLookersOffset;
	TraceResult tr;

	if (0 == flMaxDist)
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if (flMinDist > 0.5 * flMaxDist)
	{
#if _DEBUG
		AILogger->debug("FindCover MinDist ({:.0f}) too close to MaxDist ({:.0f})", flMinDist, flMaxDist);
#endif
		flMinDist = 0.5 * flMaxDist;
	}

	if (0 == WorldGraph.m_fGraphPresent || 0 == WorldGraph.m_fGraphPointersSet)
	{
		AILogger->debug("Graph not ready for findcover!");
		return false;
	}

	iMyNode = WorldGraph.FindNearestNode(pev->origin, this);
	iThreatNode = WorldGraph.FindNearestNode(vecThreat, this);
	iMyHullIndex = WorldGraph.HullIndex(this);

	if (iMyNode == NO_NODE)
	{
		AILogger->debug("FindCover() - {} has no nearest node!", STRING(pev->classname));
		return false;
	}
	if (iThreatNode == NO_NODE)
	{
		// AILogger->debug("FindCover() - Threat has no nearest node!");
		iThreatNode = iMyNode;
		// return false;
	}

	vecLookersOffset = vecThreat + vecViewOffset; // calculate location of enemy's eyes

	// we'll do a rough sample to find nodes that are relatively nearby
	for (i = 0; i < WorldGraph.m_cNodes; i++)
	{
		int nodeNumber = (i + WorldGraph.m_iLastCoverSearch) % WorldGraph.m_cNodes;

		CNode& node = WorldGraph.Node(nodeNumber);
		WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.

		// could use an optimization here!!
		flDist = (pev->origin - node.m_vecOrigin).Length();

		// DON'T do the trace check on a node that is farther away than a node that we've already found to
		// provide cover! Also make sure the node is within the mins/maxs of the search.
		if (flDist >= flMinDist && flDist < flMaxDist)
		{
			UTIL_TraceLine(node.m_vecOrigin + vecViewOffset, vecLookersOffset, ignore_monsters, ignore_glass, edict(), &tr);

			// if this node will block the threat's line of sight to me...
			if (tr.flFraction != 1.0)
			{
				// ..and is also closer to me than the threat, or the same distance from myself and the threat the node is good.
				if ((iMyNode == iThreatNode) || WorldGraph.PathLength(iMyNode, nodeNumber, iMyHullIndex, m_afCapability) <= WorldGraph.PathLength(iThreatNode, nodeNumber, iMyHullIndex, m_afCapability))
				{
					if (FValidateCover(node.m_vecOrigin) && MoveToLocation(ACT_RUN, 0, node.m_vecOrigin))
					{
						/*
						MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
							WRITE_BYTE( TE_SHOWLINE);

							WRITE_COORD( node.m_vecOrigin.x );
							WRITE_COORD( node.m_vecOrigin.y );
							WRITE_COORD( node.m_vecOrigin.z );

							WRITE_COORD( vecLookersOffset.x );
							WRITE_COORD( vecLookersOffset.y );
							WRITE_COORD( vecLookersOffset.z );
						MESSAGE_END();
						*/

						return true;
					}
				}
			}
		}
	}
	return false;
}

bool CBaseMonster::BuildNearestRoute(Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist)
{
	int i;
	int iMyHullIndex;
	int iMyNode;
	float flDist;
	Vector vecLookersOffset;
	TraceResult tr;

	if (0 == flMaxDist)
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if (flMinDist > 0.5 * flMaxDist)
	{
		AILogger->trace("FindCover MinDist ({:.0f}) too close to MaxDist ({:.0f})", flMinDist, flMaxDist);
		flMinDist = 0.5 * flMaxDist;
	}

	if (0 == WorldGraph.m_fGraphPresent || 0 == WorldGraph.m_fGraphPointersSet)
	{
		AILogger->debug("Graph not ready for BuildNearestRoute!");
		return false;
	}

	iMyNode = WorldGraph.FindNearestNode(pev->origin, this);
	iMyHullIndex = WorldGraph.HullIndex(this);

	if (iMyNode == NO_NODE)
	{
		AILogger->debug("BuildNearestRoute() - {} has no nearest node!", STRING(pev->classname));
		return false;
	}

	vecLookersOffset = vecThreat + vecViewOffset; // calculate location of enemy's eyes

	// we'll do a rough sample to find nodes that are relatively nearby
	for (i = 0; i < WorldGraph.m_cNodes; i++)
	{
		int nodeNumber = (i + WorldGraph.m_iLastCoverSearch) % WorldGraph.m_cNodes;

		CNode& node = WorldGraph.Node(nodeNumber);
		WorldGraph.m_iLastCoverSearch = nodeNumber + 1; // next monster that searches for cover node will start where we left off here.

		// can I get there?
		if (WorldGraph.NextNodeInRoute(iMyNode, nodeNumber, iMyHullIndex, 0) != iMyNode)
		{
			flDist = (vecThreat - node.m_vecOrigin).Length();

			// is it close?
			if (flDist > flMinDist && flDist < flMaxDist)
			{
				// can I see where I want to be from there?
				UTIL_TraceLine(node.m_vecOrigin + pev->view_ofs, vecLookersOffset, ignore_monsters, edict(), &tr);

				if (tr.flFraction == 1.0)
				{
					// try to actually get there
					if (BuildRoute(node.m_vecOrigin, bits_MF_TO_LOCATION, nullptr))
					{
						flMaxDist = flDist;
						m_vecMoveGoal = node.m_vecOrigin;
						return true; // UNDONE: keep looking for something closer!
					}
				}
			}
		}
	}

	return false;
}

CBaseEntity* CBaseMonster::BestVisibleEnemy()
{
	CBaseEntity* pReturn;
	CBaseEntity* pNextEnt;
	int iNearest;
	int iDist;

	iNearest = 8192; // so first visible entity will become the closest.
	pNextEnt = m_pLink;
	pReturn = nullptr;
	Relationship iBestRelationship = Relationship::None;

	while (pNextEnt != nullptr)
	{
		if (pNextEnt->IsAlive())
		{
			if (IRelationship(pNextEnt) > iBestRelationship)
			{
				// this entity is disliked MORE than the entity that we
				// currently think is the best visible enemy. No need to do
				// a distance check, just get mad at this one for now.
				iBestRelationship = IRelationship(pNextEnt);
				iNearest = (pNextEnt->pev->origin - pev->origin).Length();
				pReturn = pNextEnt;
			}
			else if (IRelationship(pNextEnt) == iBestRelationship)
			{
				// this entity is disliked just as much as the entity that
				// we currently think is the best visible enemy, so we only
				// get mad at it if it is closer.
				iDist = (pNextEnt->pev->origin - pev->origin).Length();

				if (iDist <= iNearest)
				{
					iNearest = iDist;
					iBestRelationship = IRelationship(pNextEnt);
					pReturn = pNextEnt;
				}
			}
		}

		pNextEnt = pNextEnt->m_pLink;
	}

	return pReturn;
}

void CBaseMonster::MakeIdealYaw(Vector vecTarget)
{
	Vector vecProjection;

	// strafing monster needs to face 90 degrees away from its goal
	if (m_movementActivity == ACT_STRAFE_LEFT)
	{
		vecProjection.x = -vecTarget.y;
		vecProjection.y = vecTarget.x;

		pev->ideal_yaw = VectorToYaw(vecProjection - pev->origin);
	}
	else if (m_movementActivity == ACT_STRAFE_RIGHT)
	{
		vecProjection.x = vecTarget.y;
		vecProjection.y = vecTarget.x;

		pev->ideal_yaw = VectorToYaw(vecProjection - pev->origin);
	}
	else
	{
		pev->ideal_yaw = VectorToYaw(vecTarget - pev->origin);
	}
}

float CBaseMonster::FlYawDiff()
{
	float flCurrentYaw;

	flCurrentYaw = UTIL_AngleMod(pev->angles.y);

	if (flCurrentYaw == pev->ideal_yaw)
	{
		return 0;
	}


	return UTIL_AngleDiff(pev->ideal_yaw, flCurrentYaw);
}

float CBaseMonster::ChangeYaw(int yawSpeed)
{
	float ideal, current, move, speed;

	current = UTIL_AngleMod(pev->angles.y);
	ideal = pev->ideal_yaw;
	if (current != ideal)
	{
		float delta = gpGlobals->time - m_flLastYawTime;

		m_flLastYawTime = gpGlobals->time;

		if (delta > 0.25)
		{
			delta = 0.25;
		}

		speed = yawSpeed * delta * 2;
		move = ideal - current;

		if (ideal > current)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}

		if (move > 0)
		{ // turning to the monster's left
			if (move > speed)
				move = speed;
		}
		else
		{ // turning to the monster's right
			if (move < -speed)
				move = -speed;
		}

		pev->angles.y = UTIL_AngleMod(current + move);

		// turn head in desired direction only if they have a turnable head
		if ((m_afCapability & bits_CAP_TURN_HEAD) != 0)
		{
			float yaw = pev->ideal_yaw - pev->angles.y;
			if (yaw > 180)
				yaw -= 360;
			if (yaw < -180)
				yaw += 360;
			// yaw *= 0.8;
			SetBoneController(0, yaw);
		}
	}
	else
		move = 0;

	return move;
}

float CBaseMonster::VecToYaw(Vector vecDir)
{
	if (vecDir.x == 0 && vecDir.y == 0 && vecDir.z == 0)
		return pev->angles.y;

	return VectorToYaw(vecDir);
}

void CBaseMonster::SetEyePosition()
{
	Vector vecEyePosition;
	void* pmodel = GET_MODEL_PTR(edict());

	GetEyePosition(pmodel, vecEyePosition);

	pev->view_ofs = vecEyePosition;

	if (pev->view_ofs == g_vecZero)
	{
		AILogger->debug("{} has no view_ofs!", STRING(pev->classname));
	}
}

void CBaseMonster::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case SCRIPT_EVENT_DEAD:
		if (m_MonsterState == MONSTERSTATE_SCRIPT)
		{
			pev->deadflag = DEAD_DYING;
			// Kill me now! (and fade out when CineCleanup() is called)
			AILogger->trace("Death event: {}", STRING(pev->classname));
			pev->health = 0;
		}
		else
			AILogger->trace("INVALID death event:{}", STRING(pev->classname));
		break;
	case SCRIPT_EVENT_NOT_DEAD:
		if (m_MonsterState == MONSTERSTATE_SCRIPT)
		{
			pev->deadflag = DEAD_NO;
			// This is for life/death sequences where the player can determine whether a character is dead or alive after the script
			pev->health = pev->max_health;
		}
		break;

	case SCRIPT_EVENT_SOUND: // Play a named wave file
		EmitSound(CHAN_BODY, pEvent->options, 1.0, ATTN_IDLE);
		break;

	case SCRIPT_EVENT_SOUND_VOICE:
		EmitSound(CHAN_VOICE, pEvent->options, 1.0, ATTN_IDLE);
		break;

	case SCRIPT_EVENT_SOUND_VOICE_BODY:
		EmitSound(CHAN_BODY, pEvent->options, VOL_NORM, ATTN_NORM);
		break;

	case SCRIPT_EVENT_SOUND_VOICE_VOICE:
		EmitSound(CHAN_VOICE, pEvent->options, VOL_NORM, ATTN_NORM);
		break;

	case SCRIPT_EVENT_SOUND_VOICE_WEAPON:
		EmitSound(CHAN_WEAPON, pEvent->options, VOL_NORM, ATTN_NORM);
		break;

	case SCRIPT_EVENT_SENTENCE_RND1: // Play a named sentence group 33% of the time
		if (RANDOM_LONG(0, 2) == 0)
			break;
		[[fallthrough]];
	case SCRIPT_EVENT_SENTENCE: // Play a named sentence group
		sentences::g_Sentences.PlayRndSz(this, pEvent->options, 1.0, ATTN_IDLE, 0, 100);
		break;

	case SCRIPT_EVENT_FIREEVENT: // Fire a trigger
		FireTargets(pEvent->options, this, this, USE_TOGGLE, 0);
		break;

	case SCRIPT_EVENT_NOINTERRUPT: // Can't be interrupted from now on
		if (m_pCine)
			m_pCine->AllowInterrupt(false);
		break;

	case SCRIPT_EVENT_CANINTERRUPT: // OK to interrupt now
		if (m_pCine)
			m_pCine->AllowInterrupt(true);
		break;

#if 0
	case SCRIPT_EVENT_INAIR:			// Don't DROP_TO_FLOOR()
	case SCRIPT_EVENT_ENDANIMATION:		// Set ending animation sequence to
		break;
#endif

	case MONSTER_EVENT_BODYDROP_HEAVY:
		if ((pev->flags & FL_ONGROUND) != 0)
		{
			if (RANDOM_LONG(0, 1) == 0)
			{
				EmitSoundDyn(CHAN_BODY, "common/bodydrop3.wav", 1, ATTN_NORM, 0, 90);
			}
			else
			{
				EmitSoundDyn(CHAN_BODY, "common/bodydrop4.wav", 1, ATTN_NORM, 0, 90);
			}
		}
		break;

	case MONSTER_EVENT_BODYDROP_LIGHT:
		if ((pev->flags & FL_ONGROUND) != 0)
		{
			if (RANDOM_LONG(0, 1) == 0)
			{
				EmitSound(CHAN_BODY, "common/bodydrop3.wav", 1, ATTN_NORM);
			}
			else
			{
				EmitSound(CHAN_BODY, "common/bodydrop4.wav", 1, ATTN_NORM);
			}
		}
		break;

	case MONSTER_EVENT_SWISHSOUND:
	{
		// NO MONSTER may use this anim event unless that monster's precache precaches this sound!!!
		EmitSound(CHAN_BODY, "zombie/claw_miss2.wav", 1, ATTN_NORM);
		break;
	}

	default:
		AILogger->warn("Unhandled animation event {} for {}", pEvent->event, STRING(pev->classname));
		break;
	}
}

// Combat

Vector CBaseMonster::GetGunPosition()
{
	UTIL_MakeVectors(pev->angles);

	// Vector vecSrc = pev->origin + gpGlobals->v_forward * 10;
	// vecSrc.z = pevShooter->absmin.z + pevShooter->size.z * 0.7;
	// vecSrc.z = pev->origin.z + (pev->view_ofs.z - 4);
	Vector vecSrc = pev->origin + gpGlobals->v_forward * m_HackedGunPos.y + gpGlobals->v_right * m_HackedGunPos.x + gpGlobals->v_up * m_HackedGunPos.z;

	return vecSrc;
}

//=========================================================
// NODE GRAPH
//=========================================================

bool CBaseMonster::FGetNodeRoute(Vector vecDest)
{
	int iPath[MAX_PATH_SIZE];
	int iSrcNode, iDestNode;
	int iResult;
	int i;
	int iNumToCopy;

	iSrcNode = WorldGraph.FindNearestNode(pev->origin, this);
	iDestNode = WorldGraph.FindNearestNode(vecDest, this);

	if (iSrcNode == -1)
	{
		// no node nearest self
		//		AILogger->debug("FGetNodeRoute: No valid node near self!");
		return false;
	}
	else if (iDestNode == -1)
	{
		// no node nearest target
		//		AILogger->debug("FGetNodeRoute: No valid node near target!");
		return false;
	}

	// valid src and dest nodes were found, so it's safe to proceed with
	// find shortest path
	int iNodeHull = WorldGraph.HullIndex(this); // make this a monster virtual function
	iResult = WorldGraph.FindShortestPath(iPath, iSrcNode, iDestNode, iNodeHull, m_afCapability);

	if (0 == iResult)
	{
#if 1
		AILogger->debug("No Path from {} to {}!", iSrcNode, iDestNode);
		return false;
#else
		bool bRoutingSave = WorldGraph.m_fRoutingComplete;
		WorldGraph.m_fRoutingComplete = false;
		iResult = WorldGraph.FindShortestPath(iPath, iSrcNode, iDestNode, iNodeHull, m_afCapability);
		WorldGraph.m_fRoutingComplete = bRoutingSave;
		if (0 == iResult)
		{
			AILogger->debug("No Path from {} to {}!", iSrcNode, iDestNode);
			return false;
		}
		else
		{
			AILogger->debug("Routing is inconsistent!");
		}
#endif
	}

	// there's a valid path within iPath now, so now we will fill the route array
	// up with as many of the waypoints as it will hold.

	// don't copy ROUTE_SIZE entries if the path returned is shorter
	// than ROUTE_SIZE!!!
	if (iResult < ROUTE_SIZE)
	{
		iNumToCopy = iResult;
	}
	else
	{
		iNumToCopy = ROUTE_SIZE;
	}

	for (i = 0; i < iNumToCopy; i++)
	{
		m_Route[i].vecLocation = WorldGraph.m_pNodes[iPath[i]].m_vecOrigin;
		m_Route[i].iType = bits_MF_TO_NODE;
	}

	if (iNumToCopy < ROUTE_SIZE)
	{
		m_Route[iNumToCopy].vecLocation = vecDest;
		m_Route[iNumToCopy].iType |= bits_MF_IS_GOAL;
	}

	return true;
}

int CBaseMonster::FindHintNode()
{
	int i;
	TraceResult tr;

	if (0 == WorldGraph.m_fGraphPresent)
	{
		AILogger->debug("find_hintnode: graph not ready!");
		return NO_NODE;
	}

	if (WorldGraph.m_iLastActiveIdleSearch >= WorldGraph.m_cNodes)
	{
		WorldGraph.m_iLastActiveIdleSearch = 0;
	}

	for (i = 0; i < WorldGraph.m_cNodes; i++)
	{
		int nodeNumber = (i + WorldGraph.m_iLastActiveIdleSearch) % WorldGraph.m_cNodes;
		CNode& node = WorldGraph.Node(nodeNumber);

		if (0 != node.m_sHintType)
		{
			// this node has a hint. Take it if it is visible, the monster likes it, and the monster has an animation to match the hint's activity.
			if (FValidateHintType(node.m_sHintType))
			{
				if (0 == node.m_sHintActivity || LookupActivity(node.m_sHintActivity) != ACTIVITY_NOT_AVAILABLE)
				{
					UTIL_TraceLine(pev->origin + pev->view_ofs, node.m_vecOrigin + pev->view_ofs, ignore_monsters, edict(), &tr);

					if (tr.flFraction == 1.0)
					{
						WorldGraph.m_iLastActiveIdleSearch = nodeNumber + 1; // next monster that searches for hint nodes will start where we left off.
						return nodeNumber;									 // take it!
					}
				}
			}
		}
	}

	WorldGraph.m_iLastActiveIdleSearch = 0; // start at the top of the list for the next search.

	return NO_NODE;
}

void CBaseMonster::ReportAIState()
{
	const spdlog::level::level_enum level = spdlog::level::debug;

	if (!AILogger->should_log(level))
	{
		return;
	}

	static constexpr const char* pStateNames[] = {"None", "Idle", "Combat", "Alert", "Hunt", "Prone", "Scripted", "PlayDead", "Dead"};

	static_assert(std::size(pStateNames) == MONSTERSTATE_COUNT, "You forgot to update the array of monster state names");

	eastl::fixed_string<char, 512 + 1> buffer;

	auto inserter = std::back_inserter(buffer);

	fmt::format_to(inserter, "{}: ", STRING(pev->classname));
	if ((std::size_t)m_MonsterState < std::size(pStateNames))
		fmt::format_to(inserter, "State: {}, ", pStateNames[m_MonsterState]);
	int i = 0;
	while (activity_map[i].type != 0)
	{
		if (activity_map[i].type == (int)m_Activity)
		{
			fmt::format_to(inserter, "Activity {}, ", activity_map[i].name);
			break;
		}
		i++;
	}

	if (m_pSchedule)
	{
		const char* pName = nullptr;
		pName = m_pSchedule->pName;
		if (!pName)
			pName = "Unknown";
		fmt::format_to(inserter, "Schedule {}, ", pName);
		const Task_t* pTask = GetTask();
		if (pTask)
			fmt::format_to(inserter, "Task {} (#{}), ", pTask->iTask, m_iScheduleIndex);
	}
	else
		fmt::format_to(inserter, "No Schedule, ");

	if (m_hEnemy != nullptr)
		fmt::format_to(inserter, "\nEnemy is {}", STRING(m_hEnemy->pev->classname));
	else
		fmt::format_to(inserter, "No enemy");

	if (IsMoving())
	{
		fmt::format_to(inserter, " Moving ");
		if (m_flMoveWaitFinished > gpGlobals->time)
			fmt::format_to(inserter, ": Stopped for {:.2f}. ", m_flMoveWaitFinished - gpGlobals->time);
		else if (m_IdealActivity == GetStoppedActivity())
			fmt::format_to(inserter, ": In stopped anim. ");
	}

	CSquadMonster* pSquadMonster = MySquadMonsterPointer();

	if (pSquadMonster)
	{
		if (!pSquadMonster->InSquad())
		{
			fmt::format_to(inserter, "not ");
		}

		fmt::format_to(inserter, "In Squad, ");

		if (!pSquadMonster->IsLeader())
		{
			fmt::format_to(inserter, "not ");
		}

		fmt::format_to(inserter, "Leader.");
	}

	fmt::format_to(inserter, "\n");
	fmt::format_to(inserter, "Yaw speed:{:3.1f},Health: {:3.1f}\n", pev->yaw_speed, pev->health);
	if ((pev->spawnflags & SF_MONSTER_PRISONER) != 0)
		fmt::format_to(inserter, " PRISONER! ");
	if ((pev->spawnflags & SF_MONSTER_PREDISASTER) != 0)
		fmt::format_to(inserter, " Pre-Disaster! ");

	AILogger->log(level, spdlog::string_view_t{buffer.c_str(), buffer.size()});
}

bool CBaseMonster::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "TriggerTarget"))
	{
		m_iszTriggerTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "TriggerCondition"))
	{
		m_iTriggerCondition = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "allow_item_dropping"))
	{
		m_AllowItemDropping = atoi(pkvd->szValue) != 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "is_player_ally"))
	{
		m_PlayerAllyRelationship = std::clamp(
			PlayerAllyRelationship(atoi(pkvd->szValue)), PlayerAllyRelationship::Default, PlayerAllyRelationship::Yes);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "allow_follow"))
	{
		m_AllowFollow = atoi(pkvd->szValue) != 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "UseSentence"))
	{
		m_iszUse = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "UnUseSentence"))
	{
		m_iszUnUse = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

bool CBaseMonster::FCheckAITrigger()
{
	bool fFireTarget;

	if (m_iTriggerCondition == AITRIGGER_NONE)
	{
		// no conditions, so this trigger is never fired.
		return false;
	}

	fFireTarget = false;

	switch (m_iTriggerCondition)
	{
	case AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER:
		if (m_hEnemy != nullptr && m_hEnemy->IsPlayer() && HasConditions(bits_COND_SEE_ENEMY))
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_SEEPLAYER_UNCONDITIONAL:
		if (HasConditions(bits_COND_SEE_CLIENT))
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_SEEPLAYER_NOT_IN_COMBAT:
		if (HasConditions(bits_COND_SEE_CLIENT) &&
			m_MonsterState != MONSTERSTATE_COMBAT &&
			m_MonsterState != MONSTERSTATE_PRONE &&
			m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_TAKEDAMAGE:
		if ((m_afConditions & (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE)) != 0)
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_DEATH:
		if (pev->deadflag != DEAD_NO)
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_HALFHEALTH:
		if (IsAlive() && pev->health <= (pev->max_health / 2))
		{
			fFireTarget = true;
		}
		break;
		/*

		  // !!!UNDONE - no persistant game state that allows us to track these two.

			case AITRIGGER_SQUADMEMBERDIE:
				break;
			case AITRIGGER_SQUADLEADERDIE:
				break;
		*/
	case AITRIGGER_HEARWORLD:
		if ((m_afConditions & bits_COND_HEAR_SOUND) != 0 && (m_afSoundTypes & bits_SOUND_WORLD) != 0)
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_HEARPLAYER:
		if ((m_afConditions & bits_COND_HEAR_SOUND) != 0 && (m_afSoundTypes & bits_SOUND_PLAYER) != 0)
		{
			fFireTarget = true;
		}
		break;
	case AITRIGGER_HEARCOMBAT:
		if ((m_afConditions & bits_COND_HEAR_SOUND) != 0 && (m_afSoundTypes & bits_SOUND_COMBAT) != 0)
		{
			fFireTarget = true;
		}
		break;
	}

	if (fFireTarget)
	{
		// fire the target, then set the trigger conditions to NONE so we don't fire again
		AILogger->debug("AI Trigger Fire Target");
		FireTargets(STRING(m_iszTriggerTarget), this, this, USE_TOGGLE, 0);
		m_iTriggerCondition = AITRIGGER_NONE;
		return true;
	}

	return false;
}

bool CBaseMonster::CanPlaySequence(bool fDisregardMonsterState, int interruptLevel)
{
	if (m_pCine || !IsAlive() || m_MonsterState == MONSTERSTATE_PRONE)
	{
		// monster is already running a scripted sequence or dead!
		return false;
	}

	if (fDisregardMonsterState)
	{
		// ok to go, no matter what the monster state. (scripted AI)
		return true;
	}

	if (m_MonsterState == MONSTERSTATE_NONE || m_MonsterState == MONSTERSTATE_IDLE || m_IdealMonsterState == MONSTERSTATE_IDLE)
	{
		// ok to go, but only in these states
		return true;
	}

	if (m_MonsterState == MONSTERSTATE_ALERT && interruptLevel >= SS_INTERRUPT_BY_NAME)
		return true;

	// unknown situation
	return false;
}

#define COVER_CHECKS 5 // how many checks are made
#define COVER_DELTA 48 // distance between checks

bool CBaseMonster::FindLateralCover(const Vector& vecThreat, const Vector& vecViewOffset)
{
	TraceResult tr;
	Vector vecLeftTest;
	Vector vecRightTest;
	Vector vecStepRight;
	int i;

	UTIL_MakeVectors(pev->angles);
	vecStepRight = gpGlobals->v_right * COVER_DELTA;
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = pev->origin;

	for (i = 0; i < COVER_CHECKS; i++)
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine(vecThreat + vecViewOffset, vecLeftTest + pev->view_ofs, ignore_monsters, ignore_glass, edict() /*pentIgnore*/, &tr);

		if (tr.flFraction != 1.0)
		{
			if (FValidateCover(vecLeftTest) && CheckLocalMove(pev->origin, vecLeftTest, nullptr, nullptr) == LOCALMOVE_VALID)
			{
				if (MoveToLocation(ACT_RUN, 0, vecLeftTest))
				{
					return true;
				}
			}
		}

		// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
		UTIL_TraceLine(vecThreat + vecViewOffset, vecRightTest + pev->view_ofs, ignore_monsters, ignore_glass, edict() /*pentIgnore*/, &tr);

		if (tr.flFraction != 1.0)
		{
			if (FValidateCover(vecRightTest) && CheckLocalMove(pev->origin, vecRightTest, nullptr, nullptr) == LOCALMOVE_VALID)
			{
				if (MoveToLocation(ACT_RUN, 0, vecRightTest))
				{
					return true;
				}
			}
		}
	}

	return false;
}

Vector CBaseMonster::ShootAtEnemy(const Vector& shootOrigin)
{
	CBaseEntity* pEnemy = m_hEnemy;

	if (pEnemy)
	{
		return ((pEnemy->BodyTarget(shootOrigin) - pEnemy->pev->origin) + m_vecEnemyLKP - shootOrigin).Normalize();
	}
	else
		return gpGlobals->v_forward;
}

bool CBaseMonster::FacingIdeal()
{
	if (fabs(FlYawDiff()) <= 0.006) //!!!BUGBUG - no magic numbers!!!
	{
		return true;
	}

	return false;
}

bool CBaseMonster::FCanActiveIdle()
{
	/*
	if ( m_MonsterState == MONSTERSTATE_IDLE && m_IdealMonsterState == MONSTERSTATE_IDLE && !IsMoving() )
	{
		return true;
	}
	*/
	return false;
}

void CBaseMonster::PlaySentence(const char* pszSentence, float duration, float volume, float attenuation)
{
	ASSERT(pszSentence != nullptr);

	if (!pszSentence || !CanPlaySentence(true))
	{
		return;
	}

	PlaySentenceCore(pszSentence, duration, volume, attenuation);
}

void CBaseMonster::PlaySentenceCore(const char* pszSentence, float duration, float volume, float attenuation)
{
	if (pszSentence[0] == '!')
		EmitSound(CHAN_VOICE, pszSentence, volume, attenuation);
	else
		sentences::g_Sentences.PlayRndSz(this, pszSentence, volume, attenuation, 0, PITCH_NORM);
}

void CBaseMonster::PlayScriptedSentence(const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener)
{
	PlaySentence(pszSentence, duration, volume, attenuation);
}

void CBaseMonster::SentenceStop()
{
	// TODO: use common/null.wav here once all sounds are routed through the new sound system.
	StopSound(CHAN_VOICE, "!NULLSENT");
}

void CBaseMonster::CorpseFallThink()
{
	if ((pev->flags & FL_ONGROUND) != 0)
	{
		SetThink(nullptr);

		SetSequenceBox();
		SetOrigin(pev->origin); // link into world.
	}
	else
		pev->nextthink = gpGlobals->time + 0.1;
}

void CBaseMonster::MonsterInitDead()
{
	InitBoneControllers();

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_TOSS; // so he'll fall to ground

	pev->frame = 0;
	ResetSequenceInfo();
	pev->framerate = 0;

	// Copy health
	pev->max_health = pev->health;
	pev->deadflag = DEAD_DEAD;

	SetSize(g_vecZero, g_vecZero);
	SetOrigin(pev->origin);

	// Setup health counters, etc.
	BecomeDead();
	SetThink(&CBaseMonster::CorpseFallThink);
	pev->nextthink = gpGlobals->time + 0.5;
}

bool CBaseMonster::BBoxFlat()
{
	TraceResult tr;
	Vector vecPoint;
	float flXSize, flYSize;
	float flLength;
	float flLength2;

	flXSize = pev->size.x / 2;
	flYSize = pev->size.y / 2;

	vecPoint.x = pev->origin.x + flXSize;
	vecPoint.y = pev->origin.y + flYSize;
	vecPoint.z = pev->origin.z;

	UTIL_TraceLine(vecPoint, vecPoint - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
	flLength = (vecPoint - tr.vecEndPos).Length();

	vecPoint.x = pev->origin.x - flXSize;
	vecPoint.y = pev->origin.y - flYSize;

	UTIL_TraceLine(vecPoint, vecPoint - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
	flLength2 = (vecPoint - tr.vecEndPos).Length();
	if (flLength2 > flLength)
	{
		return false;
	}
	flLength = flLength2;

	vecPoint.x = pev->origin.x - flXSize;
	vecPoint.y = pev->origin.y + flYSize;
	UTIL_TraceLine(vecPoint, vecPoint - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
	flLength2 = (vecPoint - tr.vecEndPos).Length();
	if (flLength2 > flLength)
	{
		return false;
	}
	flLength = flLength2;

	vecPoint.x = pev->origin.x + flXSize;
	vecPoint.y = pev->origin.y - flYSize;
	UTIL_TraceLine(vecPoint, vecPoint - Vector(0, 0, 100), ignore_monsters, edict(), &tr);
	flLength2 = (vecPoint - tr.vecEndPos).Length();
	if (flLength2 > flLength)
	{
		return false;
	}
	flLength = flLength2;

	return true;
}

bool CBaseMonster::GetEnemy()
{
	CBaseEntity* pNewEnemy;

	if (HasConditions(bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_NEMESIS))
	{
		pNewEnemy = BestVisibleEnemy();

		if (pNewEnemy != m_hEnemy && pNewEnemy != nullptr)
		{
			// DO NOT mess with the monster's m_hEnemy pointer unless the schedule the monster is currently running will be interrupted
			// by COND_NEW_ENEMY. This will eliminate the problem of monsters getting a new enemy while they are in a schedule that doesn't care,
			// and then not realizing it by the time they get to a schedule that does. I don't feel this is a good permanent fix.

			if (m_pSchedule)
			{
				if ((m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY) != 0)
				{
					PushEnemy(m_hEnemy, m_vecEnemyLKP);
					SetConditions(bits_COND_NEW_ENEMY);
					m_hEnemy = pNewEnemy;
					m_vecEnemyLKP = m_hEnemy->pev->origin;
				}
				// if the new enemy has an owner, take that one as well
				if (pNewEnemy->pev->owner != nullptr)
				{
					CBaseEntity* pOwner = Instance(pNewEnemy->GetOwner())->MyMonsterPointer();
					if (pOwner && (pOwner->pev->flags & FL_MONSTER) != 0 && IRelationship(pOwner) != Relationship::None)
						PushEnemy(pOwner, m_vecEnemyLKP);
				}
			}
		}
	}

	// remember old enemies
	if (m_hEnemy == nullptr && PopEnemy())
	{
		if (m_pSchedule)
		{
			if ((m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY) != 0)
			{
				SetConditions(bits_COND_NEW_ENEMY);
			}
		}
	}

	if (m_hEnemy != nullptr)
	{
		// monster has an enemy.
		return true;
	}

	return false; // monster has no enemy
}

CBaseEntity* CBaseMonster::DropItem(const char* pszItemName, const Vector& vecPos, const Vector& vecAng)
{
	if (!pszItemName)
	{
		AILogger->debug("DropItem() - No item name!");
		return nullptr;
	}

	if (GetSkillFloat("allow_npc_item_dropping") == 0)
	{
		return nullptr;
	}

	if (!m_AllowItemDropping)
	{
		return nullptr;
	}

	auto entity = g_EntityDictionary->Create(pszItemName);

	if (entity)
	{
		UTIL_InitializeKeyValues( entity, m_SharedKey, m_SharedValue, m_SharedKeyValues );
		entity->SetOwner(this);
		entity->pev->origin = vecPos;
		entity->pev->angles = vecAng;

		if (auto item = entity->MyItemPointer(); item)
		{
			item->m_RespawnDelay = ITEM_NEVER_RESPAWN_DELAY;
		}

		DispatchSpawn(entity->edict());

		// do we want this behavior to be default?! (sjb)
		entity->pev->velocity = pev->velocity;
		entity->pev->avelocity = Vector(0, RANDOM_FLOAT(0, 100), 0);
		return entity;
	}
	else
	{
		AILogger->debug("DropItem() - Didn't create!");
		return nullptr;
	}
}

bool CBaseMonster::ShouldFadeOnDeath()
{
	// if flagged to fade out or I have an owner (I came from a monster spawner)
	if ((pev->spawnflags & SF_MONSTER_FADECORPSE) != 0 || !FNullEnt(pev->owner))
		return true;

	return false;
}

void CBaseMonster::AddShockEffect(float r, float g, float b, float size, float flShockDuration)
{
	if (pev->deadflag == DEAD_NO)
	{
		if (m_fShockEffect)
		{
			m_flShockDuration += flShockDuration;
		}
		else
		{
			m_iOldRenderMode = pev->rendermode;
			m_iOldRenderFX = pev->renderfx;
			m_OldRenderColor.x = pev->rendercolor.x;
			m_OldRenderColor.y = pev->rendercolor.y;
			m_OldRenderColor.z = pev->rendercolor.z;
			m_flOldRenderAmt = pev->renderamt;

			pev->rendermode = kRenderNormal;

			pev->renderfx = kRenderFxGlowShell;
			pev->rendercolor.x = r;
			pev->rendercolor.y = g;
			pev->rendercolor.z = b;
			pev->renderamt = size;

			m_fShockEffect = true;
			m_flShockDuration = flShockDuration;
			m_flShockTime = gpGlobals->time;
		}
	}
}

void CBaseMonster::UpdateShockEffect()
{
	if (m_fShockEffect && (gpGlobals->time - m_flShockTime > m_flShockDuration))
	{
		pev->rendermode = m_iOldRenderMode;
		pev->renderfx = m_iOldRenderFX;
		pev->rendercolor.x = m_OldRenderColor.x;
		pev->rendercolor.y = m_OldRenderColor.y;
		pev->rendercolor.z = m_OldRenderColor.z;
		pev->renderamt = m_flOldRenderAmt;
		m_flShockDuration = 0;
		m_fShockEffect = false;
	}
}

void CBaseMonster::ClearShockEffect()
{
	if (m_fShockEffect)
	{
		pev->rendermode = m_iOldRenderMode;
		pev->renderfx = m_iOldRenderFX;
		pev->rendercolor.x = m_OldRenderColor.x;
		pev->rendercolor.y = m_OldRenderColor.y;
		pev->rendercolor.z = m_OldRenderColor.z;
		pev->renderamt = m_flOldRenderAmt;
		m_flShockDuration = 0;
		m_fShockEffect = false;
	}
}

void CBaseMonster::StopFollowing(bool clearSchedule)
{
	if (IsFollowing())
	{
		if (IRelationship(m_hTargetEnt) == Relationship::Ally)
		{
			if (!FStringNull(m_iszUnUse))
			{
				PlaySentence(STRING(m_iszUnUse), RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
			}
		}

		if (m_movementGoal == MOVEGOAL_TARGETENT)
			RouteClear(); // Stop him from walking toward the player
		m_hTargetEnt = nullptr;
		if (clearSchedule)
			ClearSchedule();
		if (m_hEnemy != nullptr)
			m_IdealMonsterState = MONSTERSTATE_COMBAT;
	}
}

void CBaseMonster::StartFollowing(CBaseEntity* pLeader)
{
	if (m_pCine)
		m_pCine->CancelScript();

	if (m_hEnemy != nullptr)
		m_IdealMonsterState = MONSTERSTATE_ALERT;

	m_hTargetEnt = pLeader;

	if (!FStringNull(m_iszUse))
	{
		PlaySentence(STRING(m_iszUse), RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
	}

	ClearSchedule();
}

bool CBaseMonster::CanFollow()
{
	if (m_MonsterState == MONSTERSTATE_SCRIPT || m_IdealMonsterState == MONSTERSTATE_SCRIPT)
	{
		// It's possible for m_MonsterState to still be MONSTERSTATE_SCRIPT when the script has already ended.
		// We'll treat a null pointer as an uninterruptable script and wait for the NPC to change states
		// before allowing players to make them follow them again.
		if (!m_pCine  || !m_pCine->CanInterrupt())
			return false;
	}

	if (!IsAlive())
		return false;

	return !IsFollowing();
}

// UNDONE: Keep a follow time in each follower, make a list of followers in this function and do LRU
// UNDONE: Check this in Restore to keep restored monsters from joining a full list of followers
void CBaseMonster::LimitFollowers(CBaseEntity* pPlayer, int maxFollowers)
{
	int count = 0;

	// for each friend in this bsp...
	EnumFriends([&](CBaseMonster* pFriend)
		{
			if (pFriend->IsAlive())
			{
				if (pFriend->m_hTargetEnt == pPlayer)
				{
					count++;
					if (count > maxFollowers)
						pFriend->StopFollowing(true);
				}
			} },
		true);
}

void CBaseMonster::FollowerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Don't allow use during a scripted_sentence
	if (m_useTime > gpGlobals->time)
		return;

	if (pCaller != nullptr && pCaller->IsPlayer())
	{
		// Pre-disaster followers can't be used
		if ((pev->spawnflags & SF_MONSTER_PREDISASTER) != 0)
		{
			DeclineFollowing();
		}
		else if (CanFollow())
		{
			LimitFollowers(pCaller, GetMaxFollowers());

			if (IRelationship(pCaller) != Relationship::Ally)
				AILogger->debug("I'm not following you, you evil person!");
			else
			{
				StartFollowing(pCaller);
			}
		}
		else
		{
			StopFollowing(true);
		}
	}
}

bool IsFacing(CBaseEntity* pevTest, const Vector& reference)
{
	Vector vecDir = (reference - pevTest->pev->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->pev->v_angle;
	angle.x = 0;
	AngleVectors(angle, &forward, nullptr, nullptr);
	// He's facing me, he meant it
	if (DotProduct(forward, vecDir) > 0.96) // +/- 15 degrees or so
	{
		return true;
	}
	return false;
}
