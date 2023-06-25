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
	static constexpr char GameModeName[] = "deathmatch";

	CHalfLifeMultiplay();
	~CHalfLifeMultiplay() override;

	const char* GetGameModeName() const override { return GameModeName; }

	void Think() override;
	bool IsAllowedToSpawn(CBaseEntity* pEntity) override;

	bool FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) override;

	bool IsMultiplayer() override { return true; }
	bool IsDeathmatch() override { return true; }
	bool IsCoOp() override { return false; }
	bool IsCTF() override { return false; }

	bool ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) override;
	void InitHUD(CBasePlayer* pl) override;
	void ClientDisconnected(edict_t* pClient) override;
	void UpdateGameMode(CBasePlayer* pPlayer) override;

	bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker) override;

	void PlayerSpawn(CBasePlayer* pPlayer) override;
	void PlayerThink(CBasePlayer* pPlayer) override;
	bool FPlayerCanRespawn(CBasePlayer* pPlayer) override;
	float FlPlayerSpawnTime(CBasePlayer* pPlayer) override;
	CBaseEntity* GetPlayerSpawnSpot(CBasePlayer* pPlayer) override;

	bool AllowAutoTargetCrosshair() override;

	int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) override;
	void PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;
	void DeathNotice(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;

	void PlayerGotItem(CBasePlayer* player, CBaseItem* item) override;

	int DeadPlayerWeapons(CBasePlayer* pPlayer) override;

	int DeadPlayerAmmo(CBasePlayer* pPlayer) override;

	const char* GetTeamID(CBaseEntity* pEntity) override { return ""; }
	int PlayerRelationship(CBasePlayer* pPlayer, CBaseEntity* pTarget) override;

	bool PlayTextureSounds() override { return false; }
	bool PlayFootstepSounds(CBasePlayer* pl, float fvol) override;

	void EndMultiplayerGame(bool clearGlobalState) override
	{
		m_ClearGlobalState = clearGlobalState;
		GoToIntermission();
	}

protected:
	/**
	 *	@brief Server is changing to a new level, check mapcycle.json for map name and setup info
	 */
	virtual void ChangeLevel();
	virtual void GoToIntermission();
	float m_flIntermissionStartTime = 0;
	float m_flIntermissionEndTime = 0;
	bool m_iEndIntermissionButtonHit;
	bool m_ClearGlobalState = false;
	void SendMOTDToClient(CBasePlayer* player);
};
