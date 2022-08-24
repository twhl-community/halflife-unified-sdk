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

#include <memory>

#include "ClientCommandRegistry.h"

class CHalfLifeCoopplay : public CHalfLifeMultiplay
{
public:
	CHalfLifeCoopplay();

	bool IsCoOp() override { return true; }
	bool IsTeamplay() override { return true; }
	bool FAllowMonsters() override { return true; }

	void UpdateGameMode(CBasePlayer* pPlayer) override;

	void DeathNotice(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor) override {}

	void MonsterKilled(CBaseMonster* pVictim, entvars_t* pKiller, entvars_t* pInflictor) override;

	int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) override;

	bool ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target) override;

	int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) override;

	int IPointsForMonsterKill(CBasePlayer* pAttacker, CBaseMonster* pKilled) override;

	float FlPlayerFallDamage(CBasePlayer* pPlayer) override;

	bool FPlayerCanRespawn(CBasePlayer* pPlayer) override { return true; }

	float FlPlayerSpawnTime(CBasePlayer* pPlayer) override;

	float FlHealthChargerRechargeTime() override;

	int DeadPlayerWeapons(CBasePlayer* pPlayer) override;

	void PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor) override;

	void Think() override;

	int WeaponShouldRespawn(CBasePlayerItem* pWeapon) override;

	int ItemShouldRespawn(CItem* pItem) override;
	float FlItemRespawnTime(CItem* pItem) override;

	int AmmoShouldRespawn(CBasePlayerAmmo* pAmmo) override;
	float FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo) override;

	bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker) override;

	float FlWeaponTryRespawn(CBasePlayerItem* pWeapon) override;

	float FlWeaponRespawnTime(CBasePlayerItem* pWeapon) override;

	const char* GetGameDescription() override { return "HL Coopplay"; }

private:
	bool m_DisableDeathMessages;
	bool m_DisableDeathPenalty;

	ScopedClientCommand m_MenuSelectCommand;
};
