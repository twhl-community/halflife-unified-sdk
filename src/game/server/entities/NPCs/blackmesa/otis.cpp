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
#include "barney.h"

const ReplacementMap OtisSentenceReplacement{
	{{"BA_POK", "OT_POK"},
		{"BA_KILL", "OT_KILL"},
		{"BA_ATTACK", "OT_ATTACK"},
		{"BA_MAD", "OT_MAD"},
		{"BA_SHOT", "OT_SHOT"}},
	true};

namespace OtisBodyGroup
{
/**
 *	@brief See GuardBodyGroup::GuardBodyGroup for the enum that this extends.
 */
enum OtisBodyGroup
{
	Weapons = GuardBodyGroup::Weapons,
	Sleeves,
	Items
};
}

namespace OtisSleeves
{
enum OtisSleeves
{
	Random = -1,
	Long = 0,
	Short
};
}

namespace OtisItem
{
enum OtisItem
{
	None = 0,
	Clipboard,
	Donut
};
}

namespace OtisSkin
{
enum OtisSkin
{
	Random = -1,
	HeadWithHair = 0,
	Bald,
	BlackHeadWithHair
};
}

class COtis : public CBarney
{
public:
	void OnCreate() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Precache() override;
	void Spawn() override;
	void GuardFirePistol() override;

	void TalkInit() override;

protected:
	void DropWeapon() override;

private:
	int m_Sleeves;
	int m_Item;
};

LINK_ENTITY_TO_CLASS(monster_otis, COtis);

void COtis::OnCreate()
{
	CBarney::OnCreate();

	pev->health = GetSkillFloat("otis_health"sv);
	pev->model = MAKE_STRING("models/otis.mdl");

	m_iszUse = MAKE_STRING("OT_OK");
	m_iszUnUse = MAKE_STRING("OT_WAIT");

	m_SentenceReplacement = &OtisSentenceReplacement;
}

void COtis::GuardFirePistol()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_PLAYER_357);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EmitSoundDyn(CHAN_WEAPON, "weapons/de_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--; // take away a bullet!
}

bool COtis::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("sleeves", pkvd->szKeyName))
	{
		m_Sleeves = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq("item", pkvd->szKeyName))
	{
		m_Item = atoi(pkvd->szValue);
		return true;
	}

	return CBarney::KeyValue(pkvd);
}

void COtis::Precache()
{
	CBarney::Precache();
	PrecacheSound("weapons/de_shot1.wav");
}

void COtis::Spawn()
{
	CBarney::Spawn();

	if (pev->skin == OtisSkin::Random)
	{
		pev->skin = RANDOM_LONG(OtisSkin::HeadWithHair, OtisSkin::BlackHeadWithHair);
	}

	if (m_Sleeves == OtisSleeves::Random)
	{
		m_Sleeves = RANDOM_LONG(OtisSleeves::Long, OtisSleeves::Short);
	}

	SetBodygroup(OtisBodyGroup::Sleeves, m_Sleeves);
	SetBodygroup(OtisBodyGroup::Items, m_Item);
}

void COtis::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "OT_ANSWER";
	m_szGrp[TLK_QUESTION] = "OT_QUESTION";
	m_szGrp[TLK_IDLE] = "OT_IDLE";
	m_szGrp[TLK_STARE] = "OT_STARE";
	m_szGrp[TLK_STOP] = "OT_STOP";

	m_szGrp[TLK_NOSHOOT] = "OT_SCARED";
	m_szGrp[TLK_HELLO] = "OT_HELLO";

	m_szGrp[TLK_PLHURT1] = "!OT_CUREA";
	m_szGrp[TLK_PLHURT2] = "!OT_CUREB";
	m_szGrp[TLK_PLHURT3] = "!OT_CUREC";

	m_szGrp[TLK_PHELLO] = nullptr;		  //"OT_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = nullptr;		  //"OT_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "OT_PQUEST"; // UNDONE

	m_szGrp[TLK_SMELL] = "OT_SMELL";

	m_szGrp[TLK_WOUND] = "OT_WOUND";
	m_szGrp[TLK_MORTAL] = "OT_MORTAL";

	// get voice for head - just one otis voice for now
	m_voicePitch = 100;
}


void COtis::DropWeapon()
{
	Vector vecGunPos, vecGunAngles;
	GetAttachment(0, vecGunPos, vecGunAngles);
	DropItem("weapon_eagle", vecGunPos, vecGunAngles);
}

class CDeadOtis : public CBaseMonster
{
public:
	void OnCreate() override;
	void Spawn() override;

	bool HasHumanGibs() override { return true; }

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	int m_GunState = NPCWeaponState::Holstered; // Default to holstered as in the original model.
	int m_Sleeves = OtisSleeves::Long;
	int m_Item = OtisItem::None;
	static const char* m_szPoses[5];
};

const char* CDeadOtis::m_szPoses[] = {"lying_on_back", "lying_on_side", "lying_on_stomach", "stuffed_in_vent", "dead_sitting"};

void CDeadOtis::OnCreate()
{
	CBaseMonster::OnCreate();

	// Corpses have less health
	pev->health = 8; // GetSkillFloat("otis_health"sv);
	pev->model = MAKE_STRING("models/otis.mdl");

	SetClassification("player_ally");
}

bool CDeadOtis::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq("bodystate", pkvd->szKeyName))
	{
		m_GunState = atoi(pkvd->szValue);
		return true;
	}
	if (FStrEq("sleeves", pkvd->szKeyName))
	{
		m_Sleeves = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq("item", pkvd->szKeyName))
	{
		m_Item = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_otis_dead, CDeadOtis);

void CDeadOtis::Spawn()
{
	PrecacheModel(STRING(pev->model));
	SetModel(STRING(pev->model));

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	SetBodygroup(GuardBodyGroup::Weapons, m_GunState);
	SetBodygroup(OtisBodyGroup::Sleeves, m_Sleeves);
	SetBodygroup(OtisBodyGroup::Items, m_Item);

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		AILogger->debug("Dead otis with bad pose");
	}

	MonsterInitDead();
}
