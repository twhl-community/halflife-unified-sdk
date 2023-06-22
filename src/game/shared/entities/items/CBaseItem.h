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

#pragma once

#include "CBaseAnimating.h"

class CBasePlayerAmmo;
class CBasePlayerWeapon;
class CItem;

enum class ItemType
{
	InventoryItem = 0, //!< This item will be added to the player's inventory.
	Consumable		   //!< This item will be consumed on use.
};

enum class ItemFallMode
{
	Fall = 0, //!< MOVETYPE_TOSS with fall-to-ground logic
	Float,	  //!< MOVETYPE_FLY with no falling logic
};

enum class ItemAddResult
{
	NotAdded,
	Added,
	AddedAsDuplicate //!< Ammo extracted and marked for removal.
};

constexpr float ITEM_DEFAULT_RESPAWN_DELAY = -2;
constexpr float ITEM_NEVER_RESPAWN_DELAY = -1;

/**
 *	@brief Interface for item visitors.
 */
class IItemVisitor
{
public:
	virtual ~IItemVisitor() = default;

	virtual void Visit(CBasePlayerAmmo* ammo) = 0;
	virtual void Visit(CBasePlayerWeapon* weapon) = 0;
	virtual void Visit(CItem* pickupItem) = 0;
};

/**
 *	@brief Base class for all items.
 *	@details Handles item setup in the world, pickup on touch, respawning.
 */
class CBaseItem : public CBaseAnimating
{
	DECLARE_CLASS(CBaseItem, CBaseAnimating);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Precache() override;
	void Spawn() override;

	CBaseItem* MyItemPointer() override { return this; }

	virtual ItemType GetType() const = 0;

	/**
	 *	@brief Visits this item through the given visitor.
	 */
	virtual void Accept(IItemVisitor& visitor) = 0;

	/**
	 *	@brief Items that have just spawned run this think to catch them when they hit the ground.
	 *	Once we're sure that the object is grounded,
	 *	we change its solid type to trigger and set it in a large box that helps the player get it.
	 */
	void FallThink();

	void ItemTouch(CBaseEntity* pOther);

	/**
	 *	@brief Tries to given this item to the player.
	 */
	ItemAddResult AddToPlayer(CBasePlayer* player);

	/**
	 *	@brief make an item visible and tangible.
	 */
	void Materialize();

	/**
	 *	@brief the item is trying to rematerialize, should it do so now or wait longer?
	 *	the item desires to become visible and tangible, if the game rules allow for it.
	 */
	void AttemptToMaterialize();

protected:
	void SetupItem(const Vector& mins, const Vector& maxs);

	virtual ItemAddResult Apply(CBasePlayer* player) = 0;

	/**
	 *	@brief Returns the item that should be respawned after this one has been picked up.
	 *	Can be this entity if it does not need to be cloned.
	 */
	virtual CBaseItem* GetItemToRespawn(const Vector& respawnPoint);

private:
	CBaseItem* Respawn();

public:
	float m_RespawnDelay = ITEM_DEFAULT_RESPAWN_DELAY;

protected:
	ItemFallMode m_FallMode = ItemFallMode::Fall;

	bool m_StayVisibleDuringRespawn = false;

	bool m_IsRespawning = false;

	bool m_FlashOnRespawn = true;

	bool m_PlayPickupSound = true;

	/**
	 *	@brief Target to trigger when this entity spawns/respawns
	 */
	string_t m_TriggerOnSpawn;

	/**
	 *	@brief Target to trigger when this entity despawns (waiting to respawn, being removed)
	 */
	string_t m_TriggerOnDespawn;
};
