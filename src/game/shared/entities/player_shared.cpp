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

LINK_ENTITY_TO_CLASS(player, CBasePlayer);

void CBasePlayer::SelectItem(const char* pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem* pItem = nullptr;

#ifndef CLIENT_DLL
	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			pItem = m_rgpPlayerItems[i];

			while (pItem)
			{
				if (FClassnameIs(pItem->pev, pstr))
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

	if (pItem == m_pActiveItem)
		return;

	DeployWeapon(pItem);
}

void CBasePlayer::SelectLastItem()
{
	if (!m_pLastItem)
	{
		return;
	}

	if (m_pActiveItem && !m_pActiveItem->CanHolster())
	{
		return;
	}

	DeployWeapon(m_pLastItem);
}

void CBasePlayer::DeployWeapon(CBasePlayerItem* weapon)
{
	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = weapon;

	if (m_pActiveItem)
	{
		m_pActiveItem->m_ForceSendAnimations = true;
		m_pActiveItem->Deploy();
		m_pActiveItem->m_ForceSendAnimations = false;
		m_pActiveItem->UpdateItemInfo();
	}
}
