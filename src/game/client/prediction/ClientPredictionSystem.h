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

#include <array>
#include <string_view>

#include "cdll_dll.h"

#include "utils/GameSystem.h"

class CBasePlayer;
class CBasePlayerWeapon;
struct edict_t;
struct local_state_t;
struct usercmd_t;

edict_t* CreateEntity();
edict_t* FindEntityByVars(entvars_t* pvars);

class ClientPredictionSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "ClientPrediction"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override;

	void Reset();

	void InitializeEntities();

	CBasePlayer* GetPlayer() { return m_Player; }

	CBasePlayerWeapon* GetLocalWeapon(int id);

	/**
	 *	@brief Run Weapon firing code on client
	 */
	void WeaponsPostThink(local_state_t* from, local_state_t* to, usercmd_t* cmd, double time, unsigned int random_seed);

private:
	CBasePlayer* CreatePlayer();
	void PrepWeapon(std::string_view className, CBasePlayer* weaponOwner);

private:
	bool m_Initialized = false;
	bool m_WeaponInfoLinked = false;

	CBasePlayer* m_Player{};
	std::array<CBasePlayerWeapon*, MAX_WEAPONS> m_Weapons{};
};

inline ClientPredictionSystem g_ClientPrediction;
