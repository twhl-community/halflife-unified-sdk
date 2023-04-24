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
#include "AmmoTypeSystem.h"
#include "gamerules/PlayerInventory.h"

#ifdef CLIENT_DLL
#include "com_weapons.h"
#endif

BEGIN_DATAMAP(CBasePlayerWeapon)
DEFINE_FIELD(m_pPlayer, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pNext, FIELD_CLASSPTR),
	// DEFINE_FIELD(m_fKnown, FIELD_INTEGER),Reset to zero on load
	DEFINE_FIELD(m_iId, FIELD_INTEGER),
// DEFINE_FIELDm_iIdPrimary, FIELD_INTEGER),
// DEFINE_FIELDm_iIdSecondary, FIELD_INTEGER),
#if defined(CLIENT_WEAPONS)
	DEFINE_FIELD(m_flNextPrimaryAttack, FIELD_FLOAT),
	DEFINE_FIELD(m_flNextSecondaryAttack, FIELD_FLOAT),
	DEFINE_FIELD(m_flTimeWeaponIdle, FIELD_FLOAT),
#else  // CLIENT_WEAPONS
	DEFINE_FIELD(m_flNextPrimaryAttack, FIELD_TIME),
	DEFINE_FIELD(m_flNextSecondaryAttack, FIELD_TIME),
	DEFINE_FIELD(m_flTimeWeaponIdle, FIELD_TIME),
#endif // CLIENT_WEAPONS
	DEFINE_FIELD(m_iPrimaryAmmoType, FIELD_INTEGER),
	DEFINE_FIELD(m_iSecondaryAmmoType, FIELD_INTEGER),
	DEFINE_FIELD(m_iClip, FIELD_INTEGER),
	DEFINE_FIELD(m_iDefaultAmmo, FIELD_INTEGER),
	DEFINE_FIELD(m_iDefaultPrimaryAmmo, FIELD_INTEGER),
	//	DEFINE_FIELD(m_iClientClip, FIELD_INTEGER)	 , reset to zero on load so hud gets updated correctly
	//  DEFINE_FIELD(m_iClientWeaponState, FIELD_INTEGER), reset to zero on load so hud gets updated correctly
	DEFINE_FIELD(m_WorldModel, FIELD_STRING),
	DEFINE_FIELD(m_ViewModel, FIELD_STRING),
	DEFINE_FIELD(m_PlayerModel, FIELD_STRING),
	DEFINE_FUNCTION(DestroyItem),
	DEFINE_FUNCTION(CallDoRetireWeapon),
	END_DATAMAP();

// m_AmmoName isn't saved here because it's initialized by all derived classes.
// Classes that let level designers set the name should also save it.
BEGIN_DATAMAP(CBasePlayerAmmo)
DEFINE_FIELD(m_AmmoAmount, FIELD_INTEGER),
	END_DATAMAP();

void FindHullIntersection(const Vector& vecSrc, TraceResult& tr, const Vector& mins, const Vector& maxs, CBaseEntity* pEntity)
{
	int i, j, k;
	float distance;
	const Vector* minmaxs[2] = {&mins, &maxs};
	TraceResult tmpTrace;
	Vector vecHullEnd = tr.vecEndPos;
	Vector vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc) * 2);
	UTIL_TraceLine(vecSrc, vecHullEnd, dont_ignore_monsters, pEntity->edict(), &tmpTrace);
	if (tmpTrace.flFraction < 1.0)
	{
		tr = tmpTrace;
		return;
	}

	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			for (k = 0; k < 2; k++)
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i]->x;
				vecEnd.y = vecHullEnd.y + minmaxs[j]->y;
				vecEnd.z = vecHullEnd.z + minmaxs[k]->z;

				UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, pEntity->edict(), &tmpTrace);
				if (tmpTrace.flFraction < 1.0)
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if (thisDistance < distance)
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

void CBasePlayerWeapon::OnCreate()
{
	CBaseItem::OnCreate();
	LinkWeaponInfo();
}

void CBasePlayerWeapon::Spawn()
{
	m_iDefaultPrimaryAmmo = m_iDefaultAmmo;

	Precache();
	SetModel(GetModelName());
	SetupItem(vec3_origin, vec3_origin); // pointsize until it lands on the ground.
}

void CBasePlayerWeapon::LinkWeaponInfo()
{
	m_WeaponInfo = g_WeaponData.GetByName(GetClassname());

	if (!m_WeaponInfo)
	{
		m_WeaponInfo = &g_WeaponData.DummyInfo;
	}

	// (re)initialize ammo type indices.
	if (!m_WeaponInfo->AttackModeInfo[0].AmmoType.empty())
	{
		m_iPrimaryAmmoType = g_AmmoTypes.IndexOf(m_WeaponInfo->AttackModeInfo[0].AmmoType.c_str());
	}
	else
	{
		m_iPrimaryAmmoType = -1;
	}

	if (!m_WeaponInfo->AttackModeInfo[1].AmmoType.empty())
	{
		m_iSecondaryAmmoType = g_AmmoTypes.IndexOf(m_WeaponInfo->AttackModeInfo[1].AmmoType.c_str());
	}
	else
	{
		m_iSecondaryAmmoType = -1;
	}
}

void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body)
{
	m_pPlayer->pev->weaponanim = iAnim;

#ifndef CLIENT_DLL
#if defined(CLIENT_WEAPONS)
	const bool skiplocal = !m_ForceSendAnimations && UseDecrement();

	if (skiplocal && ENGINE_CANSKIP(m_pPlayer->edict()))
		return;
#endif

	MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, nullptr, m_pPlayer);
	WRITE_BYTE(iAnim);	   // sequence number
	WRITE_BYTE(pev->body); // weaponmodel bodygroup.
	MESSAGE_END();
#else
	HUD_SendWeaponAnim(iAnim, body, false);
#endif
}

bool CBasePlayerWeapon::CanDeploy()
{
	bool bHasAmmo = false;

	if (!pszAmmo1())
	{
		// this weapon doesn't use ammo, can always deploy.
		return true;
	}

	if (pszAmmo1())
	{
		bHasAmmo |= m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) != 0;
	}
	if (pszAmmo2())
	{
		bHasAmmo |= m_pPlayer->GetAmmoCountByIndex(m_iSecondaryAmmoType) != 0;
	}
	if (m_iClip > 0)
	{
		bHasAmmo |= true;
	}
	if (!bHasAmmo)
	{
		return false;
	}

	return true;
}

bool CBasePlayerWeapon::DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body)
{
	if (!CanDeploy())
		return false;

#ifndef CLIENT_DLL
	SetWeaponModels(szViewModel, szWeaponModel);
	strcpy(m_pPlayer->m_szAnimExtention, szAnimExt);
#else
	LoadVModel(szViewModel, m_pPlayer);
	g_irunninggausspred = false;
#endif

	SendWeaponAnim(iAnim, body);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_flLastFireTime = 0.0;

	return true;
}

void CBasePlayerWeapon::Holster()
{
	m_fInReload = false; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = string_t::Null;
	m_pPlayer->pev->weaponmodel = string_t::Null;

#ifdef CLIENT_DLL
	g_irunninggausspred = false;
#endif
}

bool CBasePlayerWeapon::DefaultReload(int iClipSize, int iAnim, float fDelay, int body)
{
	if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) <= 0)
		return false;

	if ((m_pPlayer->m_iItems & CTFItem::Backpack) != 0)
	{
		iClipSize *= 2;
	}

	int j = std::min(iClipSize - m_iClip, m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim(iAnim, body);

	m_fInReload = true;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	return true;
}

void CBasePlayerWeapon::ResetEmptySound()
{
	m_iPlayEmptySound = true;
}

bool CBasePlayerWeapon::PlayEmptySound()
{
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND_PREDICTED(m_pPlayer, CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM, 0, PITCH_NORM);
		m_iPlayEmptySound = false;
		return false;
	}
	return false;
}

bool CBasePlayerWeapon::IsUseable()
{
	if (m_iClip > 0)
	{
		return true;
	}

	// Weapon doesn't use ammo.
	if (m_iPrimaryAmmoType == -1)
	{
		return true;
	}

	if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) > 0)
	{
		return true;
	}

	if (m_iSecondaryAmmoType != -1)
	{
		if (m_pPlayer->GetAmmoCountByIndex(m_iSecondaryAmmoType) > 0)
		{
			return true;
		}
	}

	// clip is empty (or nonexistant) and the player has no more ammo of this type.
	return false;
}

bool CanAttack(float attack_time, float curtime, bool isPredicted)
{
#if defined(CLIENT_WEAPONS)
	if (!isPredicted)
#else
	if (1)
#endif
	{
		return (attack_time <= curtime) ? true : false;
	}
	else
	{
		return ((static_cast<int>(std::floor(attack_time * 1000.0)) * 1000.0) <= 0.0) ? true : false;
	}
}

void CBasePlayerWeapon::FinishReload(bool force)
{
	int maxClip = iMaxClip();

#ifndef CLIENT_DLL
	// Reset max clip and max ammo to default values
	if ((m_pPlayer->m_iItems & CTFItem::Backpack) == 0)
	{
		if (m_iClip > iMaxClip())
		{
			m_pPlayer->AdjustAmmoByIndex(m_iPrimaryAmmoType, m_iClip - iMaxClip());
			m_iClip = iMaxClip();
		}
	}
	else
	{
		if (maxClip > 1)
		{
			maxClip *= 2;
		}
	}
#endif

	if (force || (m_fInReload && m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase()))
	{
		// complete the reload.
		int j = std::min(maxClip - m_iClip, m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType));

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->AdjustAmmoByIndex(m_iPrimaryAmmoType, -j);

		m_fInReload = false;
	}
}

void CBasePlayerWeapon::ItemPostFrame()
{
	FinishReload(false);

	if ((m_pPlayer->pev->button & IN_ATTACK) == 0)
	{
		m_flLastFireTime = 0.0f;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) != 0 && CanAttack(m_flNextSecondaryAttack, gpGlobals->time, UseDecrement()))
	{
		if (pszAmmo2() && 0 == m_pPlayer->GetAmmoCountByIndex(m_iSecondaryAmmoType))
		{
			m_fFireOnEmpty = true;
		}

		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) != 0 && CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()))
	{
		if ((m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && 0 == m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType)))
		{
			m_fFireOnEmpty = true;
		}

		PrimaryAttack();
	}
	else if ((m_pPlayer->pev->button & IN_RELOAD) != 0 && iMaxClip() != WEAPON_NOCLIP && !m_fInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if ((m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)) == 0)
	{
		// no fire buttons down

		m_fFireOnEmpty = false;

		if (!IsUseable() && m_flNextPrimaryAttack < (UseDecrement() ? 0.0 : gpGlobals->time))
		{
#ifndef CLIENT_DLL
			// weapon isn't useable, switch.
			if ((iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) == 0 && g_pGameRules->GetNextBestWeapon(m_pPlayer, this))
			{
				m_flNextPrimaryAttack = (UseDecrement() ? 0.0 : gpGlobals->time) + 0.3;
				return;
			}
#endif
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if (m_iClip == 0 && (iFlags() & ITEM_FLAG_NOAUTORELOAD) == 0 && m_flNextPrimaryAttack < (UseDecrement() ? 0.0 : gpGlobals->time))
			{
				Reload();
				return;
			}
		}

		WeaponIdle();
		return;
	}

	// catch all
	if (ShouldWeaponIdle())
	{
		WeaponIdle();
	}
}

void CBasePlayerWeapon::SavePersistentState(PersistentWeaponState& state)
{
	state.Properties.emplace("Clip", m_iClip);
}

void CBasePlayerWeapon::LoadPersistentState(const PersistentWeaponState& state)
{
	if (auto it = state.Properties.find("Clip"); it != state.Properties.end())
	{
		if (auto value = std::any_cast<int>(&it->second); value)
		{
			m_iClip = *value;
		}
	}
}

int CBasePlayerWeapon::GetMagazine1() const
{
	return m_iClip;
}

void CBasePlayerWeapon::SetMagazine1(int count)
{
	if (count == WEAPON_NOCLIP)
	{
		m_iClip = WEAPON_NOCLIP;
		return;
	}

	m_iClip = std::max(0, count);
}

void CBasePlayerWeapon::AdjustMagazine1(int count)
{
	if (m_iClip == WEAPON_NOCLIP)
	{
		return;
	}

	if (count < 0)
	{
		// Subtract from reserve ammo first.
		if (g_Skill.GetValue("bottomless_magazines") != 0)
		{
			const int amountAdjusted = m_pPlayer->AdjustAmmoByIndex(m_iPrimaryAmmoType, count);

			count -= amountAdjusted;
		}
	}

	m_iClip = std::clamp(m_iClip + count, 0, iMaxClip());
}
