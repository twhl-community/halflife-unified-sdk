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

		if (m_PlayPickupSound)
		{
			if ((pev->spawnflags & SF_SUIT_SHORTLOGON) != 0)
				EMIT_SOUND_SUIT(player, "!HEV_A0"); // short version of suit logon,
			else
				EMIT_SOUND_SUIT(player, "!HEV_AAx"); // long version of suit logon
		}

		player->SetHasSuit(true);
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);

// Unused alias of the suit
LINK_ENTITY_TO_CLASS(item_vest, CItemSuit);

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
		if (m_PlayPickupSound)
		{
			player->SetSuitUpdate("!HEV_DET4", false, SUIT_NEXT_IN_1MIN);
		}

		player->m_rgItems[ITEM_ANTIDOTE] += 1;
		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);

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
		if (player->HasLongJump())
		{
			return false;
		}

		if (player->HasSuit())
		{
			player->SetHasLongJump(true); // player now has longjump module

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, player);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();

			if (m_PlayPickupSound)
			{
				EMIT_SOUND_SUIT(player, "!HEV_A1"); // Play the longjump sound UNDONE: Kelly? correct sound?
			}

			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump);

class CHealthKit : public CItem
{
	DECLARE_CLASS(CHealthKit, CItem);
	DECLARE_DATAMAP();

public:
	static constexpr float RefillHealthAmount = -1;

	void OnCreate() override
	{
		CItem::OnCreate();
		m_HealthAmount = GetSkillFloat("healthkit"sv);
		pev->model = MAKE_STRING("models/w_medkit.mdl");
	}

	void Precache() override
	{
		CItem::Precache();
		PrecacheSound("items/smallmedkit1.wav");
	}

	bool KeyValue(KeyValueData* pkvd) override
	{
		if (FStrEq(pkvd->szKeyName, "health_amount"))
		{
			m_HealthAmount = std::max(RefillHealthAmount, static_cast<float>(atof(pkvd->szValue)));
			return true;
		}

		return CItem::KeyValue(pkvd);
	}

	bool AddItem(CBasePlayer* player) override
	{
		if (player->pev->deadflag != DEAD_NO)
		{
			return false;
		}

		float amount = m_HealthAmount;

		if (amount == RefillHealthAmount)
		{
			amount = player->pev->max_health;
		}

		if (player->GiveHealth(amount, DMG_GENERIC))
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, player);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();

			if (m_PlayPickupSound)
			{
				player->EmitSound(CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM);
			}

			return true;
		}

		return false;
	}

protected:
	float m_HealthAmount = 0;
};

LINK_ENTITY_TO_CLASS(item_healthkit, CHealthKit);

BEGIN_DATAMAP(CHealthKit)
DEFINE_FIELD(m_HealthAmount, FIELD_FLOAT),
	END_DATAMAP();

class CItemBattery : public CItem
{
	DECLARE_CLASS(CItemBattery, CItem);
	DECLARE_DATAMAP();

public:
	static constexpr float RefillArmorAmount = -1;

	void OnCreate() override
	{
		CItem::OnCreate();
		m_ArmorAmount = GetSkillFloat("battery"sv);
		pev->model = MAKE_STRING("models/w_battery.mdl");
	}

	bool KeyValue(KeyValueData* pkvd) override
	{
		if (FStrEq(pkvd->szKeyName, "armor_amount"))
		{
			m_ArmorAmount = std::max(RefillArmorAmount, static_cast<float>(atof(pkvd->szValue)));
			return true;
		}

		return CItem::KeyValue(pkvd);
	}

	void Precache() override
	{
		CItem::Precache();
		PrecacheSound("items/gunpickup2.wav");
	}

	bool AddItem(CBasePlayer* player) override
	{
		return AddItemCore(player, true);
	}

protected:
	bool AddItemCore(CBasePlayer* player, bool playHEVSound)
	{
		if (player->pev->deadflag != DEAD_NO)
		{
			return false;
		}

		float amount = m_ArmorAmount;

		if (amount == RefillArmorAmount)
		{
			amount = MAX_NORMAL_BATTERY;
		}

		if (player->pev->armorvalue < MAX_NORMAL_BATTERY && player->HasSuit())
		{
			player->pev->armorvalue += amount;
			player->pev->armorvalue = std::min(player->pev->armorvalue, static_cast<float>(MAX_NORMAL_BATTERY));

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, nullptr, player);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();

			if (m_PlayPickupSound)
			{
				player->EmitSound(CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

				if (playHEVSound)
				{
					// Suit reports new power level
					// For some reason this wasn't working in release build -- round it.
					int pct = (int)((float)(player->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
					pct = (pct / 5);
					if (pct > 0)
						pct--;

					char szcharge[64];
					sprintf(szcharge, "!HEV_%1dP", pct);

					// EMIT_SOUND_SUIT(this, szcharge);
					player->SetSuitUpdate(szcharge, false, SUIT_NEXT_IN_30SEC);
				}
			}

			return true;
		}
		return false;
	}

protected:
	float m_ArmorAmount = 0;
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);

BEGIN_DATAMAP(CItemBattery)
DEFINE_FIELD(m_ArmorAmount, FIELD_FLOAT),
	END_DATAMAP();

class CItemHelmet : public CItemBattery
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();
		m_ArmorAmount = 40; // TODO: add to skill.json
		pev->model = MAKE_STRING("models/barney_helmet.mdl");
	}

	bool AddItem(CBasePlayer* player) override
	{
		return AddItemCore(player, false);
	}
};

LINK_ENTITY_TO_CLASS(item_helmet, CItemHelmet);

class CItemArmorVest : public CItemBattery
{
public:
	void OnCreate() override
	{
		CItem::OnCreate();
		m_ArmorAmount = 60; // TODO: add to skill.json
		pev->model = MAKE_STRING("models/barney_vest.mdl");
	}

	bool AddItem(CBasePlayer* player) override
	{
		return AddItemCore(player, false);
	}
};

LINK_ENTITY_TO_CLASS(item_armorvest, CItemArmorVest);
