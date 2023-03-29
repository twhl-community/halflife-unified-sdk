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
#include "CBaseItem.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION CBaseItem::m_SaveData[] =
	{
		DEFINE_FIELD(CBaseItem, m_RespawnDelay, FIELD_FLOAT),
		DEFINE_FIELD(CBaseItem, m_FallMode, FIELD_INTEGER),
		DEFINE_FIELD(CBaseItem, m_StayVisibleDuringRespawn, FIELD_BOOLEAN),
		DEFINE_FIELD(CBaseItem, m_IsRespawning, FIELD_BOOLEAN),
		DEFINE_FIELD(CBaseItem, m_FlashOnRespawn, FIELD_BOOLEAN),
		DEFINE_FIELD(CBaseItem, m_PlayPickupSound, FIELD_BOOLEAN),
		DEFINE_FIELD(CBaseItem, m_TriggerOnSpawn, FIELD_STRING),
		DEFINE_FIELD(CBaseItem, m_TriggerOnDespawn, FIELD_STRING)};

IMPLEMENT_SAVERESTORE(CBaseItem, CBaseAnimating);
#endif

bool CBaseItem::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "respawn_delay"))
	{
		m_RespawnDelay = atof(pkvd->szValue);

		if (m_RespawnDelay < 0)
		{
			m_RespawnDelay = ITEM_NEVER_RESPAWN_DELAY;
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "stay_visible_during_respawn"))
	{
		m_StayVisibleDuringRespawn = atoi(pkvd->szValue) != 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "flash_on_respawn"))
	{
		m_FlashOnRespawn = atoi(pkvd->szValue) != 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "play_pickup_sound"))
	{
		m_PlayPickupSound = atoi(pkvd->szValue) != 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "fall_mode"))
	{
		m_FallMode = std::clamp(static_cast<ItemFallMode>(atoi(pkvd->szValue)), ItemFallMode::Fall, ItemFallMode::Float);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "trigger_on_spawn"))
	{
		m_TriggerOnSpawn = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "trigger_on_despawn"))
	{
		m_TriggerOnDespawn = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseAnimating::KeyValue(pkvd);
}

void CBaseItem::SetupItem(const Vector& mins, const Vector& maxs)
{
	if (m_FallMode == ItemFallMode::Float)
	{
		pev->movetype = MOVETYPE_FLY;
	}
	else
	{
		pev->movetype = MOVETYPE_TOSS;
	}

	pev->solid = SOLID_TRIGGER;

	SetSize(mins, maxs);
	SetOrigin(pev->origin);

	SetTouch(&CBaseItem::ItemTouch);

	// Floating items immediately materialize
	if (m_FallMode == ItemFallMode::Float)
	{
		SetThink(&CBaseItem::Materialize);
	}
	else
	{
		SetThink(&CBaseItem::FallThink);
	}

	pev->nextthink = gpGlobals->time + 0.1;

	// All items are animated by default
	pev->framerate = 1;
}

void CBaseItem::Precache()
{
	if (auto modelName = GetModelName(); *modelName)
	{
		PrecacheModel(modelName);
	}

	PrecacheSound("items/weapondrop1.wav"); // item falls to the ground
	PrecacheSound("items/suitchargeok1.wav");
}

void CBaseItem::Spawn()
{
#ifndef CLIENT_DLL
	Precache();

	if(FStrEq(GetModelName(), ""))
	{
		Logger->warn("{}:{} has no model", GetClassname(), entindex());
	}

	SetModel(GetModelName());
	SetupItem({-16, -16, 0}, {16, 16, 16});
#endif
}

void CBaseItem::FallThink()
{
#ifndef CLIENT_DLL
	// Float mode never uses this method
	ASSERT(m_FallMode != ItemFallMode::Float);

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the item is waiting to respawn (if it's waiting, it is invisible!)
		if (!FNullEnt(GetOwner()))
		{
			EmitSoundDyn(CHAN_VOICE, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 29));
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize();
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1;
	}
#endif
}

void CBaseItem::ItemTouch(CBaseEntity* pOther)
{
#ifndef CLIENT_DLL
	// if it's not a player, ignore
	if (auto player = ToBasePlayer(pOther); player)
	{
		AddToPlayer(player);
	}
#endif
}

ItemAddResult CBaseItem::AddToPlayer(CBasePlayer* player)
{
#ifndef CLIENT_DLL
	// ok, a player is touching this item, but can he have it?
	if (!g_pGameRules->CanHaveItem(player, this))
	{
		return ItemAddResult::NotAdded;
	}

	// Try to add it.
	const ItemAddResult result = Apply(player);

	if (result == ItemAddResult::NotAdded)
	{
		return result;
	}

	// player grabbed the item.
	g_pGameRules->PlayerGotItem(player, this);

	if (g_pGameRules->ItemShouldRespawn(this))
	{
		Respawn();
	}
	else
	{
		// Consumables get removed. Inventory items reconfigure themselves.
		if (GetType() == ItemType::Consumable)
		{
			SetTouch(nullptr);
			// Make invisible immediately.
			pev->effects |= EF_NODRAW;
			UTIL_Remove(this);
		}
	}

	// Do this after doing the above so the respawned item exists and state changes have occurred.
	SUB_UseTargets(player, USE_TOGGLE, 0);

	if (!FStringNull(m_TriggerOnDespawn))
	{
		// Fire with USE_OFF so targets can be controlled more easily.
		FireTargets(STRING(m_TriggerOnDespawn), player, this, USE_OFF, 0);
	}

	return result;
#else
	return ItemAddResult::NotAdded;
#endif
}

CBaseItem* CBaseItem::GetItemToRespawn(const Vector& respawnPoint)
{
#ifndef CLIENT_DLL
	SetOrigin(respawnPoint); // move to wherever I'm supposed to respawn.
#endif

	return this;
}

CBaseItem* CBaseItem::Respawn()
{
#ifndef CLIENT_DLL
	if (auto newItem = GetItemToRespawn(g_pGameRules->ItemRespawnSpot(this)); newItem)
	{
		// not a typo! We want to know when the item the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the item that was taken.
		const float respawnTime = g_pGameRules->ItemRespawnTime(this);

		if (respawnTime > gpGlobals->time && !m_StayVisibleDuringRespawn)
		{
			newItem->pev->effects |= EF_NODRAW;
		}

		newItem->m_IsRespawning = true;

		newItem->SetTouch(nullptr);
		newItem->SetThink(&CBaseItem::AttemptToMaterialize);

		newItem->pev->nextthink = respawnTime;

		return newItem;
	}

	Logger->error("Item respawn failed to create {}!", GetClassname());
#endif

	return nullptr;
}

void CBaseItem::Materialize()
{
#ifndef CLIENT_DLL
	if (m_IsRespawning)
	{
		// changing from invisible state to visible.
		EmitSoundDyn(CHAN_WEAPON, "items/suitchargeok1.wav", VOL_NORM, ATTN_NORM, 0, 150);

		pev->effects &= ~EF_NODRAW;

		if (m_FlashOnRespawn)
		{
			pev->effects |= EF_MUZZLEFLASH;
		}

		m_IsRespawning = false;
	}

	pev->solid = SOLID_TRIGGER;
	SetOrigin(pev->origin); // link into world.
	SetTouch(&CBaseItem::ItemTouch);
	SetThink(nullptr);

	if (!FStringNull(m_TriggerOnSpawn))
	{
		FireTargets(STRING(m_TriggerOnSpawn), this, this, USE_ON, 0);
	}
#endif
}

void CBaseItem::AttemptToMaterialize()
{
#ifndef CLIENT_DLL
	const float time = g_pGameRules->ItemTryRespawn(this);

	if (time == 0)
	{
		switch (m_FallMode)
		{
		case ItemFallMode::Float:
			Materialize();
			break;

			// Fall first, then materialize (plays clatter sound)
		case ItemFallMode::Fall:
			pev->flags &= ~FL_ONGROUND;
			SetThink(&CBaseItem::FallThink);
			pev->nextthink = gpGlobals->time + 0.1;
			break;

		default: ASSERT(!"Invalid fall mode in CBaseItem::AttemptToMaterialize");
		}
		return;
	}

	pev->nextthink = time;
#endif
}
