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
#include "blackmesa/barney.h"

const ReplacementMap RecruitSentenceReplacement{
	{{"BA_POK", "RC_POK"},
		{"BA_KILL", "RC_KILL"},
		{"BA_ATTACK", "RC_ATTACK"},
		{"BA_MAD", "RC_MAD"},
		{"BA_SHOT", "RC_SHOT"}},
	true};

/**
 *	@brief A copy of Barney that speaks military
 */
class CRecruit : public CBarney
{
public:
	int ObjectCaps() override { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE; }

	void OnCreate() override
	{
		CBarney::OnCreate();

		pev->health = GetSkillFloat("barney_health"sv);
		pev->model = MAKE_STRING("models/recruit.mdl");
		SetClassification("human_military_ally");
		m_SentenceReplacement = &RecruitSentenceReplacement;
	}

	void TalkInit() override
	{
		CTalkMonster::TalkInit();

		// scientists speach group names (group names are in sentences.txt)

		m_szGrp[TLK_ANSWER] = "RC_ANSWER";
		m_szGrp[TLK_QUESTION] = "RC_QUESTION";
		m_szGrp[TLK_IDLE] = "RC_IDLE";
		m_szGrp[TLK_STARE] = "RC_STARE";
		m_szGrp[TLK_USE] = "RC_OK";
		m_szGrp[TLK_UNUSE] = "RC_WAIT";
		m_szGrp[TLK_STOP] = "RC_STOP";

		m_szGrp[TLK_NOSHOOT] = "RC_SCARED";
		m_szGrp[TLK_HELLO] = "RC_HELLO";

		m_szGrp[TLK_PLHURT1] = "!RC_CUREA";
		m_szGrp[TLK_PLHURT2] = "!RC_CUREB";
		m_szGrp[TLK_PLHURT3] = "!RC_CUREC";

		m_szGrp[TLK_PHELLO] = nullptr;		  //"BA_PHELLO";		// UNDONE
		m_szGrp[TLK_PIDLE] = nullptr;		  //"BA_PIDLE";			// UNDONE
		m_szGrp[TLK_PQUESTION] = "RC_PQUEST"; // UNDONE

		m_szGrp[TLK_SMELL] = "RC_SMELL";

		m_szGrp[TLK_WOUND] = "RC_WOUND";
		m_szGrp[TLK_MORTAL] = "RC_MORTAL";

		// get voice for head - just one barney voice for now
		m_voicePitch = 100;
	}

protected:
	// Nothing to drop
	void DropWeapon() override {}
};

LINK_ENTITY_TO_CLASS(monster_recruit, CRecruit);
