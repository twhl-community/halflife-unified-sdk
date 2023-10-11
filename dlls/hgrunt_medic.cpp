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


#include "extdll.h"
#include "plane.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "defaultai.h"
#include "animation.h"
#include "squadmonster.h"
#include "weapons.h"
#include "talkmonster.h"
#include "COFAllyMonster.h"
#include "COFSquadTalkMonster.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"
#include "explode.h"

int g_fMedicAllyQuestion; // true if an idle grunt asked a question. Cleared when someone answers.

extern DLL_GLOBAL int g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define MEDIC_DEAGLE_CLIP_SIZE 9 // how many bullets in a clip?
#define MEDIC_GLOCK_CLIP_SIZE 9	 // how many bullets in a clip?
#define GRUNT_VOL 0.35			 // volume of grunt sounds
#define GRUNT_ATTN ATTN_NORM	 // attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH 20
#define HGRUNT_DMG_HEADSHOT (DMG_BULLET | DMG_CLUB) // damage types that can kill a grunt with a single headshot.
#define HGRUNT_NUM_HEADS 2							// how many grunt heads are there?
#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE 15			// must do at least this much damage in one shot to head to score a headshot kill
#define MEDIC_SENTENCE_VOLUME (float)0.35			// volume of grunt sentences
#define TORCH_BEAM_SPRITE "sprites/xbeam3.spr"

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
#define MEDIC_AE_RELOAD (2)
#define MEDIC_AE_KICK (3)
#define MEDIC_AE_SHOOT (4)
#define MEDIC_AE_GREN_TOSS (7)
#define MEDIC_AE_GREN_DROP (9)
#define MEDIC_AE_CAUGHT_ENEMY (10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define MEDIC_AE_DROP_GUN (11)	   // grunt (probably dead) is dropping his mp5.

#define MEDIC_AE_HOLSTER_GUN 15
#define MEDIC_AE_EQUIP_NEEDLE 16
#define MEDIC_AE_HOLSTER_NEEDLE 17
#define MEDIC_AE_EQUIP_GUN 18

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_MEDIC_ALLY_SUPPRESS = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_MEDIC_ALLY_ESTABLISH_LINE_OF_FIRE, // move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_MEDIC_ALLY_COVER_AND_RELOAD,
	SCHED_MEDIC_ALLY_SWEEP,
	SCHED_MEDIC_ALLY_FOUND_ENEMY,
	SCHED_MEDIC_ALLY_REPEL,
	SCHED_MEDIC_ALLY_REPEL_ATTACK,
	SCHED_MEDIC_ALLY_REPEL_LAND,
	SCHED_MEDIC_ALLY_WAIT_FACE_ENEMY,
	SCHED_MEDIC_ALLY_TAKECOVER_FAILED, // special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_MEDIC_ALLY_HEAL_ALLY,
	SCHED_MEDIC_ALLY_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_MEDIC_ALLY_FACE_TOSS_DIR = LAST_TALKMONSTER_TASK + 1,
	TASK_MEDIC_ALLY_SPEAK_SENTENCE,
	TASK_MEDIC_ALLY_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE (bits_COND_SPECIAL1)

class COFMedicAlly : public COFSquadTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	int ISoundMask() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	bool FCanCheckAttacks() override;
	bool CheckMeleeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack1(float flDot, float flDist) override;
	bool CheckRangeAttack2(float flDot, float flDist) override;
	void CheckAmmo() override;
	void SetActivity(Activity NewActivity) override;
	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;
	void DeathSound() override;
	void PainSound() override;
	void IdleSound() override;
	Vector GetGunPosition() override;
	void Shoot();
	void PrescheduleThink() override;
	void GibMonster() override;
	void SpeakSentence();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	CBaseEntity* Kick();
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;

	bool FOkToSpeak();
	void JustSpoke();

	int ObjectCaps() override;

	void TalkInit();

	void AlertSound() override;

	void DeclineFollowing() override;

	bool KeyValue(KeyValueData* pkvd) override;

	void Killed(entvars_t* pevAttacker, int iGib) override;

	void MonsterThink() override;

	bool HealMe(COFSquadTalkMonster* pTarget) override;

	void HealOff();

	void EXPORT HealerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void HealerActivate(CBaseMonster* pTarget);

	MONSTERSTATE GetIdealState() override
	{
		return COFSquadTalkMonster::GetIdealState();
	}

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	bool m_lastAttackCheck;
	float m_flPlayerDamage;

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector m_vecTossVelocity;

	bool m_fThrowGrenade;
	bool m_fStanding;
	bool m_fFirstEncounter; // only put on the handsign show in the squad's first encounter.
	int m_cClipSize;

	int m_iHealCharge;
	bool m_fUseHealing;
	bool m_fHealing;

	float m_flLastUseTime;

	EHANDLE m_hNewTargetEnt;

	bool m_fQueueFollow;
	bool m_fHealAudioPlaying;

	float m_flFollowCheckTime;
	bool m_fFollowChecking;
	bool m_fFollowChecked;

	float m_flLastRejectAudio;

	int m_iBlackOrWhite;

	bool m_fGunHolstered;
	bool m_fHypoHolstered;
	bool m_fHealActive;

	int m_iWeaponIdx;

	float m_flLastShot;

	int m_iBrassShell;

	int m_iSentence;

	static const char* pMedicSentences[];
};

LINK_ENTITY_TO_CLASS(monster_human_medic_ally, COFMedicAlly);

TYPEDESCRIPTION COFMedicAlly::m_SaveData[] =
	{
		DEFINE_FIELD(COFMedicAlly, m_lastAttackCheck, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_flNextGrenadeCheck, FIELD_TIME),
		DEFINE_FIELD(COFMedicAlly, m_flNextPainTime, FIELD_TIME),
		//	DEFINE_FIELD( COFMedicAlly, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
		DEFINE_FIELD(COFMedicAlly, m_vecTossVelocity, FIELD_VECTOR),
		DEFINE_FIELD(COFMedicAlly, m_fThrowGrenade, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_fStanding, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_fFirstEncounter, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_cClipSize, FIELD_INTEGER),
		//	DEFINE_FIELD( COFMedicAlly, m_voicePitch, FIELD_INTEGER ),
		//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
		//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
		DEFINE_FIELD(COFMedicAlly, m_iSentence, FIELD_INTEGER),
		DEFINE_FIELD(COFMedicAlly, m_flFollowCheckTime, FIELD_FLOAT),
		DEFINE_FIELD(COFMedicAlly, m_fFollowChecking, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_fFollowChecked, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_flLastRejectAudio, FIELD_FLOAT),
		DEFINE_FIELD(COFMedicAlly, m_iBlackOrWhite, FIELD_INTEGER),
		DEFINE_FIELD(COFMedicAlly, m_iHealCharge, FIELD_INTEGER),
		DEFINE_FIELD(COFMedicAlly, m_fUseHealing, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_fHealing, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_flLastUseTime, FIELD_TIME),
		DEFINE_FIELD(COFMedicAlly, m_hNewTargetEnt, FIELD_EHANDLE),
		DEFINE_FIELD(COFMedicAlly, m_fGunHolstered, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_fHypoHolstered, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_fHealActive, FIELD_BOOLEAN),
		DEFINE_FIELD(COFMedicAlly, m_iWeaponIdx, FIELD_INTEGER),
		DEFINE_FIELD(COFMedicAlly, m_flLastShot, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(COFMedicAlly, COFSquadTalkMonster);

const char* COFMedicAlly::pMedicSentences[] =
	{
		"FG_GREN",	  // grenade scared grunt
		"FG_ALERT",	  // sees player
		"FG_MONSTER", // sees monster
		"FG_COVER",	  // running to cover
		"FG_THROW",	  // about to throw grenade
		"FG_CHARGE",  // running out to get the enemy
		"FG_TAUNT",	  // say rude things
};

enum
{
	MEDIC_SENT_NONE = -1,
	MEDIC_SENT_GREN = 0,
	MEDIC_SENT_ALERT,
	MEDIC_SENT_MONSTER,
	MEDIC_SENT_COVER,
	MEDIC_SENT_THROW,
	MEDIC_SENT_CHARGE,
	MEDIC_SENT_TAUNT,
} MEDIC_ALLY_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has
// started moving.
//=========================================================
void COFMedicAlly::SpeakSentence()
{
	if (m_iSentence == MEDIC_SENT_NONE)
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz(ENT(pev), pMedicSentences[m_iSentence], MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void COFMedicAlly::GibMonster()
{
	Vector vecGunPos;
	Vector vecGunAngles;

	//TODO: probably the wrong logic, but it was in the original
	if (m_iWeaponIdx != MedicAllyWeapon::None)
	{ // throw a gun if the grunt has one
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pGun;

		if ((pev->weapons & MedicAllyWeaponFlag::Glock) != 0)
		{
			pGun = DropItem("weapon_9mmhandgun", vecGunPos, vecGunAngles);
		}
		else if ((pev->weapons & MedicAllyWeaponFlag::DesertEagle) != 0)
		{
			pGun = DropItem("weapon_eagle", vecGunPos, vecGunAngles);
		}

		if (nullptr != pGun)
		{
			pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
			pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
		}

		m_iWeaponIdx = MedicAllyWeapon::None;

		//Note: this wasn't in the original
		SetBodygroup(MedicAllyBodygroup::Weapons, MedicAllyWeapon::None);
	}

	COFSquadTalkMonster::GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int COFMedicAlly::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_PLAYER |
		   bits_SOUND_DANGER |
		   bits_SOUND_CARCASS |
		   bits_SOUND_MEAT |
		   bits_SOUND_GARBAGE;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
bool COFMedicAlly::FOkToSpeak()
{
	// if someone else is talking, don't speak
	if (gpGlobals->time <= COFSquadTalkMonster::g_talkWaitTime)
		return false;

	if ((pev->spawnflags & SF_MONSTER_GAG) != 0)
	{
		if (m_MonsterState != MONSTERSTATE_COMBAT)
		{
			// no talking outside of combat if gagged.
			return false;
		}
	}

	// if player is not in pvs, don't speak
	//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
	//		return false;

	return true;
}

//=========================================================
//=========================================================
void COFMedicAlly::JustSpoke()
{
	COFSquadTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = MEDIC_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void COFMedicAlly::PrescheduleThink()
{
	if (InSquad() && m_hEnemy != NULL)
	{
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if (gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5)
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = true;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds.
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
bool COFMedicAlly::FCanCheckAttacks()
{
	if (!HasConditions(bits_COND_ENEMY_TOOFAR))
	{
		return true;
	}
	else
	{
		return false;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
bool COFMedicAlly::CheckMeleeAttack1(float flDot, float flDist)
{
	CBaseMonster* pEnemy;

	if (m_hEnemy != NULL)
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if (!pEnemy)
		{
			return false;
		}
	}

	if (flDist <= 64 && flDot >= 0.7 &&
		pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON)
	{
		return true;
	}
	return false;
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
bool COFMedicAlly::CheckRangeAttack1(float flDot, float flDist)
{
	//Only if we have a weapon
	if (pev->weapons != 0)
	{
		//Friendly fire is allowed
		if (!HasConditions(bits_COND_ENEMY_OCCLUDED) && flDist <= 1024 && flDot >= 0.5 /*&& NoFriendlyFire()*/)
		{
			TraceResult tr;

			auto pEnemy = m_hEnemy.Entity<CBaseEntity>();

			//if( !pEnemy->IsPlayer() && flDist <= 64 )
			//{
			//	// kick nonclients, but don't shoot at them.
			//	return false;
			//}

			//TODO: kinda odd that this doesn't use GetGunPosition like the original
			Vector vecSrc = pev->origin + Vector(0, 0, 55);

			//Fire at last known position, adjusting for target origin being offset from entity origin
			const auto targetOrigin = pEnemy->BodyTarget(vecSrc);

			const auto targetPosition = targetOrigin - pEnemy->pev->origin + m_vecEnemyLKP;

			// verify that a bullet fired from the gun will hit the enemy before the world.
			UTIL_TraceLine(vecSrc, targetPosition, dont_ignore_monsters, ENT(pev), &tr);

			m_lastAttackCheck = tr.flFraction == 1.0 ? true : tr.pHit && GET_PRIVATE(tr.pHit) == pEnemy;

			return m_lastAttackCheck;
		}
	}

	return false;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
bool COFMedicAlly::CheckRangeAttack2(float flDot, float flDist)
{
	if (!FBitSet(pev->weapons, MedicAllyWeaponFlag::HandGrenade))
	{
		return false;
	}

	// if the grunt isn't moving, it's ok to check.
	if (m_flGroundSpeed != 0)
	{
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck)
	{
		return m_fThrowGrenade;
	}

	if (!FBitSet(m_hEnemy->pev->flags, FL_ONGROUND) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z)
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if (FBitSet(pev->weapons, MedicAllyWeaponFlag::HandGrenade))
	{
		// find feet
		if (RANDOM_LONG(0, 1))
		{
			// magically know where they are
			vecTarget = Vector(m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z);
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget(pev->origin) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions(bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.medicAllyGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if (InSquad())
	{
		if (SquadMemberInRange(vecTarget, 256))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = false;
		}
	}

	if ((vecTarget - pev->origin).Length2D() <= 256)
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = false;
		return m_fThrowGrenade;
	}


	if (FBitSet(pev->weapons, MedicAllyWeaponFlag::HandGrenade))
	{
		Vector vecToss = VecCheckToss(pev, GetGunPosition(), vecTarget, 0.5);

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = true;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = false;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow(pev, GetGunPosition(), vecTarget, gSkillData.medicAllyGrenadeSpeed, 0.5);

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = true;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = false;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}



	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void COFMedicAlly::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		//TODO: disabled for ally
		if (/*GetBodygroup( HGruntAllyBodygroup::Head ) == HGruntAllyHead::GasMask &&*/ (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB)) != 0)
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet(ptr->vecEndPos, 1.0);
				flDamage = 0.01;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	//PCV absorbs some damage types
	else if ((ptr->iHitgroup == HITGROUP_CHEST || ptr->iHitgroup == HITGROUP_STOMACH) && (bitsDamageType & (DMG_BLAST | DMG_BULLET | DMG_SLASH)) != 0)
	{
		flDamage *= 0.5;
	}

	COFSquadTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
bool COFMedicAlly::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	bool ret = COFSquadTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	Forget(bits_MEMORY_INCOVER);

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) != 0)
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == NULL)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if (gpGlobals->time - m_flLastHitByPlayer < 4.0 && m_iPlayerHits > 2 && ((m_afMemory & bits_MEMORY_SUSPICIOUS) != 0 || IsFacing(pevAttacker, pev->origin)))
			{
				// Alright, now I'm pissed!
				PlaySentence("FG_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(true);
				ALERT(at_console, "HGrunt Ally is now MAD!\n");
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("FG_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);

				if (4.0 > gpGlobals->time - m_flLastHitByPlayer)
					++m_iPlayerHits;
				else
					m_iPlayerHits = 0;

				m_flLastHitByPlayer = gpGlobals->time;

				ALERT(at_console, "HGrunt Ally is now SUSPICIOUS!\n");
			}
		}
		else if (!m_hEnemy->IsPlayer())
		{
			PlaySentence("FG_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COFMedicAlly::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK2:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK2:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void COFMedicAlly::IdleSound()
{
	if (FOkToSpeak() && (0 != g_fMedicAllyQuestion || RANDOM_LONG(0, 1)))
	{
		if (0 == g_fMedicAllyQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0, 2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "FG_CHECK", MEDIC_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fMedicAllyQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "FG_QUEST", MEDIC_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fMedicAllyQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "FG_IDLE", MEDIC_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fMedicAllyQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "FG_CLEAR", MEDIC_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question
				SENTENCEG_PlayRndSz(ENT(pev), "FG_ANSWER", MEDIC_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fMedicAllyQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void COFMedicAlly::CheckAmmo()
{
	if (m_cAmmoLoaded <= 0)
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int COFMedicAlly::Classify()
{
	return CLASS_HUMAN_MILITARY_FRIENDLY;
}

//=========================================================
//=========================================================
CBaseEntity* COFMedicAlly::Kick()
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector COFMedicAlly::GetGunPosition()
{
	if (m_fStanding)
	{
		return pev->origin + Vector(0, 0, 60);
	}
	else
	{
		return pev->origin + Vector(0, 0, 48);
	}
}

//=========================================================
// Shoot
//=========================================================
void COFMedicAlly::Shoot()
{
	//Limit fire rate
	if (m_hEnemy == NULL || gpGlobals->time - m_flLastShot <= 0.11)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	const char* pszSoundName;

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

	const auto random = RANDOM_LONG(0, 20);

	const auto pitch = random <= 10 ? random + 95 : 100;

	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pszSoundName, VOL_NORM, ATTN_NORM, 0, pitch);

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
	Vector vecShootDir;
	Vector vecShootOrigin;

	switch (pEvent->event)
	{
	case MEDIC_AE_DROP_GUN:
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		// switch to body group with no gun.
		SetBodygroup(MedicAllyBodygroup::Weapons, MedicAllyWeapon::None);

		// now spawn a gun.
		if ((pev->weapons & MedicAllyWeaponFlag::Glock) != 0)
		{
			DropItem("weapon_9mmhandgun", vecGunPos, vecGunAngles);
		}
		else
		{
			DropItem("weapon_eagle", vecGunPos, vecGunAngles);
		}

		m_iWeaponIdx = MedicAllyWeapon::None;
	}
	break;

	case MEDIC_AE_RELOAD:

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

	case MEDIC_AE_GREN_TOSS:
	{
		UTIL_MakeVectors(pev->angles);
		// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
		CGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, 3.5);

		m_fThrowGrenade = false;
		m_flNextGrenadeCheck = gpGlobals->time + 6; // wait six seconds before even looking again to see if a grenade can be thrown.
													// !!!LATER - when in a group, only try to throw grenade if ordered.
	}
	break;

	case MEDIC_AE_GREN_DROP:
	{
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootTimed(pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3);
	}
	break;

	case MEDIC_AE_SHOOT:
	{
		Shoot();
	}
	break;

	case MEDIC_AE_KICK:
	{
		CBaseEntity* pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage(pev, pev, gSkillData.medicAllyDmgKick, DMG_CLUB);
		}
	}
	break;

	case MEDIC_AE_HOLSTER_GUN:
		SetBodygroup(MedicAllyBodygroup::Weapons, MedicAllyWeapon::None);
		m_fGunHolstered = true;
		break;

	case MEDIC_AE_EQUIP_NEEDLE:
		SetBodygroup(MedicAllyBodygroup::Weapons, MedicAllyWeapon::Needle);
		m_fHypoHolstered = false;
		break;

	case MEDIC_AE_HOLSTER_NEEDLE:
		SetBodygroup(MedicAllyBodygroup::Weapons, MedicAllyWeapon::None);
		m_fHypoHolstered = true;
		break;

	case MEDIC_AE_EQUIP_GUN:
		SetBodygroup(MedicAllyBodygroup::Weapons, (pev->weapons & MedicAllyWeaponFlag::Glock) != 0 ? MedicAllyWeapon::Glock : MedicAllyWeapon::DesertEagle);
		m_fGunHolstered = false;
		break;

	case MEDIC_AE_CAUGHT_ENEMY:
	{
		if (FOkToSpeak())
		{
			SENTENCEG_PlayRndSz(ENT(pev), "FG_ALERT", MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
			JustSpoke();
		}
	}

	default:
		COFSquadTalkMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void COFMedicAlly::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hgrunt_medic.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.medicAllyHealth;
	m_flFieldOfView = 0.2; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = MEDIC_SENT_NONE;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_HEAR;

	m_fEnemyEluded = false;
	m_fFirstEncounter = true; // this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	//Note: this code has been rewritten to use SetBodygroup since it relies on hardcoded offsets in the original
	pev->body = 0;

	m_flLastUseTime = 0;
	m_iHealCharge = gSkillData.medicAllyHeal;
	m_fGunHolstered = false;
	m_fHypoHolstered = true;
	m_fHealActive = false;
	m_fQueueFollow = false;
	m_fUseHealing = false;
	m_fHealing = false;
	m_hNewTargetEnt = nullptr;
	m_fFollowChecked = false;
	m_fFollowChecking = false;


	if (0 == pev->weapons)
	{
		pev->weapons |= MedicAllyWeaponFlag::Glock;
	}

	if (m_iBlackOrWhite == MedicAllyHead::Default)
	{
		m_iBlackOrWhite = (RANDOM_LONG(0, 99) % 2) == 0 ? MedicAllyHead::White : MedicAllyHead::Black;
	}

	if ((pev->weapons & MedicAllyWeaponFlag::Glock) != 0)
	{
		m_iWeaponIdx = MedicAllyWeapon::Glock;
		m_cClipSize = MEDIC_GLOCK_CLIP_SIZE;
	}
	else if ((pev->weapons & MedicAllyWeaponFlag::DesertEagle) != 0)
	{
		m_iWeaponIdx = MedicAllyWeapon::DesertEagle;
		m_cClipSize = MEDIC_DEAGLE_CLIP_SIZE;
	}
	else if ((pev->weapons & MedicAllyWeaponFlag::Needle) != 0)
	{
		m_iWeaponIdx = MedicAllyWeapon::Needle;
		m_cClipSize = 1;
		m_fGunHolstered = true;
		m_fHypoHolstered = false;
	}
	else
	{
		m_iWeaponIdx = MedicAllyWeapon::None;
		m_cClipSize = 0;
	}

	SetBodygroup(MedicAllyBodygroup::Head, m_iBlackOrWhite);
	SetBodygroup(MedicAllyBodygroup::Weapons, m_iWeaponIdx);

	m_cAmmoLoaded = m_cClipSize;

	m_flLastShot = gpGlobals->time;

	pev->skin = 0;

	if (m_iBlackOrWhite == MedicAllyHead::Black)
	{
		m_voicePitch = 95;
	}

	COFSquadTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	SetUse(&COFMedicAlly::HealerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COFMedicAlly::Precache()
{
	PRECACHE_MODEL("models/hgrunt_medic.mdl");

	TalkInit();

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

	PRECACHE_SOUND("fgrunt/death1.wav");
	PRECACHE_SOUND("fgrunt/death2.wav");
	PRECACHE_SOUND("fgrunt/death3.wav");
	PRECACHE_SOUND("fgrunt/death4.wav");
	PRECACHE_SOUND("fgrunt/death5.wav");
	PRECACHE_SOUND("fgrunt/death6.wav");

	PRECACHE_SOUND("fgrunt/pain1.wav");
	PRECACHE_SOUND("fgrunt/pain2.wav");
	PRECACHE_SOUND("fgrunt/pain3.wav");
	PRECACHE_SOUND("fgrunt/pain4.wav");
	PRECACHE_SOUND("fgrunt/pain5.wav");
	PRECACHE_SOUND("fgrunt/pain6.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/desert_eagle_fire.wav");
	PRECACHE_SOUND("weapons/desert_eagle_reload.wav");
	PRECACHE_SOUND("weapons/sbarrel1.wav");

	PRECACHE_SOUND("fgrunt/medic_give_shot.wav");
	PRECACHE_SOUND("fgrunt/medical.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav"); // because we use the basemonster SWIPE animation event

	// get voice pitch
	m_voicePitch = 105;

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");

	COFSquadTalkMonster::Precache();
}

//=========================================================
// start task
//=========================================================
void COFMedicAlly::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MEDIC_ALLY_CHECK_FIRE:
		if (!NoFriendlyFire())
		{
			SetConditions(bits_COND_GRUNT_NOFIRE);
		}
		TaskComplete();
		break;

	case TASK_MEDIC_ALLY_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget(bits_MEMORY_INCOVER);
		COFSquadTalkMonster::StartTask(pTask);
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_MEDIC_ALLY_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		COFSquadTalkMonster::StartTask(pTask);
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

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
			return COFSquadTalkMonster::StartTask(pTask);

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

			return COFSquadTalkMonster::StartTask(pTask);
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
		COFSquadTalkMonster::StartTask(pTask);
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
	case TASK_MEDIC_ALLY_FACE_TOSS_DIR:
	{
		// project a point along the toss vector and turn to face that point.
		MakeIdealYaw(pev->origin + m_vecTossVelocity * 64);
		ChangeYaw(pev->yaw_speed);

		if (FacingIdeal())
		{
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
		break;
	}

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
		COFSquadTalkMonster::RunTask(pTask);
		break;
	}
	}
}

//=========================================================
// PainSound
//=========================================================
void COFMedicAlly::PainSound()
{
	if (gpGlobals->time > m_flNextPainTime)
	{
#if 0
		if ( RANDOM_LONG(0,99) < 5 )
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz(ENT(pev), "FG_PAIN", MEDIC_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
				JustSpoke();
				return;
			}
		}
#endif
		switch (RANDOM_LONG(0, 7))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain3.wav", 1, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain4.wav", 1, ATTN_NORM);
			break;
		case 2:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain5.wav", 1, ATTN_NORM);
			break;
		case 3:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain1.wav", 1, ATTN_NORM);
			break;
		case 4:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain2.wav", 1, ATTN_NORM);
			break;
		case 5:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/pain6.wav", 1, ATTN_NORM);
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound
//=========================================================
void COFMedicAlly::DeathSound()
{
	switch (RANDOM_LONG(0, 5))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death2.wav", 1, ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death3.wav", 1, ATTN_IDLE);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death4.wav", 1, ATTN_IDLE);
		break;
	case 4:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death5.wav", 1, ATTN_IDLE);
		break;
	case 5:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "fgrunt/death6.wav", 1, ATTN_IDLE);
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t tlMedicAllyFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slMedicAllyFail[] =
	{
		{tlMedicAllyFail,
			ARRAYSIZE(tlMedicAllyFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK2,
			0,
			"Grunt Fail"},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t tlMedicAllyCombatFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slMedicAllyCombatFail[] =
	{
		{tlMedicAllyCombatFail,
			ARRAYSIZE(tlMedicAllyCombatFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2,
			0,
			"Grunt Combat Fail"},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlMedicAllyVictoryDance[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_WAIT, (float)1.5},
		{TASK_GET_PATH_TO_ENEMY_CORPSE, (float)0},
		{TASK_WALK_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE},
};

Schedule_t slMedicAllyVictoryDance[] =
	{
		{tlMedicAllyVictoryDance,
			ARRAYSIZE(tlMedicAllyVictoryDance),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE,
			0,
			"GruntVictoryDance"},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlMedicAllyEstablishLineOfFire[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_MEDIC_ALLY_ELOF_FAIL},
		{TASK_GET_PATH_TO_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_SPEAK_SENTENCE, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
};

Schedule_t slMedicAllyEstablishLineOfFire[] =
	{
		{tlMedicAllyEstablishLineOfFire,
			ARRAYSIZE(tlMedicAllyEstablishLineOfFire),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_CAN_MELEE_ATTACK2 |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"GruntEstablishLineOfFire"},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t tlMedicAllyFoundEnemy[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL1},
};

Schedule_t slMedicAllyFoundEnemy[] =
	{
		{tlMedicAllyFoundEnemy,
			ARRAYSIZE(tlMedicAllyFoundEnemy),
			bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"GruntFoundEnemy"},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t tlMedicAllyCombatFace1[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_WAIT, (float)1.5},
		{TASK_SET_SCHEDULE, (float)SCHED_MEDIC_ALLY_SWEEP},
};

Schedule_t slMedicAllyCombatFace[] =
	{
		{tlMedicAllyCombatFace1,
			ARRAYSIZE(tlMedicAllyCombatFace1),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2,
			0,
			"Combat Face"},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t tlMedicAllySignalSuppress[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL2},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMedicAllySignalSuppress[] =
	{
		{tlMedicAllySignalSuppress,
			ARRAYSIZE(tlMedicAllySignalSuppress),
			bits_COND_ENEMY_DEAD |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_GRUNT_NOFIRE |
				bits_COND_NO_AMMO_LOADED,

			bits_SOUND_DANGER,
			"SignalSuppress"},
};

Task_t tlMedicAllySuppress[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMedicAllySuppress[] =
	{
		{tlMedicAllySuppress,
			ARRAYSIZE(tlMedicAllySuppress),
			bits_COND_ENEMY_DEAD |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_GRUNT_NOFIRE |
				bits_COND_NO_AMMO_LOADED,

			bits_SOUND_DANGER,
			"Suppress"},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t tlMedicAllyWaitInCover[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)1},
};

Schedule_t slMedicAllyWaitInCover[] =
	{
		{tlMedicAllyWaitInCover,
			ARRAYSIZE(tlMedicAllyWaitInCover),
			bits_COND_NEW_ENEMY |
				bits_COND_HEAR_SOUND |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_CAN_MELEE_ATTACK1 |
				bits_COND_CAN_MELEE_ATTACK2,

			bits_SOUND_DANGER,
			"GruntWaitInCover"},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t tlMedicAllyTakeCover1[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_MEDIC_ALLY_TAKECOVER_FAILED},
		{TASK_WAIT, (float)0.2},
		{TASK_FIND_COVER_FROM_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_SPEAK_SENTENCE, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
		{TASK_SET_SCHEDULE, (float)SCHED_MEDIC_ALLY_WAIT_FACE_ENEMY},
};

Schedule_t slMedicAllyTakeCover[] =
	{
		{tlMedicAllyTakeCover1,
			ARRAYSIZE(tlMedicAllyTakeCover1),
			0,
			0,
			"TakeCover"},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlMedicAllyGrenadeCover1[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FIND_COVER_FROM_ENEMY, (float)99},
		{TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, (float)384},
		{TASK_PLAY_SEQUENCE, (float)ACT_SPECIAL_ATTACK1},
		{TASK_CLEAR_MOVE_WAIT, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_SET_SCHEDULE, (float)SCHED_MEDIC_ALLY_WAIT_FACE_ENEMY},
};

Schedule_t slMedicAllyGrenadeCover[] =
	{
		{tlMedicAllyGrenadeCover1,
			ARRAYSIZE(tlMedicAllyGrenadeCover1),
			0,
			0,
			"GrenadeCover"},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlMedicAllyTossGrenadeCover1[] =
	{
		{TASK_FACE_ENEMY, (float)0},
		{TASK_RANGE_ATTACK2, (float)0},
		{TASK_SET_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY},
};

Schedule_t slMedicAllyTossGrenadeCover[] =
	{
		{tlMedicAllyTossGrenadeCover1,
			ARRAYSIZE(tlMedicAllyTossGrenadeCover1),
			0,
			0,
			"TossGrenadeCover"},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t tlMedicAllyTakeCoverFromBestSound[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_COWER}, // duck and cover if cannot move from explosion
		{TASK_STOP_MOVING, (float)0},
		{TASK_FIND_COVER_FROM_BEST_SOUND, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
		{TASK_TURN_LEFT, (float)179},
};

Schedule_t slMedicAllyTakeCoverFromBestSound[] =
	{
		{tlMedicAllyTakeCoverFromBestSound,
			ARRAYSIZE(tlMedicAllyTakeCoverFromBestSound),
			0,
			0,
			"GruntTakeCoverFromBestSound"},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t tlMedicAllyHideReload[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD},
		{TASK_FIND_COVER_FROM_ENEMY, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_RELOAD},
};

Schedule_t slMedicAllyHideReload[] =
	{
		{tlMedicAllyHideReload,
			ARRAYSIZE(tlMedicAllyHideReload),
			bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"GruntHideReload"}};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t tlMedicAllySweep[] =
	{
		{TASK_TURN_LEFT, (float)179},
		{TASK_WAIT, (float)1},
		{TASK_TURN_LEFT, (float)179},
		{TASK_WAIT, (float)1},
};

Schedule_t slMedicAllySweep[] =
	{
		{tlMedicAllySweep,
			ARRAYSIZE(tlMedicAllySweep),

			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2 |
				bits_COND_HEAR_SOUND,

			bits_SOUND_WORLD | // sound flags
				bits_SOUND_DANGER |
				bits_SOUND_PLAYER,

			"Grunt Sweep"},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlMedicAllyRangeAttack1A[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMedicAllyRangeAttack1A[] =
	{
		{tlMedicAllyRangeAttack1A,
			ARRAYSIZE(tlMedicAllyRangeAttack1A),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_ENEMY_OCCLUDED |
				bits_COND_HEAR_SOUND |
				bits_COND_GRUNT_NOFIRE |
				bits_COND_NO_AMMO_LOADED,

			bits_SOUND_DANGER,
			"Range Attack1A"},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlMedicAllyRangeAttack1B[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_IDLE_ANGRY},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MEDIC_ALLY_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slMedicAllyRangeAttack1B[] =
	{
		{tlMedicAllyRangeAttack1B,
			ARRAYSIZE(tlMedicAllyRangeAttack1B),
			bits_COND_NEW_ENEMY |
				bits_COND_ENEMY_DEAD |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_ENEMY_OCCLUDED |
				bits_COND_NO_AMMO_LOADED |
				bits_COND_GRUNT_NOFIRE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"Range Attack1B"},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t tlMedicAllyRangeAttack2[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_MEDIC_ALLY_FACE_TOSS_DIR, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_RANGE_ATTACK2},
		{TASK_SET_SCHEDULE, (float)SCHED_MEDIC_ALLY_WAIT_FACE_ENEMY}, // don't run immediately after throwing grenade.
};

Schedule_t slMedicAllyRangeAttack2[] =
	{
		{tlMedicAllyRangeAttack2,
			ARRAYSIZE(tlMedicAllyRangeAttack2),
			0,
			0,
			"RangeAttack2"},
};


//=========================================================
// repel
//=========================================================
Task_t tlMedicAllyRepel[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_GLIDE},
};

Schedule_t slMedicAllyRepel[] =
	{
		{tlMedicAllyRepel,
			ARRAYSIZE(tlMedicAllyRepel),
			bits_COND_SEE_ENEMY |
				bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER |
				bits_SOUND_COMBAT |
				bits_SOUND_PLAYER,
			"Repel"},
};


//=========================================================
// repel
//=========================================================
Task_t tlMedicAllyRepelAttack[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_FLY},
};

Schedule_t slMedicAllyRepelAttack[] =
	{
		{tlMedicAllyRepelAttack,
			ARRAYSIZE(tlMedicAllyRepelAttack),
			bits_COND_ENEMY_OCCLUDED,
			0,
			"Repel Attack"},
};

//=========================================================
// repel land
//=========================================================
Task_t tlMedicAllyRepelLand[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_LAND},
		{TASK_GET_PATH_TO_LASTPOSITION, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_CLEAR_LASTPOSITION, (float)0},
};

Schedule_t slMedicAllyRepelLand[] =
	{
		{tlMedicAllyRepelLand,
			ARRAYSIZE(tlMedicAllyRepelLand),
			bits_COND_SEE_ENEMY |
				bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER |
				bits_SOUND_COMBAT |
				bits_SOUND_PLAYER,
			"Repel Land"},
};

Task_t tlMedicAllyFollow[] =
	{
		{TASK_MOVE_TO_TARGET_RANGE, (float)128}, // Move within 128 of target ent (client)
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE},
};

Schedule_t slMedicAllyFollow[] =
	{
		{tlMedicAllyFollow,
			ARRAYSIZE(tlMedicAllyFollow),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"Follow"},
};

Task_t tlMedicAllyFaceTarget[] =
	{
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_TARGET, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE},
};

Schedule_t slMedicAllyFaceTarget[] =
	{
		{tlMedicAllyFaceTarget,
			ARRAYSIZE(tlMedicAllyFaceTarget),
			bits_COND_CLIENT_PUSH |
				bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_PROVOKED,
			bits_SOUND_DANGER,
			"FaceTarget"},
};

Task_t tlMedicAllyIdleStand[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},			// repick IDLESTAND every two seconds.
		{TASK_TLK_HEADRESET, (float)0}, // reset head position
};

Schedule_t slMedicAllyIdleStand[] =
	{
		{tlMedicAllyIdleStand,
			ARRAYSIZE(tlMedicAllyIdleStand),
			bits_COND_NEW_ENEMY |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_SMELL |
				bits_COND_PROVOKED,

			bits_SOUND_COMBAT | // sound flags - change these, and you'll break the talking code.
				//bits_SOUND_PLAYER		|
				//bits_SOUND_WORLD		|

				bits_SOUND_DANGER |
				bits_SOUND_MEAT | // scents
				bits_SOUND_CARCASS |
				bits_SOUND_GARBAGE,
			"IdleStand"},
};

Task_t tlMedicAllyNewHealTarget[] =
	{
		{TASK_SET_FAIL_SCHEDULE, SCHED_TARGET_CHASE},
		{TASK_MOVE_TO_TARGET_RANGE, 50},
		{TASK_FACE_IDEAL, 0},
		{TASK_MEDIC_ALLY_SPEAK_SENTENCE, 0},
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
		{TASK_MEDIC_ALLY_SPEAK_SENTENCE, 0}};

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
	slMedicAllyFollow,
	slMedicAllyFaceTarget,
	slMedicAllyIdleStand,
	slMedicAllyFail,
	slMedicAllyCombatFail,
	slMedicAllyVictoryDance,
	slMedicAllyEstablishLineOfFire,
	slMedicAllyFoundEnemy,
	slMedicAllyCombatFace,
	slMedicAllySignalSuppress,
	slMedicAllySuppress,
	slMedicAllyWaitInCover,
	slMedicAllyTakeCover,
	slMedicAllyGrenadeCover,
	slMedicAllyTossGrenadeCover,
	slMedicAllyTakeCoverFromBestSound,
	slMedicAllyHideReload,
	slMedicAllySweep,
	slMedicAllyRangeAttack1A,
	slMedicAllyRangeAttack1B,
	slMedicAllyRangeAttack2,
	slMedicAllyRepel,
	slMedicAllyRepelAttack,
	slMedicAllyRepelLand,
	slMedicAllyNewHealTarget,
	slMedicAllyDrawNeedle,
	slMedicAllyDrawGun,
	slMedicAllyHealTarget,
};

IMPLEMENT_CUSTOM_SCHEDULES(COFMedicAlly, COFSquadTalkMonster);

//=========================================================
// SetActivity
//=========================================================
void COFMedicAlly::SetActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

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
	case ACT_RUN:
		if (pev->health <= HGRUNT_LIMP_HEALTH)
		{
			// limp!
			iSequence = LookupActivity(ACT_RUN_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_WALK:
		if (pev->health <= HGRUNT_LIMP_HEALTH)
		{
			// limp!
			iSequence = LookupActivity(ACT_WALK_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_IDLE:
		if (m_MonsterState == MONSTERSTATE_COMBAT)
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity(NewActivity);
		break;
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

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
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0; // Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t* COFMedicAlly::GetSchedule()
{

	// clear old sentence
	m_iSentence = MEDIC_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling.
	if (pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE)
	{
		if ((pev->flags & FL_ONGROUND) != 0)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType(SCHED_MEDIC_ALLY_REPEL_LAND);
		}
		else
		{
			// repel down a rope,
			if (m_MonsterState == MONSTERSTATE_COMBAT)
				return GetScheduleOfType(SCHED_MEDIC_ALLY_REPEL_ATTACK);
			else
				return GetScheduleOfType(SCHED_MEDIC_ALLY_REPEL);
		}
	}

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

	// grunts place HIGH priority on running away from danger sounds.
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound)
		{
			if ((pSound->m_iType & bits_SOUND_DANGER) != 0)
			{
				// dangerous sound nearby!

				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause
				// this may only affect a single individual in a squad.

				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "FG_GREN", MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			if (FOkToSpeak())
			{
				PlaySentence("FG_KILL", 4, VOL_NORM, ATTN_NORM);
			}

			// call base class, all code to handle dead enemies is centralized there.
			return COFSquadTalkMonster::GetSchedule();
		}

		if (m_hWaitMedic)
		{
			auto pMedic = m_hWaitMedic.Entity<COFSquadTalkMonster>();

			if (pMedic->pev->deadflag != DEAD_NO)
				m_hWaitMedic = nullptr;
			else
				pMedic->HealMe(nullptr);

			m_flMedicWaitTime = gpGlobals->time + 5.0;
		}

		// new enemy
		//Do not fire until fired upon
		if (HasAllConditions(bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE))
		{
			if (InSquad())
			{
				MySquadLeader()->m_fEnemyEluded = false;

				if (!IsLeader())
				{
					return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
				}
				else
				{
					//!!!KELLY - the leader of a squad of grunts has just seen the player or a
					// monster and has made it the squad's enemy. You
					// can check pev->flags for FL_CLIENT to determine whether this is the player
					// or a monster. He's going to immediately start
					// firing, though. If you'd like, we can make an alternate "first sight"
					// schedule where the leader plays a handsign anim
					// that gives us enough time to hear a short sentence or spoken command
					// before he starts pluggin away.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
							// player
							SENTENCEG_PlayRndSz(ENT(pev), "FG_ALERT", MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						else if ((m_hEnemy != NULL) &&
								 (m_hEnemy->Classify() != CLASS_PLAYER_ALLY) &&
								 (m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) &&
								 (m_hEnemy->Classify() != CLASS_MACHINE))
							// monster
							SENTENCEG_PlayRndSz(ENT(pev), "FG_MONST", MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);

						JustSpoke();
					}

					if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
					{
						return GetScheduleOfType(SCHED_MEDIC_ALLY_SUPPRESS);
					}
					else
					{
						return GetScheduleOfType(SCHED_MEDIC_ALLY_ESTABLISH_LINE_OF_FIRE);
					}
				}
			}

			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		else if (HasConditions(bits_COND_HEAVY_DAMAGE))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
		// no ammo
		//Only if the grunt has a weapon
		else if (pev->weapons != 0 && HasConditions(bits_COND_NO_AMMO_LOADED))
		{
			//!!!KELLY - this individual just realized he's out of bullet ammo.
			// He's going to try to find cover to run to and reload, but rarely, if
			// none is available, he'll drop and reload in the open here.
			return GetScheduleOfType(SCHED_MEDIC_ALLY_COVER_AND_RELOAD);
		}

		// damaged just a little
		else if (HasConditions(bits_COND_LIGHT_DAMAGE))
		{
			// if hurt:
			// 90% chance of taking cover
			// 10% chance of flinch.
			int iPercent = RANDOM_LONG(0, 99);

			if (iPercent <= 90 && m_hEnemy != NULL)
			{
				// only try to take cover if we actually have an enemy!

				//!!!KELLY - this grunt was hit and is going to run to cover.
				if (FOkToSpeak()) // && RANDOM_LONG(0,1))
				{
					//SENTENCEG_PlayRndSz( ENT(pev), "FG_COVER", MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					m_iSentence = MEDIC_SENT_COVER;
					//JustSpoke();
				}
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			}
			else
			{
				return GetScheduleOfType(SCHED_SMALL_FLINCH);
			}
		}
		// can kick
		else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}
		// can shoot
		else if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			if (InSquad())
			{
				// if the enemy has eluded the squad and a squad member has just located the enemy
				// and the enemy does not see the squad member, issue a call to the squad to waste a
				// little time and give the player a chance to turn.
				if (MySquadLeader()->m_fEnemyEluded && !HasConditions(bits_COND_ENEMY_FACING_ME))
				{
					MySquadLeader()->m_fEnemyEluded = false;
					return GetScheduleOfType(SCHED_MEDIC_ALLY_FOUND_ENEMY);
				}
			}

			if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				// try to take an available ENGAGE slot
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}
			else if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				// throw a grenade if can and no engage slots are available
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else
			{
				// hide!
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			}
		}
		// can't see enemy
		else if (HasConditions(bits_COND_ENEMY_OCCLUDED))
		{
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "FG_THROW", MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				//!!!KELLY - grunt cannot see the enemy and has just decided to
				// charge the enemy's position.
				if (FOkToSpeak()) // && RANDOM_LONG(0,1))
				{
					//SENTENCEG_PlayRndSz( ENT(pev), "FG_CHARGE", MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					m_iSentence = MEDIC_SENT_CHARGE;
					//JustSpoke();
				}

				return GetScheduleOfType(SCHED_MEDIC_ALLY_ESTABLISH_LINE_OF_FIRE);
			}
			else
			{
				//!!!KELLY - grunt is going to stay put for a couple seconds to see if
				// the enemy wanders back out into the open, or approaches the
				// grunt's covered position. Good place for a taunt, I guess?
				if (FOkToSpeak() && RANDOM_LONG(0, 1))
				{
					SENTENCEG_PlayRndSz(ENT(pev), "FG_TAUNT", MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_STANDOFF);
			}
		}

		//Only if not following a player
		if (!m_hTargetEnt || !m_hTargetEnt->IsPlayer())
		{
			if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions(bits_COND_CAN_RANGE_ATTACK1))
			{
				return GetScheduleOfType(SCHED_MEDIC_ALLY_ESTABLISH_LINE_OF_FIRE);
			}
		}

		//Don't fall through to idle schedules
		break;
	}

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		//if we're not waiting on a medic and we're hurt, call out for a medic
		if (!m_hWaitMedic && gpGlobals->time > m_flMedicWaitTime && pev->health <= 20.0)
		{
			auto pMedic = MySquadMedic();

			if (!pMedic)
			{
				pMedic = FindSquadMedic(1024);
			}

			if (pMedic)
			{
				if (pMedic->pev->deadflag == DEAD_NO)
				{
					ALERT(at_aiconsole, "Injured Grunt found Medic\n");

					if (pMedic->HealMe(this))
					{
						ALERT(at_aiconsole, "Injured Grunt called for Medic\n");

						EMIT_SOUND_DYN(edict(), CHAN_VOICE, "fgrunt/medic.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

						JustSpoke();
						m_flMedicWaitTime = gpGlobals->time + 5.0;
					}
				}
			}
		}

		if (m_hEnemy == NULL && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(false);
				break;
			}
			else
			{
				if (HasConditions(bits_COND_CLIENT_PUSH))
				{
					return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	// no special cases here, call the base class
	return COFSquadTalkMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* COFMedicAlly::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		if (InSquad())
		{
			if (g_iSkillLevel == SKILL_HARD && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "FG_THROW", MEDIC_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return slMedicAllyTossGrenadeCover;
			}
			else
			{
				return &slMedicAllyTakeCover[0];
			}
		}
		else
		{
			//if ( RANDOM_LONG(0,1) )
			//{
			return &slMedicAllyTakeCover[0];
			//}
			//else
			//{
			//	return &slMedicAllyGrenadeCover[ 0 ];
			//}
		}
	}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
	{
		return &slMedicAllyTakeCoverFromBestSound[0];
	}
	case SCHED_MEDIC_ALLY_TAKECOVER_FAILED:
	{
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_FAIL);
	}
	break;
	case SCHED_MEDIC_ALLY_ELOF_FAIL:
	{
		// human grunt is unable to move to a position that allows him to attack the enemy.
		return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;
	case SCHED_MEDIC_ALLY_ESTABLISH_LINE_OF_FIRE:
	{
		return &slMedicAllyEstablishLineOfFire[0];
	}
	break;
	case SCHED_RANGE_ATTACK1:
	{
		// randomly stand or crouch
		if (RANDOM_LONG(0, 9) == 0)
			m_fStanding = RANDOM_LONG(0, 1);

		if (m_fStanding)
			return &slMedicAllyRangeAttack1B[0];
		else
			return &slMedicAllyRangeAttack1A[0];
	}
	case SCHED_RANGE_ATTACK2:
	{
		return &slMedicAllyRangeAttack2[0];
	}
	case SCHED_COMBAT_FACE:
	{
		return &slMedicAllyCombatFace[0];
	}
	case SCHED_MEDIC_ALLY_WAIT_FACE_ENEMY:
	{
		return &slMedicAllyWaitInCover[0];
	}
	case SCHED_MEDIC_ALLY_SWEEP:
	{
		return &slMedicAllySweep[0];
	}
	case SCHED_MEDIC_ALLY_COVER_AND_RELOAD:
	{
		return &slMedicAllyHideReload[0];
	}
	case SCHED_MEDIC_ALLY_FOUND_ENEMY:
	{
		return &slMedicAllyFoundEnemy[0];
	}
	case SCHED_VICTORY_DANCE:
	{
		if (InSquad())
		{
			if (!IsLeader())
			{
				return &slMedicAllyFail[0];
			}
		}

		return &slMedicAllyVictoryDance[0];
	}

	case SCHED_MEDIC_ALLY_SUPPRESS:
	{
		if (m_hEnemy->IsPlayer() && m_fFirstEncounter)
		{
			m_fFirstEncounter = false; // after first encounter, leader won't issue handsigns anymore when he has a new enemy
			return &slMedicAllySignalSuppress[0];
		}
		else
		{
			return &slMedicAllySuppress[0];
		}
	}
	case SCHED_FAIL:
	{
		if (m_hEnemy != NULL)
		{
			// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
			return &slMedicAllyCombatFail[0];
		}

		return &slMedicAllyFail[0];
	}
	case SCHED_MEDIC_ALLY_REPEL:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slMedicAllyRepel[0];
	}
	case SCHED_MEDIC_ALLY_REPEL_ATTACK:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slMedicAllyRepelAttack[0];
	}
	case SCHED_MEDIC_ALLY_REPEL_LAND:
	{
		return &slMedicAllyRepelLand[0];
	}

	case SCHED_TARGET_CHASE:
		return slMedicAllyFollow;

	case SCHED_TARGET_FACE:
	{
		auto pSchedule = COFSquadTalkMonster::GetScheduleOfType(SCHED_TARGET_FACE);

		if (pSchedule == slIdleStand)
			return slMedicAllyFaceTarget;
		return pSchedule;
	}

	case SCHED_IDLE_STAND:
	{
		auto pSchedule = COFSquadTalkMonster::GetScheduleOfType(SCHED_IDLE_STAND);

		if (pSchedule == slIdleStand)
			return slMedicAllyIdleStand;
		return pSchedule;
	}

	case SCHED_MEDIC_ALLY_HEAL_ALLY:
		return slMedicAllyHealTarget;

	case SCHED_CANT_FOLLOW:
		return &slMedicAllyFail[0];

	default:
	{
		return COFSquadTalkMonster::GetScheduleOfType(Type);
	}
	}
}

int COFMedicAlly::ObjectCaps()
{
	//Allow healing the player by continuously using
	return FCAP_ACROSS_TRANSITION | FCAP_CONTINUOUS_USE;
}

void COFMedicAlly::TalkInit()
{
	COFSquadTalkMonster::TalkInit();

	m_szGrp[TLK_ANSWER] = "FG_ANSWER";
	m_szGrp[TLK_QUESTION] = "FG_QUESTION";
	m_szGrp[TLK_IDLE] = "FG_IDLE";
	m_szGrp[TLK_STARE] = "FG_STARE";
	m_szGrp[TLK_USE] = "FG_OK";
	m_szGrp[TLK_UNUSE] = "FG_WAIT";
	m_szGrp[TLK_STOP] = "FG_STOP";

	m_szGrp[TLK_NOSHOOT] = "FG_SCARED";
	m_szGrp[TLK_HELLO] = "FG_HELLO";

	m_szGrp[TLK_PLHURT1] = "!FG_CUREA";
	m_szGrp[TLK_PLHURT2] = "!FG_CUREB";
	m_szGrp[TLK_PLHURT3] = "!FG_CUREC";

	m_szGrp[TLK_PHELLO] = NULL;			  //"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;			  //"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "FG_PQUEST"; // UNDONE

	m_szGrp[TLK_SMELL] = "FG_SMELL";

	m_szGrp[TLK_WOUND] = "FG_WOUND";
	m_szGrp[TLK_MORTAL] = "FG_MORTAL";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}

void COFMedicAlly::AlertSound()
{
	if (m_hEnemy && FOkToSpeak())
	{
		PlaySentence("FG_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_NORM);
	}
}

void COFMedicAlly::DeclineFollowing()
{
	PlaySentence("FG_POK", 2, VOL_NORM, ATTN_NORM);
}

bool COFMedicAlly::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iBlackOrWhite = atoi(pkvd->szValue);
		return true;
	}

	return COFSquadTalkMonster::KeyValue(pkvd);
}


void COFMedicAlly::Killed(entvars_t* pevAttacker, int iGib)
{
	if (m_hTargetEnt != nullptr)
	{
		auto pSquadMonster = m_hTargetEnt->MySquadTalkMonsterPointer();

		if (pSquadMonster)
			pSquadMonster->m_hWaitMedic = nullptr;
	}

	//TODO: missing from medic?
	/*
	if( m_MonsterState != MONSTERSTATE_DEAD )
	{
		if( HasMemory( bits_MEMORY_SUSPICIOUS ) || IsFacing( pevAttacker, pev->origin ) )
		{
			Remember( bits_MEMORY_PROVOKED );

			StopFollowing( true );
		}
	}
	*/

	SetUse(nullptr);

	COFSquadTalkMonster::Killed(pevAttacker, iGib);
}

void COFMedicAlly::MonsterThink()
{
	if (m_fFollowChecking && !m_fFollowChecked && gpGlobals->time - m_flFollowCheckTime > 0.5)
	{
		m_fFollowChecking = false;

		//TODO: not suited for multiplayer
		auto pPlayer = UTIL_FindEntityByClassname(nullptr, "player");

		FollowerUse(pPlayer, pPlayer, USE_TOGGLE, 0);
	}

	COFSquadTalkMonster::MonsterThink();
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
			auto v14 = m_hTargetEnt->MySquadTalkMonsterPointer();
			if (v14)
				v14->m_hWaitMedic = nullptr;
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
					SENTENCEG_PlayRndSz(edict(), "MG_HEAL", MEDIC_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
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
				SENTENCEG_PlayRndSz(edict(), "MG_NOTHEAL", MEDIC_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
			}
		}

		m_flLastUseTime = gpGlobals->time + 0.2;
		return;
	}

	m_fFollowChecking = true;
	m_flFollowCheckTime = gpGlobals->time;
}

//=========================================================
// COFMedicAllyRepel - when triggered, spawns a monster_human_medic_ally
// repelling down a line.
//=========================================================

class COFMedicAllyRepel : public CBaseMonster
{
public:
	static TYPEDESCRIPTION m_SaveData[];

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	bool KeyValue(KeyValueData* pkvd) override;

	void Spawn() override;
	void Precache() override;
	void EXPORT RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	int m_iSpriteTexture; // Don't save, precache

	int m_iBlackOrWhite;
	int m_iszUse;
	int m_iszUnUse;
};

LINK_ENTITY_TO_CLASS(monster_medic_ally_repel, COFMedicAllyRepel);

TYPEDESCRIPTION COFMedicAllyRepel::m_SaveData[] =
	{
		DEFINE_FIELD(COFMedicAllyRepel, m_iBlackOrWhite, FIELD_INTEGER),
		DEFINE_FIELD(COFMedicAllyRepel, m_iszUse, FIELD_STRING),
		DEFINE_FIELD(COFMedicAllyRepel, m_iszUnUse, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(COFMedicAllyRepel, CBaseMonster);

bool COFMedicAllyRepel::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iBlackOrWhite = atoi(pkvd->szValue);
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

	return CBaseMonster::KeyValue(pkvd);
}

void COFMedicAllyRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;

	SetUse(&COFMedicAllyRepel::RepelUse);
}

void COFMedicAllyRepel::Precache()
{
	UTIL_PrecacheOther("monster_human_medic_ally");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}

void COFMedicAllyRepel::RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;
	*/

	CBaseEntity* pEntity = Create("monster_human_medic_ally", pev->origin, pev->angles);
	auto pGrunt = static_cast<COFMedicAlly*>(pEntity->MySquadTalkMonsterPointer());

	if (pGrunt)
	{
		pGrunt->pev->weapons = pev->weapons;
		pGrunt->pev->netname = pev->netname;

		//Carry over these spawn flags
		pGrunt->pev->spawnflags |= pev->spawnflags & (SF_MONSTER_WAIT_TILL_SEEN | SF_MONSTER_GAG | SF_MONSTER_HITMONSTERCLIP | SF_MONSTER_PRISONER | SF_SQUADMONSTER_LEADER | SF_MONSTER_PREDISASTER);

		pGrunt->m_iBlackOrWhite = m_iBlackOrWhite;
		pGrunt->m_iszUse = m_iszUse;
		pGrunt->m_iszUnUse = m_iszUnUse;

		//Run logic to set up body groups (Spawn was already called once by Create above)
		pGrunt->Spawn();

		pGrunt->pev->movetype = MOVETYPE_FLY;
		pGrunt->pev->velocity = Vector(0, 0, RANDOM_FLOAT(-196, -128));
		pGrunt->SetActivity(ACT_GLIDE);
		// UNDONE: position?
		pGrunt->m_vecLastPosition = tr.vecEndPos;

		CBeam* pBeam = CBeam::BeamCreate("sprites/rope.spr", 10);
		pBeam->PointEntInit(pev->origin + Vector(0, 0, 112), pGrunt->entindex());
		pBeam->SetFlags(BEAM_FSOLID);
		pBeam->SetColor(255, 255, 255);
		pBeam->SetThink(&CBeam::SUB_Remove);
		pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

		UTIL_Remove(this);
	}
}
