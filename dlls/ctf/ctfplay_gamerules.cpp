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
#include <algorithm>
#include <array>

#include "extdll.h"
#include "util.h"
#include "client.h"
#include "cbase.h"
#include "basemonster.h"
#include "player.h"
#include "weapons.h"
#include "game.h"
#include "gamerules.h"
#include "UserMessages.h"

#include "ctfplay_gamerules.h"
#include "CTFGoalFlag.h"
#include "CItemCTF.h"
#include "CTFGoalBase.h"

#include "CItemAcceleratorCTF.h"
#include "CItemBackpackCTF.h"
#include "CItemLongJumpCTF.h"
#include "CItemPortableHEVCTF.h"
#include "CItemRegenerationCTF.h"

#include "pm_shared.h"

extern DLL_GLOBAL bool g_fGameOver;

const int MaxTeamNameLength = 16;
const int MaxTeamCharacters = 12;
const int MaxTeamCharacterNameLength = 16;

char g_szScoreIconNameBM[40] = "item_ctfflagh";
char g_szScoreIconNameOF[40] = "item_ctfflagh";

float g_flFlagReturnTime = 30;
float g_flBaseDefendDist = 192;
float g_flDefendCarrierTime = 10;
float g_flCaptureAssistTime = 10;
float g_flPowerupRespawnTime = 30;
int g_iMapScoreMax = 0;

char* pszPlayerIPs[MAX_PLAYERS * 2];

static int team_scores[MaxTeams];
int teamscores[MaxTeams];
static int team_count[MaxTeams];

static const char team_names[MaxTeams][MaxTeamNameLength] =
	{
		{"Black Mesa"},
		{"Opposing Force"}};

//First half are first team, second half are second team
//TODO: rework so it's 2 separate lists
static const std::array<const char*, MaxTeamCharacters> team_chars =
	{
		"ctf_barney",
		"cl_suit",
		"ctf_gina",
		"ctf_gordon",
		"otis",
		"ctf_scientist",
		"beret",
		"drill",
		"grunt",
		"recruit",
		"shephard",
		"tower",
};

int ToTeamIndex(const CTFTeam team)
{
	return static_cast<int>(team) - 1;
}

const char* GetTeamName(edict_t* pEntity)
{
	if (g_pGameRules->IsCTF())
	{
		auto pPlayer = static_cast<CBasePlayer*>(CBaseEntity::Instance(pEntity));

		return team_names[(int)pPlayer->m_iTeamNum - 1];
	}
	else
	{
		//A bit counterintuitive, this basically means each player model is a team
		if (g_pGameRules->IsDeathmatch() && g_pGameRules->IsTeamplay())
		{
			return g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(pEntity), "model");
		}
		else
		{
			static char szTmp[256];

			snprintf(szTmp, sizeof(szTmp), "%i", g_engfuncs.pfnGetPlayerUserId(pEntity));
			return szTmp;
		}
	}
}

CTFTeam GetWinningTeam()
{
	if (teamscores[0] != teamscores[1])
		return (teamscores[0] <= teamscores[1]) ? CTFTeam::BlackMesa : CTFTeam::OpposingForce;
	return CTFTeam::None;
}

void GetLosingTeam(int& iTeamNum, int& iScoreDiff)
{
	iTeamNum = 0;
	iScoreDiff = 0;

	//TODO: doesn't really make sense, if team 0 is losing the score difference is 0
	if (teamscores[1] < teamscores[0])
	{
		iTeamNum = 1;

		const auto difference = teamscores[0] - teamscores[1];

		if (iScoreDiff < difference)
			iScoreDiff = difference;
	}
}

static void SendFlagIcon(CBasePlayer* player, bool isActive, const char* flagName, int teamIndex)
{
	g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgFlagIcon, nullptr, player->edict());
	g_engfuncs.pfnWriteByte(isActive ? 1 : 0);
	g_engfuncs.pfnWriteString(flagName);
	g_engfuncs.pfnWriteByte(teamIndex);

	int r, g, b;

	if (teamIndex == 0)
	{
		UnpackRGB(r, g, b, RGB_YELLOWISH);
	}
	else
	{
		UnpackRGB(r, g, b, RGB_HUD_COLOR);
	}

	g_engfuncs.pfnWriteByte(r);
	g_engfuncs.pfnWriteByte(g);
	g_engfuncs.pfnWriteByte(b);

	g_engfuncs.pfnWriteByte(teamscores[teamIndex]);
	g_engfuncs.pfnMessageEnd();
}

static void SendFlagIcons(CBasePlayer* player, CTFGoalFlag* team1Flag, CTFGoalFlag* team2Flag)
{
	if (team1Flag || team2Flag)
	{
		auto pMyFlag = static_cast<CTFGoalFlag*>(static_cast<CBaseEntity*>(player->m_pFlag));

		if (team1Flag)
		{
			const char* flagName = "";

			switch (team1Flag->m_iGoalState)
			{
			case 1:
				flagName = "item_ctfflagh";
				break;
			case 2:
				if (pMyFlag == team1Flag && pMyFlag)
				{
					flagName = "item_ctfflagg";
				}
				else
				{
					flagName = "item_ctfflagc";
				}
				break;
			case 3:
				flagName = "item_ctfflagd";
				break;
			}

			SendFlagIcon(player, true, flagName, 0);
		}

		if (team2Flag)
		{
			const char* flagName = "";

			switch (team2Flag->m_iGoalState)
			{
			case 1:
				flagName = "item_ctfflagh";
				break;
			case 2:
				if (pMyFlag == team2Flag && pMyFlag)
				{
					flagName = "item_ctfflagg";
				}
				else
				{
					flagName = "item_ctfflagc";
				}
				break;
			case 3:
				flagName = "item_ctfflagd";
				break;
			}

			SendFlagIcon(player, true, flagName, 1);
		}
	}
	else
	{
		SendFlagIcon(player, true, g_szScoreIconNameBM, 0);
		SendFlagIcon(player, true, g_szScoreIconNameOF, 1);
	}
}

void DisplayTeamFlags(CBasePlayer* pPlayer)
{
	if (0 == gmsgFlagIcon)
		return;

	CTFGoalFlag* team1Flag = nullptr;

	while ((team1Flag = static_cast<CTFGoalFlag*>(UTIL_FindEntityByClassname(team1Flag, "item_ctfflag"))) && team1Flag->m_iGoalNum != 1)
	{
	}

	CTFGoalFlag* team2Flag = nullptr;

	while ((team2Flag = static_cast<CTFGoalFlag*>(UTIL_FindEntityByClassname(team2Flag, "item_ctfflag"))) && team2Flag->m_iGoalNum != 2)
	{
	}

	if (pPlayer)
	{
		SendFlagIcons(pPlayer, team1Flag, team2Flag);
	}
	else
	{
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto player = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i));

			if (player)
			{
				SendFlagIcons(player, team1Flag, team2Flag);
			}
		}
	}
}

bool CheckForLevelEnd(int iMaxScore)
{
	if (iMaxScore == 0)
	{
		iMaxScore = 255;
	}

	if (g_iMapScoreMax > 0 && iMaxScore > g_iMapScoreMax)
		iMaxScore = g_iMapScoreMax;

	return teamscores[0] >= iMaxScore || teamscores[1] >= iMaxScore;
}

void ResetTeamScores()
{
	teamscores[0] = 0;
	teamscores[1] = 0;
}

struct ItemData
{
	using CleanupCallback_t = void (*)(CBasePlayer* pPlayer);

	const CTFItem::CTFItem Mask;
	const char* const Name;

	const color24 IconColor;

	const bool RespawnOrScatter;

	const CleanupCallback_t CleanupCallback;
};

const ItemData CTFItems[] =
	{
		{CTFItem::BlackMesaFlag, "", {1, 1, 1}, false, nullptr},
		{CTFItem::OpposingForceFlag, "", {1, 1, 2}, false, nullptr},

		{CTFItem::LongJump, "item_ctflongjump", {1, 0, 4}, true, nullptr},
		{CTFItem::PortableHEV, "item_ctfportablehev", {1, 0, 8}, true, [](auto pPlayer)
			{
				if (pPlayer->m_fPlayingAChargeSound)
				{
					STOP_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_armor_charge.wav");
					pPlayer->m_fPlayingAChargeSound = false;
				}
			}},
		{CTFItem::Backpack, "item_ctfbackpack", {1, 0, 16}, true, nullptr},
		{CTFItem::Acceleration, "item_ctfaccelerator", {1, 0, 32}, true, nullptr},
		{CTFItem::Regeneration, "item_ctfregeneration", {1, 0, 128}, true, [](auto pPlayer)
			{
				if (pPlayer->m_fPlayingHChargeSound)
				{
					STOP_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_health_charge.wav");
					pPlayer->m_fPlayingHChargeSound = false;
				}
			}},
};

template <typename ItemCallback>
static void ForEachPlayerCTFPowerup(CBasePlayer* pPlayer, ItemCallback callback)
{
	if (!pPlayer || !(pPlayer->m_iItems & CTFItem::ItemsMask))
	{
		return;
	}

	for (const auto& item : CTFItems)
	{
		if (item.RespawnOrScatter && (pPlayer->m_iItems & item.Mask))
		{
			for (CItemCTF* pItem = nullptr; (pItem = static_cast<CItemCTF*>(UTIL_FindEntityByClassname(pItem, item.Name)));)
			{
				if (pItem->team_no == CTFTeam::None || pItem->team_no == pPlayer->m_iTeamNum)
				{
					callback(pPlayer, pItem);
					break;
				}
			}

			if (item.CleanupCallback)
			{
				item.CleanupCallback(pPlayer);
			}

			pPlayer->m_iItems = (CTFItem::CTFItem)(pPlayer->m_iItems & ~item.Mask);
		}
	}
}

void RespawnPlayerCTFPowerups(CBasePlayer* pPlayer, bool bForceRespawn)
{
	ForEachPlayerCTFPowerup(pPlayer, [=](auto pPlayer, auto pItem)
		{ pItem->DropItem(pPlayer, bForceRespawn); });
}

void ScatterPlayerCTFPowerups(CBasePlayer* pPlayer)
{
	ForEachPlayerCTFPowerup(pPlayer, [=](auto pPlayer, auto pItem)
		{ pItem->ScatterItem(pPlayer); });
}

void DropPlayerCTFPowerup(CBasePlayer* pPlayer)
{
	ForEachPlayerCTFPowerup(pPlayer, [=](auto pPlayer, auto pItem)
		{ pItem->ThrowItem(pPlayer); });
}

void FlushCTFPowerupTimes()
{
	for (auto pItem : UTIL_FindEntitiesByClassname<CItemCTF>("item_ctflongjump"))
	{
		auto pPlayer = GET_PRIVATE<CBasePlayer>(pItem->pev->owner);

		if (pPlayer && pPlayer->IsPlayer())
		{
			pPlayer->m_flJumpTime += gpGlobals->time - pItem->m_flPickupTime;
		}
	}

	for (auto pItem : UTIL_FindEntitiesByClassname<CItemPortableHEVCTF>("item_ctfportablehev"))
	{
		auto pPlayer = GET_PRIVATE<CBasePlayer>(pItem->pev->owner);

		if (pPlayer && pPlayer->IsPlayer())
		{
			pPlayer->m_flShieldTime += gpGlobals->time - pItem->m_flPickupTime;
		}
	}

	for (auto pItem : UTIL_FindEntitiesByClassname<CItemRegenerationCTF>("item_ctfregeneration"))
	{
		auto pPlayer = GET_PRIVATE<CBasePlayer>(pItem->pev->owner);

		if (pPlayer && pPlayer->IsPlayer())
		{
			pPlayer->m_flHealthTime += gpGlobals->time - pItem->m_flPickupTime;
		}
	}

	for (auto pItem : UTIL_FindEntitiesByClassname<CItemBackpackCTF>("item_ctfbackpack"))
	{
		auto pPlayer = GET_PRIVATE<CBasePlayer>(pItem->pev->owner);

		if (pPlayer && pPlayer->IsPlayer())
		{
			pPlayer->m_flBackpackTime += gpGlobals->time - pItem->m_flPickupTime;
		}
	}

	for (auto pItem : UTIL_FindEntitiesByClassname<CItemAcceleratorCTF>("item_ctfaccelerator"))
	{
		auto pPlayer = GET_PRIVATE<CBasePlayer>(pItem->pev->owner);

		if (pPlayer && pPlayer->IsPlayer())
		{
			pPlayer->m_flAccelTime += gpGlobals->time - pItem->m_flPickupTime;
		}
	}
}

void InitItemsForPlayer(CBasePlayer* pPlayer)
{
	if (0 != gmsgPlayerIcon)
	{
		for (auto pOther : UTIL_FindPlayers())
		{
			for (const auto& item : CTFItems)
			{
				//TODO: this can probably be optimized by finding the last item that the player is carrying and only sending that
				if ((pOther->m_iItems & item.Mask) != 0)
				{
					g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgPlayerIcon, nullptr, pPlayer->edict());
					g_engfuncs.pfnWriteByte(pOther->entindex());
					g_engfuncs.pfnWriteByte(item.IconColor.r);
					g_engfuncs.pfnWriteByte(item.IconColor.g);
					g_engfuncs.pfnWriteByte(item.IconColor.b);
					g_engfuncs.pfnMessageEnd();
				}
			}
		}
	}
}

CHalfLifeCTFplay::CHalfLifeCTFplay()
{
	memset(team_scores, 0, sizeof(team_scores));
	memset(team_count, 0, sizeof(team_count));

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer)
		{
			auto pszTeam = pPlayer->TeamID();

			auto teamIndex = GetTeamIndex(pszTeam);

			if (teamIndex >= 0)
			{
				++team_count[teamIndex];
			}
		}
	}
}

void CHalfLifeCTFplay::Think()
{
	if (m_iStatsPhase != StatsPhase::Nothing && gpGlobals->time >= m_flNextStatsSend)
	{
		switch (m_iStatsPhase)
		{
		default:
			break;
		case StatsPhase::OpenMenu:
			MESSAGE_BEGIN(MSG_ALL, gmsgVGUIMenu);
			g_engfuncs.pfnWriteByte(MENU_STATSMENU);
			MESSAGE_END();
			m_iStatsPhase = StatsPhase::Nothing;
			break;
		case StatsPhase::SendPlayers:
			//Send up to 5 players worth of stats at a time until we've sent everything.
			for (int iStat = 0; iStat <= 5 && m_iStatsPlayer <= gpGlobals->maxClients; ++m_iStatsPlayer)
			{
				auto pPlayer = UTIL_PlayerByIndex(m_iStatsPlayer);

				if (pPlayer && pPlayer->IsPlayer())
				{
					++iStat;
					SendPlayerStatInfo(static_cast<CBasePlayer*>(pPlayer));
				}
			}

			--m_iStatsPlayer;

			//TODO: checks against an index that may not have been sent
			if (gpGlobals->maxClients <= (m_iStatsPlayer + 1))
				m_iStatsPhase = StatsPhase::OpenMenu;
			break;
		case StatsPhase::SendTeam2:
			SendTeamStatInfo(CTFTeam::OpposingForce);
			m_iStatsPhase = StatsPhase::SendPlayers;
			break;
		case StatsPhase::SendTeam1:
			SendTeamStatInfo(CTFTeam::BlackMesa);
			m_iStatsPhase = StatsPhase::SendTeam2;
			break;
		case StatsPhase::SendTeam0:
			SendTeamStatInfo(CTFTeam::None);
			m_iStatsPhase = StatsPhase::SendTeam1;
			break;
		}

		m_flNextStatsSend = gpGlobals->time + 0.2;
	}

	if (m_fRefreshScores)
	{
		DisplayTeamFlags(nullptr);

		m_fRefreshScores = false;
	}

	if (g_fGameOver)
	{
		CHalfLifeMultiplay::Think();
		return;
	}

	const auto timeLimit = 60.0 * timelimit.value;
	if (timeLimit != 0.0 && gpGlobals->time >= timeLimit)
	{
		GoToIntermission();
		return;
	}

	const auto flFragLimit = fraglimit.value;

	if (flFragLimit != 0.0)
	{
		for (int iPlayer = 1; iPlayer <= gpGlobals->maxClients; ++iPlayer)
		{
			auto pPlayer = UTIL_PlayerByIndex(iPlayer);

			if (pPlayer)
			{
				if (pPlayer->pev->frags >= flFragLimit)
				{
					GoToIntermission();
					return;
				}
			}
		}
	}

	if (CheckForLevelEnd(static_cast<int>(ctf_capture.value)))
	{
		GoToIntermission();
	}
}

bool CHalfLifeCTFplay::ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char* szRejectReason)
{
	m_fRefreshScores = true;

	int playersInTeamsCount = 0;

	for (int iPlayer = 1; iPlayer <= gpGlobals->maxClients; ++iPlayer)
	{
		auto pPlayer = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(iPlayer));

		if (pPlayer)
		{
			if (pPlayer->IsPlayer())
			{
				playersInTeamsCount += (pPlayer->m_iTeamNum > CTFTeam::None) ? 1 : 0;
			}
		}
	}

	if (playersInTeamsCount <= 1)
	{
		for (int iPlayer = 1; iPlayer <= gpGlobals->maxClients; ++iPlayer)
		{
			auto pPlayer = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(iPlayer));

			if (pPlayer && pPlayer->m_iItems != CTFItem::None)
			{
				auto pFlag = static_cast<CTFGoalFlag*>(pPlayer->m_pFlag.operator CBaseEntity*());

				if (pFlag)
				{
					pFlag->ReturnFlag();
				}

				if ((pPlayer->m_iItems & CTFItem::ItemsMask) != 0)
				{
					RespawnPlayerCTFPowerups(pPlayer, true);
				}

				ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#CTFGameReset");
			}
		}

		ResetTeamScores();
	}

	return CHalfLifeMultiplay::ClientConnected(pEntity, pszName, pszAddress, szRejectReason);
}

void CHalfLifeCTFplay::InitHUD(CBasePlayer* pPlayer)
{
	CHalfLifeMultiplay::InitHUD(pPlayer);

	DisplayTeamFlags(pPlayer);

	auto v2 = 60.0f * timelimit.value;
	auto v25 = pPlayer->edict();
	if (v2 == 0)
	{
		g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgFlagTimer, 0, v25);
		g_engfuncs.pfnWriteByte(0);
		g_engfuncs.pfnMessageEnd();
	}
	else
	{
		g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgFlagTimer, 0, v25);
		g_engfuncs.pfnWriteByte(1);
		g_engfuncs.pfnWriteShort((int)(v2 - gpGlobals->time));
		g_engfuncs.pfnMessageEnd();
	}

	RecountTeams();

	if (pPlayer->m_iTeamNum > CTFTeam::None && OBS_NONE == pPlayer->pev->iuser1)
	{
		char text[1024];
		sprintf(text, "* you are on team '%s'\n", team_names[(int)pPlayer->m_iTeamNum - 1]);
		UTIL_SayText(text, pPlayer);
		ChangePlayerTeam(pPlayer, pPlayer->m_szTeamName, false, false);
	}

	RecountTeams();

	for (int iPlayer = 1; iPlayer <= gpGlobals->maxClients; ++iPlayer)
	{
		auto otherPlayer = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(iPlayer));

		if (otherPlayer)
		{
			if (IsValidTeam(otherPlayer->TeamID()))
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgTeamInfo, nullptr, pPlayer->edict());
				g_engfuncs.pfnWriteByte(otherPlayer->entindex());
				g_engfuncs.pfnWriteString(otherPlayer->TeamID());
				g_engfuncs.pfnWriteByte((int)otherPlayer->m_iTeamNum);
				g_engfuncs.pfnMessageEnd();
			}
		}
	}
}

void CHalfLifeCTFplay::ClientDisconnected(edict_t* pClient)
{
	if (pClient)
	{
		auto v2 = (CBasePlayer*)GET_PRIVATE(pClient);
		if (v2)
		{
			if (!g_fGameOver)
			{
				auto pFlag = static_cast<CTFGoalFlag*>(v2->m_pFlag.operator CBaseEntity*());

				if (pFlag)
				{
					pFlag->DropFlag(v2);
				}

				if ((v2->m_iItems & CTFItem::ItemsMask) != 0)
					ScatterPlayerCTFPowerups(v2);
			}
			v2->m_iTeamNum = CTFTeam::None;
			v2->m_iNewTeamNum = CTFTeam::None;
			FireTargets("game_playerleave", v2, v2, USE_TOGGLE, 0.0);

			UTIL_LogPrintf(
				"\"%s<%i><%u><%s>\" disconnected\n",
				STRING(v2->pev->netname),
				g_engfuncs.pfnGetPlayerUserId(v2->edict()),
				g_engfuncs.pfnGetPlayerWONId(v2->edict()),
				GetTeamName(v2->edict()));

			//TODO: doesn't seem to be used
			//free( v2->ip );
			//v2->ip = nullptr;

			free(pszPlayerIPs[g_engfuncs.pfnIndexOfEdict(pClient)]);

			pszPlayerIPs[g_engfuncs.pfnIndexOfEdict(pClient)] = nullptr;

			v2->RemoveAllItems(true);

			g_engfuncs.pfnMessageBegin(2, gmsgSpectator, 0, 0);
			g_engfuncs.pfnWriteByte(g_engfuncs.pfnIndexOfEdict(pClient));
			g_engfuncs.pfnWriteByte(0);
			g_engfuncs.pfnMessageEnd();

			for (CBasePlayer* pPlayer = nullptr; (pPlayer = static_cast<CBasePlayer*>(UTIL_FindEntityByClassname(pPlayer, "player")));)
			{
				if (FNullEnt(pPlayer->pev))
					break;

				if (pPlayer != v2 && pPlayer->m_hObserverTarget == v2)
				{
					const auto oldMode = pPlayer->pev->iuser1;

					pPlayer->pev->iuser1 = 0;

					pPlayer->m_hObserverTarget = nullptr;

					pPlayer->Observer_SetMode(oldMode);
				}
			}
		}
	}
}

void CHalfLifeCTFplay::UpdateGameMode(CBasePlayer* pPlayer)
{
	g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgGameMode, nullptr, pPlayer->edict());
	g_engfuncs.pfnWriteByte(2);
	g_engfuncs.pfnMessageEnd();
}

bool CHalfLifeCTFplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	if (pAttacker && PlayerRelationship(pPlayer, pAttacker) == GR_TEAMMATE && pAttacker != pPlayer && friendlyfire.value == 0)
		return false;

	return CHalfLifeMultiplay::FPlayerCanTakeDamage(pPlayer, pAttacker);
}

bool CHalfLifeCTFplay::ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target)
{
	auto v4 = CBaseEntity::Instance(target);

	if (v4 && v4->IsPlayer())
	{
		return PlayerRelationship(pPlayer, v4) != GR_TEAMMATE;
	}

	return true;
}

void CHalfLifeCTFplay::PlayerSpawn(CBasePlayer* pPlayer)
{
	if (pPlayer->m_iTeamNum != CTFTeam::None)
	{
		if (OBS_NONE == pPlayer->pev->iuser1)
		{
			const int savedAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
			pPlayer->m_iAutoWepSwitch = 1;
			pPlayer->SetHasSuit(true);

			auto useDefault = true;

			for (auto pEquip : UTIL_FindEntitiesByClassname("game_player_equip"))
			{
				useDefault = false;

				pEquip->Touch(pPlayer);
			}

			MESSAGE_BEGIN(MSG_ONE, gmsgOldWeapon, nullptr, pPlayer->edict());

			g_engfuncs.pfnWriteByte(static_cast<int>(1 == oldweapons.value));
			g_engfuncs.pfnMessageEnd();

			if (useDefault)
			{
				pPlayer->GiveNamedItem("weapon_pipewrench");
				pPlayer->GiveNamedItem("weapon_eagle");
				pPlayer->GiveNamedItem("weapon_grapple");
				pPlayer->GiveAmmo(DEAGLE_DEFAULT_GIVE * 3, "357", _357_MAX_CARRY);
			}

			for (auto pFlag : UTIL_FindEntitiesByClassname<CTFGoalFlag>("item_ctfflag"))
			{
				if (pFlag->m_iGoalState == 2)
				{
					pFlag->TurnOnLight(pPlayer);
				}
			}

			for (auto pBase : UTIL_FindEntitiesByClassname<CTFGoalBase>("item_ctfbase"))
			{
				pBase->TurnOnLight(pPlayer);
			}

			int r = 128;
			int g = 128;
			int b = 128;

			switch (pPlayer->m_iTeamNum)
			{
			case CTFTeam::BlackMesa:
				UnpackRGB(r, g, b, RGB_YELLOWISH);
				break;

			case CTFTeam::OpposingForce:
				UnpackRGB(r, g, b, RGB_HUD_COLOR);
				break;
			}

			g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgHudColor, nullptr, pPlayer->edict());
			g_engfuncs.pfnWriteByte(r);
			g_engfuncs.pfnWriteByte(g);
			g_engfuncs.pfnWriteByte(b);
			g_engfuncs.pfnMessageEnd();

			InitItemsForPlayer(pPlayer);
			DisplayTeamFlags(pPlayer);

			pPlayer->m_iAutoWepSwitch = savedAutoWepSwitch;
		}
	}
}

void CHalfLifeCTFplay::PlayerThink(CBasePlayer* pPlayer)
{
	const auto vecSrc = pPlayer->GetGunPosition();

	UTIL_MakeVectors(pPlayer->pev->v_angle);

	TraceResult tr;

	if (0 != pPlayer->m_iFOV)
	{
		UTIL_TraceLine(vecSrc, vecSrc + 4096 * gpGlobals->v_forward, dont_ignore_monsters, pPlayer->edict(), &tr);
	}
	else
	{
		UTIL_TraceLine(vecSrc, vecSrc + 1280.0 * gpGlobals->v_forward, dont_ignore_monsters, pPlayer->edict(), &tr);
	}

	if (0 != gmsgPlayerBrowse && tr.flFraction < 1.0 && pPlayer->m_iLastPlayerTrace != g_engfuncs.pfnIndexOfEdict(tr.pHit))
	{
		auto pOther = CBaseEntity::Instance(tr.pHit);

		if (!pOther->IsPlayer())
		{
			g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgPlayerBrowse, nullptr, pPlayer->edict());
			g_engfuncs.pfnWriteByte(0);
			g_engfuncs.pfnWriteByte(0);
			g_engfuncs.pfnWriteString("");
			g_engfuncs.pfnMessageEnd();
		}
		else
		{
			auto pOtherPlayer = static_cast<CBasePlayer*>(pOther);

			g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgPlayerBrowse, nullptr, pPlayer->edict());
			g_engfuncs.pfnWriteByte(static_cast<int>(pOtherPlayer->m_iTeamNum == pPlayer->m_iTeamNum));

			const auto v11 = OBS_NONE == pPlayer->pev->iuser1 ? pOtherPlayer->m_iTeamNum : CTFTeam::None;

			g_engfuncs.pfnWriteByte((int)v11);
			g_engfuncs.pfnWriteString(STRING(pOtherPlayer->pev->netname));
			g_engfuncs.pfnWriteByte((byte)pOtherPlayer->m_iItems);
			//Round health up to 0 to prevent wraparound
			g_engfuncs.pfnWriteByte((byte)V_max(0, pOtherPlayer->pev->health));
			g_engfuncs.pfnWriteByte((byte)pOtherPlayer->pev->armorvalue);
			g_engfuncs.pfnMessageEnd();
		}

		pPlayer->m_iLastPlayerTrace = g_engfuncs.pfnIndexOfEdict(tr.pHit);
	}
	CHalfLifeMultiplay::PlayerThink(pPlayer);
}

bool CHalfLifeCTFplay::ClientCommand(CBasePlayer* pPlayer, const char* pcmd)
{
	if (0 == strcmp("cancelmenu", pcmd))
	{
		if (pPlayer->m_iCurrentMenu == MENU_CLASS)
		{
			pPlayer->m_iCurrentMenu = MENU_TEAM;
			g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgTeamFull, nullptr, pPlayer->edict());
			g_engfuncs.pfnWriteByte(0);
			g_engfuncs.pfnMessageEnd();
			pPlayer->m_iNewTeamNum = CTFTeam::None;
			pPlayer->Player_Menu();
		}
		else if (pPlayer->m_iCurrentMenu == MENU_TEAM)
		{
			if (pPlayer->m_iNewTeamNum != CTFTeam::None || pPlayer->m_iTeamNum != CTFTeam::None)
			{
				pPlayer->m_iCurrentMenu = MENU_NONE;
			}
			else
			{
				pPlayer->Menu_Team_Input(-1);
			}
		}

		return true;
	}
	else if (0 == strcmp("endmotd", pcmd))
	{
		pPlayer->m_iCurrentMenu = MENU_TEAM;
		pPlayer->Player_Menu();
		return true;
	}
	else if (0 == strcmp("jointeam", pcmd))
	{
		pPlayer->Menu_Team_Input(atoi(CMD_ARGV(1)));
		return true;
	}
	else if (0 == strcmp("spectate", pcmd))
	{
		pPlayer->Menu_Team_Input(-1);
		return true;
	}
	else if (0 == strcmp("selectchar", pcmd))
	{
		if (g_engfuncs.pfnCmd_Argc() > 1)
		{
			pPlayer->Menu_Char_Input(atoi(CMD_ARGV(1)));
		}

		return true;
	}

	return false;
}

void CHalfLifeCTFplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	pPlayer->SetPrefsFromUserinfo(infobuffer);

	if (pPlayer->m_szTeamModel)
	{
		auto pszNewModel = g_engfuncs.pfnInfoKeyValue(infobuffer, "model");
		if (*pszNewModel != '=')
		{
			if (0 != strcmp(pszNewModel, pPlayer->m_szTeamModel))
			{
				if (std::any_of(team_chars.begin(), team_chars.end(), [=](auto pszChar)
						{ return 0 == strcmp(pPlayer->m_szTeamModel, pszChar); }))
				{
					auto pszPlayerName = STRING(pPlayer->pev->netname);
					if (pszPlayerName && '\0' != *pszPlayerName)
					{
						UTIL_LogPrintf("\"%s<%i><%u><%s>\" changed role to \"%s\"\n",
							pszPlayerName,
							g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
							g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
							GetTeamName(pPlayer->edict()),
							pPlayer->m_szTeamModel);
					}

					g_engfuncs.pfnSetClientKeyValue(pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", pPlayer->m_szTeamModel);
					UTIL_SayText("* Not allowed to change player model through the console in CTF!\n", pPlayer);
				}
			}
		}
	}
}

int CHalfLifeCTFplay::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	if (pKilled)
	{
		if (pAttacker)
		{
			if (pAttacker != pKilled)
				return 2 * static_cast<int>(PlayerRelationship(pAttacker, pKilled) != GR_TEAMMATE) - 1;
		}

		return 1;
	}

	return 0;
}

void CHalfLifeCTFplay::PlayerKilled(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor)
{
	if (!pVictim || OBS_NONE != pVictim->pev->iuser1)
		return;

	if (!m_DisableDeathPenalty && !g_fGameOver)
	{
		auto v5 = pKiller ? CBaseEntity::Instance<CBasePlayer>(pKiller) : nullptr;

		if (pVictim->m_pFlag)
		{
			if (v5 && v5->IsPlayer() && pVictim->m_iTeamNum != v5->m_iTeamNum)
			{
				++v5->m_iCTFScore;
				++v5->m_iDefense;

				MESSAGE_BEGIN(MSG_ALL, gmsgCTFScore);
				g_engfuncs.pfnWriteByte(v5->entindex());
				g_engfuncs.pfnWriteByte(v5->m_iCTFScore);
				g_engfuncs.pfnMessageEnd();

				++v5->m_iCTFScore;

				ClientPrint(v5->pev, HUD_PRINTTALK, "#CTFScorePoint");

				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s", STRING(v5->pev->netname)));

				if (pVictim->m_iTeamNum == CTFTeam::BlackMesa)
				{
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagKilledBM");
				}
				else if (pVictim->m_iTeamNum == CTFTeam::OpposingForce)
				{
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagKilledOF");
				}

				char szTeamName1[256];
				char szTeamName2[256];

				strncpy(szTeamName1, GetTeamName(pKiller->pContainingEntity), sizeof(szTeamName1));
				strncpy(szTeamName2, GetTeamName(pVictim->edict()), sizeof(szTeamName2));

				szTeamName1[sizeof(szTeamName1) - 1] = 0;
				szTeamName2[sizeof(szTeamName2) - 1] = 0;

				UTIL_LogPrintf(
					"\"%s<%i><%u><%s>\" triggered \"FlagDefense\" against \"%s<%i><%u><%s>\"\n",
					STRING(pKiller->netname),
					g_engfuncs.pfnGetPlayerUserId(pKiller->pContainingEntity),
					g_engfuncs.pfnGetPlayerWONId(pKiller->pContainingEntity),
					szTeamName1,
					STRING(pVictim->pev->netname),
					g_engfuncs.pfnGetPlayerUserId(pVictim->edict()),
					g_engfuncs.pfnGetPlayerWONId(pVictim->edict()),
					szTeamName2);
			}

			UTIL_LogPrintf(
				"\"%s<%i><%u><%s>\" triggered \"LostFlag\"\n",
				STRING(pVictim->pev->netname),
				g_engfuncs.pfnGetPlayerUserId(pVictim->edict()),
				g_engfuncs.pfnGetPlayerWONId(pVictim->edict()),
				GetTeamName(pVictim->edict()));
		}

		if (v5 && v5->IsPlayer())
		{
			const char* pszInflictorName = nullptr;

			if (pKiller == pInflictor)
			{
				if (v5->m_pActiveItem)
					pszInflictorName = CBasePlayerItem::ItemInfoArray[v5->m_pActiveItem->m_iId].pszName;
			}
			else if (pInflictor && !FStringNull(pInflictor->classname))
			{
				pszInflictorName = STRING(pInflictor->classname);
			}

			if (pszInflictorName)
			{
				if (0 == strcmp("weapon_sniperrifle", pszInflictorName) || 0 == strcmp("weapon_crossbow", pszInflictorName))
				{
					++v5->m_iSnipeKills;
				}
				else if (0 == strcmp("weapon_grapple", pszInflictorName))
				{
					++v5->m_iBarnacleKills;
				}
			}

			if (pKiller == pVictim->pev)
				++v5->m_iSuicides;

			for (auto pBase : UTIL_FindEntitiesByClassname<CTFGoalBase>("item_ctfbase"))
			{
				if (pKiller != pVictim->pev && pBase->m_iGoalNum == (int)v5->m_iTeamNum)
				{
					if (g_flBaseDefendDist >= (pVictim->pev->origin - pBase->pev->origin).Length())
					{
						++v5->m_iCTFScore;
						++v5->m_iDefense;

						MESSAGE_BEGIN(MSG_ALL, gmsgCTFScore);
						g_engfuncs.pfnWriteByte(v5->entindex());
						g_engfuncs.pfnWriteByte(v5->m_iCTFScore);
						g_engfuncs.pfnMessageEnd();

						ClientPrint(v5->pev, HUD_PRINTTALK, "#CTFScorePoint");

						UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s", STRING(v5->pev->netname)));

						if (v5->m_iTeamNum == CTFTeam::BlackMesa)
						{
							UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagDefendedBM");
						}
						else if (v5->m_iTeamNum == CTFTeam::OpposingForce)
						{
							UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagDefendedOF");
						}
						break;
					}
				}
			}

			if (pKiller != pVictim->pev && gpGlobals->maxClients > 0)
			{
				for (auto pPlayer : UTIL_FindPlayers())
				{
					auto pFlag = pPlayer->m_pFlag.Entity<CTFGoalFlag>();

					if (pFlag)
					{
						if (pPlayer->m_nLastShotBy == pVictim->entindex() && g_flDefendCarrierTime > gpGlobals->time - pPlayer->m_flLastShotTime)
						{
							++pPlayer->m_iCTFScore;
							++pPlayer->m_iDefense;

							MESSAGE_BEGIN(MSG_ALL, gmsgCTFScore);
							g_engfuncs.pfnWriteByte(pPlayer->entindex());
							g_engfuncs.pfnWriteByte(pPlayer->m_iCTFScore);
							g_engfuncs.pfnMessageEnd();

							ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#CTFScorePoint");
							UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s", STRING(pPlayer->pev->netname)));

							if (pVictim->m_iTeamNum == CTFTeam::BlackMesa)
							{
								UTIL_ClientPrintAll(1, "#CTFCarrierDefendedOF");
							}
							else if (pVictim->m_iTeamNum == CTFTeam::OpposingForce)
							{
								UTIL_ClientPrintAll(1, "#CTFCarrierDefendedBM");
							}
						}
					}
				}
			}
		}
	}

	CHalfLifeMultiplay::PlayerKilled(pVictim, pKiller, pInflictor);

	if (pVictim->IsPlayer() && !g_fGameOver)
	{
		auto pFlag = pVictim->m_pFlag.Entity<CTFGoalFlag>();

		if (pFlag)
		{
			pFlag->DropFlag(pVictim);
		}

		if ((pVictim->m_iItems & CTFItem::ItemsMask) != 0)
			ScatterPlayerCTFPowerups(pVictim);
	}
}

void CHalfLifeCTFplay::DeathNotice(CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor)
{
	if (!m_DisableDeathMessages)
	{
		if (pKiller && pVictim && (pKiller->flags & FL_CLIENT) != 0)
		{
			auto pEntKiller = CBaseEntity::Instance<CBasePlayer>(pKiller);

			if (pEntKiller && pVictim != pEntKiller && (PlayerRelationship(pVictim, pEntKiller) == GR_TEAMMATE))
			{
				MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
				g_engfuncs.pfnWriteByte(pEntKiller->entindex());
				g_engfuncs.pfnWriteByte(pVictim->entindex());
				g_engfuncs.pfnWriteString("teammate");
				g_engfuncs.pfnMessageEnd();
				return;
			}
		}

		CHalfLifeMultiplay::DeathNotice(pVictim, pKiller, pevInflictor);
	}
}

bool CHalfLifeCTFplay::CanHaveAmmo(CBasePlayer* pPlayer, const char* pszAmmoName, int iMaxCarry)
{
	if (pszAmmoName)
	{
		const auto ammoIndex = CBasePlayer::GetAmmoIndex(pszAmmoName);

		if (ammoIndex >= 0)
			return pPlayer->AmmoInventory(ammoIndex) < iMaxCarry;
	}

	return false;
}

const char* CHalfLifeCTFplay::GetTeamID(CBaseEntity* pEntity)
{
	if (pEntity && pEntity->pev)
		return pEntity->TeamID();

	return "";
}

int CHalfLifeCTFplay::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
{
	if (pTarget && pPlayer)
	{
		if (pTarget->IsPlayer())
			return static_cast<CBasePlayer*>(pPlayer)->m_iTeamNum == static_cast<CBasePlayer*>(pTarget)->m_iTeamNum ? GR_TEAMMATE : GR_NOTTEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

int CHalfLifeCTFplay::GetTeamIndex(const char* pTeamName)
{
	if (pTeamName && '\0' != *pTeamName)
	{
		for (int i = 0; i < MaxTeams; ++i)
		{
			if (!stricmp(team_names[i], pTeamName))
			{
				return i;
			}
		}
	}

	return -1;
}

const char* CHalfLifeCTFplay::GetIndexedTeamName(int teamIndex)
{
	if (teamIndex >= 0 && teamIndex < MaxTeams)
		return team_names[teamIndex];

	return "";
}

bool CHalfLifeCTFplay::IsValidTeam(const char* pTeamName)
{
	return GetTeamIndex(pTeamName) != -1;
}

void CHalfLifeCTFplay::ChangePlayerTeam(CBasePlayer* pPlayer, const char* pCharName, bool bKill, bool bGib)
{
	auto v5 = pPlayer->entindex();

	if (pCharName)
	{
		auto team = CTFTeam::None;

		if (std::any_of(team_chars.begin(), team_chars.begin() + team_chars.size() / 2, [=](auto pszChar)
				{ return !strcmp(pCharName, pszChar); }))
		{
			team = CTFTeam::BlackMesa;
		}
		else if (std::any_of(team_chars.begin() + team_chars.size() / 2, team_chars.end(), [=](auto pszChar)
					 { return !strcmp(pCharName, pszChar); }))
		{
			team = CTFTeam::OpposingForce;
		}
		else
		{
			return;
		}

		if (pPlayer->m_iTeamNum != team)
		{
			if (bKill)
			{
				m_DisableDeathMessages = true;
				m_DisableDeathPenalty = true;

				if (!g_fGameOver)
				{
					if (pPlayer->m_pFlag)
					{
						pPlayer->m_pFlag.Entity<CTFGoalFlag>()->DropFlag(pPlayer);
					}

					if ((pPlayer->m_iItems & CTFItem::ItemsMask) != 0)
					{
						ScatterPlayerCTFPowerups(pPlayer);
					}
				}

				const auto v7 = bGib ? DMG_ALWAYSGIB : DMG_NEVERGIB;

				pPlayer->TakeDamage(CWorld::World->pev, CWorld::World->pev, 900, v7);

				m_DisableDeathMessages = false;
				m_DisableDeathPenalty = false;
			}
			pPlayer->m_iTeamNum = team;
		}

		pPlayer->m_afPhysicsFlags &= ~PFLAG_OBSERVER;
		strncpy(pPlayer->m_szTeamName, team_names[(int)team - 1], sizeof(pPlayer->m_szTeamName));

		pPlayer->m_szTeamModel = (char*)pCharName;

		g_engfuncs.pfnSetClientKeyValue(v5, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "model", (char*)pCharName);
		g_engfuncs.pfnSetClientKeyValue(v5, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", pPlayer->m_szTeamName);

		//Send a ScoreInfo message so all clients have all of this client's info.
		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		g_engfuncs.pfnWriteByte(v5);
		g_engfuncs.pfnWriteShort((int)pPlayer->pev->frags);
		g_engfuncs.pfnWriteShort(pPlayer->m_iDeaths);
		g_engfuncs.pfnMessageEnd();

		MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
		g_engfuncs.pfnWriteByte(v5);
		g_engfuncs.pfnWriteString(pPlayer->m_szTeamName);
		g_engfuncs.pfnWriteByte((int)pPlayer->m_iTeamNum);
		g_engfuncs.pfnMessageEnd();

		//Reset FOV (could have been inherited from observer mode)
		pPlayer->m_iClientFOV = 0;
		pPlayer->m_iFOV = 0;

		g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgSetFOV, nullptr, pPlayer->edict());
		g_engfuncs.pfnWriteByte(0);
		g_engfuncs.pfnMessageEnd();

		const auto pszTeamName = GetTeamName(pPlayer->edict());

		UTIL_LogPrintf(
			"\"%s<%i><%u><%s>\" joined team \"%s\"\n",
			STRING(pPlayer->pev->netname),
			g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
			g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
			pszTeamName,
			pszTeamName);

		const auto v19 = STRING(pPlayer->pev->netname);

		if (v19 && '\0' != *v19)
		{
			UTIL_LogPrintf("\"%s<%i><%u><%s>\" changed role to \"%s\"\n",
				v19,
				g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
				g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
				pszTeamName,
				pCharName);
		}
	}
	else
	{
		if (pPlayer->pev->health <= 0.0)
		{
			respawn(pPlayer->pev, false);
		}

		pPlayer->pev->effects |= EF_NODRAW;
		pPlayer->pev->flags |= FL_SPECTATOR;
		pPlayer->pev->solid = SOLID_NOT;
		pPlayer->pev->movetype = MOVETYPE_NOCLIP;
		pPlayer->pev->takedamage = DAMAGE_NO;
		pPlayer->m_iHideHUD |= HIDEHUD_HEALTH | HIDEHUD_WEAPONS;
		pPlayer->m_afPhysicsFlags |= PFLAG_OBSERVER;
		pPlayer->m_iNewTeamNum = CTFTeam::None;
		pPlayer->m_iCurrentMenu = MENU_NONE;
		pPlayer->m_iTeamNum = CTFTeam::None;
		pPlayer->SetSuitUpdate(nullptr, false, SUIT_REPEAT_OK);
		pPlayer->m_iClientHealth = 100;

		g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgHealth, nullptr, pPlayer->edict());
		g_engfuncs.pfnWriteShort(pPlayer->m_iClientHealth);
		g_engfuncs.pfnMessageEnd();

		g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgCurWeapon, nullptr, pPlayer->edict());
		g_engfuncs.pfnWriteByte(0);
		g_engfuncs.pfnWriteByte(255);
		g_engfuncs.pfnWriteByte(255);
		g_engfuncs.pfnMessageEnd();

		pPlayer->m_iClientFOV = 0;
		pPlayer->m_iFOV = 0;

		g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgSetFOV, nullptr, pPlayer->edict());
		g_engfuncs.pfnWriteByte(0);
		g_engfuncs.pfnMessageEnd();

		if (pPlayer->m_pTank)
		{
			auto v42 = pPlayer->m_pTank.operator CBaseEntity*();

			v42->Use(pPlayer, pPlayer, USE_OFF, 0);

			pPlayer->m_pTank = nullptr;
		}

		pPlayer->m_afPhysicsFlags &= ~PFLAG_DUCKING;
		pPlayer->pev->flags &= ~FL_DUCKING;
		pPlayer->Observer_SetMode(OBS_CHASE_FREE);
		pPlayer->pev->deadflag = DEAD_RESPAWNABLE;

		if (pPlayer->HasNamedPlayerItem("weapon_satchel"))
			DeactivateSatchels(pPlayer);

		pPlayer->RemoveAllItems(false);

		if (!g_fGameOver)
		{
			if (pPlayer->m_pFlag)
			{
				pPlayer->m_pFlag.Entity<CTFGoalFlag>()->DropFlag(pPlayer);
			}

			if ((pPlayer->m_iItems & CTFItem::ItemsMask) != 0)
			{
				ScatterPlayerCTFPowerups(pPlayer);
			}
		}

		pPlayer->m_iDeaths = 0;
		pPlayer->m_iFlagCaptures = 0;
		pPlayer->pev->frags = 0;

		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		g_engfuncs.pfnWriteByte(pPlayer->entindex());
		g_engfuncs.pfnWriteShort((int)pPlayer->pev->frags);
		g_engfuncs.pfnWriteShort(pPlayer->m_iDeaths);
		g_engfuncs.pfnMessageEnd();

		pPlayer->m_szTeamName[0] = 0;

		g_engfuncs.pfnSetClientKeyValue(v5, g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()), "team", pPlayer->m_szTeamName);

		MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
		g_engfuncs.pfnWriteByte(v5);
		g_engfuncs.pfnWriteString(pPlayer->m_szTeamName);
		g_engfuncs.pfnWriteByte((int)pPlayer->m_iTeamNum);
		g_engfuncs.pfnMessageEnd();

		MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
		g_engfuncs.pfnWriteByte(pPlayer->entindex());
		g_engfuncs.pfnWriteByte(1);
		g_engfuncs.pfnMessageEnd();

		UTIL_LogPrintf(
			"\"%s<%i><%u><spectator>\" joined team \"spectator\"\n",
			STRING(pPlayer->pev->netname),
			g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
			g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()));
	}
}

const char* CHalfLifeCTFplay::SetDefaultPlayerTeam(CBasePlayer* pPlayer)
{
	auto pszTeamName = TeamWithFewestPlayers();
	auto teamIndex = GetTeamIndex(pszTeamName);

	const auto character = g_engfuncs.pfnRandomLong(0, team_chars.size() / 2);

	auto pszCharacter = team_chars[(character + team_chars.size() / 2 * teamIndex)];

	const auto killPlayer = pPlayer->pev->iuser1 == 0;

	ChangePlayerTeam(pPlayer, pszCharacter, killPlayer, killPlayer);

	return pszTeamName;
}

const char* CHalfLifeCTFplay::GetCharacterType(int iTeamNum, int iCharNum)
{
	if (iCharNum < (int)(team_chars.size() / 2))
		return team_chars[iCharNum + (team_chars.size() / 2 * iTeamNum)];

	return "";
}

int CHalfLifeCTFplay::GetNumTeams()
{
	return MaxTeams;
}

const char* CHalfLifeCTFplay::TeamWithFewestPlayers()
{
	int teamCount[MaxTeams] = {};

	for (auto pPlayer : UTIL_FindPlayers())
	{
		if (pPlayer->m_iTeamNum != CTFTeam::None)
		{
			++teamCount[((int)pPlayer->m_iTeamNum) - 1];
		}
	}

	if (teamCount[0] > 99)
	{
		teamCount[0] = 100;
	}

	if (teamCount[1] < teamCount[0])
	{
		return team_names[1];
	}

	return team_names[0];
}

bool CHalfLifeCTFplay::TeamsBalanced()
{
	int teamCount[MaxTeams] = {};

	for (auto pPlayer : UTIL_FindPlayers())
	{
		if (pPlayer->m_iTeamNum != CTFTeam::None)
		{
			++teamCount[((int)pPlayer->m_iTeamNum) - 1];
		}
	}

	return teamCount[0] == teamCount[1];
}

void CHalfLifeCTFplay::GoToIntermission()
{
	m_iStatsPhase = StatsPhase::SendTeam0;
	m_iStatsPlayer = 1;
	m_flNextStatsSend = gpGlobals->time + 0.2;

	CHalfLifeMultiplay::GoToIntermission();
}

template <typename... ARGS>
static void InternalSendTeamStat(int playerIndex, char* pszPlayerFormat, char* pszNobodyFormat, const char* pszStatName, const int value, ARGS&&... args)
{
	auto pPlayer = UTIL_PlayerByIndex(playerIndex);

	if (pPlayer && !FStringNull(pPlayer->pev->netname) && STRING(pPlayer->pev->netname))
	{
		char szBuf[21];

		strncpy(szBuf, STRING(pPlayer->pev->netname), sizeof(szBuf) - 1);
		szBuf[sizeof(szBuf) - 1] = 0;

		g_engfuncs.pfnWriteString(szBuf);

		auto pEdict = pPlayer->edict();

		UTIL_LogPrintf(
			pszPlayerFormat,
			pszStatName,
			STRING(pPlayer->pev->netname),
			g_engfuncs.pfnGetPlayerUserId(pEdict),
			g_engfuncs.pfnGetPlayerWONId(pEdict),
			GetTeamName(pEdict),
			args...);
	}
	else
	{
		g_engfuncs.pfnWriteString("*Nobody*");
		UTIL_LogPrintf(pszNobodyFormat, pszStatName, args...);
	}

	g_engfuncs.pfnWriteShort(value);
}

/**
*	@brief Send a team statistic
*	Was probably a variadic macro in the original
*/
static void SendTeamStat(int playerIndex, const char* pszStatName, const int value)
{
	InternalSendTeamStat(playerIndex, "// %-20s : \"%s<%i><%u><%s>\" for %d\n", "// %-20s : *Nobody* for %d\n", pszStatName, value, value);
}

/**
*	@brief Send a team time statistic
*	Was probably a variadic macro in the original
*/
static void SendTeamTimeStat(int playerIndex, const char* pszStatName, const float value)
{
	const auto time = SecondsToTime(value);

	InternalSendTeamStat(playerIndex, "// %-40s : \"%s<%i><%u><%s>\" for %d:%02d\n", "// %-40s : *Nobody* for %d:%02d\n", pszStatName, static_cast<short>(value), time.Minutes, time.Seconds);
}

void CHalfLifeCTFplay::SendTeamStatInfo(CTFTeam iTeamNum)
{
	auto iDamageVal = 0;
	auto iSuicidesVal = 0;
	auto iDeathsVal = 0;
	auto iBarnacleVal = 0;
	auto iSnipeVal = 0;
	auto iMostJump = 0;
	auto iMostDamage = 0;
	auto iMostBarnacle = 0;
	auto iMostSuicides = 0;
	auto iMostDeaths = 0;
	auto iMostSnipe = 0;
	auto iMostDef = 0;
	auto iFrags = 0;
	auto flJumpVal = 0.0f;
	auto iPts = 0;
	auto flShieldVal = 0.0f;
	auto iMostCTFPts = 0;
	auto flHealthVal = 0.0f;
	auto iMostFrags = 0;
	auto flBackpackVal = 0.0f;
	auto iMostShield = 0;
	auto iMostHealth = 0;
	auto iMostBackpack = 0;
	auto iMostAccel = 0;
	auto iMostPts = 0;
	auto flAccelVal = 0.0f;
	auto iMostOff = 0;
	auto iDefPts = 0;
	auto iOffPts = 0;
	auto iCTFPts = 0;
	auto iNumPlayers = 0;

	for (int iPlayer = 1; iPlayer <= gpGlobals->maxClients; ++iPlayer)
	{
		auto pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(iPlayer);
		if (pPlayer && pPlayer->IsPlayer() && (iTeamNum == CTFTeam::None || pPlayer->m_iTeamNum == iTeamNum))
		{
			if (FStringNull(pPlayer->pev->netname) || !STRING(pPlayer->pev->netname))
				continue;

			auto totalScore = pPlayer->m_iCTFScore + pPlayer->pev->frags;

			if (totalScore > iPts)
			{
				iMostPts = iPlayer;
				iPts = totalScore;
			}
			if (pPlayer->pev->frags > iFrags)
			{
				iMostFrags = iPlayer;
				iFrags = pPlayer->pev->frags;
			}
			if (pPlayer->m_iCTFScore > iCTFPts)
			{
				iCTFPts = pPlayer->m_iCTFScore;
				iMostCTFPts = iPlayer;
			}

			if (pPlayer->m_iOffense > iOffPts)
			{
				iMostOff = iPlayer;
				iOffPts = pPlayer->m_iOffense;
			}

			if (pPlayer->m_iDefense > iDefPts)
			{
				iMostDef = iPlayer;
				iDefPts = pPlayer->m_iDefense;
			}

			if (pPlayer->m_iSnipeKills > iSnipeVal)
			{
				iMostSnipe = iPlayer;
				iSnipeVal = pPlayer->m_iSnipeKills;
			}

			if (pPlayer->m_iBarnacleKills > iBarnacleVal)
			{
				iMostBarnacle = iPlayer;
				iBarnacleVal = pPlayer->m_iBarnacleKills;
			}
			if (pPlayer->m_iDeaths > iDeathsVal)
			{
				iMostDeaths = iPlayer;
				iDeathsVal = pPlayer->m_iDeaths;
			}

			if (pPlayer->m_iSuicides > iSuicidesVal)
			{
				iMostSuicides = iPlayer;
				iSuicidesVal = pPlayer->m_iSuicides;
			}

			if (pPlayer->m_iMostDamage > iDamageVal)
			{
				iMostDamage = iPlayer;
				iDamageVal = pPlayer->m_iMostDamage;
			}
			if (pPlayer->m_flAccelTime > flAccelVal)
			{
				flAccelVal = pPlayer->m_flAccelTime;
				iMostAccel = iPlayer;
			}
			if (pPlayer->m_flBackpackTime > flBackpackVal)
			{
				flBackpackVal = pPlayer->m_flBackpackTime;
				iMostBackpack = iPlayer;
			}
			if (pPlayer->m_flHealthTime > flHealthVal)
			{
				flHealthVal = pPlayer->m_flHealthTime;
				iMostHealth = iPlayer;
			}
			if (pPlayer->m_flShieldTime > flShieldVal)
			{
				flShieldVal = pPlayer->m_flShieldTime;
				iMostShield = iPlayer;
			}
			if (pPlayer->m_flJumpTime > flJumpVal)
			{
				flJumpVal = pPlayer->m_flJumpTime;
				iMostJump = iPlayer;
			}

			++iNumPlayers;

			if (iTeamNum != CTFTeam::None)
				continue;

			const auto accelTime = SecondsToTime(static_cast<int>(pPlayer->m_flAccelTime));
			const auto backpackTime = SecondsToTime(static_cast<int>(pPlayer->m_flBackpackTime));
			const auto healthTime = SecondsToTime(static_cast<int>(pPlayer->m_flHealthTime));
			const auto shieldTime = SecondsToTime(static_cast<int>(pPlayer->m_flShieldTime));
			const auto jumpTime = SecondsToTime(static_cast<int>(pPlayer->m_flJumpTime));

			UTIL_LogPrintf(
				"\"%s<%i><%u><%s>\" scored \"%d\" (kills \"%d\") (deaths \"%d\") (suicides \"%d\") (ctf_points \"%d\") (ctf_offense \"%d\") (ctf_defense \"%d\") (snipe_kills \"%d\") (barnacle_kills \"%d\") (best_damage \"%d\") (accel_time \"%0d:%02d\") (ammo_time \"%0d:%02d\") (health_time \"%0d:%02d\") (shield_time \"%0d:%02d\") (jump_time \"%0d:%02d\")\n",
				STRING(pPlayer->pev->netname),
				g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
				g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
				GetTeamName(pPlayer->edict()),
				(int)totalScore,
				(int)pPlayer->pev->frags,
				pPlayer->m_iDeaths,
				pPlayer->m_iSuicides,
				pPlayer->m_iCTFScore,
				pPlayer->m_iOffense,
				pPlayer->m_iDefense,
				pPlayer->m_iSnipeKills,
				pPlayer->m_iBarnacleKills,
				pPlayer->m_iMostDamage,
				accelTime.Minutes,
				accelTime.Seconds,
				backpackTime.Minutes,
				backpackTime.Seconds,
				healthTime.Minutes,
				healthTime.Seconds,
				shieldTime.Minutes,
				shieldTime.Seconds,
				jumpTime.Minutes,
				jumpTime.Seconds);
		}
	}

	if (iTeamNum != CTFTeam::None && (ToTeamIndex(iTeamNum)) < MaxTeams)
	{
		UTIL_LogPrintf("// === Team %s Statistics ===\n", team_names[ToTeamIndex(iTeamNum)]);
	}
	else
	{
		//TODO: player count is always 0
		for (auto i = 0; i < MaxTeams; ++i)
		{
			UTIL_LogPrintf("Team \"%s\" scored \"%d\" with \"%d\" players\n", team_names[i], teamscores[i], 0);
		}

		UTIL_LogPrintf("// === End-Game Overall Statistics ===\n");

		auto winningTeam = GetWinningTeam();

		if (CTFTeam::None == winningTeam)
		{
			UTIL_LogPrintf("// %-25s : %s\n", "Winning Team", "Draw");
			UTIL_LogPrintf("World triggered \"Draw\"\n");
		}
		else
		{
			auto pszTeam = team_names[ToTeamIndex(winningTeam)];

			UTIL_LogPrintf("// %-25s : %s\n", "Winning Team", pszTeam);
			UTIL_LogPrintf("Team \"%s\" triggered \"Victory\"\n", pszTeam);
		}
	}

	g_engfuncs.pfnMessageBegin(2, gmsgStatsInfo, 0, 0);
	g_engfuncs.pfnWriteByte(static_cast<int>(iTeamNum));

	g_engfuncs.pfnWriteByte((int)GetWinningTeam());
	g_engfuncs.pfnWriteByte(iNumPlayers);
	g_engfuncs.pfnWriteByte(0);

	SendTeamStat(iMostPts, "Most Points", iPts);
	SendTeamStat(iMostFrags, "Most Frags", iFrags);
	SendTeamStat(iMostCTFPts, "Most CTF Points", iCTFPts);
	SendTeamStat(iMostOff, "Most Offense", iOffPts);
	SendTeamStat(iMostDef, "Most Defense", iDefPts);
	SendTeamStat(iMostSnipe, "Most Sniper Kills", iSnipeVal);
	SendTeamStat(iMostBarnacle, "Most Barnacle Kills", iBarnacleVal);
	g_engfuncs.pfnMessageEnd();

	g_engfuncs.pfnMessageBegin(2, gmsgStatsInfo, 0, 0);
	g_engfuncs.pfnWriteByte(static_cast<int>(iTeamNum));

	g_engfuncs.pfnWriteByte((int)GetWinningTeam());
	g_engfuncs.pfnWriteByte(iNumPlayers);
	g_engfuncs.pfnWriteByte(1);

	SendTeamStat(iMostDeaths, "Most Deaths", iDeathsVal);
	SendTeamStat(iMostSuicides, "Most Suicides", iSuicidesVal);
	SendTeamStat(iMostDamage, "Most Damage", iDamageVal);
	SendTeamTimeStat(iMostAccel, "Most Damage Powerup Time", flAccelVal);
	SendTeamTimeStat(iMostBackpack, "Most Backpack Powerup Time", flBackpackVal);
	SendTeamTimeStat(iMostHealth, "Most Health Powerup Time", flHealthVal);
	SendTeamTimeStat(iMostShield, "Most Shield Powerup Time", flShieldVal);
	SendTeamTimeStat(iMostJump, "Most Jumppack Powerup Time", flJumpVal);
	g_engfuncs.pfnMessageEnd();
}

void CHalfLifeCTFplay::SendPlayerStatInfo(CBasePlayer* pPlayer)
{
	g_engfuncs.pfnMessageBegin(
		MSG_ONE,
		gmsgStatsPlayer,
		0,
		pPlayer->edict());

	g_engfuncs.pfnWriteByte(pPlayer->entindex());

	g_engfuncs.pfnWriteShort((int)(pPlayer->m_iCTFScore + pPlayer->pev->frags));
	g_engfuncs.pfnWriteShort((int)pPlayer->pev->frags);
	g_engfuncs.pfnWriteShort(pPlayer->m_iCTFScore);
	g_engfuncs.pfnWriteShort(pPlayer->m_iOffense);
	g_engfuncs.pfnWriteShort(pPlayer->m_iDefense);
	g_engfuncs.pfnWriteShort(pPlayer->m_iSnipeKills);
	g_engfuncs.pfnWriteShort(pPlayer->m_iBarnacleKills);
	g_engfuncs.pfnWriteShort(pPlayer->m_iDeaths);
	g_engfuncs.pfnWriteShort(pPlayer->m_iSuicides);
	g_engfuncs.pfnWriteShort(pPlayer->m_iMostDamage);
	g_engfuncs.pfnWriteShort((short)pPlayer->m_flAccelTime);
	g_engfuncs.pfnWriteShort((short)pPlayer->m_flBackpackTime);
	g_engfuncs.pfnWriteShort((short)pPlayer->m_flHealthTime);
	g_engfuncs.pfnWriteShort((short)pPlayer->m_flShieldTime);
	g_engfuncs.pfnWriteShort((short)pPlayer->m_flJumpTime);
	g_engfuncs.pfnMessageEnd();
}

void CHalfLifeCTFplay::RecountTeams()
{
	team_count[1] = 0;
	team_count[0] = 0;

	for (int iPlayer = 1; iPlayer <= gpGlobals->maxClients; ++iPlayer)
	{
		auto pPlayer = UTIL_PlayerByIndex(iPlayer);

		if (pPlayer)
		{
			auto pszTeam = pPlayer->TeamID();

			auto teamIndex = GetTeamIndex(pszTeam);

			if (teamIndex >= 0)
			{
				++team_count[teamIndex];
			}
		}
	}
}
