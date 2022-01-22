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
#include "talkmonster.h"
#include "defaultai.h"
#include "scripted.h"
#include "blackmesa/barney.h"

/**
*	@brief A copy of Barney that speaks military
*/
class CRecruit : public CBarney
{
public:
	int Classify() override { return CLASS_HUMAN_MILITARY_FRIENDLY; }

	void DeclineFollowing() override;

	void Spawn() override;

	void AlertSound() override;

	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;

	void Precache() override;

	void TalkInit();

	int ObjectCaps() override { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE; }

protected:
	void DropWeapon() override;

	void SpeakKilledEnemy() override;
};

LINK_ENTITY_TO_CLASS(monster_recruit, CRecruit);

void CRecruit::DeclineFollowing()
{
	PlaySentence("RC_POK", 2, VOL_NORM, ATTN_NORM);
}

void CRecruit::SpeakKilledEnemy()
{
	PlaySentence("RC_KILL", 4, VOL_NORM, ATTN_NORM);
}

void CRecruit::DropWeapon()
{
	//Nothing to drop
}

void CRecruit::Spawn()
{
	SpawnCore("models/recruit.mdl", gSkillData.barneyHealth);
}

void CRecruit::AlertSound()
{
	if (m_hEnemy != nullptr)
	{
		if (FOkToSpeak())
		{
			PlaySentence("RC_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}
}

bool CRecruit::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	bool ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) != 0)
	{
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == nullptr)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) != 0 || IsFacing(pevAttacker, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence("RC_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(true);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("RC_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("RC_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}

void CRecruit::Precache()
{
	PrecacheCore("models/recruit.mdl");
}

void CRecruit::TalkInit()
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

	m_szGrp[TLK_PHELLO] = nullptr;			  //"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = nullptr;			  //"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "RC_PQUEST"; // UNDONE

	m_szGrp[TLK_SMELL] = "RC_SMELL";

	m_szGrp[TLK_WOUND] = "RC_WOUND";
	m_szGrp[TLK_MORTAL] = "RC_MORTAL";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}
