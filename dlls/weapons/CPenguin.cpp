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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

#include "CPenguin.h"

#ifndef CLIENT_DLL
//TODO: this isn't in vanilla Op4 so it won't save properly there
TYPEDESCRIPTION CPenguin::m_SaveData[] =
	{
		DEFINE_FIELD(CPenguin, m_fJustThrown, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CPenguin, CPenguin::BaseClass);
#endif

LINK_ENTITY_TO_CLASS(weapon_penguin, CPenguin);

void CPenguin::Precache()
{
	g_engfuncs.pfnPrecacheModel("models/w_penguinnest.mdl");
	g_engfuncs.pfnPrecacheModel("models/v_penguin.mdl");
	g_engfuncs.pfnPrecacheModel("models/p_penguin.mdl");
	g_engfuncs.pfnPrecacheSound("squeek/sqk_hunt2.wav");
	g_engfuncs.pfnPrecacheSound("squeek/sqk_hunt3.wav");
	UTIL_PrecacheOther("monster_penguin");
	m_usPenguinFire = g_engfuncs.pfnPrecacheEvent(1, "events/penguinfire.sc");
}

void CPenguin::Spawn()
{
	Precache();

	m_iId = WEAPON_PENGUIN;
	g_engfuncs.pfnSetModel(edict(), "models/w_penguinnest.mdl");
	FallInit();

	m_iDefaultAmmo = PENGUIN_MAX_CLIP;
	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1;
}

bool CPenguin::Deploy()
{
	if (g_engfuncs.pfnRandomFloat(0.0, 1.0) <= 0.5)
		EMIT_SOUND(edict(), CHAN_VOICE, "squeek/sqk_hunt2.wav", VOL_NORM, ATTN_NORM);
	else
		EMIT_SOUND(edict(), CHAN_VOICE, "squeek/sqk_hunt3.wav", VOL_NORM, ATTN_NORM);

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	return DefaultDeploy("models/v_penguin.mdl", "models/p_penguin.mdl", PENGUIN_UP, "penguin");
}

void CPenguin::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		SendWeaponAnim(PENGUIN_DOWN);

		EMIT_SOUND(edict(), CHAN_WEAPON, "common/null.wav", VOL_NORM, ATTN_NORM);
	}
	else
	{
		m_pPlayer->ClearWeaponBit(m_iId);
		SetThink(&CPenguin::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CPenguin::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fJustThrown)
	{
		m_fJustThrown = false;

		if (0 != m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()])
		{
			SendWeaponAnim(PENGUIN_UP);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		}
		else
		{
			RetireWeapon();
		}
	}
	else
	{
		const float fRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);

		int iAnim;

		if (fRand <= 0.75)
		{
			iAnim = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.75;
		}
		else if (fRand <= 0.875)
		{
			iAnim = 1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.375;
		}
		else
		{
			iAnim = 2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
		}

		SendWeaponAnim(iAnim);
	}
}

void CPenguin::PrimaryAttack()
{
	if (0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);

		Vector vecSrc = m_pPlayer->pev->origin;

		if ((m_pPlayer->pev->flags & FL_DUCKING) != 0)
		{
			vecSrc.z += 18;
		}

		const Vector vecStart = vecSrc + (gpGlobals->v_forward * 20);
		const Vector vecEnd = vecSrc + (gpGlobals->v_forward * 64);

		TraceResult tr;
		UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, nullptr, &tr);

		int flags;
#if defined(CLIENT_WEAPONS)
		flags = UTIL_DefaultPlaybackFlags();
#else
		flags = 0;
#endif

		PLAYBACK_EVENT(flags, edict(), m_usPenguinFire);

		if (0 == tr.fAllSolid && 0 == tr.fStartSolid && tr.flFraction > 0.25)
		{
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
			auto penguin = CBaseEntity::Create("monster_penguin", tr.vecEndPos, m_pPlayer->pev->v_angle, m_pPlayer->edict());

			penguin->pev->velocity = m_pPlayer->pev->velocity + (gpGlobals->v_forward * 200);
#endif

			if (g_engfuncs.pfnRandomFloat(0, 1) <= 0.5)
				EMIT_SOUND_DYN(edict(), CHAN_VOICE, "squeek/sqk_hunt2.wav", VOL_NORM, ATTN_NORM, 0, 105);
			else
				EMIT_SOUND_DYN(edict(), CHAN_VOICE, "squeek/sqk_hunt3.wav", VOL_NORM, ATTN_NORM, 0, 105);

			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
			--m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
			m_fJustThrown = true;

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.9;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.8;
		}
	}
}

void CPenguin::SecondaryAttack()
{
	//Nothing
}

int CPenguin::iItemSlot()
{
	return 5;
}

bool CPenguin::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = "Penguins";
	p->iMaxAmmo1 = PENGUIN_MAX_CARRY;
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_PENGUIN;
	p->iWeight = PENGUIN_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return true;
}
