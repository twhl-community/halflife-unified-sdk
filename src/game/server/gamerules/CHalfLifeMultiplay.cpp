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

#include "cbase.h"
#include "CHalfLifeMultiplay.h"

#include "items.h"
#include "voice_gamemgr.h"
#include "hltv.h"
#include "UserMessages.h"

#include "CHalfLifeCTFplay.h"
#include "MapCycleSystem.h"
#include "PlayerInventory.h"
#include "items/CBaseItem.h"
#include "items/weapons/CSatchelCharge.h"

CVoiceGameMgr g_VoiceGameMgr;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	bool CanPlayerHearPlayer(CBasePlayer* pListener, CBasePlayer* pTalker) override
	{
		if (g_pGameRules->IsTeamplay())
		{
			if (g_pGameRules->PlayerRelationship(pListener, pTalker) != GR_TEAMMATE)
			{
				return false;
			}
		}

		return true;
	}
};
static CMultiplayGameMgrHelper g_GameMgrHelper;

CHalfLifeMultiplay::CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);
}

CHalfLifeMultiplay::~CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Shutdown();
}

// longest the intermission can last, in seconds
constexpr int MAX_INTERMISSION_TIME = 120;

static int GetChatTime()
{
	// bounds check
	const int time = int(mp_chattime.value);

	const int clampedTime = std::clamp(time, 1, MAX_INTERMISSION_TIME);

	if (time != clampedTime)
	{
		g_engfuncs.pfnCvar_DirectSet(&mp_chattime, UTIL_ToString(clampedTime).c_str());
	}

	return clampedTime;
}

void CHalfLifeMultiplay::Think()
{
	g_VoiceGameMgr.Update(gpGlobals->frametime);

	///// Check game rules /////
	if (g_fGameOver) // someone else quit the game already
	{
		m_flIntermissionEndTime = m_flIntermissionStartTime + GetChatTime();

		// check to see if we should change levels now
		if (m_flIntermissionEndTime < gpGlobals->time)
		{
			if (m_iEndIntermissionButtonHit // check that someone has pressed a key, or the max intermission time is over
				|| ((m_flIntermissionStartTime + MAX_INTERMISSION_TIME) < gpGlobals->time))
				ChangeLevel(); // intermission is over
		}

		return;
	}

	const float flTimeLimit = timelimit.value * 60;

	if (flTimeLimit != 0 && gpGlobals->time >= flTimeLimit)
	{
		GoToIntermission();
		return;
	}

	const float flFragLimit = fraglimit.value;

	int frags_remaining = 0;

	if (0 != flFragLimit)
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

			if (pPlayer && pPlayer->pev->frags >= flFragLimit)
			{
				GoToIntermission();
				return;
			}


			if (pPlayer)
			{
				remain = flFragLimit - pPlayer->pev->frags;
				if (remain < bestfrags)
				{
					bestfrags = remain;
				}
			}
		}
		frags_remaining = bestfrags;
	}

	static int last_frags;

	// Updates when frags change
	if (frags_remaining != last_frags)
	{
		g_engfuncs.pfnCvar_DirectSet(&fragsleft, UTIL_ToString(frags_remaining).c_str());
	}

	const int time_remaining = (int)(0 != flTimeLimit ? (flTimeLimit - gpGlobals->time) : 0);

	static int last_time;

	// Updates once per second
	if (timeleft.value != last_time)
	{
		g_engfuncs.pfnCvar_DirectSet(&timeleft, UTIL_ToString(time_remaining).c_str());
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}

bool CHalfLifeMultiplay::FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
	if (!pWeapon->CanDeploy())
	{
		// that weapon can't deploy anyway.
		return false;
	}

	if (!pPlayer->m_pActiveWeapon)
	{
		// player doesn't have an active item!
		return true;
	}

	if (!pPlayer->m_pActiveWeapon->CanHolster())
	{
		// can't put away the active item.
		return false;
	}

	// Never switch
	if (pPlayer->m_AutoWepSwitch == WeaponSwitchMode::Never)
	{
		return false;
	}

	// Only switch if not attacking
	if (pPlayer->m_AutoWepSwitch == WeaponSwitchMode::IfBetter && (pPlayer->m_afButtonLast & (IN_ATTACK | IN_ATTACK2)) != 0)
	{
		return false;
	}

	if (pWeapon->iWeight() > pPlayer->m_pActiveWeapon->iWeight())
	{
		return true;
	}

	return false;
}

bool CHalfLifeMultiplay::ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	g_VoiceGameMgr.ClientConnected(pEntity);

	int playersInTeamsCount = 0;

	for (int iPlayer = 1; iPlayer <= gpGlobals->maxClients; ++iPlayer)
	{
		auto pPlayer = UTIL_PlayerByIndex(iPlayer);

		if (pPlayer)
		{
			playersInTeamsCount += (pPlayer->m_iTeamNum > CTFTeam::None) ? 1 : 0;
		}
	}

	if (playersInTeamsCount <= 1)
	{
		for (int iPlayer = 1; iPlayer <= gpGlobals->maxClients; ++iPlayer)
		{
			auto pPlayer = UTIL_PlayerByIndex(iPlayer);

			if (pPlayer && pPlayer->m_iItems != CTFItem::None)
			{
				if ((pPlayer->m_iItems & CTFItem::ItemsMask) != 0)
				{
					RespawnPlayerCTFPowerups(pPlayer, true);
				}

				ClientPrint(pPlayer, HUD_PRINTCENTER, "#CTFGameReset");
			}
		}
	}

	if (pEntity)
	{
		// TODO: really shouldn't be modifying this directly
		auto portNumber = strchr(const_cast<char*>(pszAddress), ':');

		if (portNumber)
			*portNumber = '\0';

		pszPlayerIPs[ENTINDEX(pEntity)] = strdup(pszAddress);
	}

	return true;
}

void CHalfLifeMultiplay::UpdateGameMode(CBasePlayer* pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, nullptr, pPlayer);
	WRITE_BYTE(0); // game mode none
	MESSAGE_END();
}

void CHalfLifeMultiplay::InitHUD(CBasePlayer* pl)
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s has joined the game\n",
											 (!FStringNull(pl->pev->netname) && STRING(pl->pev->netname)[0] != 0) ? STRING(pl->pev->netname) : "unconnected"));

	Logger->trace("{} entered the game", PlayerLogInfo{*pl});

	UpdateGameMode(pl);

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	pl->SendScoreInfo(pl);

	SendMOTDToClient(pl);

	if (IsCTF())
	{
		pl->m_iCurrentMenu = MENU_TEAM;
		pl->Player_Menu();
	}

	// loop through all active players and send their score info to the new client
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer* plr = UTIL_PlayerByIndex(i);

		if (plr)
		{
			plr->SendScoreInfo(pl);
		}
	}

	if (g_fGameOver)
	{
		MESSAGE_BEGIN(MSG_ONE, SVC_INTERMISSION, nullptr, pl);
		MESSAGE_END();
	}
}

void CHalfLifeMultiplay::ClientDisconnected(edict_t* pClient)
{
	if (pClient)
	{
		CBasePlayer* pPlayer = ToBasePlayer(pClient);

		if (pPlayer)
		{
			if (!g_fGameOver && (pPlayer->m_iItems & CTFItem::ItemsMask) != 0)
				ScatterPlayerCTFPowerups(pPlayer);

			FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

			Logger->trace("{} disconnected", PlayerLogInfo{*pPlayer});

			const int playerIndex = pPlayer->entindex();

			free(pszPlayerIPs[playerIndex]);
			pszPlayerIPs[playerIndex] = nullptr;

			pPlayer->RemoveAllItems(true); // destroy all of the players weapons and items

			MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
			g_engfuncs.pfnWriteByte(pPlayer->entindex());
			g_engfuncs.pfnWriteByte(0);
			g_engfuncs.pfnMessageEnd();

			for (auto entity : UTIL_FindPlayers())
			{
				if (entity->pev && entity != pPlayer && entity->m_hObserverTarget == pPlayer)
				{
					const int savedIUser1 = entity->pev->iuser1;
					entity->pev->iuser1 = 0;
					entity->m_hObserverTarget = nullptr;
					entity->Observer_SetMode(savedIUser1);
				}
			}
		}
	}
}

bool CHalfLifeMultiplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	return true;
}

void CHalfLifeMultiplay::PlayerThink(CBasePlayer* pPlayer)
{
	if ((pPlayer->m_iItems & CTFItem::PortableHEV) != 0)
	{
		if (pPlayer->m_flNextHEVCharge <= gpGlobals->time)
		{
			if (pPlayer->pev->armorvalue < 150)
			{
				pPlayer->pev->armorvalue += 1;

				if (!pPlayer->m_fPlayingAChargeSound)
				{
					pPlayer->EmitSound(CHAN_STATIC, "ctf/pow_armor_charge.wav", VOL_NORM, ATTN_NORM);
					pPlayer->m_fPlayingAChargeSound = true;
				}
			}
			else if (pPlayer->m_fPlayingAChargeSound)
			{
				pPlayer->StopSound(CHAN_STATIC, "ctf/pow_armor_charge.wav");
				pPlayer->m_fPlayingAChargeSound = false;
			}

			pPlayer->m_flNextHEVCharge = gpGlobals->time + 0.5;
		}
	}
	else if (pPlayer->pev->armorvalue > 100 && pPlayer->m_flNextHEVCharge <= gpGlobals->time)
	{
		pPlayer->pev->armorvalue -= 1;
		pPlayer->m_flNextHEVCharge = gpGlobals->time + 0.5;
	}

	if ((pPlayer->m_iItems & CTFItem::Regeneration) != 0)
	{
		if (pPlayer->m_flNextHealthCharge <= gpGlobals->time)
		{
			if (pPlayer->pev->health < 150.0)
			{
				pPlayer->pev->health += 1;

				if (!pPlayer->m_fPlayingHChargeSound)
				{
					pPlayer->EmitSound(CHAN_STATIC, "ctf/pow_health_charge.wav", VOL_NORM, ATTN_NORM);
					pPlayer->m_fPlayingHChargeSound = true;
				}
			}
			else if (pPlayer->m_fPlayingHChargeSound)
			{
				pPlayer->StopSound(CHAN_STATIC, "ctf/pow_health_charge.wav");
				pPlayer->m_fPlayingHChargeSound = false;
			}

			pPlayer->m_flNextHealthCharge = gpGlobals->time + 0.5;
		}
	}
	else if (pPlayer->pev->health > 100.0 && gpGlobals->time >= pPlayer->m_flNextHealthCharge)
	{
		pPlayer->pev->health -= 1;
		pPlayer->m_flNextHealthCharge = gpGlobals->time + 0.5;
	}

	if (pPlayer->m_pActiveWeapon && pPlayer->m_flNextAmmoCharge <= gpGlobals->time && (pPlayer->m_iItems & CTFItem::Backpack) != 0)
	{
		pPlayer->m_pActiveWeapon->IncrementAmmo(pPlayer);
		pPlayer->m_flNextAmmoCharge = gpGlobals->time + 0.75;
	}

	if (gpGlobals->time - pPlayer->m_flLastDamageTime > 0.15)
	{
		if (pPlayer->m_iMostDamage < pPlayer->m_iLastDamage)
			pPlayer->m_iMostDamage = pPlayer->m_iLastDamage;

		pPlayer->m_flLastDamageTime = 0;
		pPlayer->m_iLastDamage = 0;
	}

	if (g_fGameOver)
	{
		// check for button presses
		if ((pPlayer->m_afButtonPressed & (IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP)) != 0)
			m_iEndIntermissionButtonHit = true;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

void CHalfLifeMultiplay::PlayerSpawn(CBasePlayer* pPlayer)
{
	CGameRules::PlayerSpawn(pPlayer);

	InitItemsForPlayer(pPlayer);
}

bool CHalfLifeMultiplay::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return true;
}

float CHalfLifeMultiplay::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return gpGlobals->time; // now!
}

bool CHalfLifeMultiplay::AllowAutoTargetCrosshair()
{
	return (aimcrosshair.value != 0);
}

int CHalfLifeMultiplay::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	return 1;
}

void CHalfLifeMultiplay::PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor)
{
	DeathNotice(pVictim, pKiller, inflictor);

	pVictim->m_iDeaths += 1;

	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);
	CBasePlayer* peKiller = ToBasePlayer(pKiller);

	if (pVictim == pKiller)
	{ // killed self
		pKiller->pev->frags -= 1;
	}
	else if (peKiller)
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->pev->frags += IPointsForKill(peKiller, pVictim);

		FireTargets("game_playerkill", peKiller, peKiller, USE_TOGGLE, 0);
	}
	else
	{ // killed by the world
		pKiller->pev->frags -= 1;
	}

	// update the scores
	// killed scores
	pVictim->SendScoreInfoAll();

	// killers score, if it's a player
	if (peKiller)
	{
		peKiller->SendScoreInfoAll();

		// let the killer paint another decal as soon as he'd like.
		peKiller->m_flNextDecalTime = gpGlobals->time;
	}

	if (pVictim->HasNamedPlayerWeapon("weapon_satchel"))
	{
		DeactivateSatchels(pVictim);
	}

	if (pVictim->IsPlayer() && !g_fGameOver && (pVictim->m_iItems & CTFItem::ItemsMask) != 0)
	{
		ScatterPlayerCTFPowerups(pVictim);
	}
}

void CHalfLifeMultiplay::DeathNotice(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor)
{
	// Work out what killed the player, and send a message to all clients about it
	const char* killer_weapon_name = "world"; // by default, the player is killed by the world
	int killer_index = 0;

	// Hack to fix name change
	const char* tau = "tau_cannon";
	const char* gluon = "gluon gun";

	if ((pKiller->pev->flags & FL_CLIENT) != 0)
	{
		killer_index = pKiller->entindex();

		if (inflictor)
		{
			if (inflictor == pKiller)
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer* pPlayer = ToBasePlayer(pKiller);

				if (pPlayer->m_pActiveWeapon)
				{
					killer_weapon_name = pPlayer->m_pActiveWeapon->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING(inflictor->pev->classname); // it's just that easy
			}
		}
	}
	else
	{
		killer_weapon_name = STRING(inflictor->pev->classname);
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if (strncmp(killer_weapon_name, "ARgr", 4) == 0)
		killer_weapon_name += 2;
	else if (strncmp(killer_weapon_name, "weapon_", 7) == 0)
		killer_weapon_name += 7;
	else if (strncmp(killer_weapon_name, "monster_", 8) == 0)
		killer_weapon_name += 8;
	else if (strncmp(killer_weapon_name, "func_", 5) == 0)
		killer_weapon_name += 5;

	MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
	WRITE_BYTE(killer_index);		  // the killer
	WRITE_BYTE(pVictim->entindex());  // the victim
	WRITE_STRING(killer_weapon_name); // what they were killed by (should this be a string?)
	MESSAGE_END();

	// replace the code names with the 'real' names
	if (0 == strcmp(killer_weapon_name, "egon"))
		killer_weapon_name = gluon;
	else if (0 == strcmp(killer_weapon_name, "gauss"))
		killer_weapon_name = tau;

	if (pVictim == pKiller)
	{
		// killed self
		Logger->trace("{} committed suicide with \"{}\"", PlayerLogInfo{*pVictim}, killer_weapon_name);
	}
	else if ((pKiller->pev->flags & FL_CLIENT) != 0)
	{
		CBasePlayer* Killer = ToBasePlayer(pKiller);
		Logger->trace("{} killed {} with \"{}\"", PlayerLogInfo{*Killer}, PlayerLogInfo{*pVictim}, killer_weapon_name);
	}
	else
	{
		// killed by the world
		Logger->trace("{} committed suicide with \"{}\" (world)", PlayerLogInfo{*pVictim}, killer_weapon_name);
	}

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);					  // command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);		  // player killed
	WRITE_SHORT(pVictim->entindex()); // index number of primary entity
	if (inflictor)
		WRITE_SHORT(inflictor->entindex()); // index number of secondary entity
	else
		WRITE_SHORT(pKiller->entindex()); // index number of secondary entity
	WRITE_LONG(7 | DRC_FLAG_DRAMATIC);	  // eventflags (priority and flags)
	MESSAGE_END();

	//  Print a standard message
	// TODO: make this go direct to console
	return; // just remove for now
			/*
	char	szText[ 128 ];
		
	if ( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was killed by a monster.\n" );
		return;
	}
		
	if ( pKiller == pVictim->pev )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " commited suicide.\n" );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		strcpy ( szText, STRING( pKiller->netname ) );
		
		strcat( szText, " : " );
		strcat( szText, killer_weapon_name );
		strcat( szText, " : " );
		
		strcat ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, "\n" );
	}
	else if (pKiller->ClassnameIs("worldspawn"))
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " fell or drowned or something.\n" );
	}
	else if ( pKiller->solid == SOLID_BSP )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was mooshed.\n" );
	}
	else
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " died mysteriously.\n" );
	}
		
	UTIL_ClientPrintAll( szText );
*/
}

void CHalfLifeMultiplay::PlayerGotItem(CBasePlayer* player, CBaseItem* item)
{
}

bool CHalfLifeMultiplay::IsAllowedToSpawn(CBaseEntity* pEntity)
{
	//	if ( pEntity->pev->flags & FL_MONSTER )
	//		return false;

	return true;
}

int CHalfLifeMultiplay::DeadPlayerWeapons(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

int CHalfLifeMultiplay::DeadPlayerAmmo(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

CBaseEntity* CHalfLifeMultiplay::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	CBaseEntity* pSpawnSpot = CGameRules::GetPlayerSpawnSpot(pPlayer);
	if (IsMultiplayer() && !FStringNull(pSpawnSpot->pev->target))
	{
		FireTargets(STRING(pSpawnSpot->pev->target), pPlayer, pPlayer, USE_TOGGLE, 0);
	}

	return pSpawnSpot;
}

int CHalfLifeMultiplay::PlayerRelationship(CBasePlayer* pPlayer, CBaseEntity* pTarget)
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

bool CHalfLifeMultiplay::PlayFootstepSounds(CBasePlayer* pl, float fvol)
{
	if (g_footsteps && g_footsteps->value == 0)
		return false;

	if (pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220)
		return true; // only make step sounds in multiplayer if the player is moving fast enough

	return false;
}

void CHalfLifeMultiplay::GoToIntermission()
{
	if (g_fGameOver)
		return; // intermission has already been triggered, so ignore.

	FlushCTFPowerupTimes();

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	m_flIntermissionEndTime = gpGlobals->time + GetChatTime();
	m_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = true;
	m_iEndIntermissionButtonHit = false;
}

/**
 *	@brief Determine the current # of active players on the server for map cycling logic
 */
int CountPlayers()
{
	int num = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pEnt = UTIL_PlayerByIndex(i);

		if (pEnt)
		{
			num = num + 1;
		}
	}

	return num;
}

void CHalfLifeMultiplay::ChangeLevel()
{
	const int curplayers = CountPlayers();

	const MapCycleItem* item = g_MapCycleSystem.GetMapCycle()->GetNextMap(curplayers);

	eastl::fixed_string<char, cchMapNameMost> nextMap;

	if (item && IS_MAP_VALID(item->MapName.c_str()))
	{
		nextMap = item->MapName;
	}
	else
	{
		// Fall back to restarting the current map if we can't determine which map to switch to.
		nextMap = STRING(gpGlobals->mapname);
	}

	g_fGameOver = true;

	CGameRules::Logger->debug("CHANGE LEVEL: {}", nextMap.c_str());

	if (item && (item->MinPlayers != 0 || item->MaxPlayers != 0))
	{
		CGameRules::Logger->debug("PLAYER COUNT:  min {} max {} current {}", item->MinPlayers, item->MaxPlayers, curplayers);
	}

	if (m_ClearGlobalState)
	{
		ResetGlobalState();
	}

	CHANGE_LEVEL(nextMap.c_str(), nullptr);
}

#define MAX_MOTD_CHUNK 60
#define MAX_MOTD_LENGTH 1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay::SendMOTDToClient(CBasePlayer* player)
{
	// read from the MOTD.txt file
	const auto fileContents = FileSystem_LoadFileIntoBuffer(CVAR_GET_STRING("motdfile"), FileContentFormat::Text);

	int char_count = 0;
	const char* const aFileList = reinterpret_cast<const char*>(fileContents.data());
	const char* pFileList = aFileList;

	// send the server name
	MESSAGE_BEGIN(MSG_ONE, gmsgServerName, nullptr, player);
	WRITE_STRING(CVAR_GET_STRING("hostname"));
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while (pFileList && '\0' != *pFileList && char_count < MAX_MOTD_LENGTH)
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		if (strlen(pFileList) < MAX_MOTD_CHUNK)
		{
			strcpy(chunk, pFileList);
		}
		else
		{
			strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
			chunk[MAX_MOTD_CHUNK] = 0; // strncpy doesn't always append the null terminator
		}

		char_count += strlen(chunk);

		pFileList = aFileList + char_count;

		const bool moreToCome = pFileList[0] != '\0' && char_count < MAX_MOTD_LENGTH;

		MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, nullptr, player);
		WRITE_BYTE(static_cast<int>(!moreToCome)); // false means there is still more message to come
		WRITE_STRING(chunk);
		MESSAGE_END();
	}
}
