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

/**
 *	@brief rules for the single player Half-Life game
 */
class CHalfLifeRules : public CGameRules
{
public:
	static constexpr char GameModeName[] = "singleplayer";

	CHalfLifeRules();

	const char* GetGameModeName() const override { return GameModeName; }

	void Think() override;
	bool IsAllowedToSpawn(CBaseEntity* pEntity) override;
	bool FAllowFlashlight() override { return true; }

	bool FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;
	bool GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon, bool alwaysSearch = false) override;

	bool IsMultiplayer() override { return false; }
	bool IsDeathmatch() override { return false; }
	bool IsCoOp() override { return false; }
	bool IsCTF() override { return false; }

	bool ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) override;
	void InitHUD(CBasePlayer* pl) override;
	void ClientDisconnected(edict_t* pClient) override;

	float FlPlayerFallDamage(CBasePlayer* pPlayer) override;

	void PlayerThink(CBasePlayer* pPlayer) override;
	bool FPlayerCanRespawn(CBasePlayer* pPlayer) override;
	float FlPlayerSpawnTime(CBasePlayer* pPlayer) override;

	bool AllowAutoTargetCrosshair() override;

	int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) override;
	void PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;
	void DeathNotice(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;

	bool CanHaveItem(CBasePlayer* player, CBaseItem* item) override;
	void PlayerGotItem(CBasePlayer* player, CBaseItem* item) override;

	bool ItemShouldRespawn(CBaseItem* item) override;
	float ItemRespawnTime(CBaseItem* item) override;
	Vector ItemRespawnSpot(CBaseItem* item) override;
	float ItemTryRespawn(CBaseItem* item) override;

	int DeadPlayerWeapons(CBasePlayer* pPlayer) override;

	int DeadPlayerAmmo(CBasePlayer* pPlayer) override;

	bool FAllowMonsters() override;

	const char* GetTeamID(CBaseEntity* pEntity) override { return ""; }
	int PlayerRelationship(CBasePlayer* pPlayer, CBaseEntity* pTarget) override;

private:
	ScopedClientCommand m_VModEnableCommand;
};
