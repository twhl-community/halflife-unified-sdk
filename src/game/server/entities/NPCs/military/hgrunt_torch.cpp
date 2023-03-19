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
#include "squadmonster.h"
#include "talkmonster.h"
#include "COFSquadTalkMonster.h"
#include "customentity.h"
#include "explode.h"
#include "hgrunt_ally_base.h"

#define TORCH_DEAGLE_CLIP_SIZE 8 //!< how many bullets in a clip?
#define TORCH_BEAM_SPRITE "sprites/xbeam3.spr"

namespace TorchAllyBodygroup
{
enum TorchAllyBodygroup
{
	Head = 1,
	Weapons = 3,
	Torch = 4,
};
}

namespace TorchTorchState
{
enum TorchTorchState
{
	Blank = 0,
	Drawn
};
}

namespace TorchAllyWeaponFlag
{
enum TorchAllyWeaponFlag
{
	DesertEagle = 1 << 0,
	Torch = 1 << 1,
	HandGrenade = 1 << 2,
};
}

#define TORCH_AE_HOLSTER_TORCH 17
#define TORCH_AE_HOLSTER_GUN 18
#define TORCH_AE_HOLSTER_BOTH 19
#define TORCH_AE_ACTIVATE_TORCH 20
#define TORCH_AE_DEACTIVATE_TORCH 21

class COFTorchAlly : public CBaseHGruntAlly
{
public:
	void OnCreate() override;

	void Spawn() override;
	void Precache() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void Shoot();
	void GibMonster() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	void Killed(CBaseEntity* attacker, int iGib) override;

	void MonsterThink() override;

	static TYPEDESCRIPTION m_SaveData[];

	bool m_fTorchActive;

	CBeam* m_pTorchBeam;

	float m_flLastShot;

protected:
	void DropWeapon(bool applyVelocity) override;

	bool CanRangeAttack() const override { return GetBodygroup(TorchAllyBodygroup::Weapons) == NPCWeaponState::Drawn; }

	std::tuple<int, Activity> GetSequenceForActivity(Activity NewActivity) override;

	Schedule_t* GetTorchSchedule() override;

	bool CanTakeCoverAndReload() const override { return true; }
};

LINK_ENTITY_TO_CLASS(monster_human_torch_ally, COFTorchAlly);

TYPEDESCRIPTION COFTorchAlly::m_SaveData[] =
	{
		DEFINE_FIELD(COFTorchAlly, m_fTorchActive, FIELD_BOOLEAN),
		DEFINE_FIELD(COFTorchAlly, m_flLastShot, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(COFTorchAlly, CBaseHGruntAlly);

void COFTorchAlly::OnCreate()
{
	CBaseHGruntAlly::OnCreate();

	pev->health = GetSkillFloat("torch_ally_health"sv);
	pev->model = MAKE_STRING("models/hgrunt_torch.mdl");

	// get voice pitch
	m_voicePitch = 95;
}

void COFTorchAlly::DropWeapon(bool applyVelocity)
{
	if (GetBodygroup(TorchAllyBodygroup::Weapons) != NPCWeaponState::Blank)
	{ // throw a gun if the grunt has one
		Vector vecGunPos, vecGunAngles;
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pGun = DropItem("weapon_eagle", vecGunPos, vecGunAngles);

		if (pGun && applyVelocity)
		{
			pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
			pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
		}

		SetBodygroup(TorchAllyBodygroup::Weapons, NPCWeaponState::Blank);
	}
}

void COFTorchAlly::GibMonster()
{
	if (m_fTorchActive)
	{
		m_fTorchActive = false;
		UTIL_Remove(m_pTorchBeam);
		m_pTorchBeam = nullptr;
	}

	CBaseHGruntAlly::GibMonster();
}

void COFTorchAlly::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// check for Torch fuel tank hit
	if (ptr->iHitgroup == 8)
	{
		// Make sure it kills this grunt
		bitsDamageType = DMG_ALWAYSGIB | DMG_BLAST;
		flDamage = pev->health;
		ExplosionCreate(ptr->vecEndPos, pev->angles, edict(), 100, true);
	}

	CBaseHGruntAlly::TraceAttack(attacker, flDamage, vecDir, ptr, bitsDamageType);
}

void COFTorchAlly::Shoot()
{
	// Limit fire rate
	if (m_hEnemy == nullptr || gpGlobals->time - m_flLastShot <= 0.11)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_PLAYER_357, 0); // shoot +-5 degrees

	const auto random = RANDOM_LONG(0, 20);

	const auto pitch = random <= 10 ? random + 95 : 100;

	EmitSoundDyn(CHAN_WEAPON, "weapons/desert_eagle_fire.wav", VOL_NORM, ATTN_NORM, 0, pitch);

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--; // take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);

	m_flLastShot = gpGlobals->time;
}

void COFTorchAlly::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case HGRUNT_AE_RELOAD:
		EmitSound(CHAN_WEAPON, "weapons/desert_eagle_reload.wav", 1, ATTN_NORM);

		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case HGRUNT_AE_BURST1:
		Shoot();
		break;

	case TORCH_AE_HOLSTER_TORCH:
	{
		SetBodygroup(TorchAllyBodygroup::Weapons, NPCWeaponState::Drawn);
		SetBodygroup(TorchAllyBodygroup::Torch, TorchTorchState::Blank);
		break;
	}

	case TORCH_AE_HOLSTER_GUN:
	{
		SetBodygroup(TorchAllyBodygroup::Weapons, NPCWeaponState::Holstered);
		SetBodygroup(TorchAllyBodygroup::Torch, TorchTorchState::Drawn);
		break;
	}

	case TORCH_AE_HOLSTER_BOTH:
	{
		SetBodygroup(TorchAllyBodygroup::Weapons, NPCWeaponState::Holstered);
		SetBodygroup(TorchAllyBodygroup::Torch, TorchTorchState::Blank);
		break;
	}

	case TORCH_AE_ACTIVATE_TORCH:
	{
		m_fTorchActive = true;
		m_pTorchBeam = CBeam::BeamCreate(TORCH_BEAM_SPRITE, 5);

		if (m_pTorchBeam)
		{
			Vector vecTorchPos, vecTorchAng;
			GetAttachment(2, vecTorchPos, vecTorchAng);

			m_pTorchBeam->EntsInit(entindex(), entindex());

			m_pTorchBeam->SetStartAttachment(4);
			m_pTorchBeam->SetEndAttachment(3);

			m_pTorchBeam->SetColor(0, 0, 255);
			m_pTorchBeam->SetBrightness(255);
			m_pTorchBeam->SetWidth(5);
			m_pTorchBeam->SetFlags(BEAM_FSHADEIN);
			m_pTorchBeam->SetScrollRate(20);

			m_pTorchBeam->pev->spawnflags |= SF_BEAM_SPARKEND;
			m_pTorchBeam->DoSparks(vecTorchPos, vecTorchPos);
		}
		break;
	}

	case TORCH_AE_DEACTIVATE_TORCH:
	{
		if (m_pTorchBeam)
		{
			m_fTorchActive = false;
			UTIL_Remove(m_pTorchBeam);
			m_pTorchBeam = nullptr;
		}
		break;
	}

	default:
		CBaseHGruntAlly::HandleAnimEvent(pEvent);
		break;
	}
}

void COFTorchAlly::Spawn()
{
	SpawnCore();

	m_fTorchActive = false;

	if (0 == pev->weapons)
	{
		pev->weapons |= TorchAllyWeaponFlag::DesertEagle;
	}

	if ((pev->weapons & TorchAllyWeaponFlag::DesertEagle) != 0)
	{
		SetBodygroup(TorchAllyBodygroup::Weapons, NPCWeaponState::Drawn);
		m_cClipSize = TORCH_DEAGLE_CLIP_SIZE;
	}
	else
	{
		SetBodygroup(TorchAllyBodygroup::Weapons, NPCWeaponState::Holstered);
		SetBodygroup(TorchAllyBodygroup::Torch, TorchTorchState::Drawn);
		m_cClipSize = 0;
	}

	m_cAmmoLoaded = m_cClipSize;

	m_flLastShot = gpGlobals->time;
	m_flMedicWaitTime = gpGlobals->time;
}

void COFTorchAlly::Precache()
{
	PrecacheModel(TORCH_BEAM_SPRITE);

	PrecacheSound("weapons/desert_eagle_fire.wav");
	PrecacheSound("weapons/desert_eagle_reload.wav");

	PrecacheSound("fgrunt/torch_light.wav");
	PrecacheSound("fgrunt/torch_cut_loop.wav");

	CBaseHGruntAlly::Precache();
}

std::tuple<int, Activity> COFTorchAlly::GetSequenceForActivity(Activity NewActivity)
{
	int iSequence = ACTIVITY_NOT_AVAILABLE;

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
	default:
		return CBaseHGruntAlly::GetSequenceForActivity(NewActivity);
	}

	return {iSequence, NewActivity};
}

Schedule_t* COFTorchAlly::GetTorchSchedule()
{
	if (GetBodygroup(TorchAllyBodygroup::Torch) == TorchTorchState::Drawn)
	{
		return COFSquadTalkMonster::GetSchedule();
	}

	return nullptr;
}

void COFTorchAlly::Killed(CBaseEntity* attacker, int iGib)
{
	// TODO: is this even correct? Torch grunts have no medic capabilities
	if (m_hTargetEnt != nullptr)
	{
		m_hTargetEnt.Entity<COFSquadTalkMonster>()->m_hWaitMedic = nullptr;
	}

	if (m_fTorchActive)
	{
		m_fTorchActive = false;
		UTIL_Remove(m_pTorchBeam);
		m_pTorchBeam = nullptr;
	}

	CBaseHGruntAlly::Killed(attacker, iGib);
}

void COFTorchAlly::MonsterThink()
{
	if (m_fTorchActive && m_pTorchBeam)
	{
		Vector vecTorchPos;
		Vector vecTorchAng;
		Vector vecEndPos;
		Vector vecEndAng;

		GetAttachment(2, vecTorchPos, vecTorchAng);
		GetAttachment(3, vecEndPos, vecEndAng);

		TraceResult tr;
		UTIL_TraceLine(vecTorchPos, (vecEndPos - vecTorchPos).Normalize() * 4 + vecTorchPos, ignore_monsters, edict(), &tr);

		if (tr.flFraction != 1.0)
		{
			m_pTorchBeam->pev->spawnflags &= ~SF_BEAM_SPARKSTART;
			// TODO: looks like a bug to me, shouldn't be bitwise inverting
			m_pTorchBeam->pev->spawnflags |= ~SF_BEAM_SPARKEND;

			UTIL_DecalTrace(&tr, RANDOM_LONG(0, 4));
			m_pTorchBeam->DoSparks(tr.vecEndPos, tr.vecEndPos);
		}

		m_pTorchBeam->SetBrightness(RANDOM_LONG(192, 255));
	}

	CBaseHGruntAlly::MonsterThink();
}

/**
 *	@brief when triggered, spawns a monster_human_torch_ally repelling down a line.
 */
class COFTorchAllyRepel : public CBaseHGruntAllyRepel
{
protected:
	const char* GetMonsterClassname() const override { return "monster_human_torch_ally"; }
};

LINK_ENTITY_TO_CLASS(monster_torch_ally_repel, COFTorchAllyRepel);
