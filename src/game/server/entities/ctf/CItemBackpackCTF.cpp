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
#include "CItemBackpackCTF.h"
#include "gamerules.h"
#include "ctfplay_gamerules.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(item_ctfbackpack, CItemBackpackCTF);

void CItemBackpackCTF::Precache()
{
	g_engfuncs.pfnPrecacheModel("models/w_backpack.mdl");
	g_engfuncs.pfnPrecacheSound("ctf/pow_backpack.wav");
}

void CItemBackpackCTF::RemoveEffect(CBasePlayer* pPlayer)
{
	pPlayer->m_flBackpackTime += gpGlobals->time - m_flPickupTime;
}

bool CItemBackpackCTF::MyTouch(CBasePlayer* pPlayer)
{
	if ((pPlayer->m_iItems & CTFItem::Backpack) == 0)
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
				pPlayer->m_iItems = static_cast<CTFItem::CTFItem>(pPlayer->m_iItems | CTFItem::Backpack);
				g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgItemPickup, 0, pPlayer->edict());
				g_engfuncs.pfnWriteString(STRING(pev->classname));
				g_engfuncs.pfnMessageEnd();

				EMIT_SOUND_DYN(edict(), CHAN_VOICE, "items/ammopickup1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

				pPlayer->GiveAmmo(AMMO_URANIUMBOX_GIVE, "uranium", URANIUM_MAX_CARRY);
				pPlayer->GiveAmmo(AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY);
				pPlayer->GiveAmmo(AMMO_357BOX_GIVE, "357", _357_MAX_CARRY);
				pPlayer->GiveAmmo(AMMO_BUCKSHOTBOX_GIVE, "buckshot", BUCKSHOT_MAX_CARRY);
				pPlayer->GiveAmmo(CROSSBOW_DEFAULT_GIVE, "bolts", BOLT_MAX_CARRY);
				pPlayer->GiveAmmo(1, "rockets", ROCKET_MAX_CARRY);
				pPlayer->GiveAmmo(HANDGRENADE_DEFAULT_GIVE, "Hand Grenade", HANDGRENADE_MAX_CARRY);
				pPlayer->GiveAmmo(SNARK_DEFAULT_GIVE, "Snarks", SNARK_MAX_CARRY);
				pPlayer->GiveAmmo(SPORELAUNCHER_DEFAULT_GIVE, "spores", SPORELAUNCHER_MAX_CARRY);
				pPlayer->GiveAmmo(SNIPERRIFLE_DEFAULT_GIVE, "762", SNIPERRIFLE_MAX_CARRY);
				pPlayer->GiveAmmo(M249_MAX_CARRY, "556", M249_MAX_CARRY);

				return true;
			}
		}
	}

	return false;
}

void CItemBackpackCTF::Spawn()
{
	//TODO: precache calls should be in Precache
	if (!FStringNull(pev->model))
		g_engfuncs.pfnPrecacheModel((char*)STRING(pev->model));

	g_engfuncs.pfnPrecacheSound("ctf/itemthrow.wav");
	g_engfuncs.pfnPrecacheSound("items/ammopickup1.wav");

	Precache();

	//TODO: shouldn't this be using pev->model?
	g_engfuncs.pfnSetModel(edict(), "models/w_backpack.mdl");

	pev->spawnflags |= SF_NORESPAWN;
	pev->oldorigin = pev->origin;

	CItemCTF::Spawn();

	m_iItemFlag = CTFItem::Backpack;
	m_pszItemName = "Ammo";
}

int CItemBackpackCTF::Classify()
{
	return CLASS_CTFITEM;
}
