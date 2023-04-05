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
#include "CCrowbar.h"

#define CROWBAR_BODYHIT_VOLUME 128
#define CROWBAR_WALLHIT_VOLUME 512

BEGIN_DATAMAP(CCrowbar)
DEFINE_FUNCTION(SwingAgain),
	DEFINE_FUNCTION(Smack),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(weapon_crowbar, CCrowbar);

void CCrowbar::OnCreate()
{
	CBasePlayerWeapon::OnCreate();
	m_iId = WEAPON_CROWBAR;
	m_iClip = WEAPON_NOCLIP;
	m_WorldModel = pev->model = MAKE_STRING("models/w_crowbar.mdl");
}

void CCrowbar::Precache()
{
	CBasePlayerWeapon::Precache();
	PrecacheModel("models/v_crowbar.mdl");
	PrecacheModel(STRING(m_WorldModel));
	PrecacheModel("models/p_crowbar.mdl");
	PrecacheSound("weapons/cbar_hit1.wav");
	PrecacheSound("weapons/cbar_hit2.wav");
	PrecacheSound("weapons/cbar_hitbod1.wav");
	PrecacheSound("weapons/cbar_hitbod2.wav");
	PrecacheSound("weapons/cbar_hitbod3.wav");
	PrecacheSound("weapons/cbar_miss1.wav");

	m_usCrowbar = PRECACHE_EVENT(1, "events/crowbar.sc");
}

bool CCrowbar::GetWeaponInfo(WeaponInfo& info)
{
	info.Name = STRING(pev->classname);
	info.AttackModeInfo[0].MagazineSize = WEAPON_NOCLIP;
	info.Slot = 0;
	info.Position = 0;
	info.Id = WEAPON_CROWBAR;
	info.Weight = CROWBAR_WEIGHT;
	return true;
}

bool CCrowbar::Deploy()
{
	return DefaultDeploy("models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar");
}

void CCrowbar::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(CROWBAR_HOLSTER);
}

void CCrowbar::PrimaryAttack()
{
	if (!Swing(true))
	{
		SetThink(&CCrowbar::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CCrowbar::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}

void CCrowbar::SwingAgain()
{
	Swing(false);
}

bool CCrowbar::Swing(bool fFirst)
{
	bool fDidHit = false;

	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer);
			vecEnd = tr.vecEndPos; // This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	if (fFirst)
	{
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usCrowbar,
			0.0, g_vecZero, g_vecZero, 0, 0, 0,
			0.0, 0, 0.0);
	}


	if (tr.flFraction >= 1.0)
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay(0.5);

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
	}
	else
	{
		switch (((m_iSwing++) % 2) + 1)
		{
		case 0:
			SendWeaponAnim(CROWBAR_ATTACK1HIT);
			break;
		case 1:
			SendWeaponAnim(CROWBAR_ATTACK2HIT);
			break;
		case 2:
			SendWeaponAnim(CROWBAR_ATTACK3HIT);
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		fDidHit = true;
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase()) || g_pGameRules->IsMultiplayer())
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer, GetSkillFloat("plr_crowbar"sv), gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer, GetSkillFloat("plr_crowbar"sv) / 2, gpGlobals->v_forward, &tr, DMG_CLUB);
		}
		ApplyMultiDamage(m_pPlayer, m_pPlayer);

#endif

		m_flNextPrimaryAttack = GetNextAttackDelay(0.25);

#ifndef CLIENT_DLL
		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool fHitWorld = true;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch (RANDOM_LONG(0, 2))
				{
				case 0:
					m_pPlayer->EmitSound(CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM);
					break;
				case 1:
					m_pPlayer->EmitSound(CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM);
					break;
				case 2:
					m_pPlayer->EmitSound(CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM);
					break;
				}
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return true;
				else
					flVol = 0.1;

				fHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				m_pPlayer->EmitSoundDyn(CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				m_pPlayer->EmitSoundDyn(CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		SetThink(&CCrowbar::Smack);
		pev->nextthink = gpGlobals->time + 0.2;
	}
	return fDidHit;
}
