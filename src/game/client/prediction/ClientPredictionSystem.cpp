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

#include <cassert>

#include "cbase.h"
#include "cl_dll.h"
#include "ClientPredictionSystem.h"

#include "com_weapons.h"
#include "entity_state.h"
#include "usercmd.h"

// Pool of client side entities/entvars_t
static edict_t entities[MAX_WEAPONS + 1];
static std::size_t num_ents = 0;

edict_t* CreateEntity()
{
	assert(num_ents < std::size(entities));

	edict_t* edict = &entities[num_ents++];
	std::memset(edict, 0, sizeof(edict_t));

	edict->v.pContainingEntity = edict;

	return edict;
}

edict_t* FindEntityByVars(entvars_t* pvars)
{
	for (auto& edict : entities)
	{
		if (&edict.v == pvars)
		{
			return &edict;
		}
	}

	return nullptr;
}

bool ClientPredictionSystem::Initialize()
{
	return true;
}

void ClientPredictionSystem::Shutdown()
{
	if (m_Initialized)
	{
		for (auto& weapon : m_Weapons)
		{
			delete weapon;
			weapon = nullptr;
		}

		delete m_Player;
		m_Player = nullptr;

		std::memset(entities, 0, sizeof(entities));
		num_ents = 0;

		m_Initialized = false;
	}
}

void ClientPredictionSystem::InitializeEntities()
{
	if (m_Initialized)
	{
		return;
	}

	m_Initialized = true;

	// Set up weapons, player needed to run weapons code client-side.
	// Event code depends on this stuff, so always initialize it.
	// This has to be done here because event precache requires the server's event list to be present on the client.

	// Allocate a slot for the local player
	m_Player = CreatePlayer();

	// Precache weapons in a well-defined order so the client initializes its local data the same way as the server.
	// TODO: This is only necessary until weapon data is sent over the network.
	const auto classNames = g_WeaponDictionary->GetClassNames();

	std::vector<std::string_view> sortedClassNames{classNames.begin(), classNames.end()};

	std::ranges::sort(sortedClassNames);

	for (const auto& className : sortedClassNames)
	{
		PrepWeapon(className, m_Player);
	}
}

CBasePlayerWeapon* ClientPredictionSystem::GetLocalWeapon(int id)
{
	if (id > 0 || id < MAX_WEAPONS)
	{
		return m_Weapons[id];
	}

	return nullptr;
}

void ClientPredictionSystem::WeaponsPostThink(local_state_t* from, local_state_t* to, usercmd_t* cmd, double time, unsigned int random_seed)
{
	// Get current clock
	// Use actual time instead of prediction frame time because that time value breaks anything that uses absolute time values.
	gpGlobals->time = gEngfuncs.GetClientTime(); // time;

	// Lets weapons code use frametime to decrement timers and stuff.
	gpGlobals->frametime = cmd->msec / 1000.0f;

	// Fill in data based on selected weapon
	// FIXME, make this a method in each weapon?  where you pass in an entity_state_t *?
	auto pWeapon = GetLocalWeapon(from->client.m_iId);

	auto localPlayer = GetPlayer();

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = to;

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if (g_runfuncs)
	{
		static int lasthealth = 0;

		if (to->client.health <= 0 && lasthealth > 0)
		{
			localPlayer->Killed(nullptr, 0);
		}
		else if (to->client.health > 0 && lasthealth <= 0)
		{
			localPlayer->Spawn();
		}

		lasthealth = to->client.health;
	}

	// We are not predicting the current weapon, just bow out here.
	if (!pWeapon)
		return;

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		auto pCurrent = GetLocalWeapon(i);
		if (!pCurrent)
		{
			continue;
		}

		auto pfrom = &from->weapondata[i];

		pCurrent->m_fInReload = 0 != pfrom->m_fInReload;
		pCurrent->m_fInSpecialReload = pfrom->m_fInSpecialReload;
		//		pCurrent->m_flPumpTime			= pfrom->m_flPumpTime;
		pCurrent->m_iClip = pfrom->m_iClip;
		pCurrent->m_flNextPrimaryAttack = pfrom->m_flNextPrimaryAttack;
		pCurrent->m_flNextSecondaryAttack = pfrom->m_flNextSecondaryAttack;
		pCurrent->m_flTimeWeaponIdle = pfrom->m_flTimeWeaponIdle;
		pCurrent->pev->fuser1 = pfrom->fuser1;
		pCurrent->m_flStartThrow = pfrom->fuser2;
		pCurrent->m_flReleaseThrow = pfrom->fuser3;
		pCurrent->m_chargeReady = pfrom->iuser1;
		pCurrent->m_fInAttack = pfrom->iuser2;
		pCurrent->m_fireState = pfrom->iuser3;

		pCurrent->m_iSecondaryAmmoType = (int)from->client.vuser3[2];
		pCurrent->m_iPrimaryAmmoType = (int)from->client.vuser4[0];
		localPlayer->m_rgAmmo[pCurrent->m_iPrimaryAmmoType] = (int)from->client.vuser4[1];
		localPlayer->m_rgAmmo[pCurrent->m_iSecondaryAmmoType] = (int)from->client.vuser4[2];

		pCurrent->SetWeaponData(*pfrom);
	}

	// For random weapon events, use this seed to seed random # generator
	localPlayer->random_seed = random_seed;

	// Get old buttons from previous state.
	localPlayer->m_afButtonLast = from->playerstate.oldbuttons;

	// Which buttsons chave changed
	const int buttonsChanged = (localPlayer->m_afButtonLast ^ cmd->buttons); // These buttons have changed this frame

	// Debounced button codes for pressed/released
	// The changed ones still down are "pressed"
	localPlayer->m_afButtonPressed = buttonsChanged & cmd->buttons;
	// The ones not down are "released"
	localPlayer->m_afButtonReleased = buttonsChanged & (~cmd->buttons);

	// Set player variables that weapons code might check/alter
	localPlayer->pev->button = cmd->buttons;

	localPlayer->pev->velocity = from->client.velocity;
	localPlayer->pev->flags = from->client.flags;

	localPlayer->pev->deadflag = from->client.deadflag;
	localPlayer->pev->waterlevel = from->client.waterlevel;
	localPlayer->pev->maxspeed = from->client.maxspeed;
	localPlayer->m_iFOV = from->client.fov;
	localPlayer->pev->weaponanim = from->client.weaponanim;
	localPlayer->pev->viewmodel = static_cast<string_t>(from->client.viewmodel);
	localPlayer->m_flNextAttack = from->client.m_flNextAttack;
	localPlayer->m_flNextAmmoBurn = from->client.fuser2;
	localPlayer->m_flAmmoStartCharge = from->client.fuser3;
	localPlayer->m_iItems = static_cast<CTFItem::CTFItem>(from->client.iuser4);

	// Stores all our ammo info, so the client side weapons can use them.
	localPlayer->ammo_9mm = (int)from->client.vuser1[0];
	localPlayer->ammo_357 = (int)from->client.vuser1[1];
	localPlayer->ammo_argrens = (int)from->client.vuser1[2];
	localPlayer->ammo_bolts = (int)from->client.ammo_nails; // is an int anyways...
	localPlayer->ammo_buckshot = (int)from->client.ammo_shells;
	localPlayer->ammo_uranium = (int)from->client.ammo_cells;
	localPlayer->ammo_hornets = (int)from->client.vuser2[0];
	localPlayer->ammo_rockets = (int)from->client.ammo_rockets;
	localPlayer->ammo_spores = (int)from->client.vuser2.y;
	localPlayer->ammo_762 = (int)from->client.vuser2.z;


	// Point to current weapon object
	if (WEAPON_NONE != from->client.m_iId)
	{
		localPlayer->m_pActiveItem = GetLocalWeapon(from->client.m_iId);
	}

	if (localPlayer->m_pActiveItem->m_iId == WEAPON_RPG)
	{
		((CRpg*)localPlayer->m_pActiveItem)->m_fSpotActive = 0 != from->client.vuser2[1];
		((CRpg*)localPlayer->m_pActiveItem)->m_cActiveRockets = (int)from->client.vuser2[2];
	}

	// Don't go firing anything if we have died or are spectating
	// Or if we don't have a weapon model deployed
	if ((localPlayer->pev->deadflag != (DEAD_DISCARDBODY + 1)) &&
		!CL_IsDead() && !FStringNull(localPlayer->pev->viewmodel) && 0 == g_iUser1)
	{
		if (localPlayer->m_flNextAttack <= 0)
		{
			pWeapon->ItemPostFrame();
		}
	}

	// Assume that we are not going to switch weapons
	to->client.m_iId = from->client.m_iId;

	// Now see if we issued a changeweapon command ( and we're not dead )
	if (0 != cmd->weaponselect && (localPlayer->pev->deadflag != (DEAD_DISCARDBODY + 1)))
	{
		// Switched to a different weapon?
		if (from->weapondata[cmd->weaponselect].m_iId == cmd->weaponselect)
		{
			CBasePlayerWeapon* pNew = GetLocalWeapon(cmd->weaponselect);
			if (pNew && (pNew != pWeapon))
			{
				// Put away old weapon
				if (localPlayer->m_pActiveItem)
					localPlayer->m_pActiveItem->Holster();

				localPlayer->m_pLastItem = localPlayer->m_pActiveItem;
				localPlayer->m_pActiveItem = pNew;

				// Deploy new weapon
				if (localPlayer->m_pActiveItem)
				{
					localPlayer->m_pActiveItem->Deploy();
				}

				// Update weapon id so we can predict things correctly.
				to->client.m_iId = cmd->weaponselect;
			}
		}
	}

	// Copy in results of prediction code
	to->client.viewmodel = static_cast<int>(localPlayer->pev->viewmodel.m_Value);
	to->client.fov = localPlayer->m_iFOV;
	to->client.weaponanim = localPlayer->pev->weaponanim;
	to->client.m_flNextAttack = localPlayer->m_flNextAttack;
	to->client.fuser2 = localPlayer->m_flNextAmmoBurn;
	to->client.fuser3 = localPlayer->m_flAmmoStartCharge;
	to->client.maxspeed = localPlayer->pev->maxspeed;
	to->client.iuser4 = localPlayer->m_iItems;

	// HL Weapons
	to->client.vuser1[0] = localPlayer->ammo_9mm;
	to->client.vuser1[1] = localPlayer->ammo_357;
	to->client.vuser1[2] = localPlayer->ammo_argrens;

	to->client.ammo_nails = localPlayer->ammo_bolts;
	to->client.ammo_shells = localPlayer->ammo_buckshot;
	to->client.ammo_cells = localPlayer->ammo_uranium;
	to->client.vuser2[0] = localPlayer->ammo_hornets;
	to->client.ammo_rockets = localPlayer->ammo_rockets;
	to->client.vuser2.y = localPlayer->ammo_spores;
	to->client.vuser2.z = localPlayer->ammo_762;

	if (localPlayer->m_pActiveItem->m_iId == WEAPON_RPG)
	{
		to->client.vuser2[1] = static_cast<float>(((CRpg*)localPlayer->m_pActiveItem)->m_fSpotActive);
		to->client.vuser2[2] = ((CRpg*)localPlayer->m_pActiveItem)->m_cActiveRockets;
	}

	// Make sure that weapon animation matches what the game .dll is telling us
	//  over the wire ( fixes some animation glitches )
	if (g_runfuncs && (HUD_GetWeaponAnim() != to->client.weaponanim))
	{
		// Make sure the 357 has the right body
		if (FClassnameIs(pWeapon->pev, "weapon_357"))
		{
			pWeapon->pev->body = UTIL_IsMultiplayer() ? 1 : 0;
		}

		// Force a fixed anim down to viewmodel
		HUD_SendWeaponAnim(to->client.weaponanim, pWeapon->pev->body, true);
	}

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		auto pCurrent = GetLocalWeapon(i);

		auto pto = &to->weapondata[i];

		if (!pCurrent)
		{
			memset(pto, 0, sizeof(weapon_data_t));
			continue;
		}

		pto->m_fInReload = static_cast<int>(pCurrent->m_fInReload);
		pto->m_fInSpecialReload = pCurrent->m_fInSpecialReload;
		//		pto->m_flPumpTime				= pCurrent->m_flPumpTime;
		pto->m_iClip = pCurrent->m_iClip;
		pto->m_flNextPrimaryAttack = pCurrent->m_flNextPrimaryAttack;
		pto->m_flNextSecondaryAttack = pCurrent->m_flNextSecondaryAttack;
		pto->m_flTimeWeaponIdle = pCurrent->m_flTimeWeaponIdle;
		pto->fuser1 = pCurrent->pev->fuser1;
		pto->fuser2 = pCurrent->m_flStartThrow;
		pto->fuser3 = pCurrent->m_flReleaseThrow;
		pto->iuser1 = pCurrent->m_chargeReady;
		pto->iuser2 = pCurrent->m_fInAttack;
		pto->iuser3 = pCurrent->m_fireState;

		// Decrement weapon counters, server does this at same time ( during post think, after doing everything else )
		pto->m_flNextReload -= cmd->msec / 1000.0;
		pto->m_fNextAimBonus -= cmd->msec / 1000.0;
		pto->m_flNextPrimaryAttack -= cmd->msec / 1000.0;
		pto->m_flNextSecondaryAttack -= cmd->msec / 1000.0;
		pto->m_flTimeWeaponIdle -= cmd->msec / 1000.0;
		pto->fuser1 -= cmd->msec / 1000.0;

		to->client.vuser3[2] = pCurrent->m_iSecondaryAmmoType;
		to->client.vuser4[0] = pCurrent->m_iPrimaryAmmoType;
		to->client.vuser4[1] = localPlayer->m_rgAmmo[pCurrent->m_iPrimaryAmmoType];
		to->client.vuser4[2] = localPlayer->m_rgAmmo[pCurrent->m_iSecondaryAmmoType];

		pCurrent->DecrementTimers();

		pCurrent->GetWeaponData(*pto);

		/*		if ( pto->m_flPumpTime != -9999 )
				{
					pto->m_flPumpTime -= cmd->msec / 1000.0;
					if ( pto->m_flPumpTime < -0.001 )
						pto->m_flPumpTime = -0.001;
				}*/

		if (pto->m_fNextAimBonus < -1.0)
		{
			pto->m_fNextAimBonus = -1.0;
		}

		if (pto->m_flNextPrimaryAttack < -1.1)
		{
			pto->m_flNextPrimaryAttack = -1.1;
		}

		if (pto->m_flNextSecondaryAttack < -0.001)
		{
			pto->m_flNextSecondaryAttack = -0.001;
		}

		if (pto->m_flTimeWeaponIdle < -0.001)
		{
			pto->m_flTimeWeaponIdle = -0.001;
		}

		if (pto->m_flNextReload < -0.001)
		{
			pto->m_flNextReload = -0.001;
		}

		if (pto->fuser1 < -0.001)
		{
			pto->fuser1 = -0.001;
		}
	}

	// m_flNextAttack is now part of the weapons, but is part of the player instead
	to->client.m_flNextAttack -= cmd->msec / 1000.0;
	if (to->client.m_flNextAttack < -0.001)
	{
		to->client.m_flNextAttack = -0.001;
	}

	to->client.fuser2 -= cmd->msec / 1000.0;
	if (to->client.fuser2 < -0.001)
	{
		to->client.fuser2 = -0.001;
	}

	to->client.fuser3 -= cmd->msec / 1000.0;
	if (to->client.fuser3 < -0.001)
	{
		to->client.fuser3 = -0.001;
	}

	// Store off the last position from the predicted state.
	HUD_SetLastOrg();

	// Wipe it so we can't use it after this frame
	g_finalstate = nullptr;
}

CBasePlayer* ClientPredictionSystem::CreatePlayer()
{
	auto player = g_EntityDictionary->Create<CBasePlayer>("player");

	player->Precache();
	player->Spawn();

	return player;
}

void ClientPredictionSystem::PrepWeapon(std::string_view className, CBasePlayer* weaponOwner)
{
	auto entity = g_WeaponDictionary->Create(className);

	entity->Precache();
	entity->Spawn();

	entity->m_pPlayer = weaponOwner;

	ItemInfo info;

	entity->GetItemInfo(&info);

	// If this assert hits then you have more than one weapon with the same id.
	assert(!m_Weapons[info.iId]);

	m_Weapons[info.iId] = entity;

	CBasePlayerItem::ItemInfoArray[info.iId] = info;

	const char* weaponName = ((info.iFlags & ITEM_FLAG_EXHAUSTIBLE) != 0) ? STRING(entity->pev->classname) : nullptr;

	if (info.pszAmmo1 && '\0' != *info.pszAmmo1)
	{
		AddAmmoNameToAmmoRegistry(info.pszAmmo1, weaponName);
	}

	if (info.pszAmmo2 && '\0' != *info.pszAmmo2)
	{
		AddAmmoNameToAmmoRegistry(info.pszAmmo2, weaponName);
	}
}
