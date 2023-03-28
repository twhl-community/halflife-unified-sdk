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

#include <tuple>

#define GRUNT_CLIP_SIZE 36	 //!< how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define GRUNT_VOL 0.35		 //!< volume of grunt sounds
#define GRUNT_ATTN ATTN_NORM //!< attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH 20
#define HGRUNT_DMG_HEADSHOT (DMG_BULLET | DMG_CLUB) //!< damage types that can kill a grunt with a single headshot.
#define HGRUNT_NUM_HEADS 2							//!< how many grunt heads are there?
#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE 15			//!< must do at least this much damage in one shot to head to score a headshot kill
#define HGRUNT_SENTENCE_VOLUME (float)0.35			//!< volume of grunt sentences

#define HGRUNT_9MMAR (1 << 0)
#define HGRUNT_HANDGRENADE (1 << 1)
#define HGRUNT_GRENADELAUNCHER (1 << 2)
#define HGRUNT_SHOTGUN (1 << 3)

namespace HGruntBodyGroup
{
enum HGruntBodyGroup
{
	Head = 1,
	Weapons = 2
};
}

namespace HGruntHead
{
enum HGruntHead
{
	Grunt = 0,
	Commander,
	Shotgun,
	M203
};
}

namespace HGruntWeapon
{
enum HGruntWeapon
{
	Blank = 0,
	MP5,
	Shotgun,
};
}

#define HGRUNT_AE_RELOAD (2)
#define HGRUNT_AE_KICK (3)
#define HGRUNT_AE_BURST1 (4)
#define HGRUNT_AE_BURST2 (5)
#define HGRUNT_AE_BURST3 (6)
#define HGRUNT_AE_GREN_TOSS (7)
#define HGRUNT_AE_GREN_LAUNCH (8)
#define HGRUNT_AE_GREN_DROP (9)
#define HGRUNT_AE_CAUGHT_ENEMY (10) //!< grunt established sight with an enemy (player only) that had previously eluded the squad.
#define HGRUNT_AE_DROP_GUN (11)		//!< grunt (probably dead) is dropping his mp5.

enum
{
	SCHED_GRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE, //!< move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_GRUNT_COVER_AND_RELOAD,
	SCHED_GRUNT_SWEEP,
	SCHED_GRUNT_FOUND_ENEMY,
	SCHED_GRUNT_REPEL,
	SCHED_GRUNT_REPEL_ATTACK,
	SCHED_GRUNT_REPEL_LAND,
	SCHED_GRUNT_WAIT_FACE_ENEMY,
	SCHED_GRUNT_TAKECOVER_FAILED, //!< special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_GRUNT_ELOF_FAIL,
};

enum
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_GRUNT_SPEAK_SENTENCE,
	TASK_GRUNT_CHECK_FIRE,
};

#define bits_COND_GRUNT_NOFIRE (bits_COND_SPECIAL1)

enum HGRUNT_SENTENCE_TYPES
{
	HGRUNT_SENT_NONE = -1,
	HGRUNT_SENT_GREN = 0,
	HGRUNT_SENT_ALERT,
	HGRUNT_SENT_MONSTER,
	HGRUNT_SENT_COVER,
	HGRUNT_SENT_THROW,
	HGRUNT_SENT_CHARGE,
	HGRUNT_SENT_TAUNT,
};

class CHGrunt : public CSquadMonster
{
public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;

	/**
	*	@brief Overridden for human grunts because they hear the DANGER sound
	*	that is made by hand grenades and other dangerous items.
	*/
	int ISoundMask() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	/**
	*	@brief this is overridden for human grunts because they can throw/shoot grenades when they can't see their
	*	target and the base class doesn't check attacks if the monster cannot see its enemy.
	*	@details !!!BUGBUG - this gets called before a 3-round burst is fired
	*	which means that a friendly can still be hit with up to 2 rounds.
	*	ALSO, grenades will not be tossed if there is a friendly in front, this is a bad bug.
	*	Friendly machine gun fire avoidance will unecessarily prevent the throwing of a grenade as well.
	*/
	bool FCanCheckAttacks() override;

	bool CheckMeleeAttack1(float flDot, float flDist) override;

	/**
	*	@brief overridden for HGrunt, cause FCanCheckAttacks() doesn't disqualify all attacks based on
	*	whether or not the enemy is occluded because unlike the base class,
	*	the HGrunt can attack when the enemy is occluded (throw grenade over wall, etc).
	*	We must disqualify the machine gun attack if the enemy is occluded.
	*/
	bool CheckRangeAttack1(float flDot, float flDist) override;

	/**
	*	@brief this checks the Grunt's grenade attack.
	*/
	bool CheckRangeAttack2(float flDot, float flDist) override;

	/**
	*	@brief overridden for the grunt because he actually uses ammo! (base class doesn't)
	*/
	void CheckAmmo() override;
	void SetActivity(Activity NewActivity) override;
	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;
	void DeathSound() override;
	void PainSound() override;
	void IdleSound() override;

	/**
	*	@brief return the end of the barrel
	*/
	Vector GetGunPosition() override;

	virtual void Shoot(bool firstShotInBurst);

	void PrescheduleThink() override;

	/**
	*	@brief make gun fly through the air.
	*/
	void GibMonster() override;

	/**
	*	@brief say your cued up sentence.
	*	@details Some grunt sentences (take cover and charge) rely on actually being able to execute the intended action.
	*	It's really lame when a grunt says 'COVER ME' and then doesn't move.
	*	The problem is that the sentences were played when the decision to TRY to move to cover was made.
	*	Now the sentence is played after we know for sure that there is a valid path.
	*	The schedule may still fail but in most cases, well after the grunt has started moving.
	*/
	void SpeakSentence();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	CBaseEntity* Kick();
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;

	/**
	*	@brief make sure we're not taking it in the helmet
	*/
	void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	/**
	*	@brief overridden for the grunt because the grunt needs to forget that he is in cover if he's hurt.
	*	(Obviously not in a safe place anymore).
	*/
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	/**
	*	@brief overridden because Alien Grunts are Human Grunt's nemesis.
	*/
	int IRelationship(CBaseEntity* pTarget) override;

	/**
	*	@brief someone else is talking - don't speak
	*/
	virtual bool FOkToSpeak();
	void JustSpoke();

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;

	Vector m_vecTossVelocity;

	bool m_fThrowGrenade;
	bool m_fStanding;
	bool m_fFirstEncounter; //!< only put on the handsign show in the squad's first encounter.
	int m_cClipSize;

	int m_voicePitch;

	int m_iBrassShell;
	int m_iShotgunShell;

	int m_iSentence;

	static const char* pGruntSentences[];

protected:
	bool CheckRangeAttack2Core(float flDot, float flDist, float grenadeSpeed);

	virtual std::tuple<int, Activity> GetSequenceForActivity(Activity NewActivity);

	/**
	 *	@brief Gets a reference to the global grunt question variable used to communicate between grunts
	 */
	virtual int& GetGruntQuestion();
};

/**
 *	@brief when triggered, spawns a monster_human_grunt repelling down a line.
 */
class CHGruntRepel : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RepelUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	int m_iSpriteTexture; // Don't save, precache

protected:
	/**
	 *	@brief Precaches all repel assets
	 *	@param classname Must be a string literal
	 */
	void PrecacheCore(const char* classname);

	/**
	 *	@brief Creates a monster in a repel state
	 *	@param classname Must be a string literal
	 */
	void CreateMonster(const char* classname);
};

class CDeadHGrunt : public CBaseMonster
{
public:
	void OnCreate() override;
	void Spawn() override;
	int Classify() override { return CLASS_HUMAN_MILITARY; }

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	static constexpr const char* m_szPoses[3] = {"deadstomach", "deadside", "deadsitting"};

protected:
	void SpawnCore();
};
