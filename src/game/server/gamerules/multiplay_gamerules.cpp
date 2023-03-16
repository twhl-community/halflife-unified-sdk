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
//
// teamplay_gamerules.cpp
//
#include "cbase.h"
#include "gamerules.h"

#include "items.h"
#include "voice_gamemgr.h"
#include "hltv.h"
#include "UserMessages.h"

#include "ctfplay_gamerules.h"

CVoiceGameMgr g_VoiceGameMgr;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	bool CanPlayerHearPlayer(CBasePlayer* pListener, CBasePlayer* pTalker) override
	{
		if (g_teamplay)
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

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay::CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);

	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)

	// This has been deprecated and replaced with JSON-based config files.
	// See ServerLibrary::LoadServerConfigFiles
	/*
	if (IS_DEDICATED_SERVER())
	{
		// this code has been moved into engine, to only run server.cfg once
	}
	else
	{
		// listen server
		const char* lservercfgfile = CVAR_GET_STRING("lservercfgfile");

		if (lservercfgfile && lservercfgfile[0])
		{
			char szCommand[256];

			Logger->debug("Executing listen server config file");
			sprintf(szCommand, "exec %s\n", lservercfgfile);
			SERVER_COMMAND(szCommand);
		}
	}
	*/
}

CHalfLifeMultiplay::~CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Shutdown();
}

void CHalfLifeMultiplay::ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer)
{
	pPlayer->SetPrefsFromUserinfo(infobuffer);
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME 120

//=========================================================
//=========================================================
void CHalfLifeMultiplay::Think()
{
	g_VoiceGameMgr.Update(gpGlobals->frametime);

	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if (g_fGameOver) // someone else quit the game already
	{
		// bounds check
		int time = (int)CVAR_GET_FLOAT("mp_chattime");
		if (time < 1)
			CVAR_SET_STRING("mp_chattime", "1");
		else if (time > MAX_INTERMISSION_TIME)
			CVAR_SET_STRING("mp_chattime", UTIL_ToString(MAX_INTERMISSION_TIME).c_str());

		m_flIntermissionEndTime = m_flIntermissionStartTime + mp_chattime.value;

		// check to see if we should change levels now
		if (m_flIntermissionEndTime < gpGlobals->time)
		{
			if (m_iEndIntermissionButtonHit // check that someone has pressed a key, or the max intermission time is over
				|| ((m_flIntermissionStartTime + MAX_INTERMISSION_TIME) < gpGlobals->time))
				ChangeLevel(); // intermission is over
		}

		return;
	}

	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;

	time_remaining = (int)(0 != flTimeLimit ? (flTimeLimit - gpGlobals->time) : 0);

	if (flTimeLimit != 0 && gpGlobals->time >= flTimeLimit)
	{
		GoToIntermission();
		return;
	}

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

	// Updates when frags change
	if (frags_remaining != last_frags)
	{
		g_engfuncs.pfnCvar_DirectSet(&fragsleft, UTIL_VarArgs("%i", frags_remaining));
	}

	// Updates once per second
	if (timeleft.value != last_time)
	{
		g_engfuncs.pfnCvar_DirectSet(&timeleft, UTIL_VarArgs("%i", time_remaining));
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}


//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsMultiplayer()
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsDeathmatch()
{
	return true;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsCoOp()
{
	return 0 != gpGlobals->coop;
}

//=========================================================
//=========================================================
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
	if (pPlayer->m_iAutoWepSwitch == 0)
	{
		return false;
	}

	// Only switch if not attacking
	if (pPlayer->m_iAutoWepSwitch == 2 && (pPlayer->m_afButtonLast & (IN_ATTACK | IN_ATTACK2)) != 0)
	{
		return false;
	}

	if (pWeapon->iWeight() > pPlayer->m_pActiveWeapon->iWeight())
	{
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
	g_VoiceGameMgr.ClientConnected(pEntity);

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
				if ((pPlayer->m_iItems & CTFItem::ItemsMask) != 0)
				{
					RespawnPlayerCTFPowerups(pPlayer, true);
				}

				ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#CTFGameReset");
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
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, nullptr, pPlayer->edict());
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

	SendMOTDToClient(pl->edict());

	if (IsCTF())
	{
		pl->m_iCurrentMenu = MENU_TEAM;
		pl->Player_Menu();
	}

	// loop through all active players and send their score info to the new client
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer* plr = (CBasePlayer*)UTIL_PlayerByIndex(i);

		if (plr)
		{
			plr->SendScoreInfo(pl);
		}
	}

	if (g_fGameOver)
	{
		MESSAGE_BEGIN(MSG_ONE, SVC_INTERMISSION, nullptr, pl->edict());
		MESSAGE_END();
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::ClientDisconnected(edict_t* pClient)
{
	if (pClient)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance(pClient);

		if (pPlayer)
		{
			if (!g_fGameOver && (pPlayer->m_iItems & CTFItem::ItemsMask) != 0)
				ScatterPlayerCTFPowerups(pPlayer);

			FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);

			Logger->trace("{} disconnected", PlayerLogInfo{*pPlayer});

			const int playerIndex = ENTINDEX(pClient);

			free(pszPlayerIPs[playerIndex]);
			pszPlayerIPs[playerIndex] = nullptr;

			pPlayer->RemoveAllItems(true); // destroy all of the players weapons and items

			g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgSpectator, nullptr, nullptr);
			g_engfuncs.pfnWriteByte(ENTINDEX(pClient));
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

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerFallDamage(CBasePlayer* pPlayer)
{
	int iFallDamage = (int)falldamage.value;

	switch (iFallDamage)
	{
	case 1: // progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0: // fixed
		return 10;
		break;
	}
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker)
{
	return true;
}

//=========================================================
//=========================================================
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

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerSpawn(CBasePlayer* pPlayer)
{
	bool addDefault;
	CBaseEntity* pWeaponEntity = nullptr;

	// Ensure the player switches to the Glock on spawn regardless of setting
	const int originalAutoWepSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;

	pPlayer->SetHasSuit(true);

	addDefault = true;

	while (pWeaponEntity = UTIL_FindEntityByClassname(pWeaponEntity, "game_player_equip"))
	{
		pWeaponEntity->Touch(pPlayer);
		addDefault = false;
	}

	if (addDefault)
	{
		pPlayer->GiveNamedItem("weapon_crowbar");
		pPlayer->GiveNamedItem("weapon_9mmhandgun");
		pPlayer->GiveAmmo(68, "9mm"); // 4 full reloads
	}

	InitItemsForPlayer(pPlayer);

	pPlayer->m_iAutoWepSwitch = originalAutoWepSwitch;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FPlayerCanRespawn(CBasePlayer* pPlayer)
{
	return true;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerSpawnTime(CBasePlayer* pPlayer)
{
	return gpGlobals->time; // now!
}

bool CHalfLifeMultiplay::AllowAutoTargetCrosshair()
{
	return (aimcrosshair.value != 0);
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeMultiplay::IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled)
{
	return 1;
}


//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeMultiplay::PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor)
{
	DeathNotice(pVictim, pKiller, inflictor);

	pVictim->m_iDeaths += 1;


	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);
	CBasePlayer* peKiller = nullptr;
	CBaseEntity* ktmp = CBaseEntity::Instance(pKiller);
	if (ktmp && ktmp->IsPlayer())
		peKiller = (CBasePlayer*)ktmp;

	if (pVictim == pKiller)
	{ // killed self
		pKiller->pev->frags -= 1;
	}
	else if (ktmp && ktmp->IsPlayer())
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->pev->frags += IPointsForKill(peKiller, pVictim);

		FireTargets("game_playerkill", ktmp, ktmp, USE_TOGGLE, 0);
	}
	else
	{ // killed by the world
		pKiller->pev->frags -= 1;
	}

	// update the scores
	// killed scores
	pVictim->SendScoreInfoAll();

	// killers score, if it's a player
	CBaseEntity* ep = CBaseEntity::Instance(pKiller);
	if (ep && ep->IsPlayer())
	{
		CBasePlayer* PK = (CBasePlayer*)ep;

		PK->SendScoreInfoAll();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
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

//=========================================================
// Deathnotice.
//=========================================================
void CHalfLifeMultiplay::DeathNotice(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor)
{
	// Work out what killed the player, and send a message to all clients about it
	CBasePlayer* Killer = static_cast<CBasePlayer*>(CBaseEntity::Instance(pKiller));

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
				CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance(pKiller);

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
	WRITE_BYTE(killer_index);				// the killer
	WRITE_BYTE(ENTINDEX(pVictim->edict())); // the victim
	WRITE_STRING(killer_weapon_name);		// what they were killed by (should this be a string?)
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
		Logger->trace("{} killed {} with \"{}\"", PlayerLogInfo{*Killer}, PlayerLogInfo{*pVictim}, killer_weapon_name);
	}
	else
	{
		// killed by the world
		Logger->trace("{} committed suicide with committed suicide with \"{}\" (world)", PlayerLogInfo{*pVictim}, killer_weapon_name);
	}

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);							 // command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);				 // player killed
	WRITE_SHORT(ENTINDEX(pVictim->edict())); // index number of primary entity
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
	else if ( FClassnameIs ( pKiller, "worldspawn" ) )
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

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay::PlayerGotWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon)
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeMultiplay::FlWeaponRespawnTime(CBasePlayerWeapon* pWeapon)
{
	if (weaponstay.value > 0)
	{
		// make sure it's only certain weapons
		if ((pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) == 0)
		{
			return gpGlobals->time + 0; // weapon respawns almost instantly
		}
	}

	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeMultiplay::FlWeaponTryRespawn(CBasePlayerWeapon* pWeapon)
{
	if (pWeapon && WEAPON_NONE != pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) != 0)
	{
		if (NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE))
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime(pWeapon);
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecWeaponRespawnSpot(CBasePlayerWeapon* pWeapon)
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay::WeaponShouldRespawn(CBasePlayerWeapon* pWeapon)
{
	if ((pWeapon->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CHalfLifeMultiplay::CanHavePlayerWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pItem)
{
	if (weaponstay.value > 0)
	{
		if ((pItem->iFlags() & ITEM_FLAG_LIMITINWORLD) != 0)
			return CGameRules::CanHavePlayerWeapon(pPlayer, pItem);

		// check if the player already has this weapon
		for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
		{
			CBasePlayerWeapon* it = pPlayer->m_rgpPlayerWeapons[i];

			while (it != nullptr)
			{
				if (it->m_iId == pItem->m_iId)
				{
					return false;
				}

				it = it->m_pNext;
			}
		}
	}

	return CGameRules::CanHavePlayerWeapon(pPlayer, pItem);
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::CanHaveItem(CBasePlayer* pPlayer, CItem* pItem)
{
	return true;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem(CBasePlayer* pPlayer, CItem* pItem)
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn(CItem* pItem)
{
	if ((pItem->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime(CItem* pItem)
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot(CItem* pItem)
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo(CBasePlayer* pPlayer, char* szName, int iCount)
{
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::IsAllowedToSpawn(CBaseEntity* pEntity)
{
	//	if ( pEntity->pev->flags & FL_MONSTER )
	//		return false;

	return true;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn(CBasePlayerAmmo* pAmmo)
{
	if ((pAmmo->pev->spawnflags & SF_NORESPAWN) != 0)
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime(CBasePlayerAmmo* pAmmo)
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot(CBasePlayerAmmo* pAmmo)
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime()
{
	return 60;
}


float CHalfLifeMultiplay::FlHEVChargerRechargeTime()
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons(CBasePlayer* pPlayer)
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
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


//=========================================================
//=========================================================
int CHalfLifeMultiplay::PlayerRelationship(CBaseEntity* pPlayer, CBaseEntity* pTarget)
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

bool CHalfLifeMultiplay::FAllowFlashlight()
{
	return flashlight.value != 0;
}

//=========================================================
//=========================================================
bool CHalfLifeMultiplay::FAllowMonsters()
{
	return (allowmonsters.value != 0);
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME 6

void CHalfLifeMultiplay::GoToIntermission()
{
	if (g_fGameOver)
		return; // intermission has already been triggered, so ignore.

	FlushCTFPowerupTimes();

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	// bounds check
	int time = (int)CVAR_GET_FLOAT("mp_chattime");
	if (time < 1)
		CVAR_SET_STRING("mp_chattime", "1");
	else if (time > MAX_INTERMISSION_TIME)
		CVAR_SET_STRING("mp_chattime", UTIL_ToString(MAX_INTERMISSION_TIME).c_str());

	m_flIntermissionEndTime = gpGlobals->time + ((int)mp_chattime.value);
	m_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = true;
	m_iEndIntermissionButtonHit = false;
}

#define MAX_RULE_BUFFER 1024

struct mapcycle_item_t
{
	mapcycle_item_t* next;

	char mapname[32];
	int minplayers, maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
};

struct mapcycle_t
{
	mapcycle_item_t* items;
	mapcycle_item_t* next_item;
};

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle(mapcycle_t* cycle)
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if (p)
	{
		start = p;
		p = p->next;
		while (p != start)
		{
			n = p->next;
			delete p;
			p = n;
		}

		delete cycle->items;
	}
	cycle->items = nullptr;
	cycle->next_item = nullptr;
}

/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
bool ReloadMapCycleFile(const char* filename, mapcycle_t* cycle)
{
	char szBuffer[MAX_RULE_BUFFER];
	char szMap[32];

	const auto fileContents = FileSystem_LoadFileIntoBuffer(filename, FileContentFormat::Text);

	bool hasbuffer;
	mapcycle_item_t *item, *newlist = nullptr, *next;

	if (fileContents.size() > 1)
	{
		const char* pFileList = reinterpret_cast<const char*>(fileContents.data());

		// the first map name in the file becomes the default
		while (true)
		{
			hasbuffer = false;
			memset(szBuffer, 0, MAX_RULE_BUFFER);

			pFileList = COM_Parse(pFileList);
			if (strlen(com_token) <= 0)
				break;

			strcpy(szMap, com_token);

			// Any more tokens on this line?
			if (COM_TokenWaiting(pFileList))
			{
				pFileList = COM_Parse(pFileList);
				if (strlen(com_token) > 0)
				{
					hasbuffer = true;
					strcpy(szBuffer, com_token);
				}
			}

			// Check map
			if (IS_MAP_VALID(szMap))
			{
				// Create entry
				char* s;

				item = new mapcycle_item_t;

				strcpy(item->mapname, szMap);

				item->minplayers = 0;
				item->maxplayers = 0;

				memset(item->rulebuffer, 0, MAX_RULE_BUFFER);

				if (hasbuffer)
				{
					s = g_engfuncs.pfnInfoKeyValue(szBuffer, "minplayers");
					if (s && '\0' != s[0])
					{
						item->minplayers = atoi(s);
						item->minplayers = V_max(item->minplayers, 0);
						item->minplayers = V_min(item->minplayers, gpGlobals->maxClients);
					}
					s = g_engfuncs.pfnInfoKeyValue(szBuffer, "maxplayers");
					if (s && '\0' != s[0])
					{
						item->maxplayers = atoi(s);
						item->maxplayers = V_max(item->maxplayers, 0);
						item->maxplayers = V_min(item->maxplayers, gpGlobals->maxClients);
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey(szBuffer, "minplayers");
					g_engfuncs.pfnInfo_RemoveKey(szBuffer, "maxplayers");

					strcpy(item->rulebuffer, szBuffer);
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				CGameRules::Logger->debug("Skipping {} from mapcycle, not a valid map", szMap);
			}
		}
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while (item)
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if (!item)
	{
		return false;
	}

	while (item->next)
	{
		item = item->next;
	}
	item->next = cycle->items;

	cycle->next_item = item->next;

	return true;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
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

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString(char* s, char* szCommand)
{
	// Now make rules happen
	char pkey[512];
	char value[512]; // use two buffers so compares
					 // work without stomping on each other
	char* o;

	if (*s == '\\')
		s++;

	while (true)
	{
		o = pkey;
		while (*s != '\\')
		{
			if ('\0' == *s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && '\0' != *s)
		{
			if ('\0' == *s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat(szCommand, pkey);
		if (strlen(value) > 0)
		{
			strcat(szCommand, " ");
			strcat(szCommand, value);
		}
		strcat(szCommand, "\n");

		if ('\0' == *s)
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay::ChangeLevel()
{
	static char szPreviousMapCycleFile[256];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[1500];
	char szRules[1500];
	int minplayers = 0, maxplayers = 0;
	strcpy(szFirstMapInList, "hldm1"); // the absolute default level is hldm1

	int curplayers;
	bool do_cycle = true;

	// find the map to change to
	const char* mapcfile = CVAR_GET_STRING("mapcyclefile");
	ASSERT(mapcfile != nullptr);

	szCommands[0] = '\0';
	szRules[0] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
	if (stricmp(mapcfile, szPreviousMapCycleFile))
	{
		strcpy(szPreviousMapCycleFile, mapcfile);

		DestroyMapCycle(&mapcycle);

		if (!ReloadMapCycleFile(mapcfile, &mapcycle) || (!mapcycle.items))
		{
			CGameRules::Logger->debug("Unable to load map cycle file {}", mapcfile);
			do_cycle = false;
		}
	}

	if (do_cycle && mapcycle.items)
	{
		bool keeplooking = false;
		bool found = false;
		mapcycle_item_t* item;

		// Assume current map
		strcpy(szNextMap, STRING(gpGlobals->mapname));
		strcpy(szFirstMapInList, STRING(gpGlobals->mapname));

		// Traverse list
		for (item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next)
		{
			keeplooking = false;

			ASSERT(item != nullptr);

			if (item->minplayers != 0)
			{
				if (curplayers >= item->minplayers)
				{
					found = true;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if (item->maxplayers != 0)
			{
				if (curplayers <= item->maxplayers)
				{
					found = true;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = true;
				}
			}

			if (keeplooking)
				continue;

			found = true;
			break;
		}

		if (!found)
		{
			item = mapcycle.next_item;
		}

		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy(szNextMap, item->mapname);

		ExtractCommandString(item->rulebuffer, szCommands);
		strcpy(szRules, item->rulebuffer);
	}

	if (!IS_MAP_VALID(szNextMap))
	{
		strcpy(szNextMap, szFirstMapInList);
	}

	g_fGameOver = true;

	CGameRules::Logger->debug("CHANGE LEVEL: {}", szNextMap);
	if (0 != minplayers || 0 != maxplayers)
	{
		CGameRules::Logger->debug("PLAYER COUNT:  min {} max {} current {}", minplayers, maxplayers, curplayers);
	}
	if (strlen(szRules) > 0)
	{
		CGameRules::Logger->debug("RULES:  {}", szRules);
	}

	CHANGE_LEVEL(szNextMap, nullptr);
	if (strlen(szCommands) > 0)
	{
		SERVER_COMMAND(szCommands);
	}
}

#define MAX_MOTD_CHUNK 60
#define MAX_MOTD_LENGTH 1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay::SendMOTDToClient(edict_t* client)
{
	// read from the MOTD.txt file
	const auto fileContents = FileSystem_LoadFileIntoBuffer(CVAR_GET_STRING("motdfile"), FileContentFormat::Text);

	int char_count = 0;
	const char* const aFileList = reinterpret_cast<const char*>(fileContents.data());
	const char* pFileList = aFileList;

	// send the server name
	MESSAGE_BEGIN(MSG_ONE, gmsgServerName, nullptr, client);
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

		MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, nullptr, client);
		WRITE_BYTE(static_cast<int>(!moreToCome)); // false means there is still more message to come
		WRITE_STRING(chunk);
		MESSAGE_END();
	}
}
