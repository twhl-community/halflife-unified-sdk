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

#include "CTFDefs.h"

const int MaxTeams = 2;

extern int teamscores[MaxTeams];

extern char g_szScoreIconNameBM[];
extern char g_szScoreIconNameOF[];

extern float g_flFlagReturnTime;
extern float g_flBaseDefendDist;
extern float g_flDefendCarrierTime;
extern float g_flCaptureAssistTime;
extern float g_flPowerupRespawnTime;
extern int g_iMapScoreMax;

void DisplayTeamFlags(CBasePlayer* pPlayer);

void ResetTeamScores();

/**
*	@brief Opposing Force CTF gamemode rules
*/
class CHalfLifeCTFplay : public CHalfLifeMultiplay
{
private:
	enum class StatsPhase
	{
		Nothing,
		SendTeam0,
		SendTeam1,
		SendTeam2,
		SendPlayers,
		OpenMenu
	};

public:
	CHalfLifeCTFplay();

	void Think() override;

	bool IsTeamplay() override { return true; }
	bool IsCTF() override { return true; }

	const char* GetGameDescription() override { return "OpFor CTF"; }

	bool ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char* szRejectReason) override;

	void InitHUD(CBasePlayer* pPlayer) override;

	void ClientDisconnected(edict_t* pClient) override;

	void UpdateGameMode(CBasePlayer* pPlayer) override;

	bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker) override;

	bool ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target) override;

	void PlayerSpawn(CBasePlayer* pPlayer) override;

	void PlayerThink(CBasePlayer* pPlayer) override;

	bool ClientCommand(CBasePlayer* pPlayer, const char* pcmd) override;

	void ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer) override;

	int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) override;

	void PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor) override;

	void DeathNotice(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor) override;

	bool CanHaveAmmo(CBasePlayer* pPlayer, const char* pszAmmoName, int iMaxCarry) override;

	const char* GetTeamID(CBaseEntity* pEntity) override;

	int PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget) override;

	int GetTeamIndex(const char* pTeamName) override;

	const char* GetIndexedTeamName(int teamIndex) override;

	bool IsValidTeam(const char* pTeamName) override;

	void ChangePlayerTeam(CBasePlayer* pPlayer, const char* pCharName, bool bKill, bool bGib) override;

	const char* SetDefaultPlayerTeam(CBasePlayer* pPlayer) override;

	const char* GetCharacterType(int iTeamNum, int iCharNum) override;

	int GetNumTeams() override;

	const char* TeamWithFewestPlayers() override;

	bool TeamsBalanced() override;

	void GoToIntermission() override;

private:
	void SendTeamStatInfo(CTFTeam iTeamNum);
	void SendPlayerStatInfo(CBasePlayer* pPlayer);

	void RecountTeams();

private:
	bool m_DisableDeathMessages = false;
	bool m_DisableDeathPenalty = false;
	bool m_fRefreshScores = false;
	float m_flNextStatsSend;
	StatsPhase m_iStatsPhase = StatsPhase::Nothing;
	//Use a sane default to avoid lockups
	int m_iStatsPlayer = 1;
};

extern char* pszPlayerIPs[MAX_PLAYERS * 2];

const char* GetTeamName(edict_t* pEntity);

void GetLosingTeam(int& iTeamNum, int& iScoreDiff);

void RespawnPlayerCTFPowerups(CBasePlayer* pPlayer, bool bForceRespawn);
void ScatterPlayerCTFPowerups(CBasePlayer* pPlayer);
void DropPlayerCTFPowerup(CBasePlayer* pPlayer);
void FlushCTFPowerupTimes();
void InitItemsForPlayer(CBasePlayer* pPlayer);
