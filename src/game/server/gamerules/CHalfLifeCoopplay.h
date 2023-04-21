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

#include "CHalfLifeMultiplay.h"

class CHalfLifeCoopplay : public CHalfLifeMultiplay
{
public:
	static constexpr char GameModeName[] = "coop";

	CHalfLifeCoopplay();

	const char* GetGameModeName() const override { return GameModeName; }

	bool IsCoOp() override { return true; }
	bool IsTeamplay() override { return true; }

	void UpdateGameMode(CBasePlayer* pPlayer) override;

	void DeathNotice(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override {}

	void MonsterKilled(CBaseMonster* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;

	int PlayerRelationship(CBasePlayer* pPlayer, CBaseEntity* pTarget) override;

	bool ShouldAutoAim(CBasePlayer* pPlayer, CBaseEntity* target) override;

	int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) override;

	int IPointsForMonsterKill(CBasePlayer* pAttacker, CBaseMonster* pKilled) override;

	bool FPlayerCanRespawn(CBasePlayer* pPlayer) override { return true; }

	float FlPlayerSpawnTime(CBasePlayer* pPlayer) override;

	int DeadPlayerWeapons(CBasePlayer* pPlayer) override;

	void PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) override;

	void Think() override;

	bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker) override;

	const char* GetGameDescription() override { return "HL Coopplay"; }

private:
	bool m_DisableDeathMessages = false;
	bool m_DisableDeathPenalty = false;

	ScopedClientCommand m_MenuSelectCommand;
};
