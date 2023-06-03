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

const ReplacementMap DrillSergeantSentenceReplacement{
	{{"BA_POK", "DR_POK"},
		{"BA_KILL", "DR_KILL"},
		{"BA_ATTACK", "DR_ATTACK"},
		{"BA_MAD", "DR_MAD"},
		{"BA_SHOT", "DR_SHOT"}},
	true};

/**
 *	@brief A copy of Barney that speaks military loudly
 */
class CDrillSergeant : public CBarney
{
public:
	int ObjectCaps() override { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE; }

	void OnCreate() override
	{
		CBarney::OnCreate();

		pev->health = GetSkillFloat("barney_health"sv);
		pev->model = MAKE_STRING("models/drill.mdl");

		m_iszUse = MAKE_STRING("DR_OK");
		m_iszUnUse = MAKE_STRING("DR_WAIT");

		SetClassification("human_military_ally");

		m_SentenceReplacement = &DrillSergeantSentenceReplacement;
	}

	void TalkInit() override
	{
		CTalkMonster::TalkInit();

		// scientists speach group names (group names are in sentences.txt)

		m_szGrp[TLK_ANSWER] = "DR_ANSWER";
		m_szGrp[TLK_QUESTION] = "DR_QUESTION";
		m_szGrp[TLK_IDLE] = "DR_IDLE";
		m_szGrp[TLK_STARE] = "DR_STARE";
		m_szGrp[TLK_STOP] = "DR_STOP";

		m_szGrp[TLK_NOSHOOT] = "DR_SCARED";
		m_szGrp[TLK_HELLO] = "DR_HELLO";

		m_szGrp[TLK_PLHURT1] = "!DR_CUREA";
		m_szGrp[TLK_PLHURT2] = "!DR_CUREB";
		m_szGrp[TLK_PLHURT3] = "!DR_CUREC";

		m_szGrp[TLK_PHELLO] = nullptr;		  //"BA_PHELLO";		// UNDONE
		m_szGrp[TLK_PIDLE] = nullptr;		  //"BA_PIDLE";			// UNDONE
		m_szGrp[TLK_PQUESTION] = "DR_PQUEST"; // UNDONE

		m_szGrp[TLK_SMELL] = "DR_SMELL";

		m_szGrp[TLK_WOUND] = "DR_WOUND";
		m_szGrp[TLK_MORTAL] = "DR_MORTAL";

		// get voice for head - just one barney voice for now
		m_voicePitch = 100;
	}

protected:
	// Nothing to drop
	void DropWeapon() override {}
};

LINK_ENTITY_TO_CLASS(monster_drillsergeant, CDrillSergeant);
