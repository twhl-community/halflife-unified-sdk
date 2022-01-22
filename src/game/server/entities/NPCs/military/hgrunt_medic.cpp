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
//=========================================================
// hgrunt
//=========================================================

//=========================================================
// Hit groups!
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include "cbase.h"
#include "squadmonster.h"
#include "weapons.h"
#include "talkmonster.h"
#include "COFSquadTalkMonster.h"
#include "effects.h"
#include "customentity.h"
#include "hgrunt_ally_base.h"

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define MEDIC_DEAGLE_CLIP_SIZE 9 // how many bullets in a clip?
#define MEDIC_GLOCK_CLIP_SIZE 9	 // how many bullets in a clip?

namespace MedicAllyBodygroup
{
enum MedicAllyBodygroup
{
	Head = 2,
	Weapons = 3
};
}

namespace MedicAllyHead
{
enum MedicAllyHead
{
	Default = -1,
	White = 0,
	Black
};
}

namespace MedicAllyWeapon
{
enum MedicAllyWeapon
{
	DesertEagle = 0,
	Glock,
	Needle,
	None
};
}

namespace MedicAllyWeaponFlag
{
enum MedicAllyWeaponFlag
{
	DesertEagle = 1 << 0,
	Glock = 1 << 1,
	Needle = 1 << 2,
	HandGrenade = 1 << 3,
};
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define MEDIC_AE_HOLSTER_GUN 15
#define MEDIC_AE_EQUIP_NEEDLE 16
#define MEDIC_AE_HOLSTER_NEEDLE 17
#define MEDIC_AE_EQUIP_GUN 18

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_MEDIC_ALLY_HEAL_ALLY = LAST_GRUNT_SCHEDULE + 1,
};

class COFMedicAlly : public CBaseHGruntAlly
{
public:
	void Spawn() override;
	void Precache() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;
	void Shoot();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	Schedule_t* GetScheduleOfType(int Type) override;

	int ObjectCaps() override;

	void Killed(entvars_t* pevAttacker, int iGib) override;

	void MonsterThink() override;

	bool HealMe(COFSquadTalkMonster* pTarget) override;

	void HealOff();

	void EXPORT HealerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void HealerActivate(CBaseMonster* pTarget);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	int m_iHealCharge;
	bool m_fUseHealing;
	bool m_fHealing;

	float m_flLastUseTime;

	bool m_fHealAudioPlaying;

	float m_flFollowCheckTime;
	bool m_fFollowChecking;
	bool m_fFollowChecked;

	float m_flLastRejectAudio;

	bool m_fHealActive;

	float m_flLastShot;

	int m_iBrassShell; //TODO: no shell is being ejected atm

protected:
	void DropWeapon(bool applyVelocity) override;

	std::tuple<int, Activity> GetSequenceForActivity(Activity NewActivity) override;

	Schedule_t* GetHealSchedule() override;
};

LINK_ENTITY_TO_CLASS(monster_human_medic_ally, COFMedicAlly);

TYPEDESCRIPTION COFMedicAlly::m_SaveData[] =
	{
		DEFINE_FIELD(COFMedicAlly, m_flFollowCheckTime, FIELD_FLOAT),
		DEFINE_FIELD(COFMedicAlly, m_fFollowChecking, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_fFollowChecked, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_flLastRejectAudio, FIELD_FLOAT),
		DEFINE_FIELD(COFMedicAlly, m_iHealCharge, FIELD_INTEGER),
		DEFINE_FIELD(COFMedicAlly, m_fUseHealing, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_fHealing, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_flLastUseTime, FIELD_TIME),
		DEFINE_FIELD(COFMedicAlly, m_fHealActive, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_flLastShot, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(COFMedicAlly, CBaseHGruntAlly);

void COFMedicAlly::DropWeapon(bool applyVelocity)
{
	if (GetBodygroup(MedicAllyBodygroup::Weapons) != MedicAllyWeapon::None)
	{ // throw a gun if the grunt has one

		Vector vecGunPos, vecGunAngles;
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pGun = nullptr;

		if ((pev->weapons & MedicAllyWeaponFlag::Glock) != 0)
		{
			pGun = DropItem("weapon_9mmhandgun", vecGunPos, vecGunAngles);
		}
		else if ((pev->weapons & MedicAllyWeaponFlag::DesertEagle) != 0)
		{
			pGun = DropItem("weapon_eagle", vecGunPos, vecGunAngles);
		}

		if (pGun && applyVelocity)
		{
			pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
			pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
		}

		SetBodygroup(MedicAllyBodygroup::Weapons, MedicAllyWeapon::None);
	}
}

//=========================================================
// Shoot
//=========================================================
void COFMedicAlly::Shoot()
{
	//Limit fire rate
	if (m_hEnemy == nullptr || gpGlobals->time - m_flLastShot <= 0.11)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	const char* pszSoundName = nullptr;

	if ((pev->weapons & MedicAllyWeaponFlag::Glock) != 0)
	{
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM); // shoot +-5 degrees
		pszSoundName = "weapons/pl_gun3.wav";
	}
	else if ((pev->weapons & MedicAllyWeaponFlag::DesertEagle) != 0)
	{
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_PLAYER_357); // shoot +-5 degrees
		pszSoundName = "weapons/desert_eagle_fire.wav";
	}

	if (pszSoundName)
	{
		const auto random = RANDOM_LONG(0, 20);

		const auto pitch = random <= 10 ? random + 95 : 100;

		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pszSoundName, VOL_NORM, ATTN_NORM, 0, pitch);
	}

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--; // take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);

	m_flLastShot = gpGlobals->time;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void COFMedicAlly::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case HGRUNT_AE_RELOAD:

		if ((pev->weapons & MedicAllyWeaponFlag::DesertEagle) != 0)
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/desert_eagle_reload.wav", 1, ATTN_NORM);
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM);
		}

		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case HGRUNT_AE_BURST1:
	{
		Shoot();
	}
	break;

	case MEDIC_AE_HOLSTER_GUN:
		SetBodygroup(MedicAllyBodygroup::Weapons, MedicAllyWeapon::None);
		break;

	case MEDIC_AE_EQUIP_NEEDLE:
		SetBodygroup(MedicAllyBodygroup::Weapons, MedicAllyWeapon::Needle);
		break;

	case MEDIC_AE_HOLSTER_NEEDLE:
		SetBodygroup(MedicAllyBodygroup::Weapons, MedicAllyWeapon::None);
		break;

	case MEDIC_AE_EQUIP_GUN:
		SetBodygroup(MedicAllyBodygroup::Weapons, (pev->weapons & MedicAllyWeaponFlag::Glock) != 0 ? MedicAllyWeapon::Glock : MedicAllyWeapon::DesertEagle);
		break;

	default:
		CBaseHGruntAlly::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void COFMedicAlly::Spawn()
{
	SpawnCore("models/hgrunt_medic.mdl", gSkillData.medicAllyHealth);

	m_flLastUseTime = 0;
	m_iHealCharge = gSkillData.medicAllyHeal;
	m_fHealActive = false;
	m_fUseHealing = false;
	m_fHealing = false;
	m_fFollowChecked = false;
	m_fFollowChecking = false;

	if (0 == pev->weapons)
	{
		pev->weapons |= MedicAllyWeaponFlag::Glock;
	}

	if (m_iGruntHead == MedicAllyHead::Default)
	{
		m_iGruntHead = RANDOM_LONG(0, 99) % 2 == 0 ? MedicAllyHead::White : MedicAllyHead::Black;
	}

	int weaponIndex = 0;

	if ((pev->weapons & MedicAllyWeaponFlag::Glock) != 0)
	{
		weaponIndex = MedicAllyWeapon::Glock;
		m_cClipSize = MEDIC_GLOCK_CLIP_SIZE;
	}
	else if ((pev->weapons & MedicAllyWeaponFlag::DesertEagle) != 0)
	{
		weaponIndex = MedicAllyWeapon::DesertEagle;
		m_cClipSize = MEDIC_DEAGLE_CLIP_SIZE;
	}
	else if ((pev->weapons & MedicAllyWeaponFlag::Needle) != 0)
	{
		weaponIndex = MedicAllyWeapon::Needle;
		m_cClipSize = 1;
	}
	else
	{
		weaponIndex = MedicAllyWeapon::None;
		m_cClipSize = 0;
	}

	SetBodygroup(MedicAllyBodygroup::Head, m_iGruntHead);
	SetBodygroup(MedicAllyBodygroup::Weapons, weaponIndex);

	m_cAmmoLoaded = m_cClipSize;

	m_flLastShot = gpGlobals->time;

	if (m_iGruntHead == MedicAllyHead::Black)
	{
		m_voicePitch = 95;
	}

	SetUse(&COFMedicAlly::HealerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COFMedicAlly::Precache()
{
	PRECACHE_MODEL("models/hgrunt_medic.mdl");

	PRECACHE_SOUND("weapons/desert_eagle_fire.wav");
	PRECACHE_SOUND("weapons/desert_eagle_reload.wav");

	PRECACHE_SOUND("fgrunt/medic_give_shot.wav");
	PRECACHE_SOUND("fgrunt/medical.wav");

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");

	CBaseHGruntAlly::Precache();

	// get voice pitch
	if (!UTIL_IsRestoring())
	{
		m_voicePitch = 105;
	}
}

//=========================================================
// start task
//=========================================================
void COFMedicAlly::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK2:
	{
		m_IdealActivity = ACT_MELEE_ATTACK2;

		if (!m_fHealAudioPlaying)
		{
			EMIT_SOUND(edict(), CHAN_WEAPON, "fgrunt/medic_give_shot.wav", VOL_NORM, ATTN_NORM);
			m_fHealAudioPlaying = true;
		}
		break;
	}

	case TASK_WAIT_FOR_MOVEMENT:
	{
		if (!m_fHealing)
			return CBaseHGruntAlly::StartTask(pTask);

		if (m_hTargetEnt)
		{
			auto pTarget = m_hTargetEnt.Entity<CBaseEntity>();
			auto pTargetMonster = pTarget->MySquadTalkMonsterPointer();

			if (pTargetMonster)
				pTargetMonster->m_hWaitMedic = nullptr;

			m_fHealing = false;
			m_fUseHealing = false;

			STOP_SOUND(edict(), CHAN_WEAPON, "fgrunt/medic_give_shot.wav");

			m_fFollowChecked = false;
			m_fFollowChecking = false;

			if (m_movementGoal == MOVEGOAL_TARGETENT)
				RouteClear();

			m_hTargetEnt = nullptr;

			m_fHealActive = false;

			return CBaseHGruntAlly::StartTask(pTask);
		}

		m_fHealing = false;
		m_fUseHealing = false;

		STOP_SOUND(edict(), CHAN_WEAPON, "fgrunt/medic_give_shot.wav");

		m_fFollowChecked = false;
		m_fFollowChecking = false;

		if (m_movementGoal == MOVEGOAL_TARGETENT)
			RouteClear();

		m_IdealActivity = ACT_DISARM;
		m_fHealActive = false;
		break;
	}

	default:
		CBaseHGruntAlly::StartTask(pTask);
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void COFMedicAlly::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK2:
	{
		if (m_fSequenceFinished)
		{
			if (m_fUseHealing)
			{
				if (gpGlobals->time - m_flLastUseTime > 0.3)
					m_Activity = ACT_RESET;
			}
			else
			{
				m_fHealActive = true;

				if (m_hTargetEnt)
				{
					auto pHealTarget = m_hTargetEnt.Entity<CBaseEntity>();

					const auto toHeal = V_min(5, pHealTarget->pev->max_health - pHealTarget->pev->health);

					if (toHeal != 0 && pHealTarget->TakeHealth(toHeal, DMG_GENERIC))
					{
						m_iHealCharge -= toHeal;
					}
					else
					{
						m_Activity = ACT_RESET;
					}
				}
				else
				{
					m_Activity = m_fHealing ? ACT_MELEE_ATTACK2 : ACT_RESET;
				}
			}

			TaskComplete();
		}

		break;
	}

	default:
	{
		CBaseHGruntAlly::RunTask(pTask);
		break;
	}
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Task_t tlMedicAllyNewHealTarget[] =
	{
		{TASK_SET_FAIL_SCHEDULE, SCHED_TARGET_CHASE},
		{TASK_MOVE_TO_TARGET_RANGE, 50},
		{TASK_FACE_IDEAL, 0},
		{TASK_GRUNT_SPEAK_SENTENCE, 0},
};

Schedule_t slMedicAllyNewHealTarget[] =
	{
		{tlMedicAllyNewHealTarget,
			ARRAYSIZE(tlMedicAllyNewHealTarget),
			0,
			0,
			"Draw Needle"},
};

Task_t tlMedicAllyDrawNeedle[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_PLAY_SEQUENCE_FACE_TARGET, ACT_ARM},
		{TASK_SET_FAIL_SCHEDULE, SCHED_TARGET_CHASE},
		{TASK_MOVE_TO_TARGET_RANGE, 50},
		{
			TASK_FACE_IDEAL,
			0,
		},
		{TASK_GRUNT_SPEAK_SENTENCE, 0}};

Schedule_t slMedicAllyDrawNeedle[] =
	{
		{tlMedicAllyDrawNeedle,
			ARRAYSIZE(tlMedicAllyDrawNeedle),
			0,
			0,
			"Draw Needle"},
};

Task_t tlMedicAllyDrawGun[] =
	{
		{TASK_PLAY_SEQUENCE, ACT_DISARM},
		{TASK_WAIT_FOR_MOVEMENT, 0},
};

Schedule_t slMedicAllyDrawGun[] =
	{
		{tlMedicAllyDrawGun,
			ARRAYSIZE(tlMedicAllyDrawGun),
			0,
			0,
			"Draw Gun"},
};

Task_t tlMedicAllyHealTarget[] =
	{
		{TASK_MELEE_ATTACK2, 0},
		{TASK_WAIT, 0.2f},
		{TASK_TLK_HEADRESET, 0},
};

Schedule_t slMedicAllyHealTarget[] =
	{
		{tlMedicAllyHealTarget,
			ARRAYSIZE(tlMedicAllyHealTarget),
			0,
			0,
			"Medic Ally Heal Target"},
};

DEFINE_CUSTOM_SCHEDULES(COFMedicAlly){
	slMedicAllyNewHealTarget,
	slMedicAllyDrawNeedle,
	slMedicAllyDrawGun,
	slMedicAllyHealTarget,
};

IMPLEMENT_CUSTOM_SCHEDULES(COFMedicAlly, CBaseHGruntAlly);

std::tuple<int, Activity> COFMedicAlly::GetSequenceForActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (m_fStanding)
		{
			// get aimable sequence
			iSequence = LookupSequence("standing_mp5");
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence("crouching_mp5");
		}
		break;
	default:
		return CBaseHGruntAlly::GetSequenceForActivity(NewActivity);
	}

	return {iSequence, NewActivity};
}

Schedule_t* COFMedicAlly::GetHealSchedule()
{
	if (m_fHealing)
	{
		if (m_hTargetEnt)
		{
			auto pHealTarget = m_hTargetEnt.Entity<CBaseEntity>();

			if ((pHealTarget->pev->origin - pev->origin).Make2D().Length() <= 50.0 && (!m_fUseHealing || gpGlobals->time - m_flLastUseTime <= 0.25) && 0 != m_iHealCharge && pHealTarget->IsAlive() && pHealTarget->pev->health != pHealTarget->pev->max_health)
			{
				return slMedicAllyHealTarget;
			}
		}

		return slMedicAllyDrawGun;
	}

	return nullptr;
}

//=========================================================
//=========================================================
Schedule_t* COFMedicAlly::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_MEDIC_ALLY_HEAL_ALLY:
		return slMedicAllyHealTarget;

	default:
		return CBaseHGruntAlly::GetScheduleOfType(Type);
	}
}

int COFMedicAlly::ObjectCaps()
{
	//Allow healing the player by continuously using
	return FCAP_ACROSS_TRANSITION | FCAP_CONTINUOUS_USE;
}

void COFMedicAlly::Killed(entvars_t* pevAttacker, int iGib)
{
	//Clear medic handle from patient
	if (m_hTargetEnt != nullptr)
	{
		auto pSquadMonster = m_hTargetEnt->MySquadTalkMonsterPointer();

		if (pSquadMonster)
			pSquadMonster->m_hWaitMedic = nullptr;
	}

	CBaseHGruntAlly::Killed(pevAttacker, iGib);
}

void COFMedicAlly::MonsterThink()
{
	//Check if we need to start following the player again after healing them
	if (m_fFollowChecking && !m_fFollowChecked && gpGlobals->time - m_flFollowCheckTime > 0.5)
	{
		m_fFollowChecking = false;

		//TODO: not suited for multiplayer
		auto pPlayer = UTIL_FindEntityByClassname(nullptr, "player");

		FollowerUse(pPlayer, pPlayer, USE_TOGGLE, 0);
	}

	CBaseHGruntAlly::MonsterThink();
}

bool COFMedicAlly::HealMe(COFSquadTalkMonster* pTarget)
{
	if (pTarget)
	{
		if (m_hTargetEnt && !m_hTargetEnt->IsPlayer())
		{
			auto pCurrentTarget = m_hTargetEnt->MySquadTalkMonsterPointer();

			if (pCurrentTarget && pCurrentTarget->MySquadLeader() == MySquadLeader())
			{
				return false;
			}

			if (pTarget->MySquadLeader() != MySquadLeader())
			{
				return false;
			}
		}

		if (m_MonsterState != MONSTERSTATE_COMBAT && 0 != m_iHealCharge)
		{
			HealerActivate(pTarget);
			return true;
		}
	}
	else
	{
		if (m_hTargetEnt)
		{
			auto target = m_hTargetEnt->MySquadTalkMonsterPointer();
			if (target)
				target->m_hWaitMedic = nullptr;
		}

		m_hTargetEnt = nullptr;

		if (m_movementGoal == MOVEGOAL_TARGETENT)
			RouteClear();

		ClearSchedule();
		ChangeSchedule(slMedicAllyDrawGun);
	}

	return false;
}

void COFMedicAlly::HealOff()
{
	m_fHealing = false;

	if (m_movementGoal == MOVEGOAL_TARGETENT)
		RouteClear();

	m_hTargetEnt = nullptr;
	ClearSchedule();

	SetThink(nullptr);
	pev->nextthink = 0;
}

void COFMedicAlly::HealerActivate(CBaseMonster* pTarget)
{
	if (m_hTargetEnt)
	{
		auto pMonster = m_hTargetEnt->MySquadTalkMonsterPointer();

		if (pMonster)
			pMonster->m_hWaitMedic = nullptr;

		//TODO: could just change the type of pTarget since this is the only type passed in
		auto pSquadTarget = static_cast<COFSquadTalkMonster*>(pTarget);

		pSquadTarget->m_hWaitMedic = this;

		m_hTargetEnt = pTarget;

		m_fHealing = false;

		ClearSchedule();

		ChangeSchedule(slMedicAllyNewHealTarget);
	}
	else if (m_iHealCharge > 0 && pTarget->IsAlive() && pTarget->pev->max_health > pTarget->pev->health && !m_fHealing)
	{
		if (m_hTargetEnt && m_hTargetEnt->IsPlayer())
		{
			StopFollowing(false);
		}

		m_hTargetEnt = pTarget;

		auto pMonster = pTarget->MySquadTalkMonsterPointer();

		if (pMonster)
			pMonster->m_hWaitMedic = this;

		m_fHealing = true;

		ClearSchedule();

		EMIT_SOUND(edict(), CHAN_VOICE, "fgrunt/medical.wav", VOL_NORM, ATTN_NORM);

		ChangeSchedule(slMedicAllyDrawNeedle);
	}
}

void COFMedicAlly::HealerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (m_useTime > gpGlobals->time || m_flLastUseTime > gpGlobals->time || pActivator == m_hEnemy)
	{
		return;
	}

	if (m_fFollowChecked || m_fFollowChecking)
	{
		if (!m_fFollowChecked && m_fFollowChecking)
		{
			if (gpGlobals->time - m_flFollowCheckTime < 0.3)
				return;

			m_fFollowChecked = true;
			m_fFollowChecking = false;
		}

		const auto newTarget = !m_fUseHealing && nullptr != m_hTargetEnt && m_fHealing;

		if (newTarget)
		{
			if (pActivator->pev->health >= pActivator->pev->max_health)
				return;

			m_fHealing = false;

			auto pMonster = m_hTargetEnt->MySquadTalkMonsterPointer();

			if (pMonster)
				pMonster->m_hWaitMedic = nullptr;
		}

		if (m_iHealCharge > 0 && pActivator->IsAlive() && pActivator->pev->max_health > pActivator->pev->health)
		{
			if (!m_fHealing)
			{
				if (m_hTargetEnt && m_hTargetEnt->IsPlayer())
				{
					StopFollowing(false);
				}

				m_hTargetEnt = pActivator;

				m_fHealing = true;
				m_fUseHealing = true;

				ClearSchedule();

				m_fHealAudioPlaying = false;

				if (newTarget)
				{
					ChangeSchedule(slMedicAllyNewHealTarget);
				}
				else
				{
					SENTENCEG_PlayRndSz(edict(), "MG_HEAL", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
					ChangeSchedule(slMedicAllyDrawNeedle);
				}
			}

			if (m_fHealActive)
			{
				if (pActivator->TakeHealth(2, DMG_GENERIC))
				{
					m_iHealCharge -= 2;
				}
			}
			else if (pActivator->TakeHealth(1, DMG_GENERIC))
			{
				--m_iHealCharge;
			}
		}
		else
		{
			m_fFollowChecked = false;
			m_fFollowChecking = false;

			if (gpGlobals->time - m_flLastRejectAudio > 4.0 && m_iHealCharge <= 0 && !m_fHealing)
			{
				m_flLastRejectAudio = gpGlobals->time;
				SENTENCEG_PlayRndSz(edict(), "MG_NOTHEAL", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
			}
		}

		m_flLastUseTime = gpGlobals->time + 0.2;
		return;
	}

	m_fFollowChecking = true;
	m_flFollowCheckTime = gpGlobals->time;
}

/**
*	@brief when triggered, spawns a monster_human_medic_ally repelling down a line.
*/
class COFMedicAllyRepel : public CBaseHGruntAllyRepel
{
protected:
	const char* GetMonsterClassname() const override { return "monster_human_medic_ally"; }
};

LINK_ENTITY_TO_CLASS(monster_medic_ally_repel, COFMedicAllyRepel);
