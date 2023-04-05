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
*	@brief for any monster that forms squads.
*/
class COFSquadTalkMonster : public CTalkMonster
{
	DECLARE_CLASS(COFSquadTalkMonster, CTalkMonster);
	DECLARE_DATAMAP();

public:
	// squad leader info
	EntityHandle<COFSquadTalkMonster> m_hSquadLeader; //!< who is my leader
	EntityHandle<COFSquadTalkMonster> m_hSquadMember[MAX_SQUAD_MEMBERS - 1]; //!< valid only for leader
	int m_afSquadSlots;
	float m_flLastEnemySightTime; //!< last time anyone in the squad saw the enemy
	bool m_fEnemyEluded;

	EntityHandle<COFSquadTalkMonster> m_hWaitMedic;
	float m_flMedicWaitTime;
	float m_flLastHitByPlayer;
	int m_iPlayerHits;

	// squad member info
	int m_iMySlot; //!< this is the behaviour slot that the monster currently holds in the squad.

	bool CheckEnemy(CBaseEntity* pEnemy) override;
	void StartMonster() override;
	void VacateSlot();
	void ScheduleChange() override;
	void Killed(CBaseEntity* attacker, int iGib) override;

	/**
	*	@brief if any slots of the passed slots are available, the monster will be assigned to one.
	*/
	bool OccupySlot(int iDesiredSlot);

	/**
	*	@brief checks for possibility of friendly fire
	*	@details Builds a large box in front of the grunt and checks to see
	*	if any squad members are in that box.
	*/
	bool NoFriendlyFire();

	// squad functions still left in base class
	COFSquadTalkMonster* MySquadLeader()
	{
		COFSquadTalkMonster* pSquadLeader = m_hSquadLeader;
		if (pSquadLeader != nullptr)
			return pSquadLeader;
		return this;
	}
	COFSquadTalkMonster* MySquadMember(int i)
	{
		if (i >= MAX_SQUAD_MEMBERS - 1)
			return this;
		else
			return m_hSquadMember[i];
	}
	bool InSquad() { return m_hSquadLeader != nullptr; }
	bool IsLeader() { return m_hSquadLeader == this; }

	/**
	*	@brief get some monsters of my classification and link them as a group.
	*	returns the group size
	*/
	int SquadRecruit(int searchRadius, int maxMembers);

	/**
	*	@brief return the number of members of this squad
	*	callable from leaders & followers
	*/
	int SquadCount();

	/**
	*	@brief remove pRemove from my squad.
	*	If I am pRemove, promote m_pSquadNext to leader
	*/
	void SquadRemove(COFSquadTalkMonster* pRemove);

	bool SquadAdd(COFSquadTalkMonster* pAdd);

	/**
	*	@brief makes everyone in the squad angry at the same entity.
	*/
	void SquadMakeEnemy(CBaseEntity* pEnemy);

	/**
	*	@brief called by squad members that have current info on the enemy so that it can be stored
	*	for members who don't have current info.
	*/
	void SquadPasteEnemyInfo();

	/**
	*	@brief called by squad members who don't have current info on the enemy.
	*	Reads from the same fields in the leader's data that other squad members write to,
	*	so the most recent data is always available here.
	*/
	void SquadCopyEnemyInfo();

	/**
	*	@brief returns true if not all squad members are fighting the same enemy.
	*/
	bool SquadEnemySplit();

	/**
	*	@brief determines whether or not the chosen cover location is a good one to move to.
	*	(currently based on proximity to others in the squad)
	*/
	bool SquadMemberInRange(const Vector& vecLocation, float flDist);

	COFSquadTalkMonster* MySquadTalkMonsterPointer() override { return this; }

	/**
	*	@brief determines whether or not the chosen cover location is a good one to move to.
	*	(currently based on proximity to others in the squad)
	*/
	bool FValidateCover(const Vector& vecCoverLocation) override;

	/**
	*	@brief surveys the Conditions information available and finds the best new state for a monster.
	*/
	MONSTERSTATE GetIdealState() override;
	Schedule_t* GetScheduleOfType(int iType) override;

	void FollowerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	COFSquadTalkMonster* MySquadMedic();

	COFSquadTalkMonster* FindSquadMedic(int searchRadius);

	virtual bool HealMe(COFSquadTalkMonster* pTarget);

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
};
