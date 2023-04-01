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

#include "cbase.h"
#include "talkmonster.h"
#include "squadmonster.h"
#include "COFSquadTalkMonster.h"
#include "plane.h"

TYPEDESCRIPTION COFSquadTalkMonster::m_SaveData[] =
	{
		DEFINE_FIELD(COFSquadTalkMonster, m_hSquadLeader, FIELD_EHANDLE),
		DEFINE_ARRAY(COFSquadTalkMonster, m_hSquadMember, FIELD_EHANDLE, MAX_SQUAD_MEMBERS - 1),

		// DEFINE_FIELD( COFSquadTalkMonster, m_afSquadSlots, FIELD_INTEGER ), // these need to be reset after transitions!
		DEFINE_FIELD(COFSquadTalkMonster, m_fEnemyEluded, FIELD_BOOLEAN),
		DEFINE_FIELD(COFSquadTalkMonster, m_flLastEnemySightTime, FIELD_TIME),

		DEFINE_FIELD(COFSquadTalkMonster, m_iMySlot, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(COFSquadTalkMonster, CTalkMonster);

bool COFSquadTalkMonster::OccupySlot(int iDesiredSlots)
{
	int i;
	int iMask;
	int iSquadSlots;

	if (!InSquad())
	{
		return true;
	}

	if (SquadEnemySplit())
	{
		// if the squad members aren't all fighting the same enemy, slots are disabled
		// so that a squad member doesn't get stranded unable to engage his enemy because
		// all of the attack slots are taken by squad members fighting other enemies.
		m_iMySlot = bits_SLOT_SQUAD_SPLIT;
		return true;
	}

	COFSquadTalkMonster* pSquadLeader = MySquadLeader();

	if ((iDesiredSlots ^ pSquadLeader->m_afSquadSlots) == 0)
	{
		// none of the desired slots are available.
		return false;
	}

	iSquadSlots = pSquadLeader->m_afSquadSlots;

	for (i = 0; i < NUM_SLOTS; i++)
	{
		iMask = 1 << i;
		if ((iDesiredSlots & iMask) != 0) // am I looking for this bit?
		{
			if ((iSquadSlots & iMask) == 0) // Is it already taken?
			{
				// No, use this bit
				pSquadLeader->m_afSquadSlots |= iMask;
				m_iMySlot = iMask;
				//				AILogger->debug("Took slot {} - {}", i, m_hSquadLeader->m_afSquadSlots);
				return true;
			}
		}
	}

	return false;
}

void COFSquadTalkMonster::VacateSlot()
{
	if (m_iMySlot != bits_NO_SLOT && InSquad())
	{
		//		AILogger->debug("Vacated Slot {} - {}", m_iMySlot, m_hSquadLeader->m_afSquadSlots);
		MySquadLeader()->m_afSquadSlots &= ~m_iMySlot;
		m_iMySlot = bits_NO_SLOT;
	}
}

void COFSquadTalkMonster::ScheduleChange()
{
	VacateSlot();
}

void COFSquadTalkMonster::Killed(CBaseEntity* attacker, int iGib)
{
	VacateSlot();

	if (InSquad())
	{
		MySquadLeader()->SquadRemove(this);
	}

	CTalkMonster::Killed(attacker, iGib);
}

void COFSquadTalkMonster::SquadRemove(COFSquadTalkMonster* pRemove)
{
	ASSERT(pRemove != nullptr);
	ASSERT(this->IsLeader());
	ASSERT(pRemove->m_hSquadLeader == this);

	// If I'm the leader, get rid of my squad
	if (pRemove == MySquadLeader())
	{
		for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++)
		{
			COFSquadTalkMonster* pMember = MySquadMember(i);
			if (pMember)
			{
				pMember->m_hSquadLeader = nullptr;
				m_hSquadMember[i] = nullptr;
			}
		}
	}
	else
	{
		COFSquadTalkMonster* pSquadLeader = MySquadLeader();
		if (pSquadLeader)
		{
			for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++)
			{
				if (pSquadLeader->m_hSquadMember[i] == this)
				{
					pSquadLeader->m_hSquadMember[i] = nullptr;
					break;
				}
			}
		}
	}

	pRemove->m_hSquadLeader = nullptr;
}

bool COFSquadTalkMonster::SquadAdd(COFSquadTalkMonster* pAdd)
{
	ASSERT(pAdd != nullptr);
	ASSERT(!pAdd->InSquad());
	ASSERT(this->IsLeader());

	for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++)
	{
		if (m_hSquadMember[i] == nullptr)
		{
			m_hSquadMember[i] = pAdd;
			pAdd->m_hSquadLeader = this;
			return true;
		}
	}
	return false;
	// should complain here
}

void COFSquadTalkMonster::SquadPasteEnemyInfo()
{
	COFSquadTalkMonster* pSquadLeader = MySquadLeader();
	if (pSquadLeader)
		pSquadLeader->m_vecEnemyLKP = m_vecEnemyLKP;
}

void COFSquadTalkMonster::SquadCopyEnemyInfo()
{
	COFSquadTalkMonster* pSquadLeader = MySquadLeader();
	if (pSquadLeader)
		m_vecEnemyLKP = pSquadLeader->m_vecEnemyLKP;
}

void COFSquadTalkMonster::SquadMakeEnemy(CBaseEntity* pEnemy)
{
	if (m_MonsterState == MONSTERSTATE_SCRIPT)
	{
		return;
	}

	if (!InSquad())
	{
		// TODO: pEnemy could be null here
		if (m_hEnemy != nullptr)
		{
			// remember their current enemy
			PushEnemy(m_hEnemy, m_vecEnemyLKP);
		}

		AILogger->debug("Non-Squad friendly grunt adopted enemy of type {}", STRING(pEnemy->pev->classname));

		// give them a new enemy
		m_hEnemy = pEnemy;
		m_vecEnemyLKP = pEnemy->pev->origin;
		SetConditions(bits_COND_NEW_ENEMY);
	}

	if (!pEnemy)
	{
		AILogger->error("ERROR: SquadMakeEnemy() - pEnemy is nullptr!");
		return;
	}

	auto squadLeader = MySquadLeader();

	const bool fLeaderIsFollowing = squadLeader->m_hTargetEnt != nullptr && squadLeader->m_hTargetEnt->IsPlayer();
	const bool fImFollowing = m_hTargetEnt != nullptr && m_hTargetEnt->IsPlayer();

	if (!IsLeader() && fLeaderIsFollowing != fImFollowing)
	{
		AILogger->debug("Squad Member is not leader, and following state doesn't match in MakeEnemy");
		return;
	}

	for (auto& squadMemberHandle : squadLeader->m_hSquadMember)
	{
		COFSquadTalkMonster* squadMember = squadMemberHandle;

		if (squadMember)
		{
			const bool isFollowing = squadMember->m_hTargetEnt != nullptr && squadMember->m_hTargetEnt->IsPlayer();

			// reset members who aren't activly engaged in fighting
			if (fLeaderIsFollowing == isFollowing && squadMember->m_hEnemy != pEnemy && !squadMember->HasConditions(bits_COND_SEE_ENEMY))
			{
				if (squadMember->m_hEnemy != nullptr)
				{
					// remember their current enemy
					squadMember->PushEnemy(squadMember->m_hEnemy, squadMember->m_vecEnemyLKP);
				}

				AILogger->debug("Non-Squad friendly grunt adopted enemy of type {}", STRING(pEnemy->pev->classname));

				// give them a new enemy
				squadMember->m_hEnemy = pEnemy;
				squadMember->m_vecEnemyLKP = pEnemy->pev->origin;
				squadMember->SetConditions(bits_COND_NEW_ENEMY);
			}
		}
	}

	// Seems a bit redundant to recalculate this now
	const bool leaderIsStillFollowing = squadLeader->m_hTargetEnt != nullptr && squadLeader->m_hTargetEnt->IsPlayer();

	// reset members who aren't activly engaged in fighting
	if (fLeaderIsFollowing == leaderIsStillFollowing && squadLeader->m_hEnemy != pEnemy && !squadLeader->HasConditions(bits_COND_SEE_ENEMY))
	{
		if (squadLeader->m_hEnemy != nullptr)
		{
			// remember their current enemy
			squadLeader->PushEnemy(squadLeader->m_hEnemy, squadLeader->m_vecEnemyLKP);
		}

		AILogger->debug("Non-Squad friendly grunt adopted enemy of type {}", STRING(pEnemy->pev->classname));

		// give them a new enemy
		squadLeader->m_hEnemy = pEnemy;
		squadLeader->m_vecEnemyLKP = pEnemy->pev->origin;
		squadLeader->SetConditions(bits_COND_NEW_ENEMY);
	}

	// reset members who aren't activly engaged in fighting
	if (squadLeader->m_hEnemy != pEnemy && !squadLeader->HasConditions(bits_COND_SEE_ENEMY))
	{
		if (squadLeader->m_hEnemy != nullptr)
		{
			// remember their current enemy
			squadLeader->PushEnemy(squadLeader->m_hEnemy, squadLeader->m_vecEnemyLKP);
		}

		AILogger->debug("Squad Leader friendly grunt adopted enemy of type {}", STRING(pEnemy->pev->classname));

		// give them a new enemy
		squadLeader->m_hEnemy = pEnemy;
		squadLeader->m_vecEnemyLKP = pEnemy->pev->origin;
		squadLeader->SetConditions(bits_COND_NEW_ENEMY);
	}
}

int COFSquadTalkMonster::SquadCount()
{
	if (!InSquad())
		return 0;

	COFSquadTalkMonster* pSquadLeader = MySquadLeader();
	int squadCount = 0;
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		if (pSquadLeader->MySquadMember(i) != nullptr)
			squadCount++;
	}

	return squadCount;
}

int COFSquadTalkMonster::SquadRecruit(int searchRadius, int maxMembers)
{
	int squadCount;
	int iMyClass = Classify(); // cache this monster's class


	// Don't recruit if I'm already in a group
	if (InSquad())
		return 0;

	if (maxMembers < 2)
		return 0;

	// I am my own leader
	m_hSquadLeader = this;
	squadCount = 1;

	CBaseEntity* pEntity = nullptr;

	if (!FStringNull(pev->netname))
	{
		// I have a netname, so unconditionally recruit everyone else with that name.
		pEntity = UTIL_FindEntityByString(pEntity, "netname", STRING(pev->netname));
		while (pEntity)
		{
			COFSquadTalkMonster* pRecruit = pEntity->MySquadTalkMonsterPointer();

			if (pRecruit)
			{
				if (!pRecruit->InSquad() && pRecruit->Classify() == iMyClass && pRecruit != this)
				{
					// minimum protection here against user error.in worldcraft.
					if (!SquadAdd(pRecruit))
						break;
					squadCount++;
				}
			}

			pEntity = UTIL_FindEntityByString(pEntity, "netname", STRING(pev->netname));
		}
	}
	else
	{
		while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, searchRadius)) != nullptr)
		{
			COFSquadTalkMonster* pRecruit = pEntity->MySquadTalkMonsterPointer();

			if (pRecruit && pRecruit != this && pRecruit->IsAlive() && !pRecruit->m_pCine)
			{
				// Can we recruit this guy?
				if (!pRecruit->InSquad() && pRecruit->Classify() == iMyClass &&
					((iMyClass != CLASS_ALIEN_MONSTER) || FStrEq(STRING(pev->classname), STRING(pRecruit->pev->classname))) &&
					FStringNull(pRecruit->pev->netname))
				{
					TraceResult tr;
					UTIL_TraceLine(pev->origin + pev->view_ofs, pRecruit->pev->origin + pev->view_ofs, ignore_monsters, pRecruit->edict(), &tr); // try to hit recruit with a traceline.
					if (tr.flFraction == 1.0)
					{
						if (!SquadAdd(pRecruit))
							break;

						squadCount++;
					}
				}
			}
		}
	}

	// no single member squads
	if (squadCount == 1)
	{
		m_hSquadLeader = nullptr;
	}

	return squadCount;
}

bool COFSquadTalkMonster::CheckEnemy(CBaseEntity* pEnemy)
{
	bool iUpdatedLKP;

	iUpdatedLKP = CTalkMonster::CheckEnemy(m_hEnemy);

	// communicate with squad members about the enemy IF this individual has the same enemy as the squad leader.
	if (InSquad() && m_hEnemy == MySquadLeader()->m_hEnemy)
	{
		if (iUpdatedLKP)
		{
			// have new enemy information, so paste to the squad.
			SquadPasteEnemyInfo();
		}
		else
		{
			// enemy unseen, copy from the squad knowledge.
			SquadCopyEnemyInfo();
		}
	}

	return iUpdatedLKP;
}

void COFSquadTalkMonster::StartMonster()
{
	CTalkMonster::StartMonster();

	if ((m_afCapability & bits_CAP_SQUAD) != 0 && !InSquad())
	{
		if (!FStringNull(pev->netname))
		{
			// if I have a groupname, I can only recruit if I'm flagged as leader
			if ((pev->spawnflags & SF_SQUADMONSTER_LEADER) == 0)
			{
				return;
			}
		}

		// try to form squads now.
		int iSquadSize = SquadRecruit(1024, 4);

		if (0 != iSquadSize)
		{
			AILogger->debug("Squad of {} {} formed", iSquadSize, STRING(pev->classname));
		}
	}

	m_flLastHitByPlayer = gpGlobals->time;
	m_iPlayerHits = 0;
}

bool COFSquadTalkMonster::NoFriendlyFire()
{
	if (!InSquad())
	{
		return true;
	}

	CPlane backPlane;
	CPlane leftPlane;
	CPlane rightPlane;

	Vector vecLeftSide;
	Vector vecRightSide;
	Vector v_left;

	//!!!BUGBUG - to fix this, the planes must be aligned to where the monster will be firing its gun, not the direction it is facing!!!

	if (m_hEnemy != nullptr)
	{
		UTIL_MakeVectors(UTIL_VecToAngles(m_hEnemy->Center() - pev->origin));
	}
	else
	{
		// if there's no enemy, pretend there's a friendly in the way, so the grunt won't shoot.
		return false;
	}

	// UTIL_MakeVectors ( pev->angles );

	vecLeftSide = pev->origin - (gpGlobals->v_right * (pev->size.x * 1.5));
	vecRightSide = pev->origin + (gpGlobals->v_right * (pev->size.x * 1.5));
	v_left = gpGlobals->v_right * -1;

	leftPlane.InitializePlane(gpGlobals->v_right, vecLeftSide);
	rightPlane.InitializePlane(v_left, vecRightSide);
	backPlane.InitializePlane(gpGlobals->v_forward, pev->origin);

	/*
		AILogger->debug("LeftPlane: {} : {}", leftPlane.m_vecNormal, leftPlane.m_flDist);
		AILogger->debug("RightPlane: {} : {}", rightPlane.m_vecNormal, rightPlane.m_flDist);
		AILogger->debug("BackPlane: {} : {}", backPlane.m_vecNormal, backPlane.m_flDist);
	*/

	COFSquadTalkMonster* pSquadLeader = MySquadLeader();
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		COFSquadTalkMonster* pMember = pSquadLeader->MySquadMember(i);
		if (pMember && pMember != this)
		{

			if (backPlane.PointInFront(pMember->pev->origin) &&
				leftPlane.PointInFront(pMember->pev->origin) &&
				rightPlane.PointInFront(pMember->pev->origin))
			{
				// this guy is in the check volume! Don't shoot!
				return false;
			}
		}
	}

	return true;
}

MONSTERSTATE COFSquadTalkMonster::GetIdealState()
{
	int iConditions;

	iConditions = IScheduleFlags();

	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if (HasConditions(bits_COND_NEW_ENEMY) && InSquad())
		{
			SquadMakeEnemy(m_hEnemy);
		}
		break;
	}

	return CTalkMonster::GetIdealState();
}

bool COFSquadTalkMonster::FValidateCover(const Vector& vecCoverLocation)
{
	if (!InSquad())
	{
		return true;
	}

	if (SquadMemberInRange(vecCoverLocation, 128))
	{
		// another squad member is too close to this piece of cover.
		return false;
	}

	return true;
}

bool COFSquadTalkMonster::SquadEnemySplit()
{
	if (!InSquad())
		return false;

	COFSquadTalkMonster* pSquadLeader = MySquadLeader();
	CBaseEntity* pEnemy = pSquadLeader->m_hEnemy;

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		COFSquadTalkMonster* pMember = pSquadLeader->MySquadMember(i);
		if (pMember != nullptr && pMember->m_hEnemy != nullptr && pMember->m_hEnemy != pEnemy)
		{
			return true;
		}
	}
	return false;
}

bool COFSquadTalkMonster::SquadMemberInRange(const Vector& vecLocation, float flDist)
{
	if (!InSquad())
		return false;

	COFSquadTalkMonster* pSquadLeader = MySquadLeader();

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		COFSquadTalkMonster* pSquadMember = pSquadLeader->MySquadMember(i);
		if (nullptr != pSquadMember && (vecLocation - pSquadMember->pev->origin).Length2D() <= flDist)
			return true;
	}
	return false;
}

extern Schedule_t slChaseEnemyFailed[];

Schedule_t* COFSquadTalkMonster::GetScheduleOfType(int iType)
{
	switch (iType)
	{

	case SCHED_CHASE_ENEMY_FAILED:
	{
		return &slChaseEnemyFailed[0];
	}

	default:
		return CTalkMonster::GetScheduleOfType(iType);
	}
}

void COFSquadTalkMonster::FollowerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
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
			// Player can form squads of up to 6 NPCs
			LimitFollowers(pCaller, 6);

			if ((m_afMemory & bits_MEMORY_PROVOKED) != 0)
				AILogger->debug("I'm not following you, you evil person!");
			else
			{
				StartFollowing(pCaller);
				SetBits(m_bitsSaid, bit_saidHelloPlayer); // Don't say hi after you've started following
			}
		}
		else
		{
			StopFollowing(true);
		}
	}
}

COFSquadTalkMonster* COFSquadTalkMonster::MySquadMedic()
{
	for (auto& member : m_hSquadMember)
	{
		COFSquadTalkMonster* pMember = member;

		if (pMember && pMember->ClassnameIs("monster_human_medic_ally"))
		{
			return pMember;
		}
	}

	return nullptr;
}

COFSquadTalkMonster* COFSquadTalkMonster::FindSquadMedic(int searchRadius)
{
	for (CBaseEntity* pEntity = nullptr; (pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, searchRadius));)
	{
		auto pMonster = pEntity->MySquadTalkMonsterPointer();

		if (pMonster && pMonster != this && pMonster->IsAlive() && !pMonster->m_pCine && pMonster->ClassnameIs("monster_human_medic_ally"))
		{
			return pMonster;
		}
	}

	return nullptr;
}

bool COFSquadTalkMonster::HealMe(COFSquadTalkMonster* pTarget)
{
	return false;
}

bool COFSquadTalkMonster::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (m_MonsterState == MONSTERSTATE_SCRIPT)
	{
		return CTalkMonster::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);
	}

	// If this attack deals enough damage to instakill me...
	if (pev->deadflag == DEAD_NO && flDamage >= pev->max_health)
	{
		// Tell my squad mates...
		auto pSquadLeader = MySquadLeader();

		for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
		{
			COFSquadTalkMonster* pSquadMember = pSquadLeader->MySquadMember(i);

			// If they're alive and have no enemy...
			if (pSquadMember && pSquadMember->IsAlive() && !pSquadMember->m_hEnemy)
			{
				// If they're not being eaten by a barnacle and the attacker is a player...
				if (m_MonsterState != MONSTERSTATE_PRONE && (attacker->pev->flags & FL_CLIENT) != 0)
				{
					// Friendly fire!
					pSquadMember->Remember(bits_MEMORY_PROVOKED);
				}
				// Attacked by an NPC...
				else
				{
					g_vecAttackDir = ((attacker->pev->origin + attacker->pev->view_ofs) - (pSquadMember->pev->origin + pSquadMember->pev->view_ofs)).Normalize();

					const Vector vecStart = pSquadMember->pev->origin + pSquadMember->pev->view_ofs;
					const Vector vecEnd = attacker->pev->origin + attacker->pev->view_ofs + (g_vecAttackDir * m_flDistLook);
					TraceResult tr;

					UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, pSquadMember->edict(), &tr);

					// If they didn't see any enemy...
					if (tr.flFraction == 1.0)
					{
						// Hunt for enemies
						m_IdealMonsterState = MONSTERSTATE_HUNT;
					}
					// They can see an enemy
					else
					{
						// Make the enemy an enemy of my squadmate
						pSquadMember->m_hEnemy = CBaseEntity::Instance(tr.pHit);
						pSquadMember->m_vecEnemyLKP = attacker->pev->origin;
						pSquadMember->SetConditions(bits_COND_NEW_ENEMY);
					}
				}
			}
		}
	}

	m_flWaitFinished = gpGlobals->time;

	return CTalkMonster::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);
}
