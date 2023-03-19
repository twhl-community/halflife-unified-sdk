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
#include "items.h"
#include "UserMessages.h"

#define SF_SUIT_SHORTLOGON 0x0001

class CItemSuit : public CItem
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();

		pev->model = MAKE_STRING("models/w_suit.mdl");
	}

	bool AddItem(CBasePlayer* player) override
	{
		if (player->HasSuit())
			return false;

		if ((pev->spawnflags & SF_SUIT_SHORTLOGON) != 0)
			EMIT_SOUND_SUIT(player, "!HEV_A0"); // short version of suit logon,
		else
			EMIT_SOUND_SUIT(player, "!HEV_AAx"); // long version of suit logon

		player->SetHasSuit(true);
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
	bool AddItem(CBasePlayer* player) override
	{
		if (player->pev->deadflag != DEAD_NO)
		{
			return false;
		}

		if ((player->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			player->HasSuit())
		{
			int pct;
			char szcharge[64];

			player->pev->armorvalue += GetSkillFloat("battery"sv);
			player->pev->armorvalue = V_min(player->pev->armorvalue, MAX_NORMAL_BATTERY);

			player->EmitSound(CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, player->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();


			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)((float)(player->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;

			sprintf(szcharge, "!HEV_%1dP", pct);

			// EMIT_SOUND_SUIT(this, szcharge);
			player->SetSuitUpdate(szcharge, false, SUIT_NEXT_IN_30SEC);
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
	bool AddItem(CBasePlayer* player) override
	{
		player->SetSuitUpdate("!HEV_DET4", false, SUIT_NEXT_IN_1MIN);

		player->m_rgItems[ITEM_ANTIDOTE] += 1;
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
	bool AddItem(CBasePlayer* player) override
	{
		player->m_rgItems[ITEM_SECURITY] += 1;
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
	bool AddItem(CBasePlayer* player) override
	{
		if (player->m_fLongJump)
		{
			return false;
		}

		if (player->HasSuit())
		{
			player->m_fLongJump = true; // player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue(player->edict(), "slj", "1");

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, player->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();

			EMIT_SOUND_SUIT(player, "!HEV_A1"); // Play the longjump sound UNDONE: Kelly? correct sound?
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
	bool AddItem(CBasePlayer* player) override
	{
		if ((player->pev->armorvalue < MAX_NORMAL_BATTERY) && player->HasSuit())
		{
			int pct;
			char szcharge[64];

			player->pev->armorvalue += 40;
			player->pev->armorvalue = V_min(player->pev->armorvalue, MAX_NORMAL_BATTERY);

			player->EmitSound(CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, player->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();


			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)((float)(player->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;

			sprintf(szcharge, "!HEV_%1dP", pct);

			// EMIT_SOUND_SUIT(this, szcharge);
			player->SetSuitUpdate(szcharge, false, SUIT_NEXT_IN_30SEC);
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
	bool AddItem(CBasePlayer* player) override
	{
		if ((player->pev->armorvalue < MAX_NORMAL_BATTERY) && player->HasSuit())
		{
			int pct;
			char szcharge[64];

			player->pev->armorvalue += 60;
			player->pev->armorvalue = V_min(player->pev->armorvalue, MAX_NORMAL_BATTERY);

			player->EmitSound(CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, player->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();


			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)((float)(player->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;

			sprintf(szcharge, "!HEV_%1dP", pct);

			// EMIT_SOUND_SUIT(this, szcharge);
			player->SetSuitUpdate(szcharge, false, SUIT_NEXT_IN_30SEC);
			return true;
		}

		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_armorvest, CItemArmorVest);
