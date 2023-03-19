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
#include "CItemCTF.h"
#include "CItemPortableHEVCTF.h"
#include "CHalfLifeCTFplay.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(item_ctfportablehev, CItemPortableHEVCTF);

void CItemPortableHEVCTF::OnCreate()
{
	CItemCTF::OnCreate();

	pev->model = MAKE_STRING("models/w_porthev.mdl");
}

void CItemPortableHEVCTF::Precache()
{
	PrecacheModel(STRING(pev->model));
	PrecacheSound("ctf/pow_armor_charge.wav");
}

void CItemPortableHEVCTF::RemoveEffect(CBasePlayer* pPlayer)
{
	pPlayer->m_flShieldTime += gpGlobals->time - m_flPickupTime;
}

bool CItemPortableHEVCTF::MyTouch(CBasePlayer* pPlayer)
{
	if ((pPlayer->m_iItems & CTFItem::PortableHEV) == 0)
	{
		if (0 == multipower.value)
		{
			if ((pPlayer->m_iItems & ~(CTFItem::BlackMesaFlag | CTFItem::OpposingForceFlag)) != 0)
				return false;
		}

		if (static_cast<int>(team_no) <= 0 || team_no == pPlayer->m_iTeamNum)
		{
			if (pPlayer->HasSuit())
			{
				pPlayer->m_iItems = static_cast<CTFItem::CTFItem>(pPlayer->m_iItems | CTFItem::PortableHEV);
				pPlayer->m_fPlayingAChargeSound = false;

				g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgItemPickup, nullptr, pPlayer->edict());
				g_engfuncs.pfnWriteString(STRING(pev->classname));
				g_engfuncs.pfnMessageEnd();

				EmitSound(CHAN_VOICE, "items/ammopickup1.wav", VOL_NORM, ATTN_NORM);

				return true;
			}
		}
	}
	return false;
}

void CItemPortableHEVCTF::Spawn()
{
	PrecacheSound("ctf/itemthrow.wav");
	PrecacheSound("items/ammopickup1.wav");

	Precache();

	SetModel(STRING(pev->model));

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
