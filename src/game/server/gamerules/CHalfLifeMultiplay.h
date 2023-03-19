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
*	@brief rules for the basic half life multiplayer competition
*/
class CHalfLifeMultiplay : public CGameRules
{
public:
	CHalfLifeMultiplay();
	~CHalfLifeMultiplay() override;

	void Think() override;
	bool IsAllowedToSpawn(CBaseEntity* pEntity) override;
	bool FAllowFlashlight() override;

	bool FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;

	bool IsMultiplayer() override;
	bool IsDeathmatch() override;
	bool IsCoOp() override;
	bool IsCTF() override { return false; }

	bool ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) override;
	void InitHUD(CBasePlayer* pl) override;
	void ClientDisconnected(edict_t* pClient) override;
	void UpdateGameMode(CBasePlayer* pPlayer) override;

	float FlPlayerFallDamage(CBasePlayer* pPlayer) override;
	bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker) override;

	void PlayerSpawn(CBasePlayer* pPlayer) override;
	void PlayerThink(CBasePlayer* pPlayer) override;
	bool FPlayerCanRespawn(CBasePlayer* pPlayer) override;
	float FlPlayerSpawnTime(CBasePlayer* pPlayer) override;
	CBaseEntity* GetPlayerSpawnSpot(CBasePlayer* pPlayer) override;

	bool AllowAutoTargetCrosshair() override;
	void ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer) override;

	int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) override;
	void PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;
	void DeathNotice(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;

	void PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;
	bool CanHavePlayerWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;

	int WeaponShouldRespawn(CBasePlayerWeapon* pWeapon) override;
	float FlWeaponRespawnTime(CBasePlayerWeapon* pWeapon) override;
	float FlWeaponTryRespawn(CBasePlayerWeapon* pWeapon) override;
	Vector VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon) override;

	bool CanHaveItem(CBasePlayer* pPlayer, CItem* pItem) override;
	void PlayerGotItem(CBasePlayer* pPlayer, CItem* pItem) override;

	int ItemShouldRespawn(CItem* pItem) override;
	float FlItemRespawnTime(CItem* pItem) override;
	Vector VecItemRespawnSpot(CItem* pItem) override;

	void PlayerGotAmmo(CBasePlayer* pPlayer, char* szName, int iCount) override;

	int AmmoShouldRespawn(CBasePlayerAmmo* pAmmo) override;
	float FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo) override;
	Vector VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo) override;

	float FlHealthChargerRechargeTime() override;
	float FlHEVChargerRechargeTime() override;

	int DeadPlayerWeapons(CBasePlayer* pPlayer) override;

	int DeadPlayerAmmo(CBasePlayer* pPlayer) override;

	const char* GetTeamID(CBaseEntity* pEntity) override { return ""; }
	int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) override;

	bool PlayTextureSounds() override { return false; }
	bool PlayFootstepSounds(CBasePlayer* pl, float fvol) override;

	bool FAllowMonsters() override;

	void EndMultiplayerGame() override { GoToIntermission(); }

protected:
	virtual void ChangeLevel();
	virtual void GoToIntermission();
	float m_flIntermissionStartTime = 0;
	float m_flIntermissionEndTime = 0;
	bool m_iEndIntermissionButtonHit;
	void SendMOTDToClient(edict_t* client);
};
