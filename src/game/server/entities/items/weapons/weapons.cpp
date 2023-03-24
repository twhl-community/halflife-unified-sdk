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
/**
*	@file
*	functions governing the selection/use of weapons for players
*/

#include "cbase.h"
#include "AmmoTypeSystem.h"
#include "GameLibrary.h"
#include "weapons.h"
#include "UserMessages.h"

void ClearMultiDamage()
{
	gMultiDamage.pEntity = nullptr;
	gMultiDamage.amount = 0;
	gMultiDamage.type = 0;
}

void ApplyMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker)
{
	if (!gMultiDamage.pEntity)
		return;

	gMultiDamage.pEntity->TakeDamage(inflictor, attacker, gMultiDamage.amount, gMultiDamage.type);
}

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

/**
*	@brief Precaches the weapon and queues the weapon info for sending to clients
*/
void UTIL_PrecacheOtherWeapon(const char* szClassname)
{
	auto entity = g_WeaponDictionary->Create(szClassname);
	if (FNullEnt(entity))
	{
		CBaseEntity::Logger->error("NULL Ent in UTIL_PrecacheOtherWeapon");
		return;
	}

	entity->Precache();

	if (WeaponInfo info; entity->GetWeaponInfo(info))
	{
		g_WeaponData.Register(std::move(info));
	}

	REMOVE_ENTITY(entity->edict());
}

// called by worldspawn
void W_Precache()
{
	g_GameLogger->trace("Precaching weapon assets");

	g_WeaponData.Clear();

	Weapons_RegisterAmmoTypes();

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

TYPEDESCRIPTION CBasePlayerWeapon::m_SaveData[] =
	{
		DEFINE_FIELD(CBasePlayerWeapon, m_pPlayer, FIELD_CLASSPTR),
		DEFINE_FIELD(CBasePlayerWeapon, m_pNext, FIELD_CLASSPTR),
		// DEFINE_FIELD( CBasePlayerWeapon, m_fKnown, FIELD_INTEGER ),Reset to zero on load
		DEFINE_FIELD(CBasePlayerWeapon, m_iId, FIELD_INTEGER),
		// DEFINE_FIELD( CBasePlayerWeapon, m_iIdPrimary, FIELD_INTEGER ),
		// DEFINE_FIELD( CBasePlayerWeapon, m_iIdSecondary, FIELD_INTEGER ),
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
		DEFINE_FIELD(CBasePlayerWeapon, m_iDefaultPrimaryAmmo, FIELD_INTEGER),
		//	DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER )	 , reset to zero on load so hud gets updated correctly
		//  DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
		DEFINE_FIELD(CBasePlayerWeapon, m_WorldModel, FIELD_STRING),
		DEFINE_FIELD(CBasePlayerWeapon, m_ViewModel, FIELD_STRING),
		DEFINE_FIELD(CBasePlayerWeapon, m_PlayerModel, FIELD_STRING),
};

bool CBasePlayerWeapon::Save(CSave& save)
{
	if (!CBaseItem::Save(save))
		return false;
	return save.WriteFields("CBasePlayerWeapon", this, m_SaveData, std::size(m_SaveData));
}
bool CBasePlayerWeapon::Restore(CRestore& restore)
{
	if (!CBaseItem::Restore(restore))
		return false;
	if (!restore.ReadFields("CBasePlayerWeapon", this, m_SaveData, std::size(m_SaveData)))
	{
		return false;
	}

	// If we're part of the player's inventory and we're the active item, reset weapon strings.
	if (m_pPlayer && m_pPlayer->m_pActiveWeapon == this)
	{
		SetWeaponModels(STRING(m_ViewModel), STRING(m_PlayerModel));
	}

	return true;
}

bool CBasePlayerWeapon::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "default_ammo"))
	{
		m_iDefaultAmmo = std::max(RefillAllAmmoAmount, atoi(pkvd->szValue));
		return true;
	}

	return CBaseItem::KeyValue(pkvd);
}

void CBasePlayerWeapon::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-24, -24, 0);
	pev->absmax = pev->origin + Vector(24, 24, 16);
}

CBasePlayerWeapon* CBasePlayerWeapon::GetItemToRespawn(const Vector& respawnPoint)
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function).
	// The item spawn/respawn code will decide when to make the weapon visible and touchable.
	// Don't pass the current owner since the new weapon isn't owned by that entity
	auto newWeapon = static_cast<CBasePlayerWeapon*>(CBaseEntity::Create(GetClassname(), respawnPoint, pev->angles, nullptr, false));

	if (!newWeapon)
	{
		CBaseEntity::Logger->error("Respawn failed to create {}!", STRING(pev->classname));
		return nullptr;
	}

	// Copy over item settings
	newWeapon->pev->targetname = pev->targetname;
	newWeapon->pev->target = pev->target;
	newWeapon->m_flDelay = m_flDelay;
	newWeapon->pev->model = m_WorldModel;
	newWeapon->pev->sequence = pev->sequence;
	newWeapon->pev->avelocity = pev->avelocity;

	newWeapon->m_ModelReplacementFileName = m_ModelReplacementFileName;
	newWeapon->m_SoundReplacementFileName = m_SoundReplacementFileName;
	newWeapon->m_SentenceReplacementFileName = m_SentenceReplacementFileName;

	newWeapon->m_ModelReplacement = m_ModelReplacement;
	newWeapon->m_SoundReplacement = m_SoundReplacement;
	newWeapon->m_SentenceReplacement = m_SentenceReplacement;

	newWeapon->m_RespawnMode = m_RespawnMode;
	newWeapon->m_RespawnDelay = m_RespawnDelay;

	newWeapon->m_FallMode = m_FallMode;
	newWeapon->m_StayVisibleDuringRespawn = m_StayVisibleDuringRespawn;
	newWeapon->m_FlashOnRespawn = m_FlashOnRespawn;
	newWeapon->m_PlayPickupSound = m_PlayPickupSound;

	newWeapon->m_TriggerOnSpawn = m_TriggerOnSpawn;
	newWeapon->m_TriggerOnDespawn = m_TriggerOnDespawn;

	newWeapon->m_iDefaultPrimaryAmmo = newWeapon->m_iDefaultAmmo = m_iDefaultPrimaryAmmo;
	newWeapon->m_WorldModel = m_WorldModel;
	newWeapon->m_ViewModel = m_ViewModel;
	newWeapon->m_PlayerModel = m_PlayerModel;

	DispatchSpawn(newWeapon->edict());

	// Don't allow this weapon to be targeted from now on.
	pev->targetname = string_t::Null;

	// This weapon has been picked up, so from now own it should play pickup sounds (when dropped and picked up again).
	m_PlayPickupSound = false;

	return newWeapon;
}

ItemAddResult CBasePlayerWeapon::Apply(CBasePlayer* player)
{
	const ItemAddResult result = player->AddPlayerWeapon(this);

	if (result == ItemAddResult::Added)
	{
		AttachToPlayer(player);

		if (m_PlayPickupSound)
		{
			player->EmitSound(CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
		}
	}

	return result;
}

void CBasePlayerWeapon::DestroyItem()
{
	if (m_pPlayer)
	{
		// if attached to a player, remove.
		m_pPlayer->RemovePlayerWeapon(this);
	}

	Kill();
}

void CBasePlayerWeapon::Kill()
{
	SetTouch(nullptr);
	SetThink(&CBasePlayerWeapon::SUB_Remove);
	pev->nextthink = gpGlobals->time + .1;
}

void CBasePlayerWeapon::AttachToPlayer(CBasePlayer* pPlayer)
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

bool CBasePlayerWeapon::AddDuplicate(CBasePlayerWeapon* original)
{
	if (0 != m_iDefaultAmmo)
	{
		return ExtractAmmo(original);
	}
	else
	{
		// a dead player dropped this.
		return ExtractClipAmmo(original);
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

	m_pPlayer = pPlayer;

	pPlayer->SetWeaponBit(m_iId);
}

bool CBasePlayerWeapon::UpdateClientData(CBasePlayer* pPlayer)
{
	bool bSend = false;
	int state = 0;
	if (pPlayer->m_pActiveWeapon == this)
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
	if (this == pPlayer->m_pActiveWeapon ||
		this == pPlayer->m_pClientActiveWeapon)
	{
		if (pPlayer->m_pActiveWeapon != pPlayer->m_pClientActiveWeapon)
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

bool CBasePlayerWeapon::AddPrimaryAmmo(CBasePlayerWeapon* origin, int iCount, const char* szName, int iMaxClip)
{
	auto type = g_AmmoTypes.GetByName(szName);

	if (!type)
	{
		assert(!"Unknown ammo type");
		CBasePlayerWeapon::WeaponsLogger->error("Trying to add unknown ammo type \"{}\"", szName);
		return false;
	}

	// Don't double for single shot weapons (e.g. RPG)
	if ((m_pPlayer->m_iItems & CTFItem::Backpack) != 0 && iMaxClip > 1)
	{
		iMaxClip *= 2;
	}

	int iIdAmmo;

	if (iMaxClip <= 0)
	{
		m_iClip = WEAPON_NOCLIP;
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName);
	}
	else if (m_iClip == 0)
	{
		if (iCount == RefillAllAmmoAmount)
		{
			// Full magazine + spare.
			iCount = type->MaximumCapacity + iMaxClip;
		}

		m_iClip = V_min(iCount, iMaxClip);
		iIdAmmo = m_pPlayer->GiveAmmo(iCount - m_iClip, szName);

		// Make sure we count this as ammo taken.
		if (iCount > 0 && iIdAmmo == -1)
		{
			iIdAmmo = type->Id;
		}
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName);
	}

	// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxCarry; // hack for testing

	if (iIdAmmo > 0)
	{
		if (this != origin)
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EmitSound(CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
	}

	return iIdAmmo > 0;
}

bool CBasePlayerWeapon::AddSecondaryAmmo(int iCount, const char* szName)
{
	const int iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName);

	// m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] = iMax; // hack for testing

	if (iIdAmmo > 0)
	{
		EmitSound(CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	}
	return iIdAmmo > 0;
}

bool CBasePlayerWeapon::IsUseable()
{
	if (m_iClip > 0)
	{
		return true;
	}

	// Weapon doesn't use ammo.
	if (m_iPrimaryAmmoType == -1)
	{
		return true;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		return true;
	}

	if (m_iSecondaryAmmoType != -1)
	{
		if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] > 0)
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

bool CBasePlayerWeapon::ExtractAmmo(CBasePlayerWeapon* weapon)
{
	bool extractedAmmo = false;

	if (pszAmmo1() != nullptr)
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want.
		extractedAmmo = weapon->AddPrimaryAmmo(this, m_iDefaultAmmo, pszAmmo1(), iMaxClip());

		// Only wipe default ammo if the weapon actually took it.
		if (extractedAmmo)
		{
			m_iDefaultAmmo = 0;
		}
	}

	if (pszAmmo2() != nullptr)
	{
		extractedAmmo |= weapon->AddSecondaryAmmo(0, pszAmmo2());
	}

	return extractedAmmo;
}

bool CBasePlayerWeapon::ExtractClipAmmo(CBasePlayerWeapon* weapon)
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
	return weapon->m_pPlayer->GiveAmmo(iAmmo, pszAmmo1()) != 0;
}

void CBasePlayerWeapon::RetireWeapon()
{
	SetThink(&CBasePlayerWeapon::CallDoRetireWeapon);
	pev->nextthink = gpGlobals->time + 0.01f;
}

void CBasePlayerWeapon::DoRetireWeapon()
{
	if (!m_pPlayer || m_pPlayer->m_pActiveWeapon != this)
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
	if (CanHolster() && m_pPlayer->m_pActiveWeapon == this)
	{
		m_pPlayer->SwitchWeapon(nullptr);
	}
}

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

// m_AmmoName isn't saved here because it's initialized by all derived classes.
// Classes that let level designers set the name should also save it.
TYPEDESCRIPTION CBasePlayerAmmo::m_SaveData[] =
	{
		DEFINE_FIELD(CBasePlayerAmmo, m_AmmoAmount, FIELD_INTEGER)
};

IMPLEMENT_SAVERESTORE(CBasePlayerAmmo, CBaseItem);

bool CBasePlayerAmmo::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "ammo_amount"))
	{
		m_AmmoAmount = std::max(RefillAllAmmoAmount, atoi(pkvd->szValue));
	}

	return CBaseItem::KeyValue(pkvd);
}

void CBasePlayerAmmo::Spawn()
{
	CBaseItem::Spawn();

	// Make sure these are set.
	assert(m_AmmoAmount >= RefillAllAmmoAmount);
	assert(!FStrEq(STRING(m_AmmoName), ""));
}

void CBasePlayerAmmo::PlayPickupSound(const char* pickupSoundName)
{
	if (pickupSoundName && m_PlayPickupSound)
	{
		EmitSound(CHAN_ITEM, pickupSoundName, VOL_NORM, ATTN_NORM);
	}
}

bool CBasePlayerAmmo::GiveAmmo(CBasePlayer* player, int amount, const char* ammoName, const char* pickupSoundName)
{
	if (amount < RefillAllAmmoAmount || !ammoName || FStrEq(ammoName, ""))
	{
		CBasePlayerWeapon::WeaponsLogger->error("Invalid ammo data for entity {}:{}", GetClassname(), entindex());
		return false;
	}

	if (amount == RefillAllAmmoAmount)
	{
		if (auto type = g_AmmoTypes.GetByName(ammoName); type)
		{
			amount = type->MaximumCapacity;
		}
	}

	// Act like giving 0 ammo always succeeds. For fake ammo pickups.
	if (amount == 0 || player->GiveAmmo(amount, ammoName) != -1)
	{
		PlayPickupSound(pickupSoundName);
		return true;
	}

	return false;
}

bool CBasePlayerAmmo::DefaultGiveAmmo(CBasePlayer* player, int amount, const char* ammoName, bool playSound)
{
	return GiveAmmo(player, amount, ammoName, playSound ? DefaultItemPickupSound : nullptr);
}

bool CBasePlayerAmmo::AddAmmo(CBasePlayer* player)
{
	return DefaultGiveAmmo(player, m_AmmoAmount, STRING(m_AmmoName), true);
}
