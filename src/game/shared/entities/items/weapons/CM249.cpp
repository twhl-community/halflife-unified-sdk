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

#include "CM249.h"

BEGIN_DATAMAP(CM249)
DEFINE_FIELD(m_flReloadStartTime, FIELD_FLOAT),
	DEFINE_FIELD(m_flReloadStart, FIELD_FLOAT),
	DEFINE_FIELD(m_bReloading, FIELD_BOOLEAN),
	DEFINE_FIELD(m_iFire, FIELD_INTEGER),
	DEFINE_FIELD(m_iSmoke, FIELD_INTEGER),
	DEFINE_FIELD(m_iLink, FIELD_INTEGER),
	DEFINE_FIELD(m_iShell, FIELD_INTEGER),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(weapon_m249, CM249);

void CM249::OnCreate()
{
	BaseClass::OnCreate();
	m_iId = WEAPON_M249;
	m_iDefaultAmmo = M249_DEFAULT_GIVE;
	m_WorldModel = pev->model = MAKE_STRING("models/w_saw.mdl");
}

void CM249::Precache()
{
	CBasePlayerWeapon::Precache();
	PrecacheModel("models/v_saw.mdl");
	PrecacheModel(STRING(m_WorldModel));
	PrecacheModel("models/p_saw.mdl");

	m_iShell = PrecacheModel("models/saw_shell.mdl");
	m_iLink = PrecacheModel("models/saw_link.mdl");
	m_iSmoke = PrecacheModel("sprites/wep_smoke_01.spr");
	m_iFire = PrecacheModel("sprites/xfire.spr");

	PrecacheSound("weapons/saw_reload.wav");
	PrecacheSound("weapons/saw_reload2.wav");
	PrecacheSound("weapons/saw_fire1.wav");

	m_usFireM249 = PRECACHE_EVENT(1, "events/m249.sc");
}

bool CM249::Deploy()
{
	return DefaultDeploy("models/v_saw.mdl", "models/p_saw.mdl", M249_DRAW, "mp5");
}

void CM249::Holster()
{
	SetThink(nullptr);

	SendWeaponAnim(M249_HOLSTER);

	m_bReloading = false;

	m_fInReload = false;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10.0, 15.0);
}

void CM249::WeaponIdle()
{
	ResetEmptySound();

	// Update auto-aim
	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_bReloading && gpGlobals->time >= m_flReloadStart + 1.33)
	{
		m_bReloading = false;

		pev->body = 0;

		SendWeaponAnim(M249_RELOAD_END, pev->body);
	}

	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		const float flNextIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		int iAnim;

		if (flNextIdle <= 0.95)
		{
			iAnim = M249_SLOWIDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
		}
		else
		{
			iAnim = M249_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.16;
		}

		SendWeaponAnim(iAnim, pev->body);
	}
}

void CM249::PrimaryAttack()
{
	if (m_pPlayer->pev->waterlevel == WaterLevel::Head)
	{
		PlayEmptySound();

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if (GetMagazine1() <= 0)
	{
		if (!m_fInReload)
		{
			PlayEmptySound();

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		}

		return;
	}

	AdjustMagazine1(-1);

	pev->body = RecalculateBody(GetMagazine1());

	m_bAlternatingEject = !m_bAlternatingEject;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	m_flNextAnimTime = UTIL_WeaponTimeBase() + 0.2;

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();

	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecSpread;

	if (g_Skill.GetValue("m249_wide_spread") != 0)
	{
		if ((m_pPlayer->pev->button & IN_DUCK) != 0)
		{
			vecSpread = VECTOR_CONE_3DEGREES;
		}
		else if ((m_pPlayer->pev->button & (IN_MOVERIGHT | IN_MOVELEFT | IN_FORWARD | IN_BACK)) != 0)
		{
			vecSpread = VECTOR_CONE_15DEGREES;
		}
		else
		{
			vecSpread = VECTOR_CONE_6DEGREES;
		}
	}
	else
	{
		if ((m_pPlayer->pev->button & IN_DUCK) != 0)
		{
			vecSpread = VECTOR_CONE_2DEGREES;
		}
		else if ((m_pPlayer->pev->button & (IN_MOVERIGHT | IN_MOVELEFT | IN_FORWARD | IN_BACK)) != 0)
		{
			vecSpread = VECTOR_CONE_10DEGREES;
		}
		else
		{
			vecSpread = VECTOR_CONE_4DEGREES;
		}
	}

	Vector vecDir = m_pPlayer->FireBulletsPlayer(
		1,
		vecSrc, vecAiming, vecSpread,
		8192.0, BULLET_PLAYER_556, 2, 0,
		m_pPlayer, m_pPlayer->random_seed);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(
		flags, m_pPlayer->edict(), m_usFireM249, 0,
		g_vecZero, g_vecZero,
		vecDir.x, vecDir.y,
		pev->body, 0,
		m_bAlternatingEject ? 1 : 0, 0);

	if (0 == GetMagazine1())
	{
		if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) <= 0)
		{
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", SUIT_SENTENCE, SUIT_REPEAT_OK);
		}
	}

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.067;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2;

#ifndef CLIENT_DLL
	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT(-2, 2);

	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT(-1, 1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	const Vector& vecVelocity = m_pPlayer->pev->velocity;

	const float flZVel = m_pPlayer->pev->velocity.z;

	Vector vecInvPushDir = gpGlobals->v_forward * 35.0;

	float flNewZVel = CVAR_GET_FLOAT("sv_maxspeed");

	if (vecInvPushDir.z >= 10.0)
		flNewZVel = vecInvPushDir.z;

	if (!g_pGameRules->IsMultiplayer())
	{
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - vecInvPushDir;

		// Restore Z velocity to make deathmatch easier.
		m_pPlayer->pev->velocity.z = flZVel;
	}
	else
	{
		const float flZTreshold = -(flNewZVel + 100.0);

		if (vecVelocity.x > flZTreshold)
		{
			m_pPlayer->pev->velocity.x -= vecInvPushDir.x;
		}

		if (vecVelocity.y > flZTreshold)
		{
			m_pPlayer->pev->velocity.y -= vecInvPushDir.y;
		}

		m_pPlayer->pev->velocity.z -= vecInvPushDir.z;
	}
#endif
}

void CM249::Reload()
{
	if (DefaultReload(M249_MAX_CLIP, M249_RELOAD_START, 1.0, 0))
	{
		m_bReloading = true;

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3.78;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.78;

		m_flReloadStart = gpGlobals->time;
	}
}

int CM249::RecalculateBody(int iClip)
{
	if (iClip == 0)
	{
		return 8;
	}
	else if (iClip >= 0 && iClip <= 7)
	{
		return 9 - iClip;
	}
	else
	{
		return 0;
	}
}

bool CM249::GetWeaponInfo(WeaponInfo& info)
{
	info.AttackModeInfo[0].AmmoType = "556";
	info.Name = STRING(pev->classname);
	info.AttackModeInfo[0].MagazineSize = M249_MAX_CLIP;
	info.Slot = 5;
	info.Position = 0;
	info.Id = WEAPON_M249;
	info.Weight = M249_WEIGHT;

	return true;
}

void CM249::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "556") >= 0)
	{
		pPlayer->EmitSound(CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

void CM249::GetWeaponData(weapon_data_t& data)
{
	data.iuser1 = pev->body;
}

void CM249::SetWeaponData(const weapon_data_t& data)
{
	pev->body = data.iuser1;
}

class CAmmo556 : public CBasePlayerAmmo
{
	DECLARE_CLASS(CAmmo556, CBasePlayerAmmo);

public:
	void OnCreate() override
	{
		BaseClass::OnCreate();
		m_AmmoAmount = AMMO_M249_GIVE;
		m_AmmoName = MAKE_STRING("556");
		pev->model = MAKE_STRING("models/w_saw_clip.mdl");
	}
};

LINK_ENTITY_TO_CLASS(ammo_556, CAmmo556);
