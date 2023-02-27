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
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "cbase.h"
#include "items.h"
#include "UserMessages.h"

class CWorldItem : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	int m_iType;
};

LINK_ENTITY_TO_CLASS(world_items, CWorldItem);

bool CWorldItem::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CWorldItem::Spawn()
{
	CBaseEntity* pEntity = nullptr;

	switch (m_iType)
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create("item_battery", pev->origin, pev->angles);
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create("item_antidote", pev->origin, pev->angles);
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create("item_security", pev->origin, pev->angles);
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create("item_suit", pev->origin, pev->angles);
		break;
	}

	if (!pEntity)
	{
		CBaseEntity::Logger->debug("unable to create world_item {}", m_iType);
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}

void CItem::Precache()
{
	if (!FStringNull(pev->model))
	{
		PrecacheModel(STRING(pev->model));
	}
}

void CItem::Spawn()
{
	Precache();

	if (!FStringNull(pev->model))
	{
		SetModel(STRING(pev->model));
	}

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	SetSize(Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		CBaseEntity::Logger->error("Item {} fell out of level at {}", STRING(pev->classname), pev->origin);
		UTIL_Remove(this);
		return;
	}
}

void CItem::ItemTouch(CBaseEntity* pOther)
{
	// if it's not a player, ignore
	if (!pOther->IsPlayer())
	{
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	// ok, a player is touching this item, but can he have it?
	if (!g_pGameRules->CanHaveItem(pPlayer, this))
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch(pPlayer))
	{
		SUB_UseTargets(pOther, USE_TOGGLE, 0);
		SetTouch(nullptr);

		// player grabbed the item.
		g_pGameRules->PlayerGotItem(pPlayer, this);
		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);
		}
	}
	else if (gEvilImpulse101)
	{
		UTIL_Remove(this);
	}
}

CBaseEntity* CItem::Respawn()
{
	SetTouch(nullptr);
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin(pev, g_pGameRules->VecItemRespawnSpot(this)); // blip to whereever you should respawn.

	SetThink(&CItem::Materialize);
	pev->nextthink = g_pGameRules->FlItemRespawnTime(this);
	return this;
}

void CItem::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch(&CItem::ItemTouch);
}

#define SF_SUIT_SHORTLOGON 0x0001

class CItemSuit : public CItem
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();

		pev->model = MAKE_STRING("models/w_suit.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->HasSuit())
			return false;

		if ((pev->spawnflags & SF_SUIT_SHORTLOGON) != 0)
			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A0"); // short version of suit logon,
		else
			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_AAx"); // long version of suit logon

		pPlayer->SetHasSuit(true);
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);

// Unused alias of the suit
LINK_ENTITY_TO_CLASS(item_vest, CItemSuit);

class CItemBattery : public CItem
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();

		pev->model = MAKE_STRING("models/w_battery.mdl");
	}
	void Precache() override
	{
		CItem::Precache();
		PrecacheSound("items/gunpickup2.wav");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->pev->deadflag != DEAD_NO)
		{
			return false;
		}

		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			pPlayer->HasSuit())
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += GetSkillFloat("battery"sv);
			pPlayer->pev->armorvalue = V_min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();


			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)((float)(pPlayer->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;

			sprintf(szcharge, "!HEV_%1dP", pct);

			// EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, false, SUIT_NEXT_IN_30SEC);
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);


class CItemAntidote : public CItem
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();

		pev->model = MAKE_STRING("models/w_antidote.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		pPlayer->SetSuitUpdate("!HEV_DET4", false, SUIT_NEXT_IN_1MIN);

		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);


class CItemSecurity : public CItem
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();

		pev->model = MAKE_STRING("models/w_security.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);

class CItemLongJump : public CItem
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();

		pev->model = MAKE_STRING("models/w_longjump.mdl");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if (pPlayer->m_fLongJump)
		{
			return false;
		}

		if (pPlayer->HasSuit())
		{
			pPlayer->m_fLongJump = true; // player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "slj", "1");

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();

			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A1"); // Play the longjump sound UNDONE: Kelly? correct sound?
			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump);

class CItemHelmet : public CItem
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();

		pev->model = MAKE_STRING("models/Barney_Helmet.mdl");
	}
	void Precache() override
	{
		CItem::Precache();
		PrecacheSound("items/gunpickup2.wav");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) && pPlayer->HasSuit())
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += 40;
			pPlayer->pev->armorvalue = V_min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();


			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)((float)(pPlayer->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;

			sprintf(szcharge, "!HEV_%1dP", pct);

			// EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, false, SUIT_NEXT_IN_30SEC);
			return true;
		}

		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_helmet, CItemHelmet);

class CItemArmorVest : public CItem
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();

		pev->model = MAKE_STRING("models/Barney_Vest.mdl");
	}
	void Precache() override
	{
		CItem::Precache();
		PrecacheSound("items/gunpickup2.wav");
	}
	bool MyTouch(CBasePlayer* pPlayer) override
	{
		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) && pPlayer->HasSuit())
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += 60;
			pPlayer->pev->armorvalue = V_min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();


			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)((float)(pPlayer->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;

			sprintf(szcharge, "!HEV_%1dP", pct);

			// EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, false, SUIT_NEXT_IN_30SEC);
			return true;
		}

		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_armorvest, CItemArmorVest);
