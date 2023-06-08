/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   Use, distribution, and modification of this source code and/or resulting
 *   object code is restricted to non-commercial enhancements to products from
 *   Valve LLC.  All other use, distribution, or modification is prohibited
 *   without written permission from Valve LLC.
 *
 ****/
#include "cbase.h"
#include "UserMessages.h"

#ifndef CLIENT_DLL
#include "weapons/CSpore.h"
#endif

#include "CSporeLauncher.h"

BEGIN_DATAMAP(CSporeLauncher)
DEFINE_FIELD(m_ReloadState, FIELD_INTEGER),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(weapon_sporelauncher, CSporeLauncher);

void CSporeLauncher::OnCreate()
{
	CBasePlayerWeapon::OnCreate();
	m_iId = WEAPON_SPORELAUNCHER;
	m_iDefaultAmmo = SPORELAUNCHER_DEFAULT_GIVE;
	m_WorldModel = pev->model = MAKE_STRING("models/w_spore_launcher.mdl");
}

void CSporeLauncher::Precache()
{
	CBasePlayerWeapon::Precache();
	PrecacheModel(STRING(m_WorldModel));
	PrecacheModel("models/v_spore_launcher.mdl");
	PrecacheModel("models/p_spore_launcher.mdl");

	PrecacheSound("weapons/splauncher_fire.wav");
	PrecacheSound("weapons/splauncher_altfire.wav");
	PrecacheSound("weapons/splauncher_bounce.wav");
	PrecacheSound("weapons/splauncher_reload.wav");
	PrecacheSound("weapons/splauncher_pet.wav");

	UTIL_PrecacheOther("spore");

	m_usFireSpore = PRECACHE_EVENT(1, "events/spore.sc");
}

void CSporeLauncher::Spawn()
{
	CBasePlayerWeapon::Spawn();

	pev->sequence = 0;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1;
}

bool CSporeLauncher::Deploy()
{
	return DefaultDeploy("models/v_spore_launcher.mdl", "models/p_spore_launcher.mdl", SPLAUNCHER_DRAW1, "rpg");
}

void CSporeLauncher::Holster()
{
	m_ReloadState = ReloadState::NOT_RELOADING;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim(SPLAUNCHER_HOLSTER1);
}

bool CSporeLauncher::ShouldWeaponIdle()
{
	return true;
}

void CSporeLauncher::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		if (GetMagazine1() == 0 &&
			m_ReloadState == ReloadState::NOT_RELOADING &&
			0 != m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType))
		{
			Reload();
		}
		else if (m_ReloadState != ReloadState::NOT_RELOADING)
		{
			int maxClip = SPORELAUNCHER_MAX_CLIP;

			if ((m_pPlayer->m_iItems & CTFItem::Backpack) != 0)
			{
				maxClip *= 2;
			}

			if (GetMagazine1() != maxClip && 0 != m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType))
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim(SPLAUNCHER_AIM);

				m_ReloadState = ReloadState::NOT_RELOADING;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.83;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
			if (flRand <= 0.75)
			{
				iAnim = SPLAUNCHER_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
			}
			else if (flRand <= 0.95)
			{
				iAnim = SPLAUNCHER_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4;
			}
			else
			{
				iAnim = SPLAUNCHER_FIDGET;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4;

				m_pPlayer->EmitSound(CHAN_ITEM, "weapons/splauncher_pet.wav", 0.7, ATTN_NORM);
			}

			SendWeaponAnim(iAnim);
		}
	}
}

void CSporeLauncher::PrimaryAttack()
{
	if (0 != GetMagazine1())
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		UTIL_MakeVectors(vecAngles);

		const Vector vecSrc =
			m_pPlayer->GetGunPosition() +
			gpGlobals->v_forward * 16 +
			gpGlobals->v_right * 8 +
			gpGlobals->v_up * -8;

		vecAngles = vecAngles + m_pPlayer->GetAutoaimVectorFromPoint(vecSrc, AUTOAIM_10DEGREES);

		CSpore* pSpore = CSpore::CreateSpore(
			vecSrc, vecAngles,
			m_pPlayer,
			CSpore::SporeType::ROCKET, false, false);

		UTIL_MakeVectors(vecAngles);

		pSpore->pev->velocity = pSpore->pev->velocity + DotProduct(pSpore->pev->velocity, gpGlobals->v_forward) * gpGlobals->v_forward;
#endif

		int flags;

#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif

		PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usFireSpore);

		AdjustMagazine1(-1);
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

	m_ReloadState = ReloadState::NOT_RELOADING;
}

void CSporeLauncher::SecondaryAttack()
{
	if (0 != GetMagazine1())
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		UTIL_MakeVectors(vecAngles);

		const Vector vecSrc =
			m_pPlayer->GetGunPosition() +
			gpGlobals->v_forward * 16 +
			gpGlobals->v_right * 8 +
			gpGlobals->v_up * -8;

		vecAngles = vecAngles + m_pPlayer->GetAutoaimVectorFromPoint(vecSrc, AUTOAIM_10DEGREES);

		CSpore* pSpore = CSpore::CreateSpore(
			vecSrc, vecAngles,
			m_pPlayer,
			CSpore::SporeType::GRENADE, false, false);

		UTIL_MakeVectors(vecAngles);

		pSpore->pev->velocity = m_pPlayer->pev->velocity + 800 * gpGlobals->v_forward;
#endif

		int flags;

#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif

		PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usFireSpore);

		AdjustMagazine1(-1);
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

	m_ReloadState = ReloadState::NOT_RELOADING;
}

void CSporeLauncher::Reload()
{
	int maxClip = SPORELAUNCHER_MAX_CLIP;

	if ((m_pPlayer->m_iItems & CTFItem::Backpack) != 0)
	{
		maxClip *= 2;
	}

	if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) <= 0 || GetMagazine1() == maxClip)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_ReloadState == ReloadState::NOT_RELOADING)
	{
		SendWeaponAnim(SPLAUNCHER_RELOAD_REACH);
		m_ReloadState = ReloadState::DO_RELOAD_EFFECTS;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.66;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.66;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.66;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.66;
		return;
	}
	else if (m_ReloadState == ReloadState::DO_RELOAD_EFFECTS)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_ReloadState = ReloadState::RELOAD_ONE;

		m_pPlayer->EmitSound(CHAN_ITEM, "weapons/splauncher_reload.wav", 0.7, ATTN_NORM);

		SendWeaponAnim(SPLAUNCHER_RELOAD);

		m_flNextReload = UTIL_WeaponTimeBase() + 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	}
	else
	{
		// Add them to the clip
		AdjustMagazine1(1);
		m_pPlayer->AdjustAmmoByIndex(m_iPrimaryAmmoType, -1);
		m_ReloadState = ReloadState::DO_RELOAD_EFFECTS;
	}
}

bool CSporeLauncher::GetWeaponInfo(WeaponInfo& info)
{
	info.AttackModeInfo[0].AmmoType = "spores";
	info.Name = STRING(pev->classname);
	info.AttackModeInfo[0].MagazineSize = SPORELAUNCHER_MAX_CLIP;
	info.Slot = 6;
	info.Position = 0;
	info.Id = WEAPON_SPORELAUNCHER;
	info.Weight = SPORELAUNCHER_WEIGHT;

	return true;
}

void CSporeLauncher::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "spores") >= 0)
	{
		pPlayer->EmitSound(CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

void CSporeLauncher::GetWeaponData(weapon_data_t& data)
{
	BaseClass::GetWeaponData(data);

	// m_ReloadState is called m_fInSpecialReload in Op4.
	data.m_fInSpecialReload = static_cast<int>(m_ReloadState);
}

void CSporeLauncher::SetWeaponData(const weapon_data_t& data)
{
	BaseClass::SetWeaponData(data);

	m_ReloadState = static_cast<ReloadState>(data.m_fInSpecialReload);
}

#ifndef CLIENT_DLL
enum SporeAmmoAnim
{
	SPOREAMMO_IDLE = 0,
	SPOREAMMO_SPAWNUP,
	SPOREAMMO_SNATCHUP,
	SPOREAMMO_SPAWNDN,
	SPOREAMMO_SNATCHDN,
	SPOREAMMO_IDLE1,
	SPOREAMMO_IDLE2
};

enum SporeAmmoBody
{
	SPOREAMMOBODY_EMPTY = 0,
	SPOREAMMOBODY_FULL
};

class CSporeAmmo : public CBasePlayerAmmo
{
	DECLARE_CLASS(CSporeAmmo, CBasePlayerAmmo);
	DECLARE_DATAMAP();

public:
	void OnCreate() override
	{
		CBasePlayerAmmo::OnCreate();
		m_AmmoAmount = AMMO_SPORE_GIVE;
		m_AmmoName = MAKE_STRING("spores");
		pev->model = MAKE_STRING("models/spore_ammo.mdl");
		m_SoundOffset.z = 1;
	}

	void Precache() override
	{
		// Don't precache default pickup sound.
		CBaseItem::Precache();
		PrecacheSound("weapons/spore_ammo.wav");
	}

	void Spawn() override
	{
		Precache();

		SetModel(STRING(pev->model));

		pev->movetype = MOVETYPE_FLY;

		SetSize(Vector(-16, -16, -16), Vector(16, 16, 16));

		pev->origin.z += 16;

		SetOrigin(pev->origin);

		pev->angles.x -= 90;

		pev->sequence = SPOREAMMO_SPAWNDN;

		pev->animtime = gpGlobals->time;

		pev->nextthink = gpGlobals->time + 4;

		pev->frame = 0;
		pev->framerate = 1;

		pev->health = 1;

		pev->body = SPOREAMMOBODY_FULL;

		pev->takedamage = DAMAGE_AIM;

		pev->solid = SOLID_BBOX;

		SetTouch(&CSporeAmmo::SporeTouch);
		SetThink(&CSporeAmmo::Idling);
	}

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override
	{
		if (pev->body == SPOREAMMOBODY_EMPTY)
		{
			return false;
		}

		pev->body = SPOREAMMOBODY_EMPTY;

		pev->sequence = SPOREAMMO_SNATCHDN;

		pev->animtime = gpGlobals->time;
		pev->frame = 0;
		pev->nextthink = gpGlobals->time + 0.66;

		auto vecLaunchDir = pev->angles;

		vecLaunchDir.x -= 90;
		// Rotate it so spores that aren't rotated in Hammer point in the right direction.
		vecLaunchDir.y += 180;

		vecLaunchDir.x += RANDOM_FLOAT(-20, 20);
		vecLaunchDir.y += RANDOM_FLOAT(-20, 20);
		vecLaunchDir.z += RANDOM_FLOAT(-20, 20);

		auto pSpore = CSpore::CreateSpore(pev->origin, vecLaunchDir, this, CSpore::SporeType::GRENADE, false, true);

		UTIL_MakeVectors(vecLaunchDir);

		pSpore->pev->velocity = gpGlobals->v_forward * 800;

		return false;
	}

	bool AddAmmo(CBasePlayer* player) override
	{
		return GiveAmmo(player, m_AmmoAmount, STRING(m_AmmoName), "weapons/spore_ammo.wav");
	}

	void Idling()
	{
		switch (pev->sequence)
		{
		case SPOREAMMO_SPAWNDN:
		{
			pev->sequence = SPOREAMMO_IDLE1;
			pev->animtime = gpGlobals->time;
			pev->frame = 0;
			break;
		}

		case SPOREAMMO_SNATCHDN:
		{
			pev->sequence = SPOREAMMO_IDLE;
			pev->animtime = gpGlobals->time;
			pev->frame = 0;
			pev->nextthink = gpGlobals->time + 10;
			break;
		}

		case SPOREAMMO_IDLE:
		{
			pev->body = SPOREAMMOBODY_FULL;
			pev->sequence = SPOREAMMO_SPAWNDN;
			pev->animtime = gpGlobals->time;
			pev->frame = 0;
			pev->nextthink = gpGlobals->time + 4;
			break;
		}

		default:
			break;
		}
	}

	void SporeTouch(CBaseEntity* pOther)
	{
		auto player = ToBasePlayer(pOther);

		if (!player || pev->body == SPOREAMMOBODY_EMPTY)
			return;

		if (AddAmmo(player))
		{
			pev->body = SPOREAMMOBODY_EMPTY;

			pev->sequence = SPOREAMMO_SNATCHDN;

			pev->animtime = gpGlobals->time;
			pev->frame = 0;
			pev->nextthink = gpGlobals->time + 0.66;
		}
	}
};

BEGIN_DATAMAP(CSporeAmmo)
DEFINE_FUNCTION(Idling),
	DEFINE_FUNCTION(SporeTouch),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(ammo_spore, CSporeAmmo);
#endif
