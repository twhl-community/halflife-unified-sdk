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
#include "ReplacementMaps.h"
#include "talkmonster.h"
#include "defaultai.h"
#include "scripted.h"
#include "scientist.h"

const ReplacementMap RosenbergSentenceReplacement{
	{{"SC_POK", "RO_POK"},
		{"SC_SCREAM", "RO_SCREAM"},
		{"SC_HEAL", "RO_HEAL"},
		{"SC_PLFEAR", "RO_PLFEAR"},
		{"SC_FEAR", "RO_FEAR"}},
	true};

class CRosenberg : public CScientist
{
public:
	void OnCreate() override
	{
		CScientist::OnCreate();
		pev->model = MAKE_STRING("models/rosenberg.mdl");

		m_iszUse = MAKE_STRING("RO_OK");
		m_iszUnUse = MAKE_STRING("RO_WAIT");

		m_SentenceReplacement = &RosenbergSentenceReplacement;
	}

	void Precache() override;

	void Spawn() override
	{
		// Scientist changes pitch based on head submodel, so force it back.
		CScientist::Spawn();
		m_voicePitch = 100;
	}

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	void PainSound() override;

	void TalkInit() override;
};

LINK_ENTITY_TO_CLASS(monster_rosenberg, CRosenberg);

void CRosenberg::Precache()
{
	PrecacheModel(STRING(pev->model));
	PrecacheSound("rosenberg/ro_pain1.wav");
	PrecacheSound("rosenberg/ro_pain2.wav");
	PrecacheSound("rosenberg/ro_pain3.wav");
	PrecacheSound("rosenberg/ro_pain4.wav");
	PrecacheSound("rosenberg/ro_pain5.wav");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

void CRosenberg::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "RO_ANSWER";
	m_szGrp[TLK_QUESTION] = "RO_QUESTION";
	m_szGrp[TLK_IDLE] = "RO_IDLE";
	m_szGrp[TLK_STARE] = "RO_STARE";
	m_szGrp[TLK_STOP] = "RO_STOP";
	m_szGrp[TLK_NOSHOOT] = "RO_SCARED";
	m_szGrp[TLK_HELLO] = "RO_HELLO";

	m_szGrp[TLK_PLHURT1] = "!RO_CUREA";
	m_szGrp[TLK_PLHURT2] = "!RO_CUREB";
	m_szGrp[TLK_PLHURT3] = "!RO_CUREC";

	m_szGrp[TLK_PHELLO] = "RO_PHELLO";
	m_szGrp[TLK_PIDLE] = "RO_PIDLE";
	m_szGrp[TLK_PQUESTION] = "RO_PQUEST";
	m_szGrp[TLK_SMELL] = "RO_SMELL";

	m_szGrp[TLK_WOUND] = "RO_WOUND";
	m_szGrp[TLK_MORTAL] = "RO_MORTAL";

	m_voicePitch = 100;
}

bool CRosenberg::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	// Disable scientist damage handling so Rosenberg keeps following the player
	return CTalkMonster::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);
}

void CRosenberg::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 4))
	{
	case 0:
		EmitSound(CHAN_VOICE, "rosenberg/ro_pain1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EmitSound(CHAN_VOICE, "rosenberg/ro_pain2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EmitSound(CHAN_VOICE, "rosenberg/ro_pain3.wav", 1, ATTN_NORM);
		break;
	case 3:
		EmitSound(CHAN_VOICE, "rosenberg/ro_pain4.wav", 1, ATTN_NORM);
		break;
	case 4:
		EmitSound(CHAN_VOICE, "rosenberg/ro_pain5.wav", 1, ATTN_NORM);
		break;
	}
}
