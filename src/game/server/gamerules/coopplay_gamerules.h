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

class CHalfLifeCoopplay : public CHalfLifeMultiplay
{
public:
	CHalfLifeCoopplay();

	BOOL IsCoOp() override { return true; }
	BOOL IsTeamplay() override { return true; }
	BOOL FAllowMonsters() override { return true; }

	void UpdateGameMode(CBasePlayer* pPlayer) override;

	void DeathNotice(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor) override {}

	void MonsterKilled(CBaseMonster* pVictim, entvars_t* pKiller, entvars_t* pInflictor) override;

	int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) override;

	BOOL ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target) override;

	int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) override;

	int IPointsForMonsterKill(CBasePlayer* pAttacker, CBaseMonster* pKilled) override;

	float FlPlayerFallDamage(CBasePlayer* pPlayer) override;

	BOOL FPlayerCanRespawn(CBasePlayer* pPlayer) override { return true; }

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

	BOOL FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker) override;

	BOOL ClientCommand(CBasePlayer* pPlayer, const char* pcmd) override;

	float FlWeaponTryRespawn(CBasePlayerItem* pWeapon) override;

	float FlWeaponRespawnTime(CBasePlayerItem* pWeapon) override;

	const char* GetGameDescription() override { return "HL Coopplay"; }

private:
	BOOL m_DisableDeathMessages;
	BOOL m_DisableDeathPenalty;
};
