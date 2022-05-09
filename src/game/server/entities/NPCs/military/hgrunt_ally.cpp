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

#include "cbase.h"
#include "squadmonster.h"
#include "talkmonster.h"
#include "COFSquadTalkMonster.h"
#include "customentity.h"
#include "hgrunt_ally_base.h"

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define GRUNT_MP5_CLIP_SIZE 36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define GRUNT_SHOTGUN_CLIP_SIZE 8
#define GRUNT_SAW_CLIP_SIZE 36

namespace HGruntAllyWeaponFlag
{
enum HGruntAllyWeaponFlag
{
	MP5 = 1 << 0,
	HandGrenade = 1 << 1,
	GrenadeLauncher = 1 << 2,
	Shotgun = 1 << 3,
	Saw = 1 << 4
};
}

namespace HGruntAllyBodygroup
{
enum HGruntAllyBodygroup
{
	Head = 1,
	Torso = 2,
	Weapons = 3
};
}

namespace HGruntAllyHead
{
enum HGruntAllyHead
{
	Default = -1,
	GasMask = 0,
	BeretWhite,
	OpsMask,
	BandanaWhite,
	BandanaBlack,
	MilitaryPolice,
	Commander,
	BeretBlack,
};
}

namespace HGruntAllyTorso
{
enum HGruntAllyTorso
{
	Normal = 0,
	Saw,
	Nothing,
	Shotgun
};
}

namespace HGruntAllyWeapon
{
enum HGruntAllyWeapon
{
	MP5 = 0,
	Shotgun,
	Saw,
	None
};
}

class CHGruntAlly : public CBaseHGruntAlly
{
public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void Shoot(bool firstShotInBurst);

	int m_iBrassShell;
	int m_iShotgunShell;
	int m_iSawShell;
	int m_iSawLink;

protected:
	void DropWeapon(bool applyVelocity) override;

	std::tuple<int, Activity> GetSequenceForActivity(Activity NewActivity) override;

	PostureType GetPreferredCombatPosture() const override
	{
		//Always stand when using Saw
		if ((pev->weapons & HGruntAllyWeaponFlag::Saw) != 0)
		{
			return PostureType::Standing;
		}

		return PostureType::Random;
	}

	float GetMaximumRangeAttackDistance() const override
	{
		if ((pev->weapons & HGruntAllyWeaponFlag::Shotgun) != 0)
		{
			return 640;
		}

		return CBaseHGruntAlly::GetMaximumRangeAttackDistance();
	}

	bool CanUseThrownGrenades() const override { return FBitSet(pev->weapons, HGruntAllyWeaponFlag::HandGrenade); }

	bool CanUseGrenadeLauncher() const override { return FBitSet(pev->weapons, HGruntAllyWeaponFlag::GrenadeLauncher); }
};

LINK_ENTITY_TO_CLASS(monster_human_grunt_ally, CHGruntAlly);

void CHGruntAlly::OnCreate()
{
	CBaseHGruntAlly::OnCreate();

	pev->health = GetSkillFloat("hgrunt_ally_health"sv);
	pev->model = MAKE_STRING("models/hgrunt_opfor.mdl");
}

void CHGruntAlly::DropWeapon(bool applyVelocity)
{
	if (GetBodygroup(HGruntAllyBodygroup::Weapons) != HGruntAllyWeapon::None)
	{ // throw a gun if the grunt has one
		Vector vecGunPos, vecGunAngles;
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pGun;
		if (FBitSet(pev->weapons, HGruntAllyWeaponFlag::Shotgun))
		{
			pGun = DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
		}
		else if (FBitSet(pev->weapons, HGruntAllyWeaponFlag::Saw))
		{
			pGun = DropItem("weapon_m249", vecGunPos, vecGunAngles);
		}
		else
		{
			pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
		}
		if (pGun && applyVelocity)
		{
			pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
			pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
		}

		if (FBitSet(pev->weapons, HGruntAllyWeaponFlag::GrenadeLauncher))
		{
			pGun = DropItem("ammo_ARgrenades", vecGunPos, vecGunAngles);
			if (pGun && applyVelocity)
			{
				pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
				pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
			}
		}

		SetBodygroup(HGruntAllyBodygroup::Weapons, HGruntAllyWeapon::None);
	}
}

//=========================================================
// Shoot
//=========================================================
void CHGruntAlly::Shoot(bool firstShotInBurst)
{
	if (m_hEnemy == nullptr)
	{
		return;
	}

	const Vector vecShootOrigin = GetGunPosition();
	const Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	bool firedShot = false;

	if (FBitSet(pev->weapons, HGruntAllyWeaponFlag::MP5))
	{
		const Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5); // shoot +-5 degrees

		if (firstShotInBurst)
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

		firedShot = true;
	}
	else if (FBitSet(pev->weapons, HGruntAllyWeaponFlag::Saw))
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
		{
			const auto vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(75, 200) + gpGlobals->v_up * RANDOM_FLOAT(150, 200) + gpGlobals->v_forward * 25.0;
			EjectBrass(vecShootOrigin - vecShootDir * 6, vecShellVelocity, pev->angles.y, m_iSawLink, TE_BOUNCE_SHELL);
			break;
		}

		case 1:
		{
			const auto vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(100, 250) + gpGlobals->v_up * RANDOM_FLOAT(100, 150) + gpGlobals->v_forward * 25.0;
			EjectBrass(vecShootOrigin - vecShootDir * 6, vecShellVelocity, pev->angles.y, m_iSawShell, TE_BOUNCE_SHELL);
			break;
		}
		}

		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 8192, BULLET_PLAYER_556, 2); // shoot +-5 degrees

		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/saw_fire1.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 15) + 94);
			break;
		case 1:
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/saw_fire2.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 15) + 94);
			break;
		case 2:
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "weapons/saw_fire3.wav", VOL_NORM, ATTN_NORM, 0, RANDOM_LONG(0, 15) + 94);
			break;
		}

		firedShot = true;
	}
	else
	{
		//Check this so shotgunners don't shoot bursts if the animation happens to have the events
		if (firstShotInBurst)
		{
			const Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
			EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
			FireBullets(GetSkillFloat("hgrunt_ally_pellets"sv), vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0); // shoot +-7.5 degrees

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM);

			firedShot = true;
		}
	}

	if (firedShot)
	{
		pev->effects |= EF_MUZZLEFLASH;

		m_cAmmoLoaded--; // take away a bullet!

		Vector angDir = UTIL_VecToAngles(vecShootDir);
		SetBlending(0, angDir.x);
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHGruntAlly::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case HGRUNT_AE_RELOAD:
		if (FBitSet(pev->weapons, HGruntAllyWeaponFlag::Saw))
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/saw_reload.wav", 1, ATTN_NORM);
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM);
		}

		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case HGRUNT_AE_BURST1:
	case HGRUNT_AE_BURST2:
	case HGRUNT_AE_BURST3:
		Shoot(pEvent->event == HGRUNT_AE_BURST1);
		break;

	default:
		CBaseHGruntAlly::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHGruntAlly::Spawn()
{
	SpawnCore();

	//TODO: make torso customizable
	m_iGruntTorso = HGruntAllyTorso::Normal;

	int weaponIndex = 0;

	if ((pev->weapons & HGruntAllyWeaponFlag::MP5) != 0)
	{
		weaponIndex = HGruntAllyWeapon::MP5;
		m_cClipSize = GRUNT_MP5_CLIP_SIZE;
	}
	else if ((pev->weapons & HGruntAllyWeaponFlag::Shotgun) != 0)
	{
		m_cClipSize = GRUNT_SHOTGUN_CLIP_SIZE;
		weaponIndex = HGruntAllyWeapon::Shotgun;
		m_iGruntTorso = HGruntAllyTorso::Shotgun;
	}
	else if ((pev->weapons & HGruntAllyWeaponFlag::Saw) != 0)
	{
		weaponIndex = HGruntAllyWeapon::Saw;
		m_cClipSize = GRUNT_SAW_CLIP_SIZE;
		m_iGruntTorso = HGruntAllyTorso::Saw;
	}
	else
	{
		weaponIndex = HGruntAllyWeapon::None;
		m_cClipSize = 0;
	}

	m_cAmmoLoaded = m_cClipSize;

	if (m_iGruntHead == HGruntAllyHead::Default)
	{
		if ((pev->spawnflags & SF_SQUADMONSTER_LEADER) != 0)
		{
			m_iGruntHead = HGruntAllyHead::BeretWhite;
		}
		else if (weaponIndex == HGruntAllyWeapon::Shotgun)
		{
			m_iGruntHead = HGruntAllyHead::OpsMask;
		}
		else if (weaponIndex == HGruntAllyWeapon::Saw)
		{
			m_iGruntHead = RANDOM_LONG(0, 1) + HGruntAllyHead::BandanaWhite;
		}
		else if (weaponIndex == HGruntAllyWeapon::None)
		{
			m_iGruntHead = HGruntAllyHead::MilitaryPolice;
		}
		else
		{
			m_iGruntHead = HGruntAllyHead::GasMask;
		}
	}

	SetBodygroup(HGruntAllyBodygroup::Head, m_iGruntHead);
	SetBodygroup(HGruntAllyBodygroup::Torso, m_iGruntTorso);
	SetBodygroup(HGruntAllyBodygroup::Weapons, weaponIndex);

	//TODO: probably also needs this for head HGruntAllyHead::BeretBlack
	if (m_iGruntHead == HGruntAllyHead::OpsMask || m_iGruntHead == HGruntAllyHead::BandanaBlack)
	{
		m_voicePitch = 90;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHGruntAlly::Precache()
{
	PRECACHE_SOUND("weapons/saw_fire1.wav");
	PRECACHE_SOUND("weapons/saw_fire2.wav");
	PRECACHE_SOUND("weapons/saw_fire3.wav");
	PRECACHE_SOUND("weapons/saw_reload.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell
	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");
	m_iSawShell = PRECACHE_MODEL("models/saw_shell.mdl");
	m_iSawLink = PRECACHE_MODEL("models/saw_link.mdl");

	CBaseHGruntAlly::Precache();
}

std::tuple<int, Activity> CHGruntAlly::GetSequenceForActivity(Activity NewActivity)
{
	int iSequence;

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet(pev->weapons, HGruntAllyWeaponFlag::MP5))
		{
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
		}
		else if (FBitSet(pev->weapons, HGruntAllyWeaponFlag::Saw))
		{
			if (m_fStanding)
			{
				// get aimable sequence
				iSequence = LookupSequence("standing_saw");
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence("crouching_saw");
			}
		}
		else
		{
			if (m_fStanding)
			{
				// get aimable sequence
				iSequence = LookupSequence("standing_shotgun");
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence("crouching_shotgun");
			}
		}
		break;

	default:
		return CBaseHGruntAlly::GetSequenceForActivity(NewActivity);
	}

	return {iSequence, NewActivity};
}

/**
*	@brief when triggered, spawns a monster_human_grunt_ally repelling down a line.
*/
class CHGruntAllyRepel : public CBaseHGruntAllyRepel
{
protected:
	const char* GetMonsterClassname() const override { return "monster_human_grunt_ally"; }
};

LINK_ENTITY_TO_CLASS(monster_grunt_ally_repel, CHGruntAllyRepel);

//=========================================================
// DEAD HGRUNT PROP
//=========================================================
class CDeadHGruntAlly : public CBaseMonster
{
public:
	void OnCreate() override;
	void Spawn() override;
	int Classify() override { return CLASS_HUMAN_MILITARY_FRIENDLY; }

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	int m_iGruntHead;
	static const char* m_szPoses[7];
};

const char* CDeadHGruntAlly::m_szPoses[] = {"deadstomach", "deadside", "deadsitting", "dead_on_back", "hgrunt_dead_stomach", "dead_headcrabed", "dead_canyon"};

void CDeadHGruntAlly::OnCreate()
{
	CBaseMonster::OnCreate();

	// Corpses have less health
	pev->health = 8;
	pev->model = MAKE_STRING("models/hgrunt_opfor.mdl");
}

bool CDeadHGruntAlly::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iGruntHead = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_human_grunt_ally_dead, CDeadHGruntAlly);

//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================
void CDeadHGruntAlly::Spawn()
{
	PRECACHE_MODEL(STRING(pev->model));
	SET_MODEL(ENT(pev), STRING(pev->model));

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hgrunt with bad pose\n");
	}

	if ((pev->weapons & HGruntAllyWeaponFlag::MP5) != 0)
	{
		SetBodygroup(HGruntAllyBodygroup::Torso, HGruntAllyTorso::Normal);
		SetBodygroup(HGruntAllyBodygroup::Weapons, HGruntAllyWeapon::MP5);
	}
	else if ((pev->weapons & HGruntAllyWeaponFlag::Shotgun) != 0)
	{
		SetBodygroup(HGruntAllyBodygroup::Torso, HGruntAllyTorso::Shotgun);
		SetBodygroup(HGruntAllyBodygroup::Weapons, HGruntAllyWeapon::Shotgun);
	}
	else if ((pev->weapons & HGruntAllyWeaponFlag::Saw) != 0)
	{
		SetBodygroup(HGruntAllyBodygroup::Torso, HGruntAllyTorso::Saw);
		SetBodygroup(HGruntAllyBodygroup::Weapons, HGruntAllyWeapon::Saw);
	}
	else
	{
		SetBodygroup(HGruntAllyBodygroup::Torso, HGruntAllyTorso::Normal);
		SetBodygroup(HGruntAllyBodygroup::Weapons, HGruntAllyWeapon::None);
	}

	SetBodygroup(HGruntAllyBodygroup::Head, m_iGruntHead);

	pev->skin = 0;

	MonsterInitDead();
}
