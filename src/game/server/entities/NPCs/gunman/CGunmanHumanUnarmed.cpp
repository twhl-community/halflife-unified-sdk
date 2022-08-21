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

#include <nlohmann/detail/value_t.hpp>

#include "cbase.h"
#include "talkmonster.h"
#include "defaultai.h"
#include "scripted.h"
#include "blackmesa/barney.h"
#include "military/osprey.h"

const int SF_HUMANUNARMED_NOTSOLID = 4;

class CGunmanHumanUnarmed : public CBarney
{
public:
	int Classify() override
	{
	    return CLASS_PLAYER_ALLY;
	}

	void DeclineFollowing() override;

	void OnCreate() override;

	void Spawn() override;

	void AlertSound() override;
    
	void TalkInit() override;

	int ObjectCaps() override
	{
	    return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;
	}

	bool KeyValue(KeyValueData* pkvd) override;

	void SetYawSpeed() override;

    const char* m_szAnswerSentences;
	const char* m_szQuestionSentences;
    const char* m_szIdleSentences;
	const char* m_szStareSentences;
	const char* m_szUseSentences;
	const char* m_szUnuseSentences;
    const char* m_szPainSentences;
	const char* m_szStopSentences;
	const char* m_szNoShootSentences;
	const char* m_szHelloSentences;
	const char* m_szPlHurt1Sentences;
	const char* m_szPlHurt2Sentences;
	const char* m_szPlHurt3Sentences;
	const char* m_szPHelloSentences;
	const char* m_szPIdleSentences;
    const char* m_szPQuestionSentences;
	const char* m_szSmellSentences;
	const char* m_szWoundSentences;
	const char* m_szMortalSentences;
	const char* m_szKillSentences;
	const char* m_szAttackSentences;

	int m_iBodygroup = 0;
	int m_iDecoreGroup = 0;

protected:
	void DropWeapon() override;
	void SpeakKilledEnemy() override;
};

LINK_ENTITY_TO_CLASS(monster_human_unarmed, CGunmanHumanUnarmed);

bool CGunmanHumanUnarmed::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "gn_answer"))
	{
		m_szAnswerSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_question"))
	{
		m_szQuestionSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_idle"))
	{
		m_szIdleSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_stare"))
	{
		m_szStareSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_use"))
	{
		m_szUseSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_unuse"))
	{
		m_szUnuseSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_stop"))
	{
		m_szStopSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_noshoot"))
	{
		m_szNoShootSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_hello"))
	{
		m_szHelloSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_plhurt1"))
	{
		m_szPlHurt1Sentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_plhurt2"))
	{
		m_szPlHurt2Sentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_plhurt3"))
	{
		m_szPlHurt3Sentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_phello"))
	{
		m_szPHelloSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_pidle"))
	{
		m_szPIdleSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_pquestion"))
	{
		m_szPQuestionSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_smell"))
	{
		m_szSmellSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_wound"))
	{
		m_szWoundSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_mortal"))
	{
		m_szMortalSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_kill"))
	{
		m_szKillSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_attack"))
	{
		m_szAttackSentences = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gunstate"))
	{
		m_iBodygroup = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "decore"))
	{
		m_iDecoreGroup = atoi(pkvd->szValue);
		return true;
	}

	return CBarney::KeyValue(pkvd);
}

void CGunmanHumanUnarmed::OnCreate()
{
	CBarney::OnCreate();

	pev->health = GetSkillFloat("barney_health"sv);
	pev->model = MAKE_STRING("models/gunmantrooper_ng.mdl");
}

void CGunmanHumanUnarmed::Spawn()
{
	Precache();

	SetModel(STRING(pev->model));

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->view_ofs = Vector(0, 0, 50);  // position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

    if ((pev->spawnflags & SF_HUMANUNARMED_NOTSOLID) != 0)
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

    if (!pev->netname)
		pev->netname = ALLOC_STRING("Unarmed Gunman");

	SetBodygroup(1, m_iBodygroup);
	SetBodygroup(2, m_iDecoreGroup);

	MonsterInit();
}

void CGunmanHumanUnarmed::SetYawSpeed()
{
    int ys = 0;

	switch (m_Activity)
	{
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CGunmanHumanUnarmed::DeclineFollowing()
{
	PlaySentence("GN_POK", 2, VOL_NORM, ATTN_NORM);
}

void CGunmanHumanUnarmed::SpeakKilledEnemy()
{
	PlaySentence(m_szKillSentences != nullptr ? m_szKillSentences : "GN_KILL", 4, VOL_NORM, ATTN_NORM);
}

void CGunmanHumanUnarmed::DropWeapon()
{
	//Nothing to drop
}

void CGunmanHumanUnarmed::AlertSound()
{
	if (m_hEnemy != nullptr)
	{
		if (FOkToSpeak())
		{
			PlaySentence(m_szAttackSentences != nullptr ? m_szAttackSentences : "GN_ATTACK", RANDOM_FLOAT(2.8f, 3.2f), VOL_NORM, ATTN_IDLE);
		}
	}
}

void CGunmanHumanUnarmed::TalkInit()
{
	CTalkMonster::TalkInit();

	m_szGrp[TLK_ANSWER] = m_szAnswerSentences != nullptr ? m_szAnswerSentences : "GN_ANSWER";
	m_szGrp[TLK_QUESTION] = m_szQuestionSentences != nullptr ? m_szQuestionSentences : "GN_QUESTION";
	m_szGrp[TLK_IDLE] = m_szIdleSentences != nullptr ? m_szIdleSentences : "GN_IDLE";
	m_szGrp[TLK_STARE] = m_szStareSentences != nullptr ? m_szStareSentences : "GN_STARE";
	m_szGrp[TLK_USE] = m_szUseSentences != nullptr ? m_szUseSentences : "GN_OK";
	m_szGrp[TLK_UNUSE] = m_szUnuseSentences != nullptr ? m_szUnuseSentences : "GN_WAIT";
	m_szGrp[TLK_STOP] = m_szStopSentences != nullptr ? m_szStopSentences : "GN_STOP";

	m_szGrp[TLK_NOSHOOT] = m_szNoShootSentences != nullptr ? m_szNoShootSentences : "GN_SCARED";
	m_szGrp[TLK_HELLO] = m_szHelloSentences != nullptr ? m_szHelloSentences : "GN_HELLO";

	m_szGrp[TLK_PLHURT1] = m_szPlHurt1Sentences != nullptr ? m_szPlHurt1Sentences : "!GN_CUREA";
	m_szGrp[TLK_PLHURT2] = m_szPlHurt2Sentences != nullptr ? m_szPlHurt2Sentences : "!GN_CUREB";
	m_szGrp[TLK_PLHURT3] = m_szPlHurt3Sentences != nullptr ? m_szPlHurt3Sentences : "!GN_CUREC";

	m_szGrp[TLK_PHELLO] = m_szPHelloSentences != nullptr ? m_szPHelloSentences : nullptr;
	m_szGrp[TLK_PIDLE] = m_szPIdleSentences != nullptr ? m_szPIdleSentences : nullptr;
	m_szGrp[TLK_PQUESTION] = m_szPQuestionSentences != nullptr ? m_szPQuestionSentences : "GN_PQUEST";

	m_szGrp[TLK_SMELL] = m_szSmellSentences != nullptr ? m_szSmellSentences : "GN_SMELL";

	m_szGrp[TLK_WOUND] = m_szWoundSentences != nullptr ? m_szWoundSentences : "GN_WOUND";
	m_szGrp[TLK_MORTAL] = m_szMortalSentences != nullptr ? m_szMortalSentences : "GN_MORTAL";


	//m_szPainSentences != nullptr ? m_szPainSentences : 

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}

/*********************************************************************************************************/

const int SF_TRAININGBOT_NOTSOLID = 4;
const int SF_TRAININGBOT_NORINGS = 1024;

enum trainingbot_atchmnt
{
	TRAININGBOT_ATTACHMENT_BODYSPIKE = 1,
	TRAININGBOT_ATTACHMENT_LEG1,
	TRAININGBOT_ATTACHMENT_LEG2,
	TRAININGBOT_ATTACHMENT_LEG3
};

const int TRAININGBOT_BEAM_PARENTATTACHMENT = TRAININGBOT_ATTACHMENT_BODYSPIKE;

const int TRAININGBOT_MAX_BEAMS = 8;

class CGunmanTrainingBot : public CBaseMonster
{
public:
	int Classify() override {
		return CLASS_MACHINE;
	}

	void OnCreate() override;

	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	void Think() override;
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;

	void DeathSound() override;
	void IdleSound() override;
	void PainSound() override;

	void ExplodeDie();
	void UpdateGoal();

	void CreateBeams();
	void CreateBeam(int attachment);
	void ClearBeams();

	void CreateSparkBall(int attachment);
	void ClearSparkBall();

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	int ObjectCaps() override
	{
		return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;
	}

    void EXPORT FlyThink();
	void EXPORT LinearMove();
	void EXPORT LinearMoveDone();

    void EXPORT CrashTouch(CBaseEntity* pOther);
	void EXPORT DyingThink();

	void Flight();

	bool KeyValue(KeyValueData* pkvd) override;

    bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

    static constexpr const char* pIdleSounds[] = {
        "drone/drone_idle.wav",
    };

	static constexpr const char* pPainSounds[] = {
			"buttons/spark5.wav",
			"buttons/spark6.wav"
	};

    static constexpr const char* pDeathSounds[] = {
		"drone/drone_flinch1.wav",
		"drone/drone_flinch2.wav"
    };

	int m_iGibModelindex;
	int m_iExplModelindex;
	int m_iSmokeModelIndex;

    int m_iBeams;
	CBeam* m_pBeam[TRAININGBOT_MAX_BEAMS];
	CSprite* m_hSparkBall;

	int m_isUsingOspreyMovement;
    
	Vector m_vel1;
	Vector m_vel2;
	Vector m_pos1;
	Vector m_pos2;
	Vector m_ang1;
	Vector m_ang2;

	float m_startTime;
	float m_dTime;

	Vector m_velocity;
};

TYPEDESCRIPTION CGunmanTrainingBot::m_SaveData[] = {
	DEFINE_FIELD(CGunmanTrainingBot, m_hSparkBall, FIELD_CLASSPTR),
	DEFINE_ARRAY(CGunmanTrainingBot, m_pBeam, FIELD_CLASSPTR, TRAININGBOT_MAX_BEAMS),
	DEFINE_FIELD(CGunmanTrainingBot, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(CGunmanTrainingBot, m_vecFinalDest, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CGunmanTrainingBot, m_pGoalEnt, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CGunmanTrainingBot, CBaseMonster);

LINK_ENTITY_TO_CLASS(monster_trainingbot, CGunmanTrainingBot);

bool CGunmanTrainingBot::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "isUsingOspreyMovement"))
	{
		m_isUsingOspreyMovement = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

void CGunmanTrainingBot::SetYawSpeed()
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
	default:
		ys = 45;
	}

	pev->yaw_speed = ys;
}

void CGunmanTrainingBot::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = 200;
	pev->model = MAKE_STRING("models/batterybot.mdl");
}

void CGunmanTrainingBot::Spawn()
{
	Precache();

	SetModel(STRING(pev->model));

	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_NOCLIP;
	pev->flags |= FL_MONSTER;
	pev->spawnflags |= FL_FLY;
	m_bloodColor = DONT_BLEED;
	pev->takedamage = DAMAGE_AIM;
	pev->view_ofs = VEC_VIEW;
	m_flFieldOfView = 0.5;
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_DOORS_GROUP;

    m_pos2 = pev->origin;
	m_ang2 = pev->angles;
	m_vel2 = pev->velocity;
	m_startTime = gpGlobals->time;

	MonsterInit();

    if ((pev->spawnflags & SF_TRAININGBOT_NOTSOLID) != 0)
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

	if (!pev->netname)
		pev->netname = ALLOC_STRING("Training Bot");

	if (pev->speed <= 0.0) {
		pev->speed = 150.0f;
	}

	// SetUse( UseFunction( this.Use ) );

	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG(0, 0xFF);

	InitBoneControllers();

	IdleSound();

	CreateBeams();
	CreateSparkBall(TRAININGBOT_BEAM_PARENTATTACHMENT);
}

void CGunmanTrainingBot::Precache()
{
	PrecacheModel(STRING(pev->model));
    
	m_iGibModelindex = PrecacheModel("models/battgib.mdl");
	m_iExplModelindex = PrecacheModel("sprites/zerogxplode.spr");
	m_iSmokeModelIndex = PrecacheModel("sprites/steam1.spr");

	PrecacheModel("sprites/lgtning.spr");
	PrecacheModel("sprites/ballspark.spr");
    
	PRECACHE_SOUND_ARRAY(pIdleSounds);
    PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

void CGunmanTrainingBot::Think()
{
    StudioFrameAdvance();
	UpdateShockEffect();

    if (m_isUsingOspreyMovement) {
		SetThink(&CGunmanTrainingBot::FlyThink);
	} else {
		SetThink(&CGunmanTrainingBot::LinearMove);
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

bool CGunmanTrainingBot::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// Always-Gib
	if ((bitsDamageType & DMG_CRUSH) != 0 || (bitsDamageType & DMG_FALL) != 0 || (bitsDamageType & DMG_BLAST) != 0 || (bitsDamageType & DMG_ENERGYBEAM) != 0 || (bitsDamageType & DMG_ACID) != 0 || (bitsDamageType & DMG_SLOWBURN) != 0 || (bitsDamageType & DMG_MORTAR) != 0)
	{
		if ((bitsDamageType & DMG_NEVERGIB) != 0)
			bitsDamageType &= ~DMG_NEVERGIB;
		if ((bitsDamageType & DMG_ALWAYSGIB) == 0)
			bitsDamageType |= DMG_ALWAYSGIB;
	}
	else
	// Never Gib
	{
		if ((bitsDamageType & DMG_ALWAYSGIB) != 0)
			bitsDamageType &= ~DMG_ALWAYSGIB;
		if ((bitsDamageType & DMG_NEVERGIB) == 0)
			bitsDamageType |= DMG_NEVERGIB;
	}

	if (IsAlive())
		PainSound();

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CGunmanTrainingBot::Killed(entvars_t* pevAttacker, int iGib)
{
	SetThink(nullptr);

    ClearBeams();
	ClearSparkBall();

	EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, RANDOM_SOUND_ARRAY(pIdleSounds), 0.0, 0.0, SND_STOP, PITCH_NORM);
	DeathSound();

    if (iGib != GIB_NEVER) {
		ExplodeDie();
	} else {
		pev->deadflag = DEAD_DEAD;

		pev->framerate = 0;
		pev->effects = EF_NOINTERP;
		pev->movetype = MOVETYPE_TOSS;

		pev->gravity = 0.3f;
		pev->avelocity = Vector(RANDOM_FLOAT(-20, 20), 0, RANDOM_FLOAT(-50, 50));
		pev->size = Vector(-32, -32, -16), Vector(32, 32, 0);

		SetTouch(&CGunmanTrainingBot::CrashTouch);
		SetThink(&CGunmanTrainingBot::DyingThink);

		pev->nextthink = gpGlobals->time + 0.1f;
		m_startTime = gpGlobals->time + 6.0f;
	}

	CBaseMonster::Killed(pevAttacker, iGib);
}

void CGunmanTrainingBot::DyingThink()
{
	StudioFrameAdvance();

	if (pev->angles.x < 180.0f)
		pev->angles.x += 6.0f;

	// still falling?
	if (m_startTime > gpGlobals->time)
	{
		// lots of smoke
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(m_iSmokeModelIndex);
		WRITE_BYTE(25); // scale * 10
		WRITE_BYTE(10);					 // framerate
		MESSAGE_END();

	    Create("spark_shower", pev->origin, 
			Vector(RANDOM_FLOAT(-150, 150), RANDOM_FLOAT(-150, 150), RANDOM_FLOAT(-150, -50)), nullptr);

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	// falling time is out
	else
	{
		SetTouch(nullptr);
		m_startTime = gpGlobals->time;
		ExplodeDie();
	}
}

void CGunmanTrainingBot::CrashTouch(CBaseEntity* pOther)
{
	// only crash if we hit something solid
	if (pev->solid == SOLID_BSP)
	{
		SetTouch(nullptr);
		m_startTime = gpGlobals->time;
		ExplodeDie();
	}
}

void CGunmanTrainingBot::ExplodeDie() {
	TraceResult tr;

    Vector vecSpot = pev->origin + Vector(0, 0, 8);

    UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

    if (tr.flFraction != 1.0) {
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (200 - 24) * 0.6);
	} else {
		pev->origin = pev->origin;
	}

    if (RANDOM_FLOAT(0, 1) < 0.5) {
		UTIL_DecalTrace(&tr, DECAL_SCORCH1);
	} else {
		UTIL_DecalTrace(&tr, DECAL_SCORCH2);
	}

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT(m_iExplModelindex);
	WRITE_BYTE(10);
	WRITE_BYTE(15);
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

    MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLODEMODEL);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 16.0f);
	WRITE_COORD(300.0);
	WRITE_SHORT(m_iGibModelindex);
	WRITE_SHORT(4);
	WRITE_BYTE(200);
	MESSAGE_END();

	RadiusDamage(pev, pev, 100, CLASS_NONE, DMG_BLAST);
    UTIL_Remove(this);
}

void CGunmanTrainingBot::DeathSound() {
	EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, ATTN_NORM, 0, PITCH_NORM);
}

void CGunmanTrainingBot::IdleSound() {
	if (RANDOM_LONG(0, 2) == 0) {
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, RANDOM_SOUND_ARRAY(pIdleSounds), 1.0, ATTN_NORM, 0, PITCH_NORM);
	}
}

void CGunmanTrainingBot::PainSound()
{
	float flVolume = RANDOM_FLOAT(0.7, 1.0);
	EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, flVolume);
}

void CGunmanTrainingBot::CreateBeams()
{
    CreateBeam(TRAININGBOT_ATTACHMENT_LEG1);
    CreateBeam(TRAININGBOT_ATTACHMENT_LEG2);
    CreateBeam(TRAININGBOT_ATTACHMENT_LEG3);
}

void CGunmanTrainingBot::CreateBeam(int attachment)
{
	if (m_iBeams >= TRAININGBOT_MAX_BEAMS)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);

	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->EntsInit(entindex(), entindex());
	m_pBeam[m_iBeams]->SetStartAttachment(attachment);
	m_pBeam[m_iBeams]->SetEndAttachment(TRAININGBOT_BEAM_PARENTATTACHMENT);
	m_pBeam[m_iBeams]->SetColor(127, 255, 212);
	m_pBeam[m_iBeams]->SetBrightness(255);
	m_pBeam[m_iBeams]->SetNoise(80);

	m_iBeams++;
}

void CGunmanTrainingBot::ClearBeams()
{
	for (int i = 0; i < TRAININGBOT_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = nullptr;
		}
	}
	m_iBeams = 0;
}

void CGunmanTrainingBot::CreateSparkBall(int attachment)
{
	if (m_hSparkBall)
		return;

	m_hSparkBall = CSprite::SpriteCreate("sprites/ballspark.spr", pev->origin, true);
    if (m_hSparkBall)
    {
		m_hSparkBall->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone);
		m_hSparkBall->SetAttachment(edict(), attachment);
    }
}

void CGunmanTrainingBot::ClearSparkBall()
{
	if (m_hSparkBall)
	{
		UTIL_Remove(m_hSparkBall);
		m_hSparkBall = nullptr;
	}
}

void CGunmanTrainingBot::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);

    UTIL_Sparks(ptr->vecEndPos);
}

void CGunmanTrainingBot::Flight()
{
	float t = (gpGlobals->time - m_startTime);
	float scale = 1.0f / m_dTime;

	float f = UTIL_SplineFraction(t * scale, 1.0);
    
	Vector pos = (m_pos1 + m_vel1 * t) * (1.0f - f) + (m_pos2 - m_vel2 * (m_dTime - t)) * f;
	Vector ang = (m_ang1) * (1.0f - f) + (m_ang2)*f;
	m_velocity = m_vel1 * (1.0f - f) + m_vel2 * f;

    UTIL_SetOrigin(pev, pos);
	pev->angles = ang;

	UTIL_MakeAimVectors(pev->angles);
	DotProduct(gpGlobals->v_forward, m_velocity);
}

void CGunmanTrainingBot::FlyThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	if (m_pGoalEnt == nullptr && !FStringNull(pev->target)) // this monster has a target
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));
		UpdateGoal();
	}

	if (gpGlobals->time > m_startTime + m_dTime)
	{
		if (m_pGoalEnt != nullptr)
		{
			m_pGoalEnt = UTIL_FindEntityByTargetname(nullptr, STRING(m_pGoalEnt->pev->target));
			UpdateGoal();
		}
	}

	Flight();
}

void CGunmanTrainingBot::UpdateGoal()
{
	if (m_pGoalEnt)
	{
		m_pos1 = m_pos2;
		m_ang1 = m_ang2;
		m_vel1 = m_vel2;
		m_pos2 = m_pGoalEnt->pev->origin;
		m_ang2 = m_pGoalEnt->pev->angles;
		UTIL_MakeAimVectors(Vector(0, m_ang2.y, 0));
		m_vel2 = gpGlobals->v_forward * m_pGoalEnt->pev->speed;

		m_startTime = m_startTime + m_dTime;
		m_dTime = 2.0f * (m_pos1 - m_pos2).Length() / (m_vel1.Length() + pev->speed);

		if (m_ang1.y - m_ang2.y < -180)
		{
			m_ang1.y += 360;
		}
		else if (m_ang1.y - m_ang2.y > 180)
		{
			m_ang1.y -= 360;
		}
	}
}

void CGunmanTrainingBot::LinearMove() {
	StudioFrameAdvance();

	if (m_pGoalEnt == nullptr && !FStringNull(pev->target)) {
		m_pGoalEnt = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));
	}
	
	if (m_pGoalEnt && pev->speed > 0.0f) {
		m_vecFinalDest = m_pGoalEnt->pev->origin;

        if (m_vecFinalDest == pev->origin) {
			LinearMoveDone();
			return;
		}

		// set destdelta to the vector needed to move
		Vector vecDestDelta = m_vecFinalDest - pev->origin;

		// divide vector length by speed to get time to reach dest
		float flTravelTime = vecDestDelta.Length() / pev->speed;

		// set nextthink to trigger a call to LinearMoveDone when dest is reached
		SetThink(&CGunmanTrainingBot::LinearMoveDone);
		pev->nextthink = gpGlobals->time + flTravelTime;

		// scale the destdelta vector by the time spent traveling to get velocity
		pev->velocity = vecDestDelta / flTravelTime;
	}
}

void CGunmanTrainingBot::LinearMoveDone()
{
	SetThink(nullptr);

	UTIL_SetOrigin(pev, m_vecFinalDest);
	pev->velocity = g_vecZero;
	pev->nextthink = -1;

    m_pGoalEnt = UTIL_FindEntityByTargetname(nullptr, STRING(m_pGoalEnt->pev->target));
	Think();
}