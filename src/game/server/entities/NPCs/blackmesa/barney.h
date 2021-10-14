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

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )

namespace GuardBodyGroup
{
enum GuardBodyGroup
{
	Weapons = 1,
	Heads = 2
};
}

namespace GuardWeapon
{
enum GuardWeapon
{
	Random = -1,
	None = 0,
	Gun,
};
}

namespace GuardHead
{
enum GuardHead
{
	Random = -1,
	Default = 0,
};
}

class CBarney : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int  ISoundMask() override;
	virtual void GuardFirePistol();
	void AlertSound() override;
	int  Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void RunTask(Task_t* pTask) override;
	void StartTask(Task_t* pTask) override;
	int	ObjectCaps() override { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	BOOL CheckRangeAttack1(float flDot, float flDist) override;

	void DeclineFollowing() override;

	// Override these to set behavior
	Schedule_t* GetScheduleOfType(int Type) override;
	Schedule_t* GetSchedule() override;
	MONSTERSTATE GetIdealState() override;

	void DeathSound() override;
	void PainSound() override;

	void TalkInit() override;

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;

	void KeyValue(KeyValueData* pkvd) override;

	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;


	//Used during spawn to set initial values, not restored
	int m_iGuardBody;
	int m_iGuardHead;

	CUSTOM_SCHEDULES;

protected:
	/**
	*	@brief Precaches all Security Guard assets
	*	@param model Must be a string literal
	*/
	void PrecacheCore(const char* model);

	/**
	*	@brief Spawns the Security Guard
	*	@param model Must be a string literal
	*/
	void SpawnCore(const char* model, float health);

	virtual void DropWeapon();

	virtual void SpeakKilledEnemy();
};
