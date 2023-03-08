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

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "cbase.h"
#include "GameLibrary.h"
#include "weapons.h"
#include "UserMessages.h"

//=========================================================
// MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a
// player can carry.
//=========================================================
int MaxAmmoCarry(string_t iszName)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (CBasePlayerItem::ItemInfoArray[i].pszAmmo1 && 0 == strcmp(STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo1))
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo1;
		if (CBasePlayerItem::ItemInfoArray[i].pszAmmo2 && 0 == strcmp(STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo2))
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo2;
	}

	CBaseEntity::Logger->error("MaxAmmoCarry() doesn't recognize '{}'!", STRING(iszName));
	return -1;
}


/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

//
// ClearMultiDamage - resets the global multi damage accumulator
//
void ClearMultiDamage()
{
	gMultiDamage.pEntity = nullptr;
	gMultiDamage.amount = 0;
	gMultiDamage.type = 0;
}


//
// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
//
// GLOBALS USED:
//		gMultiDamage

void ApplyMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker)
{
	Vector vecSpot1; // where blood comes from
	Vector vecDir;	 // direction blood should go
	TraceResult tr;

	if (!gMultiDamage.pEntity)
		return;

	gMultiDamage.pEntity->TakeDamage(inflictor, attacker, gMultiDamage.amount, gMultiDamage.type);
}


// GLOBALS USED:
//		gMultiDamage

void AddMultiDamage(CBaseEntity* inflictor, CBaseEntity* pEntity, float flDamage, int bitsDamageType)
{
	if (!pEntity)
		return;

	gMultiDamage.type |= bitsDamageType;

	if (pEntity != gMultiDamage.pEntity)
	{
		ApplyMultiDamage(inflictor, inflictor); // UNDONE: wrong attacker!
		gMultiDamage.pEntity = pEntity;
		gMultiDamage.amount = 0;
	}

	gMultiDamage.amount += flDamage;
}

/*
================
SpawnBlood
================
*/
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage)
{
	UTIL_BloodDrips(vecSpot, g_vecAttackDir, bloodColor, (int)flDamage);
}


int DamageDecal(CBaseEntity* pEntity, int bitsDamageType)
{
	if (!pEntity)
		return (DECAL_GUNSHOT1 + RANDOM_LONG(0, 4));

	return pEntity->DamageDecal(bitsDamageType);
}

void DecalGunshot(TraceResult* pTrace, int iBulletType)
{
	// Is the entity valid
	if (!UTIL_IsValidEntity(pTrace->pHit))
		return;

	if (VARS(pTrace->pHit)->solid == SOLID_BSP || VARS(pTrace->pHit)->movetype == MOVETYPE_PUSHSTEP)
	{
		CBaseEntity* pEntity = nullptr;
		// Decal the wall with a gunshot
		if (!FNullEnt(pTrace->pHit))
			pEntity = CBaseEntity::Instance(pTrace->pHit);

		switch (iBulletType)
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:
			// smoke and decal
			UTIL_GunshotDecalTrace(pTrace, DamageDecal(pEntity, DMG_BULLET));
			break;
		case BULLET_MONSTER_12MM:
			// smoke and decal
			UTIL_GunshotDecalTrace(pTrace, DamageDecal(pEntity, DMG_BULLET));
			break;
		case BULLET_PLAYER_CROWBAR:
			// wall decal
			UTIL_DecalTrace(pTrace, DamageDecal(pEntity, DMG_CLUB));
			break;
		}
	}
}



//
// EjectBrass - tosses a brass shell from passed origin at passed velocity
//
void EjectBrass(const Vector& vecOrigin, const Vector& vecVelocity, float rotation, int model, int soundtype)
{
	// FIX: when the player shoots, their gun isn't in the same position as it is on the model other players see.

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecOrigin);
	WRITE_BYTE(TE_MODEL);
	WRITE_COORD(vecOrigin.x);
	WRITE_COORD(vecOrigin.y);
	WRITE_COORD(vecOrigin.z);
	WRITE_COORD(vecVelocity.x);
	WRITE_COORD(vecVelocity.y);
	WRITE_COORD(vecVelocity.z);
	WRITE_ANGLE(rotation);
	WRITE_SHORT(model);
	WRITE_BYTE(soundtype);
	WRITE_BYTE(25); // 2.5 seconds
	MESSAGE_END();
}


#if 0
// UNDONE: This is no longer used?
void ExplodeModel(const Vector& vecOrigin, float speed, int model, int count)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecOrigin);
	WRITE_BYTE(TE_EXPLODEMODEL);
	WRITE_COORD(vecOrigin.x);
	WRITE_COORD(vecOrigin.y);
	WRITE_COORD(vecOrigin.z);
	WRITE_COORD(speed);
	WRITE_SHORT(model);
	WRITE_SHORT(count);
	WRITE_BYTE(15);// 1.5 seconds
	MESSAGE_END();
}
#endif

// Precaches the weapon and queues the weapon info for sending to clients
void UTIL_PrecacheOtherWeapon(const char* szClassname)
{
	auto entity = g_WeaponDictionary->Create(szClassname);
	if (FNullEnt(entity))
	{
		CBaseEntity::Logger->error("NULL Ent in UTIL_PrecacheOtherWeapon");
		return;
	}

	ItemInfo II;
	entity->Precache();
	memset(&II, 0, sizeof II);
	if (entity->GetItemInfo(&II))
	{
		CBasePlayerItem::ItemInfoArray[II.iId] = II;

		const char* weaponName = ((II.iFlags & ITEM_FLAG_EXHAUSTIBLE) != 0) ? STRING(entity->pev->classname) : nullptr;

		if (II.pszAmmo1 && '\0' != *II.pszAmmo1)
		{
			AddAmmoNameToAmmoRegistry(II.pszAmmo1, weaponName);
		}

		if (II.pszAmmo2 && '\0' != *II.pszAmmo2)
		{
			AddAmmoNameToAmmoRegistry(II.pszAmmo2, weaponName);
		}

		memset(&II, 0, sizeof II);
	}

	REMOVE_ENTITY(entity->edict());
}

// called by worldspawn
void W_Precache()
{
	g_GameLogger->trace("Precaching weapon assets");

	memset(CBasePlayerItem::ItemInfoArray, 0, sizeof(CBasePlayerItem::ItemInfoArray));
	memset(CBasePlayerItem::AmmoInfoArray, 0, sizeof(CBasePlayerItem::AmmoInfoArray));
	giAmmoIndex = 0;

	// custom items...

	// common world objects
	UTIL_PrecacheOther("item_suit");
	UTIL_PrecacheOther("item_battery");
	UTIL_PrecacheOther("item_antidote");
	UTIL_PrecacheOther("item_security");
	UTIL_PrecacheOther("item_longjump");

	// Precache weapons in a well-defined order so the client initializes its local data the same way as the server.
	// TODO: This is only necessary until weapon data is sent over the network.
	const auto classNames = g_WeaponDictionary->GetClassNames();

	std::vector<std::string_view> sortedClassNames{classNames.begin(), classNames.end()};

	std::ranges::sort(sortedClassNames);

	// This will also try to precache cycler_weapon but it doesn't return any item data so that's fine.
	for (const auto& className : sortedClassNames)
	{
		UTIL_PrecacheOtherWeapon(className.data());
	}

	UTIL_PrecacheOther("ammo_buckshot");
	UTIL_PrecacheOther("ammo_9mmclip");
	UTIL_PrecacheOther("ammo_9mmAR");
	UTIL_PrecacheOther("ammo_ARgrenades");
	UTIL_PrecacheOther("ammo_357");
	UTIL_PrecacheOther("ammo_gaussclip");
	UTIL_PrecacheOther("ammo_rpgclip");
	UTIL_PrecacheOther("ammo_crossbow");
	UTIL_PrecacheOther("ammo_556");
	UTIL_PrecacheOther("ammo_spore");
	UTIL_PrecacheOther("ammo_762");

	UTIL_PrecacheSound("weapons/spore_hit1.wav");
	UTIL_PrecacheSound("weapons/spore_hit2.wav");
	UTIL_PrecacheSound("weapons/spore_hit3.wav");

	if (g_pGameRules->IsDeathmatch())
	{
		UTIL_PrecacheOther("weaponbox"); // container for dropped deathmatch weapons
	}

	g_sModelIndexFireball = UTIL_PrecacheModel("sprites/zerogxplode.spr");	// fireball
	g_sModelIndexWExplosion = UTIL_PrecacheModel("sprites/WXplo1.spr");		// underwater fireball
	g_sModelIndexSmoke = UTIL_PrecacheModel("sprites/steam1.spr");			// smoke
	g_sModelIndexBubbles = UTIL_PrecacheModel("sprites/bubble.spr");		// bubbles
	g_sModelIndexBloodSpray = UTIL_PrecacheModel("sprites/bloodspray.spr"); // initial blood
	g_sModelIndexBloodDrop = UTIL_PrecacheModel("sprites/blood.spr");		// splattered blood

	g_sModelIndexLaser = UTIL_PrecacheModel(g_pModelNameLaser);
	g_sModelIndexLaserDot = UTIL_PrecacheModel("sprites/laserdot.spr");


	// used by explosions
	UTIL_PrecacheModel("models/grenade.mdl");
	UTIL_PrecacheModel("sprites/explode1.spr");

	UTIL_PrecacheSound("weapons/debris1.wav"); // explosion aftermaths
	UTIL_PrecacheSound("weapons/debris2.wav"); // explosion aftermaths
	UTIL_PrecacheSound("weapons/debris3.wav"); // explosion aftermaths

	UTIL_PrecacheSound("weapons/grenade_hit1.wav"); // grenade
	UTIL_PrecacheSound("weapons/grenade_hit2.wav"); // grenade
	UTIL_PrecacheSound("weapons/grenade_hit3.wav"); // grenade

	UTIL_PrecacheSound("weapons/bullet_hit1.wav"); // hit by bullet
	UTIL_PrecacheSound("weapons/bullet_hit2.wav"); // hit by bullet

	UTIL_PrecacheSound("items/weapondrop1.wav"); // weapon falls to the ground
}




TYPEDESCRIPTION CBasePlayerItem::m_SaveData[] =
	{
		DEFINE_FIELD(CBasePlayerItem, m_pPlayer, FIELD_CLASSPTR),
		DEFINE_FIELD(CBasePlayerItem, m_pNext, FIELD_CLASSPTR),
		// DEFINE_FIELD( CBasePlayerItem, m_fKnown, FIELD_INTEGER ),Reset to zero on load
		DEFINE_FIELD(CBasePlayerItem, m_iId, FIELD_INTEGER),
		// DEFINE_FIELD( CBasePlayerItem, m_iIdPrimary, FIELD_INTEGER ),
		// DEFINE_FIELD( CBasePlayerItem, m_iIdSecondary, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE(CBasePlayerItem, CBaseAnimating);


TYPEDESCRIPTION CBasePlayerWeapon::m_SaveData[] =
	{
#if defined(CLIENT_WEAPONS)
		DEFINE_FIELD(CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_FLOAT),
		DEFINE_FIELD(CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_FLOAT),
		DEFINE_FIELD(CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_FLOAT),
#else  // CLIENT_WEAPONS
		DEFINE_FIELD(CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME),
		DEFINE_FIELD(CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME),
		DEFINE_FIELD(CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME),
#endif // CLIENT_WEAPONS
		DEFINE_FIELD(CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayerWeapon, m_iClip, FIELD_INTEGER),
		DEFINE_FIELD(CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER),
		//	DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER )	 , reset to zero on load so hud gets updated correctly
		//  DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
		DEFINE_FIELD(CBasePlayerWeapon, m_WorldModel, FIELD_STRING),
		DEFINE_FIELD(CBasePlayerWeapon, m_ViewModel, FIELD_STRING),
		DEFINE_FIELD(CBasePlayerWeapon, m_PlayerModel, FIELD_STRING),
};

bool CBasePlayerWeapon::Save(CSave& save)
{
	if (!CBasePlayerItem::Save(save))
		return false;
	return save.WriteFields("CBasePlayerWeapon", this, m_SaveData, std::size(m_SaveData));
}
bool CBasePlayerWeapon::Restore(CRestore& restore)
{
	if (!CBasePlayerItem::Restore(restore))
		return false;
	if (!restore.ReadFields("CBasePlayerWeapon", this, m_SaveData, std::size(m_SaveData)))
	{
		return false;
	}

	// If we're part of the player's inventory and we're the active item, reset weapon strings.
	if (m_pPlayer && m_pPlayer->m_pActiveItem == this)
	{
		SetWeaponModels(STRING(m_ViewModel), STRING(m_PlayerModel));
	}

	return true;
}

void CBasePlayerItem::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-24, -24, 0);
	pev->absmax = pev->origin + Vector(24, 24, 16);
}


//=========================================================
// Sets up movetype, size, solidtype for a new weapon.
//=========================================================
void CBasePlayerItem::FallInit()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin(pev, pev->origin);
	SetSize(Vector(0, 0, 0), Vector(0, 0, 0)); // pointsize until it lands on the ground.

	SetTouch(&CBasePlayerItem::DefaultTouch);
	SetThink(&CBasePlayerItem::FallThink);

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// FallThink - Items that have just spawned run this think
// to catch them when they hit the ground. Once we're sure
// that the object is grounded, we change its solid type
// to trigger and set it in a large box that helps the
// player get it.
//=========================================================
void CBasePlayerItem::FallThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if (!FNullEnt(pev->owner))
		{
			int pitch = 95 + RANDOM_LONG(0, 29);
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "items/weapondrop1.wav", 1, ATTN_NORM, 0, pitch);
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize();
	}
}

//=========================================================
// Materialize - make a CBasePlayerItem visible and tangible
//=========================================================
void CBasePlayerItem::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin(pev, pev->origin); // link into world.
	SetTouch(&CBasePlayerItem::DefaultTouch);
	SetThink(nullptr);
}

//=========================================================
// AttemptToMaterialize - the item is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CBasePlayerItem::AttemptToMaterialize()
{
	float time = g_pGameRules->FlWeaponTryRespawn(this);

	if (time == 0)
	{
		Materialize();
		return;
	}

	pev->nextthink = gpGlobals->time + time;
}

//=========================================================
// CheckRespawn - a player is taking this weapon, should
// it respawn?
//=========================================================
void CBasePlayerItem::CheckRespawn()
{
	switch (g_pGameRules->WeaponShouldRespawn(this))
	{
	case GR_WEAPON_RESPAWN_YES:
		Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		return;
		break;
	}
}

//=========================================================
// Respawn- this item is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity* CBasePlayerItem::Respawn()
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity* pNewWeapon = CBaseEntity::Create(STRING(pev->classname), g_pGameRules->VecWeaponRespawnSpot(this), pev->angles, pev->owner);

	if (pNewWeapon)
	{
		pNewWeapon->pev->effects |= EF_NODRAW; // invisible for now
		pNewWeapon->SetTouch(nullptr);		   // no touch
		pNewWeapon->SetThink(&CBasePlayerItem::AttemptToMaterialize);

		pNewWeapon->pev->model = pev->model;

		DROP_TO_FLOOR(ENT(pev));

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime(this);
	}
	else
	{
		CBaseEntity::Logger->error("Respawn failed to create {}!", STRING(pev->classname));
	}

	return pNewWeapon;
}

void CBasePlayerItem::DefaultTouch(CBaseEntity* pOther)
{
	// if it's not a player, ignore
	if (!pOther->IsPlayer())
		return;

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	// can I have this?
	if (!g_pGameRules->CanHavePlayerItem(pPlayer, this))
	{
		if (gEvilImpulse101)
		{
			UTIL_Remove(this);
		}
		return;
	}

	if (pPlayer->AddPlayerItem(this))
	{
		AttachToPlayer(pPlayer);
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
	}

	SUB_UseTargets(pOther, USE_TOGGLE, 0); // UNDONE: when should this happen?
}

void CBasePlayerItem::DestroyItem()
{
	if (m_pPlayer)
	{
		// if attached to a player, remove.
		m_pPlayer->RemovePlayerItem(this);
	}

	Kill();
}

void CBasePlayerItem::AddToPlayer(CBasePlayer* pPlayer)
{
	m_pPlayer = pPlayer;
}

void CBasePlayerItem::Drop()
{
	SetTouch(nullptr);
	SetThink(&CBasePlayerItem::SUB_Remove);
	pev->nextthink = gpGlobals->time + .1;
}

void CBasePlayerItem::Kill()
{
	SetTouch(nullptr);
	SetThink(&CBasePlayerItem::SUB_Remove);
	pev->nextthink = gpGlobals->time + .1;
}

void CBasePlayerItem::Holster()
{
	m_pPlayer->pev->viewmodel = string_t::Null;
	m_pPlayer->pev->weaponmodel = string_t::Null;
}

void CBasePlayerItem::AttachToPlayer(CBasePlayer* pPlayer)
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW; // ??
	pev->modelindex = 0;	  // server won't send down to clients if modelindex == 0
	pev->model = string_t::Null;
	pev->owner = pPlayer->edict();
	pev->nextthink = gpGlobals->time + .1;
	SetTouch(nullptr);
}

// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
bool CBasePlayerWeapon::AddDuplicate(CBasePlayerItem* pOriginal)
{
	if (0 != m_iDefaultAmmo)
	{
		return ExtractAmmo((CBasePlayerWeapon*)pOriginal);
	}
	else
	{
		// a dead player dropped this.
		return ExtractClipAmmo((CBasePlayerWeapon*)pOriginal);
	}
}


void CBasePlayerWeapon::AddToPlayer(CBasePlayer* pPlayer)
{
	/*
	if ((iFlags() & ITEM_FLAG_EXHAUSTIBLE) != 0 && m_iDefaultAmmo == 0 && m_iClip <= 0)
	{
		//This is an exhaustible weapon that has no ammo left. Don't add it, queue it up for destruction instead.
		SetThink(&CSatchel::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
		return false;
	}
	*/

	CBasePlayerItem::AddToPlayer(pPlayer);

	pPlayer->SetWeaponBit(m_iId);

	if (0 == m_iPrimaryAmmoType)
	{
		m_iPrimaryAmmoType = pPlayer->GetAmmoIndex(pszAmmo1());
		m_iSecondaryAmmoType = pPlayer->GetAmmoIndex(pszAmmo2());
	}
}

bool CBasePlayerWeapon::UpdateClientData(CBasePlayer* pPlayer)
{
	bool bSend = false;
	int state = 0;
	if (pPlayer->m_pActiveItem == this)
	{
		if (pPlayer->m_fOnTarget)
			state = WEAPON_IS_ONTARGET;
		else
			state = 1;
	}

	// Forcing send of all data!
	if (!pPlayer->m_fWeapon)
	{
		bSend = true;
	}

	// This is the current or last weapon, so the state will need to be updated
	if (this == pPlayer->m_pActiveItem ||
		this == pPlayer->m_pClientActiveItem)
	{
		if (pPlayer->m_pActiveItem != pPlayer->m_pClientActiveItem)
		{
			bSend = true;
		}
	}

	// If the ammo, state, or fov has changed, update the weapon
	if (m_iClip != m_iClientClip ||
		state != m_iClientWeaponState ||
		pPlayer->m_iFOV != pPlayer->m_iClientFOV)
	{
		bSend = true;
	}

	if (bSend)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, nullptr, pPlayer->pev);
		WRITE_BYTE(state);
		WRITE_BYTE(m_iId);
		WRITE_BYTE(m_iClip);
		MESSAGE_END();

		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
		pPlayer->m_fWeapon = true;
	}

	if (m_pNext)
		m_pNext->UpdateClientData(pPlayer);

	return true;
}


void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int body)
{
	const bool skiplocal = !m_ForceSendAnimations && UseDecrement();

	m_pPlayer->pev->weaponanim = iAnim;

#if defined(CLIENT_WEAPONS)
	if (skiplocal && ENGINE_CANSKIP(m_pPlayer->edict()))
		return;
#endif

	MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, nullptr, m_pPlayer->pev);
	WRITE_BYTE(iAnim);	   // sequence number
	WRITE_BYTE(pev->body); // weaponmodel bodygroup.
	MESSAGE_END();
}

bool CBasePlayerWeapon::AddPrimaryAmmo(int iCount, const char* szName, int iMaxClip, int iMaxCarry)
{
	int iIdAmmo;

	// Don't double for single shot weapons (e.g. RPG)
	if ((m_pPlayer->m_iItems & CTFItem::Backpack) != 0 && iMaxClip > 1)
	{
		iMaxClip *= 2;
	}

	if (iMaxClip < 1)
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName, iMaxCarry);
	}
	else if (m_iClip == 0)
	{
		int i;
		i = V_min(m_iClip + iCount, iMaxClip) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo(iCount - i, szName, iMaxCarry);
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName, iMaxCarry);
	}

	// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxCarry; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iPrimaryAmmoType = iIdAmmo;
		if (m_pPlayer->HasPlayerItem(this))
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
	}

	return iIdAmmo > 0 ? true : false;
}


bool CBasePlayerWeapon::AddSecondaryAmmo(int iCount, const char* szName, int iMax)
{
	int iIdAmmo;

	iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName, iMax);

	// m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] = iMax; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iSecondaryAmmoType = iIdAmmo;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	}
	return iIdAmmo > 0 ? true : false;
}

//=========================================================
// IsUseable - this function determines whether or not a
// weapon is useable by the player in its current state.
// (does it have ammo loaded? do I have any ammo for the
// weapon?, etc)
//=========================================================
bool CBasePlayerWeapon::IsUseable()
{
	if (m_iClip > 0)
	{
		return true;
	}

	// Player has unlimited ammo for this weapon or does not use magazines
	if (iMaxAmmo1() == WEAPON_NOCLIP)
	{
		return true;
	}

	if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0)
	{
		return true;
	}

	if (pszAmmo2())
	{
		// Player has unlimited ammo for this weapon or does not use magazines
		if (iMaxAmmo2() == WEAPON_NOCLIP)
		{
			return true;
		}

		if (m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] > 0)
		{
			return true;
		}
	}

	// clip is empty (or nonexistant) and the player has no more ammo of this type.
	return false;
}

void CBasePlayerWeapon::SetWeaponModels(const char* viewModel, const char* weaponModel)
{
	// These are stored off to restore the models on save game load, so they must not use the replaced name.
	m_ViewModel = MAKE_STRING(viewModel);
	m_PlayerModel = MAKE_STRING(weaponModel);

	m_pPlayer->pev->viewmodel = MAKE_STRING(UTIL_CheckForGlobalModelReplacement(viewModel));
	m_pPlayer->pev->weaponmodel = MAKE_STRING(UTIL_CheckForGlobalModelReplacement(weaponModel));
}

bool CBasePlayerWeapon::DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body)
{
	if (!CanDeploy())
		return false;

	m_pPlayer->TabulateAmmo();
	SetWeaponModels(szViewModel, szWeaponModel);
	strcpy(m_pPlayer->m_szAnimExtention, szAnimExt);
	SendWeaponAnim(iAnim, body);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_flLastFireTime = 0.0;

	return true;
}

bool CBasePlayerWeapon::PlayEmptySound()
{
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND_PREDICTED(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM, 0, PITCH_NORM);
		m_iPlayEmptySound = false;
		return false;
	}
	return false;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::PrimaryAmmoIndex()
{
	return m_iPrimaryAmmoType;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::SecondaryAmmoIndex()
{
	return m_iSecondaryAmmoType;
}

void CBasePlayerWeapon::Holster()
{
	m_fInReload = false; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = string_t::Null;
	m_pPlayer->pev->weaponmodel = string_t::Null;
}

void CBasePlayerAmmo::Precache()
{
	PrecacheModel(STRING(pev->model));
}

void CBasePlayerAmmo::Spawn()
{
	Precache();

	SetModel(STRING(pev->model));
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	SetSize(Vector(-16, -16, 0), Vector(16, 16, 16));
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CBasePlayerAmmo::DefaultTouch);
}

CBaseEntity* CBasePlayerAmmo::Respawn()
{
	pev->effects |= EF_NODRAW;
	SetTouch(nullptr);

	UTIL_SetOrigin(pev, g_pGameRules->VecAmmoRespawnSpot(this)); // move to wherever I'm supposed to repawn.

	SetThink(&CBasePlayerAmmo::Materialize);
	pev->nextthink = g_pGameRules->FlAmmoRespawnTime(this);

	return this;
}

void CBasePlayerAmmo::Materialize()
{
	if ((pev->effects & EF_NODRAW) != 0)
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch(&CBasePlayerAmmo::DefaultTouch);
}

void CBasePlayerAmmo::DefaultTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
	{
		return;
	}

	auto player = static_cast<CBasePlayer*>(pOther);

	if (AddAmmo(player))
	{
		if (g_pGameRules->AmmoShouldRespawn(this) == GR_AMMO_RESPAWN_YES)
		{
			Respawn();
		}
		else
		{
			SetTouch(nullptr);
			SetThink(&CBasePlayerAmmo::SUB_Remove);
			pev->nextthink = gpGlobals->time + .1;
		}
	}
	else if (gEvilImpulse101)
	{
		// evil impulse 101 hack, kill always
		SetTouch(nullptr);
		SetThink(&CBasePlayerAmmo::SUB_Remove);
		pev->nextthink = gpGlobals->time + .1;
	}
}

//=========================================================
// called by the new item with the existing item as parameter
//
// if we call ExtractAmmo(), it's because the player is picking up this type of weapon for
// the first time. If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
// if  this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo, which means only the ammo in
// the weapon clip comes along.
//=========================================================
bool CBasePlayerWeapon::ExtractAmmo(CBasePlayerWeapon* pWeapon)
{
	bool iReturn = false;

	if (pszAmmo1() != nullptr)
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want.
		iReturn = pWeapon->AddPrimaryAmmo(m_iDefaultAmmo, pszAmmo1(), iMaxClip(), iMaxAmmo1());
		m_iDefaultAmmo = 0;
	}

	if (pszAmmo2() != nullptr)
	{
		iReturn |= pWeapon->AddSecondaryAmmo(0, pszAmmo2(), iMaxAmmo2());
	}

	return iReturn;
}

//=========================================================
// called by the new item's class with the existing item as parameter
//=========================================================
bool CBasePlayerWeapon::ExtractClipAmmo(CBasePlayerWeapon* pWeapon)
{
	int iAmmo;

	if (m_iClip == WEAPON_NOCLIP)
	{
		iAmmo = 0; // guns with no clips always come empty if they are second-hand
	}
	else
	{
		iAmmo = m_iClip;
	}

	// TODO: should handle -1 return as well (only return true if ammo was taken)
	return pWeapon->m_pPlayer->GiveAmmo(iAmmo, pszAmmo1(), iMaxAmmo1()) != 0; // , &m_iPrimaryAmmoType
}

//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
void CBasePlayerWeapon::RetireWeapon()
{
	SetThink(&CBasePlayerWeapon::CallDoRetireWeapon);
	pev->nextthink = gpGlobals->time + 0.01f;
}

void CBasePlayerWeapon::DoRetireWeapon()
{
	if (!m_pPlayer || m_pPlayer->m_pActiveItem != this)
	{
		// Already retired?
		return;
	}

	// first, no viewmodel at all.
	m_pPlayer->pev->viewmodel = string_t::Null;
	m_pPlayer->pev->weaponmodel = string_t::Null;
	// m_pPlayer->pev->viewmodelindex = 0;

	g_pGameRules->GetNextBestWeapon(m_pPlayer, this);

	// If we're still equipped and we couldn't switch to another weapon, dequip this one
	if (CanHolster() && m_pPlayer->m_pActiveItem == this)
	{
		m_pPlayer->SwitchWeapon(nullptr);
	}
}

//=========================================================================
// GetNextAttackDelay - An accurate way of calcualting the next attack time.
//=========================================================================
float CBasePlayerWeapon::GetNextAttackDelay(float delay)
{
	if (m_flLastFireTime == 0 || m_flNextPrimaryAttack <= -1.1)
	{
		// At this point, we are assuming that the client has stopped firing
		// and we are going to reset our book keeping variables.
		m_flLastFireTime = gpGlobals->time;
		m_flPrevPrimaryAttack = delay;
	}
	// calculate the time between this shot and the previous
	float flTimeBetweenFires = gpGlobals->time - m_flLastFireTime;
	float flCreep = 0.0f;
	if (flTimeBetweenFires > 0)
		flCreep = flTimeBetweenFires - m_flPrevPrimaryAttack; // postive or negative

	// save the last fire time
	m_flLastFireTime = gpGlobals->time;

	float flNextAttack = UTIL_WeaponTimeBase() + delay - flCreep;
	// we need to remember what the m_flNextPrimaryAttack time is set to for each shot,
	// store it as m_flPrevPrimaryAttack.
	m_flPrevPrimaryAttack = flNextAttack - UTIL_WeaponTimeBase();
	// 	char szMsg[256];
	// 	snprintf( szMsg, sizeof(szMsg), "next attack time: %0.4f\n", gpGlobals->time + flNextAttack );
	// 	OutputDebugString( szMsg );
	return flNextAttack;
}


//*********************************************************
// weaponbox code:
//*********************************************************

LINK_ENTITY_TO_CLASS(weaponbox, CWeaponBox);

TYPEDESCRIPTION CWeaponBox::m_SaveData[] =
	{
		DEFINE_ARRAY(CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
		DEFINE_ARRAY(CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_SLOTS),
		DEFINE_ARRAY(CWeaponBox, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES),
		DEFINE_FIELD(CWeaponBox, m_cAmmoTypes, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CWeaponBox, CBaseEntity);

void CWeaponBox::OnCreate()
{
	CBaseEntity::OnCreate();

	pev->model = MAKE_STRING("models/w_weaponbox.mdl");
}

//=========================================================
//
//=========================================================
void CWeaponBox::Precache()
{
	PrecacheModel(STRING(pev->model));
}

//=========================================================
//=========================================================
bool CWeaponBox::KeyValue(KeyValueData* pkvd)
{
	if (m_cAmmoTypes < MAX_AMMO_SLOTS)
	{
		PackAmmo(ALLOC_STRING(pkvd->szKeyName), atoi(pkvd->szValue));
		m_cAmmoTypes++; // count this new ammo type.

		return true;
	}
	else
	{
		CBaseEntity::Logger->error("WeaponBox too full! only {} ammotypes allowed", MAX_AMMO_SLOTS);
	}

	return false;
}

//=========================================================
// CWeaponBox - Spawn
//=========================================================
void CWeaponBox::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	SetSize(g_vecZero, g_vecZero);

	SetModel(STRING(pev->model));
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
void CWeaponBox::Kill()
{
	CBasePlayerItem* pWeapon;
	int i;

	// destroy the weapons
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		pWeapon = m_rgpPlayerItems[i];

		while (pWeapon)
		{
			pWeapon->SetThink(&CBasePlayerItem::SUB_Remove);
			pWeapon->pev->nextthink = gpGlobals->time + 0.1;
			pWeapon = pWeapon->m_pNext;
		}
	}

	// remove the box
	UTIL_Remove(this);
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================
void CWeaponBox::Touch(CBaseEntity* pOther)
{
	if ((pev->flags & FL_ONGROUND) == 0)
	{
		return;
	}

	if (!pOther->IsPlayer())
	{
		// only players may touch a weaponbox.
		return;
	}

	if (!pOther->IsAlive())
	{
		// no dead guys.
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;
	int i;

	// dole out ammo
	for (i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (!FStringNull(m_rgiszAmmo[i]))
		{
			// there's some ammo of this type.
			pPlayer->GiveAmmo(m_rgAmmo[i], STRING(m_rgiszAmmo[i]), MaxAmmoCarry(m_rgiszAmmo[i]));

			// Logger->trace("Gave {} rounds of {}", m_rgAmmo[i], STRING(m_rgiszAmmo[i]));

			// now empty the ammo from the weaponbox since we just gave it to the player
			m_rgiszAmmo[i] = string_t::Null;
			m_rgAmmo[i] = 0;
		}
	}

	// go through my weapons and try to give the usable ones to the player.
	// it's important the the player be given ammo first, so the weapons code doesn't refuse
	// to deploy a better weapon that the player may pick up because he has no ammo for it.
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			CBasePlayerItem* pItem;

			// have at least one weapon in this slot
			while (m_rgpPlayerItems[i])
			{
				// Logger->debug("trying to give {}", STRING(m_rgpPlayerItems[i]->pev->classname));

				pItem = m_rgpPlayerItems[i];
				m_rgpPlayerItems[i] = m_rgpPlayerItems[i]->m_pNext; // unlink this weapon from the box

				if (pPlayer->AddPlayerItem(pItem))
				{
					pItem->AttachToPlayer(pPlayer);
				}
			}
		}
	}

	EMIT_SOUND(pOther->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
	SetTouch(nullptr);
	UTIL_Remove(this);
}

//=========================================================
// CWeaponBox - PackWeapon: Add this weapon to the box
//=========================================================
bool CWeaponBox::PackWeapon(CBasePlayerItem* pWeapon)
{
	// is one of these weapons already packed in this box?
	if (HasWeapon(pWeapon))
	{
		return false; // box can only hold one of each weapon type
	}

	if (pWeapon->m_pPlayer)
	{
		if (!pWeapon->m_pPlayer->RemovePlayerItem(pWeapon))
		{
			// failed to unhook the weapon from the player!
			return false;
		}
	}

	int iWeaponSlot = pWeapon->iItemSlot();

	if (m_rgpPlayerItems[iWeaponSlot])
	{
		// there's already one weapon in this slot, so link this into the slot's column
		pWeapon->m_pNext = m_rgpPlayerItems[iWeaponSlot];
		m_rgpPlayerItems[iWeaponSlot] = pWeapon;
	}
	else
	{
		// first weapon we have for this slot
		m_rgpPlayerItems[iWeaponSlot] = pWeapon;
		pWeapon->m_pNext = nullptr;
	}

	pWeapon->pev->spawnflags |= SF_NORESPAWN; // never respawn
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = string_t::Null;
	pWeapon->pev->owner = edict();
	pWeapon->SetThink(nullptr); // crowbar may be trying to swing again, etc.
	pWeapon->SetTouch(nullptr);
	pWeapon->m_pPlayer = nullptr;

	// Logger->debug("packed {}", STRING(pWeapon->pev->classname));

	return true;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
bool CWeaponBox::PackAmmo(string_t iszName, int iCount)
{
	int iMaxCarry;

	if (FStringNull(iszName))
	{
		// error here
		Logger->error("NULL String in PackAmmo!");
		return false;
	}

	iMaxCarry = MaxAmmoCarry(iszName);

	if (iMaxCarry != -1 && iCount > 0)
	{
		// Logger->debug("Packed {} rounds of {}", iCount, STRING(iszName));
		GiveAmmo(iCount, STRING(iszName), iMaxCarry);
		return true;
	}

	return false;
}

//=========================================================
// CWeaponBox - GiveAmmo
//=========================================================
int CWeaponBox::GiveAmmo(int iCount, const char* szName, int iMax, int* pIndex /* = nullptr*/)
{
	int i;

	for (i = 1; i < MAX_AMMO_SLOTS && !FStringNull(m_rgiszAmmo[i]); i++)
	{
		if (stricmp(szName, STRING(m_rgiszAmmo[i])) == 0)
		{
			if (pIndex)
				*pIndex = i;

			int iAdd = V_min(iCount, iMax - m_rgAmmo[i]);
			if (iCount == 0 || iAdd > 0)
			{
				m_rgAmmo[i] += iAdd;

				return i;
			}
			return -1;
		}
	}
	if (i < MAX_AMMO_SLOTS)
	{
		if (pIndex)
			*pIndex = i;

		m_rgiszAmmo[i] = MAKE_STRING(szName);
		m_rgAmmo[i] = iCount;

		return i;
	}
	CBaseEntity::Logger->error("out of named ammo slots");
	return i;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
bool CWeaponBox::HasWeapon(CBasePlayerItem* pCheckItem)
{
	CBasePlayerItem* pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs(pItem->pev, STRING(pCheckItem->pev->classname)))
		{
			return true;
		}
		pItem = pItem->m_pNext;
	}

	return false;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
bool CWeaponBox::IsEmpty()
{
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			return false;
		}
	}

	for (i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (!FStringNull(m_rgiszAmmo[i]))
		{
			// still have a bit of this type of ammo
			return false;
		}
	}

	return true;
}

//=========================================================
//=========================================================
void CWeaponBox::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 16);
}


void CBasePlayerWeapon::PrintState()
{
	Logger->debug("primary:  {}", m_flNextPrimaryAttack);
	Logger->debug("idle   :  {}", m_flTimeWeaponIdle);

	// Logger->debug("nextrl :  {}", m_flNextReload);
	// Logger->debug("nextpum:  {}", m_flPumpTime);

	// Logger->debug("m_frt  :  {}", m_fReloadTime);
	Logger->debug("m_finre:  {}", static_cast<int>(m_fInReload));
	// Logger->debug("m_finsr:  {}", m_fInSpecialReload);

	Logger->debug("m_iclip:  {}", m_iClip);
}
