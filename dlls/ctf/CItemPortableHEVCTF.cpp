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
#include "weapons.h"
#include "game.h"
#include "CItemCTF.h"
#include "CItemPortableHEVCTF.h"
#include "gamerules.h"
#include "ctfplay_gamerules.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(item_ctfportablehev, CItemPortableHEVCTF);

void CItemPortableHEVCTF::Precache()
{
	g_engfuncs.pfnPrecacheModel("models/w_porthev.mdl");
	g_engfuncs.pfnPrecacheSound("ctf/pow_armor_charge.wav");
}

void CItemPortableHEVCTF::RemoveEffect(CBasePlayer* pPlayer)
{
	pPlayer->m_flShieldTime += gpGlobals->time - m_flPickupTime;
}

bool CItemPortableHEVCTF::MyTouch(CBasePlayer* pPlayer)
{
	if (!(pPlayer->m_iItems & CTFItem::PortableHEV))
	{
		if (!multipower.value)
		{
			if (pPlayer->m_iItems & ~(CTFItem::BlackMesaFlag | CTFItem::OpposingForceFlag))
				return false;
		}

		if (static_cast<int>(team_no) <= 0 || team_no == pPlayer->m_iTeamNum)
		{
			if (pPlayer->pev->weapons & (1 << WEAPON_SUIT))
			{
				pPlayer->m_iItems = static_cast<CTFItem::CTFItem>(pPlayer->m_iItems | CTFItem::PortableHEV);
				pPlayer->m_fPlayingAChargeSound = false;

				g_engfuncs.pfnMessageBegin(1, gmsgItemPickup, 0, pPlayer->edict());
				g_engfuncs.pfnWriteString(STRING(pev->classname));
				g_engfuncs.pfnMessageEnd();

				EMIT_SOUND_DYN(edict(), CHAN_VOICE, "items/ammopickup1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
					
				return true;
			}
		}
	}
	return false;
}

void CItemPortableHEVCTF::Spawn()
{
	if (pev->model)
		g_engfuncs.pfnPrecacheModel((char*)STRING(pev->model));

	g_engfuncs.pfnPrecacheSound("ctf/itemthrow.wav");
	g_engfuncs.pfnPrecacheSound("items/ammopickup1.wav");

	Precache();

	//TODO: shouldn't this be using pev->model?
	g_engfuncs.pfnSetModel(edict(), "models/w_porthev.mdl");

	pev->spawnflags |= SF_NORESPAWN;
	pev->oldorigin = pev->origin;

	CItemCTF::Spawn();

	m_iItemFlag = CTFItem::PortableHEV;
	m_pszItemName = "Shield";
}

int CItemPortableHEVCTF::Classify()
{
	return CLASS_CTFITEM;
}
