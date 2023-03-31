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

int CBasePlayer::AmmoInventory(int iAmmoIndex) const
{
	if (iAmmoIndex == -1)
	{
		return -1;
	}

	return m_rgAmmo[iAmmoIndex];
}

int CBasePlayer::GetAmmoIndex(const char* psz)
{
	if (!psz)
		return -1;

	return g_AmmoTypes.IndexOf(psz);
}

int CBasePlayer::GetAmmoCount(const char* ammoName) const
{
	return AmmoInventory(GetAmmoIndex(ammoName));
}

void CBasePlayer::SetAmmoCount(const char* ammoName, int count)
{
	const int index = GetAmmoIndex(ammoName);

	if (index != -1)
	{
		m_rgAmmo[index] = count;
	}
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
