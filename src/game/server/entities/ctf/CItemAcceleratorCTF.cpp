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
#include "player.h"
#include "game.h"
#include "UserMessages.h"
#include "CItemAcceleratorCTF.h"

LINK_ENTITY_TO_CLASS(item_ctfaccelerator, CItemAcceleratorCTF);

void CItemAcceleratorCTF::Precache()
{
	CItemCTF::Precache();

	PRECACHE_MODEL("models/w_accelerator.mdl");
	PRECACHE_SOUND("turret/tu_ping.wav");
}

void CItemAcceleratorCTF::Spawn()
{
	Precache();

	SET_MODEL(edict(), "models/w_accelerator.mdl");

	//TODO: is this actually used?
	pev->spawnflags |= SF_NORESPAWN;

	pev->oldorigin = pev->origin;

	CItemCTF::Spawn();

	m_iItemFlag = CTFItem::Acceleration;
	m_pszItemName = "Damage";
}

void CItemAcceleratorCTF::RemoveEffect(CBasePlayer* pPlayer)
{
	pPlayer->m_flAccelTime += gpGlobals->time - m_flPickupTime;
}

bool CItemAcceleratorCTF::MyTouch(CBasePlayer* pPlayer)
{
	if ((pPlayer->m_iItems & CTFItem::Acceleration) == 0)
	{
		if (0 == multipower.value)
		{
			if ((pPlayer->m_iItems & CTFItem::ItemsMask) != 0)
				return false;
		}

		if (team_no == CTFTeam::None || team_no == pPlayer->m_iTeamNum)
		{
			if (pPlayer->HasSuit())
			{
				pPlayer->m_iItems = static_cast<CTFItem::CTFItem>(pPlayer->m_iItems | CTFItem::Acceleration);
				MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, pPlayer->edict());
				WRITE_STRING(STRING(pev->classname));
				MESSAGE_END();
				EMIT_SOUND(edict(), CHAN_VOICE, "items/ammopickup1.wav", VOL_NORM, ATTN_NORM);
				return true;
			}
		}
	}

	return false;
}
