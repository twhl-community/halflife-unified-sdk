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

#include "CGameRules.h"

//=========================================================
// CHalfLifeRules - rules for the single player Half-Life
// game.
//=========================================================
class CHalfLifeRules : public CGameRules
{
public:
	CHalfLifeRules();

	// GR_Think
	void Think() override;
	bool IsAllowedToSpawn(CBaseEntity* pEntity) override;
	bool FAllowFlashlight() override { return true; }

	bool FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;
	bool GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon, bool alwaysSearch = false) override;

	// Functions to verify the single/multiplayer status of a game
	bool IsMultiplayer() override;
	bool IsDeathmatch() override;
	bool IsCoOp() override;
	bool IsCTF() override { return false; }

	// Client connection/disconnection
	bool ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) override;
	void InitHUD(CBasePlayer* pl) override; // the client dll is ready for updating
	void ClientDisconnected(edict_t* pClient) override;

	// Client damage rules
	float FlPlayerFallDamage(CBasePlayer* pPlayer) override;

	// Client spawn/respawn control
	void PlayerSpawn(CBasePlayer* pPlayer) override;
	void PlayerThink(CBasePlayer* pPlayer) override;
	bool FPlayerCanRespawn(CBasePlayer* pPlayer) override;
	float FlPlayerSpawnTime(CBasePlayer* pPlayer) override;

	bool AllowAutoTargetCrosshair() override;

	// Client kills/scoring
	int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) override;
	void PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;
	void DeathNotice(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;

	// Weapon retrieval
	void PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;

	// Weapon spawn/respawn control
	int WeaponShouldRespawn(CBasePlayerWeapon* pWeapon) override;
	float FlWeaponRespawnTime(CBasePlayerWeapon* pWeapon) override;
	float FlWeaponTryRespawn(CBasePlayerWeapon* pWeapon) override;
	Vector VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon) override;

	// Item retrieval
	bool CanHaveItem(CBasePlayer* pPlayer, CItem* pItem) override;
	void PlayerGotItem(CBasePlayer* pPlayer, CItem* pItem) override;

	// Item spawn/respawn control
	int ItemShouldRespawn(CItem* pItem) override;
	float FlItemRespawnTime(CItem* pItem) override;
	Vector VecItemRespawnSpot(CItem* pItem) override;

	// Ammo retrieval
	void PlayerGotAmmo(CBasePlayer* pPlayer, char* szName, int iCount) override;

	// Ammo spawn/respawn control
	int AmmoShouldRespawn(CBasePlayerAmmo* pAmmo) override;
	float FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo) override;
	Vector VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo) override;

	// Healthcharger respawn control
	float FlHealthChargerRechargeTime() override;

	// What happens to a dead player's weapons
	int DeadPlayerWeapons(CBasePlayer* pPlayer) override;

	// What happens to a dead player's ammo
	int DeadPlayerAmmo(CBasePlayer* pPlayer) override;

	// Monsters
	bool FAllowMonsters() override;

	// Teamplay stuff
	const char* GetTeamID(CBaseEntity* pEntity) override { return ""; }
	int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) override;

private:
	ScopedClientCommand m_VModEnableCommand;
};
