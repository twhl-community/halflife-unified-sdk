/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

LINK_ENTITY_TO_CLASS(player, CBasePlayer);

void CBasePlayer::OnCreate()
{
	BaseClass::OnCreate();

	SetClassification("player");
}

int CBasePlayer::GetAmmoIndex(const char* psz)
{
	if (!psz)
		return -1;

	return g_AmmoTypes.IndexOf(psz);
}

int CBasePlayer::GetAmmoCount(const char* ammoName) const
{
	return GetAmmoCountByIndex(GetAmmoIndex(ammoName));
}

int CBasePlayer::GetAmmoCountByIndex(int ammoIndex) const
{
	if (ammoIndex < 0 || ammoIndex >= MAX_AMMO_TYPES)
	{
		return -1;
	}

	if (g_Skill.GetValue("infinite_ammo") != 0)
	{
		return g_AmmoTypes.GetByIndex(ammoIndex)->MaximumCapacity;
	}

	return m_rgAmmo[ammoIndex];
}

void CBasePlayer::SetAmmoCount(const char* ammoName, int count)
{
	SetAmmoCountByIndex(GetAmmoIndex(ammoName), count);
}

void CBasePlayer::SetAmmoCountByIndex(int ammoIndex, int count)
{
	if (ammoIndex < 0 || ammoIndex >= MAX_AMMO_TYPES)
	{
		return;
	}

	if (g_Skill.GetValue("infinite_ammo") != 0)
	{
		count = g_AmmoTypes.GetByIndex(ammoIndex)->MaximumCapacity;
	}

	m_rgAmmo[ammoIndex] = std::max(0, count);
}

int CBasePlayer::AdjustAmmoByIndex(int ammoIndex, int count)
{
	if (ammoIndex < 0 || ammoIndex >= MAX_AMMO_TYPES)
	{
		return 0;
	}

	const auto ammoType = g_AmmoTypes.GetByIndex(ammoIndex);

	if (g_Skill.GetValue("infinite_ammo") != 0)
	{
		m_rgAmmo[ammoIndex] = ammoType->MaximumCapacity;
		return count;
	}

	// Don't allow ammo to overflow capacity.
	const int old = m_rgAmmo[ammoIndex];
	m_rgAmmo[ammoIndex] = std::clamp(old + count, 0, ammoType->MaximumCapacity);

	return m_rgAmmo[ammoIndex] - old;
}

void CBasePlayer::SelectItem(const char* pstr)
{
	if (!pstr)
		return;

	CBasePlayerWeapon* pItem = nullptr;

#ifndef CLIENT_DLL
	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		if (m_rgpPlayerWeapons[i])
		{
			pItem = m_rgpPlayerWeapons[i];

			while (pItem)
			{
				if (pItem->ClassnameIs(pstr))
					break;
				pItem = pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}
#endif

	if (!pItem)
		return;

	if (pItem == m_pActiveWeapon)
		return;

	DeployWeapon(pItem);
}

void CBasePlayer::SelectItem(int iId)
{
	if (iId <= WEAPON_NONE)
		return;

	CBasePlayerWeapon* pItem = nullptr;

	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		if (m_rgpPlayerWeapons[i])
		{
			pItem = m_rgpPlayerWeapons[i];

			while (pItem)
			{
				if (pItem->m_iId == iId)
					break;
				pItem = pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}

	if (!pItem)
		return;


	if (pItem == m_pActiveWeapon)
		return;

	DeployWeapon(pItem);
}

void CBasePlayer::SelectLastItem()
{
	if (!m_pLastWeapon)
	{
		return;
	}

	if (m_pActiveWeapon && !m_pActiveWeapon->CanHolster())
	{
		return;
	}

	DeployWeapon(m_pLastWeapon);
}

void CBasePlayer::DeployWeapon(CBasePlayerWeapon* weapon)
{
	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveWeapon)
		m_pActiveWeapon->Holster();

	m_pLastWeapon = m_pActiveWeapon;
	m_pActiveWeapon = weapon;

	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->m_ForceSendAnimations = true;
		m_pActiveWeapon->Deploy();
		m_pActiveWeapon->m_ForceSendAnimations = false;
		m_pActiveWeapon->UpdateItemInfo();
	}
}
