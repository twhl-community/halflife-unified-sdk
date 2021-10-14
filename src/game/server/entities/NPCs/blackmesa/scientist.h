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

#define		NUM_SCIENTIST_HEADS		4 // four heads available for scientist model
enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3 };

namespace ScientistBodygroup
{
enum ScientistBodygroup
{
	Body = 0,
	Head = 1,
	Needle = 2,
};
}

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED,
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
};

const int SF_SCIENTIST_NO_USE = 1 << 8;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL		( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

//=======================================================
// Scientist
//=======================================================

class CScientist : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;

	void SetYawSpeed() override;
	int  Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void RunTask(Task_t* pTask) override;
	void StartTask(Task_t* pTask) override;
	int	ObjectCaps() override { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	int FriendNumber(int arrayNumber) override;
	void SetActivity(Activity newActivity) override;
	Activity GetStoppedActivity() override;
	int ISoundMask() override;
	void DeclineFollowing() override;

	float	CoverRadius() override { return 1200; }		// Need more room for cover because scientists want to get far away!
	BOOL	DisregardEnemy(CBaseEntity* pEnemy) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 15; }

	BOOL	CanHeal();
	virtual void Heal();
	virtual void Scream();

	// Override these to set behavior
	Schedule_t* GetScheduleOfType(int Type) override;
	Schedule_t* GetSchedule() override;
	MONSTERSTATE GetIdealState() override;

	void DeathSound() override;
	void PainSound() override;

	void TalkInit() override;

	void			Killed(entvars_t* pevAttacker, int iGib) override;

	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

protected:
	float m_painTime;
	float m_healTime;
	float m_fearTime;

protected:
	/**
	*	@brief Spawns the Scientist
	*	@param model Must be a string literal
	*/
	void SpawnCore(const char* model, float health);

	/**
	*	@brief Precaches all of the Scientist's assets
	*	@param model Must be a string literal
	*/
	void PrecacheCore(const char* model);
};

/**
*	@brief Sitting Scientist PROP
*/
class CSittingScientist : public CScientist // kdb: changed from public CBaseMonster so he can speak
{
public:
	void Spawn() override;
	void  Precache() override;

	void EXPORT SittingThink();
	int	Classify() override;
	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void SetAnswerQuestion(CTalkMonster* pSpeaker) override;
	int FriendNumber(int arrayNumber) override;

	int FIdleSpeak();
	int		m_baseSequence;
	int		m_headTurn;
	float	m_flResponseDelay;

protected:
	/**
	*	@brief Spawns the Scientist
	*	@param model Must be a string literal
	*/
	void SpawnCore(const char* model);
};
