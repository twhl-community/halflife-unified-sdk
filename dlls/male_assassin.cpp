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
#include "animation.h"
#include "squadmonster.h"
#include "weapons.h"
#include "talkmonster.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"

extern DLL_GLOBAL int g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define MASSASSIN_MP5_CLIP_SIZE 36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define MASSN_SNIPER_CLIP_SIZE 1
#define GRUNT_VOL 0.35		 // volume of grunt sounds
#define GRUNT_ATTN ATTN_NORM // attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH 20
#define HGRUNT_DMG_HEADSHOT (DMG_BULLET | DMG_CLUB) // damage types that can kill a grunt with a single headshot.
#define HGRUNT_NUM_HEADS 2							// how many grunt heads are there?
#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE 15			// must do at least this much damage in one shot to head to score a headshot kill
#define MASSASSIN_SENTENCE_VOLUME (float)0.35		// volume of grunt sentences

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define MASSASSIN_AE_RELOAD (2)
#define MASSASSIN_AE_KICK (3)
#define MASSASSIN_AE_BURST1 (4)
#define MASSASSIN_AE_BURST2 (5)
#define MASSASSIN_AE_BURST3 (6)
#define MASSASSIN_AE_GREN_TOSS (7)
#define MASSASSIN_AE_GREN_LAUNCH (8)
#define MASSASSIN_AE_GREN_DROP (9)
#define MASSASSIN_AE_DROP_GUN (11) // grunt (probably dead) is dropping his mp5.

namespace MAssassinBodygroup
{
enum MAssassinBodygroup
{
	Heads = 1,
	Weapons = 2
};
}

namespace MAssassinHead
{
enum MAssassinHead
{
	Random = -1,
	White = 0,
	Black,
	ThermalVision
};
}

namespace MAssassinWeapon
{
enum MAssassinWeapon
{
	MP5 = 0,
	SniperRifle,
	None
};
}

namespace MAssassinWeaponFlag
{
enum MAssassinWeaponFlag
{
	MP5 = 1 << 0,
	HandGrenade = 1 << 1,
	GrenadeLauncher = 1 << 2,
	SniperRifle = 1 << 3,
};
}

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_MASSASSIN_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE, // move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_MASSASSIN_COVER_AND_RELOAD,
	SCHED_MASSASSIN_SWEEP,
	SCHED_MASSASSIN_FOUND_ENEMY,
	SCHED_MASSASSIN_REPEL,
	SCHED_MASSASSIN_REPEL_ATTACK,
	SCHED_MASSASSIN_REPEL_LAND,
	SCHED_MASSASSIN_WAIT_FACE_ENEMY,
	SCHED_MASSASSIN_TAKECOVER_FAILED, // special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_MASSASSIN_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_MASSASSIN_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_MASSASSIN_SPEAK_SENTENCE,
	TASK_MASSASSIN_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE (bits_COND_SPECIAL1)

class CMOFAssassin : public CSquadMonster
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

	int IRelationship(CBaseEntity* pTarget) override;

	bool FOkToSpeak();
	void JustSpoke();

	bool KeyValue(KeyValueData* pkvd) override;

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

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

	float m_flLastShot;
	bool m_fStandingGround;
	float m_flStandGroundRange;

	int m_voicePitch;

	int m_iBrassShell;
	int m_iShotgunShell;

	int m_iSentence;

	int m_iAssassinHead;

	static const char* pGruntSentences[];
};

LINK_ENTITY_TO_CLASS(monster_male_assassin, CMOFAssassin);

TYPEDESCRIPTION CMOFAssassin::m_SaveData[] =
	{
		DEFINE_FIELD(CMOFAssassin, m_flNextGrenadeCheck, FIELD_TIME),
		DEFINE_FIELD(CMOFAssassin, m_flNextPainTime, FIELD_TIME),
		//	DEFINE_FIELD( CMOFAssassin, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
		DEFINE_FIELD(CMOFAssassin, m_vecTossVelocity, FIELD_VECTOR),
		DEFINE_FIELD(CMOFAssassin, m_fThrowGrenade, FIELD_BOOLEAN),
		DEFINE_FIELD(CMOFAssassin, m_fStanding, FIELD_BOOLEAN),
		DEFINE_FIELD(CMOFAssassin, m_fFirstEncounter, FIELD_BOOLEAN),
		DEFINE_FIELD(CMOFAssassin, m_cClipSize, FIELD_INTEGER),
		DEFINE_FIELD(CMOFAssassin, m_voicePitch, FIELD_INTEGER),
		//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
		//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
		DEFINE_FIELD(CMOFAssassin, m_iSentence, FIELD_INTEGER),
		DEFINE_FIELD(CMOFAssassin, m_iAssassinHead, FIELD_INTEGER),
		DEFINE_FIELD(CMOFAssassin, m_flLastShot, FIELD_TIME),
		DEFINE_FIELD(CMOFAssassin, m_fStandingGround, FIELD_BOOLEAN),
		DEFINE_FIELD(CMOFAssassin, m_flStandGroundRange, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CMOFAssassin, CSquadMonster);

const char* CMOFAssassin::pGruntSentences[] =
	{
		"HG_GREN",	  // grenade scared grunt
		"HG_ALERT",	  // sees player
		"HG_MONSTER", // sees monster
		"HG_COVER",	  // running to cover
		"HG_THROW",	  // about to throw grenade
		"HG_CHARGE",  // running out to get the enemy
		"HG_TAUNT",	  // say rude things
};

enum
{
	MASSASSIN_SENT_NONE = -1,
	MASSASSIN_SENT_GREN = 0,
	MASSASSIN_SENT_ALERT,
	MASSASSIN_SENT_MONSTER,
	MASSASSIN_SENT_COVER,
	MASSASSIN_SENT_THROW,
	MASSASSIN_SENT_CHARGE,
	MASSASSIN_SENT_TAUNT,
} MASSASSIN_SENTENCE_TYPES;

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
void CMOFAssassin::SpeakSentence()
{
	if (m_iSentence == MASSASSIN_SENT_NONE)
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz(ENT(pev), pGruntSentences[m_iSentence], MASSASSIN_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are
// Human Grunt's nemesis.
//=========================================================
int CMOFAssassin::IRelationship(CBaseEntity* pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_alien_grunt") || (FClassnameIs(pTarget->pev, "monster_gargantua")))
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship(pTarget);
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CMOFAssassin::GibMonster()
{
	Vector vecGunPos;
	Vector vecGunAngles;

	if (GetBodygroup(2) != 2)
	{ // throw a gun if the grunt has one
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pGun;
		if (FBitSet(pev->weapons, MAssassinWeaponFlag::MP5))
		{
			pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
		}
		else
		{
			pGun = DropItem("weapon_sniperrifle", vecGunPos, vecGunAngles);
		}
		if (pGun)
		{
			pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
			pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
		}

		if (FBitSet(pev->weapons, MAssassinWeaponFlag::GrenadeLauncher))
		{
			pGun = DropItem("ammo_ARgrenades", vecGunPos, vecGunAngles);
			if (pGun)
			{
				pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
				pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
			}
		}
	}

	CBaseMonster::GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CMOFAssassin::ISoundMask()
{
	return bits_SOUND_WORLD |
		   bits_SOUND_COMBAT |
		   bits_SOUND_PLAYER |
		   bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
bool CMOFAssassin::FOkToSpeak()
{
	// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
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
void CMOFAssassin::JustSpoke()
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = MASSASSIN_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CMOFAssassin::PrescheduleThink()
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
bool CMOFAssassin::FCanCheckAttacks()
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
bool CMOFAssassin::CheckMeleeAttack1(float flDot, float flDist)
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
bool CMOFAssassin::CheckRangeAttack1(float flDot, float flDist)
{
	if (pev->weapons != 0)
	{
		if (m_fStandingGround && m_flStandGroundRange >= flDist)
		{
			m_fStandingGround = false;
		}

		if (!HasConditions(bits_COND_ENEMY_OCCLUDED) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire())
		{
			TraceResult tr;

			if (!m_hEnemy->IsPlayer() && flDist <= 64)
			{
				// kick nonclients, but don't shoot at them.
				return false;
			}

			Vector vecSrc = GetGunPosition();

			// verify that a bullet fired from the gun will hit the enemy before the world.
			UTIL_TraceLine(vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

			if (tr.flFraction == 1.0)
			{
				return true;
			}
		}
	}

	return false;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
bool CMOFAssassin::CheckRangeAttack2(float flDot, float flDist)
{
	if (!FBitSet(pev->weapons, (MAssassinWeaponFlag::HandGrenade | MAssassinWeaponFlag::GrenadeLauncher)))
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

	if (FBitSet(pev->weapons, MAssassinWeaponFlag::HandGrenade))
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
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.massassinGrenadeSpeed) * m_hEnemy->pev->velocity;
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


	if (FBitSet(pev->weapons, MAssassinWeaponFlag::HandGrenade))
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
		Vector vecToss = VecCheckThrow(pev, GetGunPosition(), vecTarget, gSkillData.massassinGrenadeSpeed, 0.5);

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
void CMOFAssassin::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}

	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
bool CMOFAssassin::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Forget(bits_MEMORY_INCOVER);

	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMOFAssassin::SetYawSpeed()
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

void CMOFAssassin::IdleSound()
{
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CMOFAssassin::CheckAmmo()
{
	if (pev->weapons != 0 && m_cAmmoLoaded <= 0)
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CMOFAssassin::Classify()
{
	return CLASS_HUMAN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity* CMOFAssassin::Kick()
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

Vector CMOFAssassin::GetGunPosition()
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
void CMOFAssassin::Shoot()
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	if (FBitSet(pev->weapons, MAssassinWeaponFlag::SniperRifle) && gpGlobals->time - m_flLastShot <= 0.11)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	if (FBitSet(pev->weapons, MAssassinWeaponFlag::MP5))
	{
		Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5); // shoot +-5 degrees
	}
	else
	{
		//TODO: why is this 556? is 762 too damaging?
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, BULLET_PLAYER_556);
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
void CMOFAssassin::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	Vector vecShootDir;
	Vector vecShootOrigin;

	switch (pEvent->event)
	{
	case MASSASSIN_AE_DROP_GUN:
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		// switch to body group with no gun.
		SetBodygroup(MAssassinBodygroup::Weapons, MAssassinWeapon::None);

		// now spawn a gun.
		if (FBitSet(pev->weapons, MAssassinWeaponFlag::MP5))
		{
			DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
		}
		else
		{
			DropItem("weapon_sniperrifle", vecGunPos, vecGunAngles);
		}
		if (FBitSet(pev->weapons, MAssassinWeaponFlag::GrenadeLauncher))
		{
			DropItem("ammo_ARgrenades", BodyTarget(pev->origin), vecGunAngles);
		}
	}
	break;

	case MASSASSIN_AE_RELOAD:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM);
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case MASSASSIN_AE_GREN_TOSS:
	{
		UTIL_MakeVectors(pev->angles);
		// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
		CGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, 3.5);

		m_fThrowGrenade = false;
		m_flNextGrenadeCheck = gpGlobals->time + 6; // wait six seconds before even looking again to see if a grenade can be thrown.
													// !!!LATER - when in a group, only try to throw grenade if ordered.
	}
	break;

	case MASSASSIN_AE_GREN_LAUNCH:
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
		CGrenade::ShootContact(pev, GetGunPosition(), m_vecTossVelocity);
		m_fThrowGrenade = false;
		if (g_iSkillLevel == SKILL_HARD)
			m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT(2, 5); // wait a random amount of time before shooting again
		else
			m_flNextGrenadeCheck = gpGlobals->time + 6; // wait six seconds before even looking again to see if a grenade can be thrown.
	}
	break;

	case MASSASSIN_AE_GREN_DROP:
	{
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootTimed(pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3);
	}
	break;

	case MASSASSIN_AE_BURST1:
	{
		Shoot();

		if (FBitSet(pev->weapons, MAssassinWeaponFlag::MP5))
		{
			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (RANDOM_LONG(0, 1))
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM);
			}
			else
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM);
			}
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM);
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case MASSASSIN_AE_BURST2:
	case MASSASSIN_AE_BURST3:
		Shoot();
		break;

	case MASSASSIN_AE_KICK:
	{
		CBaseEntity* pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage(pev, pev, gSkillData.massassinDmgKick, DMG_CLUB);
		}
	}
	break;

	default:
		CSquadMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CMOFAssassin::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/massn.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.massassinHealth;
	m_flFieldOfView = 0.2; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = MASSASSIN_SENT_NONE;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded = false;
	m_fFirstEncounter = true; // this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = MAssassinWeaponFlag::MP5 | MAssassinWeaponFlag::HandGrenade;
		// pev->weapons = HGRUNT_SHOTGUN;
		// pev->weapons = HGRUNT_9MMAR | HGRUNT_GRENADELAUNCHER;
	}

	if (m_iAssassinHead == MAssassinHead::Random)
	{
		m_iAssassinHead = RANDOM_LONG(MAssassinHead::White, MAssassinHead::ThermalVision);
	}

	auto weaponModel = MAssassinWeapon::None;

	if (FBitSet(pev->weapons, MAssassinWeaponFlag::MP5))
	{
		weaponModel = MAssassinWeapon::MP5;
		m_cClipSize = MASSASSIN_MP5_CLIP_SIZE;
	}
	else if (FBitSet(pev->weapons, MAssassinWeaponFlag::SniperRifle))
	{
		weaponModel = MAssassinWeapon::SniperRifle;
		m_cClipSize = MASSN_SNIPER_CLIP_SIZE;
	}
	else
	{
		weaponModel = MAssassinWeapon::None;
		m_cClipSize = 0;
	}

	SetBodygroup(MAssassinBodygroup::Heads, m_iAssassinHead);
	SetBodygroup(MAssassinBodygroup::Weapons, weaponModel);

	m_cAmmoLoaded = m_cClipSize;

	m_flLastShot = gpGlobals->time;

	pev->skin = 0;

	m_fStandingGround = m_flStandGroundRange != 0;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMOFAssassin::Precache()
{
	PRECACHE_MODEL("models/massn.mdl");

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

	PRECACHE_SOUND("hgrunt/gr_die1.wav");
	PRECACHE_SOUND("hgrunt/gr_die2.wav");
	PRECACHE_SOUND("hgrunt/gr_die3.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("weapons/sbarrel1.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav"); // because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0, 1))
		m_voicePitch = 109 + RANDOM_LONG(0, 7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell
}

//=========================================================
// start task
//=========================================================
void CMOFAssassin::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MASSASSIN_CHECK_FIRE:
		if (!NoFriendlyFire())
		{
			SetConditions(bits_COND_GRUNT_NOFIRE);
		}
		TaskComplete();
		break;

	case TASK_MASSASSIN_SPEAK_SENTENCE:
		//Assassins don't talk
		//SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget(bits_MEMORY_INCOVER);
		CSquadMonster::StartTask(pTask);
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_MASSASSIN_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster::StartTask(pTask);
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default:
		CSquadMonster::StartTask(pTask);
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CMOFAssassin::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_MASSASSIN_FACE_TOSS_DIR:
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
	default:
	{
		CSquadMonster::RunTask(pTask);
		break;
	}
	}
}

//=========================================================
// PainSound
//=========================================================
void CMOFAssassin::PainSound()
{
}

//=========================================================
// DeathSound
//=========================================================
void CMOFAssassin::DeathSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE);
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t tlOFMAssassinFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slOFMAssassinFail[] =
	{
		{tlOFMAssassinFail,
			ARRAYSIZE(tlOFMAssassinFail),
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
Task_t tlOFMAssassinCombatFail[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)2},
		{TASK_WAIT_PVS, (float)0},
};

Schedule_t slOFMAssassinCombatFail[] =
	{
		{tlOFMAssassinCombatFail,
			ARRAYSIZE(tlOFMAssassinCombatFail),
			bits_COND_CAN_RANGE_ATTACK1 |
				bits_COND_CAN_RANGE_ATTACK2,
			0,
			"Grunt Combat Fail"},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t tlOFMAssassinVictoryDance[] =
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

Schedule_t slOFMAssassinVictoryDance[] =
	{
		{tlOFMAssassinVictoryDance,
			ARRAYSIZE(tlOFMAssassinVictoryDance),
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
Task_t tlOFMAssassinEstablishLineOfFire[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_MASSASSIN_ELOF_FAIL},
		{TASK_GET_PATH_TO_ENEMY, (float)0},
		{TASK_MASSASSIN_SPEAK_SENTENCE, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
};

Schedule_t slOFMAssassinEstablishLineOfFire[] =
	{
		{tlOFMAssassinEstablishLineOfFire,
			ARRAYSIZE(tlOFMAssassinEstablishLineOfFire),
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
Task_t tlOFMAssassinFoundEnemy[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL1},
};

Schedule_t slOFMAssassinFoundEnemy[] =
	{
		{tlOFMAssassinFoundEnemy,
			ARRAYSIZE(tlOFMAssassinFoundEnemy),
			bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"GruntFoundEnemy"},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t tlOFMAssassinCombatFace1[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_WAIT, (float)1.5},
		{TASK_SET_SCHEDULE, (float)SCHED_MASSASSIN_SWEEP},
};

Schedule_t slOFMAssassinCombatFace[] =
	{
		{tlOFMAssassinCombatFace1,
			ARRAYSIZE(tlOFMAssassinCombatFace1),
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
Task_t tlOFMAssassinSignalSuppress[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL2},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slOFMAssassinSignalSuppress[] =
	{
		{tlOFMAssassinSignalSuppress,
			ARRAYSIZE(tlOFMAssassinSignalSuppress),
			bits_COND_ENEMY_DEAD |
				bits_COND_LIGHT_DAMAGE |
				bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND |
				bits_COND_GRUNT_NOFIRE |
				bits_COND_NO_AMMO_LOADED,

			bits_SOUND_DANGER,
			"SignalSuppress"},
};

Task_t tlOFMAssassinSuppress[] =
	{
		{TASK_STOP_MOVING, 0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slOFMAssassinSuppress[] =
	{
		{tlOFMAssassinSuppress,
			ARRAYSIZE(tlOFMAssassinSuppress),
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
Task_t tlOFMAssassinWaitInCover[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_ACTIVITY, (float)ACT_IDLE},
		{TASK_WAIT_FACE_ENEMY, (float)1},
};

Schedule_t slOFMAssassinWaitInCover[] =
	{
		{tlOFMAssassinWaitInCover,
			ARRAYSIZE(tlOFMAssassinWaitInCover),
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
Task_t tlOFMAssassinTakeCover1[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_MASSASSIN_TAKECOVER_FAILED},
		{TASK_WAIT, (float)0.2},
		{TASK_FIND_COVER_FROM_ENEMY, (float)0},
		{TASK_MASSASSIN_SPEAK_SENTENCE, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
		{TASK_SET_SCHEDULE, (float)SCHED_MASSASSIN_WAIT_FACE_ENEMY},
};

Schedule_t slOFMAssassinTakeCover[] =
	{
		{tlOFMAssassinTakeCover1,
			ARRAYSIZE(tlOFMAssassinTakeCover1),
			0,
			0,
			"TakeCover"},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlOFMAssassinGrenadeCover1[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FIND_COVER_FROM_ENEMY, (float)99},
		{TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, (float)384},
		{TASK_PLAY_SEQUENCE, (float)ACT_SPECIAL_ATTACK1},
		{TASK_CLEAR_MOVE_WAIT, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_SET_SCHEDULE, (float)SCHED_MASSASSIN_WAIT_FACE_ENEMY},
};

Schedule_t slOFMAssassinGrenadeCover[] =
	{
		{tlOFMAssassinGrenadeCover1,
			ARRAYSIZE(tlOFMAssassinGrenadeCover1),
			0,
			0,
			"GrenadeCover"},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t tlOFMAssassinTossGrenadeCover1[] =
	{
		{TASK_FACE_ENEMY, (float)0},
		{TASK_RANGE_ATTACK2, (float)0},
		{TASK_SET_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY},
};

Schedule_t slOFMAssassinTossGrenadeCover[] =
	{
		{tlOFMAssassinTossGrenadeCover1,
			ARRAYSIZE(tlOFMAssassinTossGrenadeCover1),
			0,
			0,
			"TossGrenadeCover"},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t tlOFMAssassinTakeCoverFromBestSound[] =
	{
		{TASK_SET_FAIL_SCHEDULE, (float)SCHED_COWER}, // duck and cover if cannot move from explosion
		{TASK_STOP_MOVING, (float)0},
		{TASK_FIND_COVER_FROM_BEST_SOUND, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_REMEMBER, (float)bits_MEMORY_INCOVER},
		{TASK_TURN_LEFT, (float)179},
};

Schedule_t slOFMAssassinTakeCoverFromBestSound[] =
	{
		{tlOFMAssassinTakeCoverFromBestSound,
			ARRAYSIZE(tlOFMAssassinTakeCoverFromBestSound),
			0,
			0,
			"GruntTakeCoverFromBestSound"},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t tlOFMAssassinHideReload[] =
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

Schedule_t slOFMAssassinHideReload[] =
	{
		{tlOFMAssassinHideReload,
			ARRAYSIZE(tlOFMAssassinHideReload),
			bits_COND_HEAVY_DAMAGE |
				bits_COND_HEAR_SOUND,

			bits_SOUND_DANGER,
			"GruntHideReload"}};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t tlOFMAssassinSweep[] =
	{
		{TASK_TURN_LEFT, (float)179},
		{TASK_WAIT, (float)1},
		{TASK_TURN_LEFT, (float)179},
		{TASK_WAIT, (float)1},
};

Schedule_t slOFMAssassinSweep[] =
	{
		{tlOFMAssassinSweep,
			ARRAYSIZE(tlOFMAssassinSweep),

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
Task_t tlOFMAssassinRangeAttack1A[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slOFMAssassinRangeAttack1A[] =
	{
		{tlOFMAssassinRangeAttack1A,
			ARRAYSIZE(tlOFMAssassinRangeAttack1A),
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
Task_t tlOFMAssassinRangeAttack1B[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_IDLE_ANGRY},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_MASSASSIN_CHECK_FIRE, (float)0},
		{TASK_RANGE_ATTACK1, (float)0},
};

Schedule_t slOFMAssassinRangeAttack1B[] =
	{
		{tlOFMAssassinRangeAttack1B,
			ARRAYSIZE(tlOFMAssassinRangeAttack1B),
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
Task_t tlOFMAssassinRangeAttack2[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_MASSASSIN_FACE_TOSS_DIR, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_RANGE_ATTACK2},
		{TASK_SET_SCHEDULE, (float)SCHED_MASSASSIN_WAIT_FACE_ENEMY}, // don't run immediately after throwing grenade.
};

Schedule_t slOFMAssassinRangeAttack2[] =
	{
		{tlOFMAssassinRangeAttack2,
			ARRAYSIZE(tlOFMAssassinRangeAttack2),
			0,
			0,
			"RangeAttack2"},
};


//=========================================================
// repel
//=========================================================
Task_t tlOFMAssassinRepel[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_IDEAL, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_GLIDE},
};

Schedule_t slOFMAssassinRepel[] =
	{
		{tlOFMAssassinRepel,
			ARRAYSIZE(tlOFMAssassinRepel),
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
Task_t tlOFMAssassinRepelAttack[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_FACE_ENEMY, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_FLY},
};

Schedule_t slOFMAssassinRepelAttack[] =
	{
		{tlOFMAssassinRepelAttack,
			ARRAYSIZE(tlOFMAssassinRepelAttack),
			bits_COND_ENEMY_OCCLUDED,
			0,
			"Repel Attack"},
};

//=========================================================
// repel land
//=========================================================
Task_t tlOFMAssassinRepelLand[] =
	{
		{TASK_STOP_MOVING, (float)0},
		{TASK_PLAY_SEQUENCE, (float)ACT_LAND},
		{TASK_GET_PATH_TO_LASTPOSITION, (float)0},
		{TASK_RUN_PATH, (float)0},
		{TASK_WAIT_FOR_MOVEMENT, (float)0},
		{TASK_CLEAR_LASTPOSITION, (float)0},
};

Schedule_t slOFMAssassinRepelLand[] =
	{
		{tlOFMAssassinRepelLand,
			ARRAYSIZE(tlOFMAssassinRepelLand),
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


DEFINE_CUSTOM_SCHEDULES(CMOFAssassin){
	slOFMAssassinFail,
	slOFMAssassinCombatFail,
	slOFMAssassinVictoryDance,
	slOFMAssassinEstablishLineOfFire,
	slOFMAssassinFoundEnemy,
	slOFMAssassinCombatFace,
	slOFMAssassinSignalSuppress,
	slOFMAssassinSuppress,
	slOFMAssassinWaitInCover,
	slOFMAssassinTakeCover,
	slOFMAssassinGrenadeCover,
	slOFMAssassinTossGrenadeCover,
	slOFMAssassinTakeCoverFromBestSound,
	slOFMAssassinHideReload,
	slOFMAssassinSweep,
	slOFMAssassinRangeAttack1A,
	slOFMAssassinRangeAttack1B,
	slOFMAssassinRangeAttack2,
	slOFMAssassinRepel,
	slOFMAssassinRepelAttack,
	slOFMAssassinRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES(CMOFAssassin, CSquadMonster);

//=========================================================
// SetActivity
//=========================================================
void CMOFAssassin::SetActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		//Sniper uses the same set
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
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown
		// grenade or fired grenade, we must determine which and pick proper sequence
		if ((pev->weapons & MAssassinWeaponFlag::HandGrenade) != 0)
		{
			// get toss anim
			iSequence = LookupSequence("throwgrenade");
		}
		else
		{
			// get launch anim
			iSequence = LookupSequence("launchgrenade");
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
Schedule_t* CMOFAssassin::GetSchedule()
{

	// clear old sentence
	m_iSentence = MASSASSIN_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling.
	if (pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE)
	{
		if ((pev->flags & FL_ONGROUND) != 0)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType(SCHED_MASSASSIN_REPEL_LAND);
		}
		else
		{
			// repel down a rope,
			if (m_MonsterState == MONSTERSTATE_COMBAT)
				return GetScheduleOfType(SCHED_MASSASSIN_REPEL_ATTACK);
			else
				return GetScheduleOfType(SCHED_MASSASSIN_REPEL);
		}
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
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		// new enemy
		if (HasConditions(bits_COND_NEW_ENEMY))
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
					if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
					{
						return GetScheduleOfType(SCHED_MASSASSIN_SUPPRESS);
					}
					else
					{
						return GetScheduleOfType(SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE);
					}
				}
			}
		}
		// no ammo
		else if (HasConditions(bits_COND_NO_AMMO_LOADED))
		{
			//!!!KELLY - this individual just realized he's out of bullet ammo.
			// He's going to try to find cover to run to and reload, but rarely, if
			// none is available, he'll drop and reload in the open here.
			return GetScheduleOfType(SCHED_MASSASSIN_COVER_AND_RELOAD);
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
		// can grenade launch

		else if (FBitSet(pev->weapons, MAssassinWeaponFlag::GrenadeLauncher) && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
		{
			// shoot a grenade if you can
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
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
					return GetScheduleOfType(SCHED_MASSASSIN_FOUND_ENEMY);
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
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				//TODO: probably don't want this here, but it's still present in op4
				//!!!KELLY - grunt cannot see the enemy and has just decided to
				// charge the enemy's position.
				if (FOkToSpeak()) // && RANDOM_LONG(0,1))
				{
					//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", MASSASSIN_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					m_iSentence = MASSASSIN_SENT_CHARGE;
					//JustSpoke();
				}

				return GetScheduleOfType(SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE);
			}
			else
			{
				return GetScheduleOfType(SCHED_STANDOFF);
			}
		}

		if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE);
		}
	}
	}

	// no special cases here, call the base class
	return CSquadMonster::GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CMOFAssassin::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		if (InSquad())
		{
			if (g_iSkillLevel == SKILL_HARD && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				return slOFMAssassinTossGrenadeCover;
			}
			else
			{
				return &slOFMAssassinTakeCover[0];
			}
		}
		else
		{
			if (RANDOM_LONG(0, 1))
			{
				return &slOFMAssassinTakeCover[0];
			}
			else
			{
				return &slOFMAssassinGrenadeCover[0];
			}
		}
	}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
	{
		return &slOFMAssassinTakeCoverFromBestSound[0];
	}
	case SCHED_MASSASSIN_TAKECOVER_FAILED:
	{
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}

		return GetScheduleOfType(SCHED_FAIL);
	}
	break;
	case SCHED_MASSASSIN_ELOF_FAIL:
	{
		// human grunt is unable to move to a position that allows him to attack the enemy.
		return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;
	case SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE:
	{
		return &slOFMAssassinEstablishLineOfFire[0];
	}
	break;
	case SCHED_RANGE_ATTACK1:
	{
		// randomly stand or crouch
		if (RANDOM_LONG(0, 9) == 0)
			m_fStanding = RANDOM_LONG(0, 1);

		if (m_fStanding)
			return &slOFMAssassinRangeAttack1B[0];
		else
			return &slOFMAssassinRangeAttack1A[0];
	}
	case SCHED_RANGE_ATTACK2:
	{
		return &slOFMAssassinRangeAttack2[0];
	}
	case SCHED_COMBAT_FACE:
	{
		return &slOFMAssassinCombatFace[0];
	}
	case SCHED_MASSASSIN_WAIT_FACE_ENEMY:
	{
		return &slOFMAssassinWaitInCover[0];
	}
	case SCHED_MASSASSIN_SWEEP:
	{
		return &slOFMAssassinSweep[0];
	}
	case SCHED_MASSASSIN_COVER_AND_RELOAD:
	{
		return &slOFMAssassinHideReload[0];
	}
	case SCHED_MASSASSIN_FOUND_ENEMY:
	{
		return &slOFMAssassinFoundEnemy[0];
	}
	case SCHED_VICTORY_DANCE:
	{
		if (InSquad())
		{
			if (!IsLeader())
			{
				return &slOFMAssassinFail[0];
			}
		}

		return &slOFMAssassinVictoryDance[0];
	}
	case SCHED_MASSASSIN_SUPPRESS:
	{
		if (m_hEnemy->IsPlayer() && m_fFirstEncounter)
		{
			m_fFirstEncounter = false; // after first encounter, leader won't issue handsigns anymore when he has a new enemy
			return &slOFMAssassinSignalSuppress[0];
		}
		else
		{
			return &slOFMAssassinSuppress[0];
		}
	}
	case SCHED_FAIL:
	{
		if (m_hEnemy != NULL)
		{
			// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
			return &slOFMAssassinCombatFail[0];
		}

		return &slOFMAssassinFail[0];
	}
	case SCHED_MASSASSIN_REPEL:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slOFMAssassinRepel[0];
	}
	case SCHED_MASSASSIN_REPEL_ATTACK:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slOFMAssassinRepelAttack[0];
	}
	case SCHED_MASSASSIN_REPEL_LAND:
	{
		return &slOFMAssassinRepelLand[0];
	}
	default:
	{
		return CSquadMonster::GetScheduleOfType(Type);
	}
	}
}

bool CMOFAssassin::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_iAssassinHead = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq("standgroundrange", pkvd->szKeyName))
	{
		m_flStandGroundRange = atof(pkvd->szValue);
		return true;
	}

	return CSquadMonster::KeyValue(pkvd);
}

//=========================================================
// CMOFAssassinRepel - when triggered, spawns a monster_male_assassin
// repelling down a line.
//=========================================================

class CMOFAssassinRepel : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	int m_iSpriteTexture; // Don't save, precache
};

LINK_ENTITY_TO_CLASS(monster_assassin_repel, CMOFAssassinRepel);

void CMOFAssassinRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;

	SetUse(&CMOFAssassinRepel::RepelUse);
}

void CMOFAssassinRepel::Precache()
{
	UTIL_PrecacheOther("monster_male_assassin");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}

void CMOFAssassinRepel::RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;
	*/

	CBaseEntity* pEntity = Create("monster_male_assassin", pev->origin, pev->angles);
	CBaseMonster* pGrunt = pEntity->MyMonsterPointer();
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



//=========================================================
// DEAD MALE ASSASSIN PROP
//=========================================================
class CDeadMOFAssassin : public CBaseMonster
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_HUMAN_MILITARY; }

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	static char* m_szPoses[3];
};

char* CDeadMOFAssassin::m_szPoses[] = {"deadstomach", "deadside", "deadsitting"};

bool CDeadMOFAssassin::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_massassin_dead, CDeadMOFAssassin);

//=========================================================
// ********** DeadMOFAssassin SPAWN **********
//=========================================================
void CDeadMOFAssassin::Spawn()
{
	PRECACHE_MODEL("models/massn.mdl");
	SET_MODEL(ENT(pev), "models/massn.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hgrunt with bad pose\n");
	}

	// Corpses have less health
	pev->health = 8;

	// map old bodies onto new bodies
	switch (pev->body)
	{
	case 0: // Grunt with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(MAssassinBodygroup::Heads, MAssassinHead::White);
		SetBodygroup(MAssassinBodygroup::Weapons, MAssassinWeapon::MP5);
		break;
	case 1: // Commander with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(MAssassinBodygroup::Heads, MAssassinHead::ThermalVision);
		SetBodygroup(MAssassinBodygroup::Weapons, MAssassinWeapon::MP5);
		break;
	case 2: // Grunt no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(MAssassinBodygroup::Heads, MAssassinHead::Black);
		SetBodygroup(MAssassinBodygroup::Weapons, MAssassinWeapon::None);
		break;
	case 3: // Commander no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(MAssassinBodygroup::Heads, MAssassinHead::ThermalVision);
		SetBodygroup(MAssassinBodygroup::Weapons, MAssassinWeapon::None);
		break;
	}

	MonsterInitDead();
}
