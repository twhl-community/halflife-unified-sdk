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
#include "CMP5.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_9mmar, CMP5);

void CMP5::OnCreate()
{
	CBasePlayerWeapon::OnCreate();
	m_iId = WEAPON_MP5;
	m_iDefaultAmmo = MP5_DEFAULT_GIVE;
	m_WorldModel = pev->model = MAKE_STRING("models/w_9mmAR.mdl");
	m_flNextGrenadeLoad = gpGlobals->time;
}

void CMP5::Precache()
{
	CBasePlayerWeapon::Precache();
	PrecacheModel("models/v_9mmAR.mdl");
	PrecacheModel(STRING(m_WorldModel));
	PrecacheModel("models/p_9mmAR.mdl");

	m_iShell = PrecacheModel("models/shell.mdl"); // brass shellTE_MODEL

	PrecacheModel("models/grenade.mdl"); // grenade

	PrecacheModel("models/w_9mmARclip.mdl");
	PrecacheSound("items/9mmclip1.wav");

	PrecacheSound("items/clipinsert1.wav");
	PrecacheSound("items/cliprelease1.wav");

	PrecacheSound("weapons/hks1.wav"); // H to the K
	PrecacheSound("weapons/hks2.wav"); // H to the K
	PrecacheSound("weapons/hks3.wav"); // H to the K

	PrecacheSound("weapons/glauncher.wav");
	PrecacheSound("weapons/glauncher2.wav");

	PrecacheSound("weapons/357_cock1.wav");

	m_usMP5 = PRECACHE_EVENT(1, "events/mp5.sc");
	m_usMP52 = PRECACHE_EVENT(1, "events/mp52.sc");
}

bool CMP5::GetWeaponInfo(WeaponInfo& info)
{
	info.Name = STRING(pev->classname);

	info.AttackModeInfo[0].AmmoType = "9mm";
	info.AttackModeInfo[0].MagazineSize = MP5_MAX_CLIP;

	info.AttackModeInfo[1].AmmoType = "ARgrenades";
	info.AttackModeInfo[1].MagazineSize = WEAPON_NOCLIP; // AR grenades don't use magazines.

	info.Slot = 2;
	info.Position = 0;
	info.Id = WEAPON_MP5;
	info.Weight = MP5_WEIGHT;

	return true;
}

void CMP5::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "9mm") >= 0)
	{
		pPlayer->EmitSound(CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}

	if (m_flNextGrenadeLoad < gpGlobals->time)
	{
		pPlayer->GiveAmmo(1, "ARgrenades");
		m_flNextGrenadeLoad = gpGlobals->time + 10;
	}
}

bool CMP5::Deploy()
{
	return DefaultDeploy("models/v_9mmAR.mdl", "models/p_9mmAR.mdl", MP5_DEPLOY, "mp5");
}

void CMP5::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == WaterLevel::Head)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (GetMagazine1() <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	AdjustMagazine1(-1);

	m_pPlayer->pev->effects = m_pPlayer->pev->effects | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

	if (g_Skill.GetValue("smg_wide_spread") != 0)
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer, m_pPlayer->random_seed);
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer, m_pPlayer->random_seed);
	}

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usMP5, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	if (0 == GetMagazine1() && m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CMP5::SecondaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == WaterLevel::Head)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_pPlayer->GetAmmoCountByIndex(m_iSecondaryAmmoType) == 0)
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;

	m_pPlayer->AdjustAmmoByIndex(m_iSecondaryAmmoType, -1);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	// we don't add in player velocity anymore.
	CGrenade::ShootContact(m_pPlayer,
		m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
		gpGlobals->v_forward * 800);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usMP52);

	m_flNextPrimaryAttack = GetNextAttackDelay(1);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5; // idle pretty soon after shooting.

	if (0 == m_pPlayer->GetAmmoCountByIndex(m_iSecondaryAmmoType))
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", 0);
}

void CMP5::Reload()
{
	if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) <= 0)
		return;

	DefaultReload(MP5_MAX_CLIP, MP5_RELOAD, 1.5);
}

void CMP5::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = MP5_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = MP5_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}

class CMP5AmmoClip : public CBasePlayerAmmo
{
public:
	void OnCreate() override
	{
		CBasePlayerAmmo::OnCreate();
		m_AmmoAmount = AMMO_MP5CLIP_GIVE;
		m_AmmoName = MAKE_STRING("9mm");
		pev->model = MAKE_STRING("models/w_9mmARclip.mdl");
	}
};

LINK_ENTITY_TO_CLASS(ammo_9mmar, CMP5AmmoClip);

class CMP5Chainammo : public CBasePlayerAmmo
{
public:
	void OnCreate() override
	{
		CBasePlayerAmmo::OnCreate();
		m_AmmoAmount = AMMO_CHAINBOX_GIVE;
		m_AmmoName = MAKE_STRING("9mm");
		pev->model = MAKE_STRING("models/w_chainammo.mdl");
	}
};

LINK_ENTITY_TO_CLASS(ammo_9mmbox, CMP5Chainammo);

class CMP5AmmoGrenade : public CBasePlayerAmmo
{
public:
	void OnCreate() override
	{
		CBasePlayerAmmo::OnCreate();
		m_AmmoAmount = AMMO_M203BOX_GIVE;
		m_AmmoName = MAKE_STRING("ARgrenades");
		pev->model = MAKE_STRING("models/w_ARgrenade.mdl");
	}
};

LINK_ENTITY_TO_CLASS(ammo_argrenades, CMP5AmmoGrenade);
