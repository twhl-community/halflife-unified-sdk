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
#include "squadmonster.h"
#include "plane.h"
#include "military/hgrunt.h"

BEGIN_DATAMAP(CSquadMonster)
DEFINE_FIELD(m_hSquadLeader, FIELD_EHANDLE),
	DEFINE_ARRAY(m_hSquadMember, FIELD_EHANDLE, MAX_SQUAD_MEMBERS - 1),

	// DEFINE_FIELD(m_afSquadSlots, FIELD_INTEGER), // these need to be reset after transitions!
	DEFINE_FIELD(m_fEnemyEluded, FIELD_BOOLEAN),
	DEFINE_FIELD(m_flLastEnemySightTime, FIELD_TIME),

	DEFINE_FIELD(m_iMySlot, FIELD_INTEGER),
	END_DATAMAP();

bool CSquadMonster::OccupySlot(int iDesiredSlots)
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

	CSquadMonster* pSquadLeader = MySquadLeader();

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

void CSquadMonster::VacateSlot()
{
	if (m_iMySlot != bits_NO_SLOT && InSquad())
	{
		//		AILogger->debug("Vacated Slot {} - {}", m_iMySlot, m_hSquadLeader->m_afSquadSlots);
		MySquadLeader()->m_afSquadSlots &= ~m_iMySlot;
		m_iMySlot = bits_NO_SLOT;
	}
}

void CSquadMonster::ScheduleChange()
{
	VacateSlot();
}

void CSquadMonster::Killed(CBaseEntity* attacker, int iGib)
{
	VacateSlot();

	if (InSquad())
	{
		MySquadLeader()->SquadRemove(this);
	}

	CBaseMonster::Killed(attacker, iGib);
}

// These functions are still awaiting conversion to CSquadMonster

void CSquadMonster::SquadRemove(CSquadMonster* pRemove)
{
	ASSERT(pRemove != nullptr);
	ASSERT(this->IsLeader());
	ASSERT(pRemove->m_hSquadLeader == this);

	// If I'm the leader, get rid of my squad
	if (pRemove == MySquadLeader())
	{
		for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++)
		{
			CSquadMonster* pMember = MySquadMember(i);
			if (pMember)
			{
				pMember->m_hSquadLeader = nullptr;
				m_hSquadMember[i] = nullptr;
			}
		}
	}
	else
	{
		CSquadMonster* pSquadLeader = MySquadLeader();
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

bool CSquadMonster::SquadAdd(CSquadMonster* pAdd)
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

void CSquadMonster::SquadPasteEnemyInfo()
{
	CSquadMonster* pSquadLeader = MySquadLeader();
	if (pSquadLeader)
		pSquadLeader->m_vecEnemyLKP = m_vecEnemyLKP;
}

void CSquadMonster::SquadCopyEnemyInfo()
{
	CSquadMonster* pSquadLeader = MySquadLeader();
	if (pSquadLeader)
		m_vecEnemyLKP = pSquadLeader->m_vecEnemyLKP;
}

void CSquadMonster::SquadMakeEnemy(CBaseEntity* pEnemy)
{
	if (!InSquad())
		return;

	if (!pEnemy)
	{
		AILogger->debug("ERROR: SquadMakeEnemy() - pEnemy is nullptr!");
		return;
	}

	CSquadMonster* pSquadLeader = MySquadLeader();
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster* pMember = pSquadLeader->MySquadMember(i);
		if (pMember)
		{
			// reset members who aren't activly engaged in fighting
			if (pMember->m_hEnemy != pEnemy && !pMember->HasConditions(bits_COND_SEE_ENEMY))
			{
				if (pMember->m_hEnemy != nullptr)
				{
					// remember their current enemy
					pMember->PushEnemy(pMember->m_hEnemy, pMember->m_vecEnemyLKP);
				}
				// give them a new enemy
				pMember->m_hEnemy = pEnemy;
				pMember->m_vecEnemyLKP = pEnemy->pev->origin;
				pMember->SetConditions(bits_COND_NEW_ENEMY);
			}
		}
	}
}

int CSquadMonster::SquadCount()
{
	if (!InSquad())
		return 0;

	CSquadMonster* pSquadLeader = MySquadLeader();
	int squadCount = 0;
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		if (pSquadLeader->MySquadMember(i) != nullptr)
			squadCount++;
	}

	return squadCount;
}

int CSquadMonster::SquadRecruit(int searchRadius, int maxMembers)
{
	int squadCount;
	const auto iMyClass = Classify(); // cache this monster's class


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
			CSquadMonster* pRecruit = pEntity->MySquadMonsterPointer();

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
			CSquadMonster* pRecruit = pEntity->MySquadMonsterPointer();

			if (pRecruit && pRecruit != this && pRecruit->IsAlive() && !pRecruit->m_pCine)
			{
				// Can we recruit this guy?
				if (!pRecruit->InSquad() && pRecruit->Classify() == iMyClass &&
					CanRecruit(pRecruit) &&
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

bool CSquadMonster::CheckEnemy(CBaseEntity* pEnemy)
{
	bool iUpdatedLKP;

	iUpdatedLKP = CBaseMonster::CheckEnemy(m_hEnemy);

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

void CSquadMonster::StartMonster()
{
	CBaseMonster::StartMonster();

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

		if (IsLeader() && ClassnameIs("monster_human_grunt"))
		{
			SetBodygroup(HGruntBodyGroup::Head, HGruntHead::Commander); // UNDONE: truly ugly hack
			pev->skin = 0;
		}
	}
}

bool CSquadMonster::NoFriendlyFire()
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

	CSquadMonster* pSquadLeader = MySquadLeader();
	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster* pMember = pSquadLeader->MySquadMember(i);
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

MONSTERSTATE CSquadMonster::GetIdealState()
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

	return CBaseMonster::GetIdealState();
}

bool CSquadMonster::FValidateCover(const Vector& vecCoverLocation)
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

bool CSquadMonster::SquadEnemySplit()
{
	if (!InSquad())
		return false;

	CSquadMonster* pSquadLeader = MySquadLeader();
	CBaseEntity* pEnemy = pSquadLeader->m_hEnemy;

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster* pMember = pSquadLeader->MySquadMember(i);
		if (pMember != nullptr && pMember->m_hEnemy != nullptr && pMember->m_hEnemy != pEnemy)
		{
			return true;
		}
	}
	return false;
}

bool CSquadMonster::SquadMemberInRange(const Vector& vecLocation, float flDist)
{
	if (!InSquad())
		return false;

	CSquadMonster* pSquadLeader = MySquadLeader();

	for (int i = 0; i < MAX_SQUAD_MEMBERS; i++)
	{
		CSquadMonster* pSquadMember = pSquadLeader->MySquadMember(i);
		if (nullptr != pSquadMember && (vecLocation - pSquadMember->pev->origin).Length2D() <= flDist)
			return true;
	}
	return false;
}

extern Schedule_t slChaseEnemyFailed[];

const Schedule_t* CSquadMonster::GetScheduleOfType(int iType)
{
	switch (iType)
	{

	case SCHED_CHASE_ENEMY_FAILED:
	{
		return &slChaseEnemyFailed[0];
	}

	default:
		return CBaseMonster::GetScheduleOfType(iType);
	}
}

bool CSquadMonster::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (flDamage >= pev->max_health)
	{
		auto squadLeader = MySquadLeader();

		for (int i = 0; i < MAX_SQUAD_MEMBERS - 1; ++i)
		{
			CSquadMonster* squadMember = squadLeader->m_hSquadMember[i];

			if (squadMember)
			{
				if (!squadMember->m_hEnemy)
				{
					g_vecAttackDir = ((attacker->pev->origin + attacker->pev->view_ofs) - (squadMember->pev->origin + squadMember->pev->view_ofs)).Normalize();

					const Vector vecStart = squadMember->pev->origin + squadMember->pev->view_ofs;
					const Vector vecEnd = attacker->pev->origin + attacker->pev->view_ofs + (g_vecAttackDir * m_flDistLook);

					TraceResult tr;

					UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, squadMember->edict(), &tr);

					if (tr.flFraction == 1.0)
					{
						m_IdealMonsterState = MONSTERSTATE_HUNT;
					}
					else
					{
						squadMember->m_hEnemy = CBaseEntity::Instance(tr.pHit);
						squadMember->m_vecEnemyLKP = attacker->pev->origin;
						squadMember->SetConditions(bits_COND_NEW_ENEMY);
					}
				}
			}
		}

		if (!squadLeader->m_hEnemy)
		{
			g_vecAttackDir = ((attacker->pev->origin + attacker->pev->view_ofs) - (squadLeader->pev->origin + squadLeader->pev->view_ofs)).Normalize();

			const Vector vecStart = squadLeader->pev->origin + squadLeader->pev->view_ofs;
			const Vector vecEnd = attacker->pev->origin + attacker->pev->view_ofs + (g_vecAttackDir * m_flDistLook);

			TraceResult tr;

			UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, squadLeader->edict(), &tr);

			if (tr.flFraction == 1.0)
			{
				m_IdealMonsterState = MONSTERSTATE_HUNT;
			}
			else
			{
				squadLeader->m_hEnemy = CBaseEntity::Instance(tr.pHit);
				squadLeader->m_vecEnemyLKP = attacker->pev->origin;
				squadLeader->SetConditions(bits_COND_NEW_ENEMY);
			}
		}
	}

	return CBaseMonster::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);
}
