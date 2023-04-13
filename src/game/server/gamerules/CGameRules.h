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

#include <memory>
#include <string_view>

#include <spdlog/logger.h>

#include "ClientCommandRegistry.h"

class CBaseItem;
class CBaseMonster;
class CBasePlayer;
class CBasePlayerWeapon;

/**
 *	@brief weapon respawning return codes
 */
enum
{
	GR_NONE = 0,

	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,

	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO,
};

/**
 *	@brief Player relationship return codes
 */
enum
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL,
};

#define ITEM_RESPAWN_TIME 30
#define WEAPON_RESPAWN_TIME 20
#define AMMO_RESPAWN_TIME 20

/**
 *	@brief when we are within this close to running out of entities,
 *	items marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn.
 */
#define ENTITY_INTOLERANCE 100

constexpr int ChargerRechargeDelayNever = -1;

/**
 *	@brief Manages the rules for the current game mode.
 *	@details Each subclass should define a <tt>static constexpr std::string_view GameModeName</tt> member and
 *		override @c GetGameModeName.
 */
class CGameRules
{
public:
	static inline std::shared_ptr<spdlog::logger> Logger;

	CGameRules();
	virtual ~CGameRules() = default;

	virtual std::string_view GetGameModeName() const = 0;

	/**
	 *	@brief runs every server frame, should handle any timer tasks, periodic events, etc.
	 */
	virtual void Think() = 0;

	/**
	 *	@brief Can this item spawn (eg monsters don't spawn in deathmatch).
	 */
	virtual bool IsAllowedToSpawn(CBaseEntity* pEntity) = 0;

	/**
	 *	@brief Are players allowed to switch on their flashlight
	 */
	virtual bool FAllowFlashlight() = 0;

	/**
	 *	@brief should the player switch to this weapon?
	 */
	virtual bool FShouldSwitchWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pWeapon) = 0;

	/**
	 *	@brief I can't use this weapon anymore, get me the next best one.
	 */
	virtual bool GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon, bool alwaysSearch = false);

	// Functions to verify the single/multiplayer status of a game

	/**
	 *	@brief is this a multiplayer game? (either coop or deathmatch)
	 */
	virtual bool IsMultiplayer() = 0;

	/**
	 *	@brief is this a deathmatch game?
	 */
	virtual bool IsDeathmatch() = 0;

	/**
	 *	@brief is this deathmatch game being played with team rules?
	 */
	virtual bool IsTeamplay() { return false; }

	/**
	 *	@brief is this a coop game?
	 */
	virtual bool IsCoOp() = 0;

	/**
	 *	@brief is this a ctf game?
	 */
	virtual bool IsCTF() = 0;

	/**
	 *	@brief this is the game name that gets seen in the server browser
	 */
	virtual const char* GetGameDescription() { return "Half-Life"; }

	// Client connection/disconnection
	/**
	 *	@brief a client just connected to the server (player hasn't spawned yet)
	 *	If ClientConnected returns false, the connection is rejected and the user is provided the reason specified in @c szRejectReason.
	 *	Only the client's name and remote address are provided to the dll for verification.
	 */
	virtual bool ClientConnected(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]) = 0;

	/**
	 *	@brief the client dll is ready for updating
	 */
	virtual void InitHUD(CBasePlayer* pl) = 0;

	/**
	 *	@brief a client just disconnected from the server
	 */
	virtual void ClientDisconnected(edict_t* pClient) = 0;

	/**
	 *	@brief the client needs to be informed of the current game mode
	 */
	virtual void UpdateGameMode(CBasePlayer* pPlayer) {}

	// Client damage rules
	/**
	 *	@brief this client just hit the ground after a fall. How much damage?
	 */
	virtual float FlPlayerFallDamage(CBasePlayer* pPlayer) = 0;

	/**
	 *	@brief can this player take damage from this attacker?
	 */
	virtual bool FPlayerCanTakeDamage(CBasePlayer* pPlayer, CBaseEntity* pAttacker) { return true; }

	virtual bool ShouldAutoAim(CBasePlayer* pPlayer, CBaseEntity* target) { return true; }

	// Client spawn/respawn control
	/**
	 *	@brief called by CBasePlayer::Spawn just before releasing player into the game
	 */
	virtual void PlayerSpawn(CBasePlayer* pPlayer) = 0;

	/**
	 *	@brief called by CBasePlayer::PreThink every frame, before physics are run and after keys are accepted
	 */
	virtual void PlayerThink(CBasePlayer* pPlayer) = 0;

	/**
	 *	@brief is this player allowed to respawn now?
	 */
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) = 0;

	/**
	 *	@brief When in the future will this player be able to spawn?
	 */
	virtual float FlPlayerSpawnTime(CBasePlayer* pPlayer) = 0;

	/**
	 *	@brief Place this player on their spawnspot and face them the proper direction.
	 */
	virtual CBaseEntity* GetPlayerSpawnSpot(CBasePlayer* pPlayer);

	virtual bool AllowAutoTargetCrosshair() { return true; }

	/**
	 *	@brief the player has changed userinfo;  can change it now
	 */
	virtual void ClientUserInfoChanged(CBasePlayer* pPlayer, char* infobuffer) {}

	// Client kills/scoring
	/**
	 *	@brief how many points do I award whoever kills this player?
	 */
	virtual int IPointsForKill(CBasePlayer* pAttacker, CBasePlayer* pKilled) = 0;

	virtual int IPointsForMonsterKill(CBasePlayer* pAttacker, CBaseMonster* pKiller) { return 0; }

	/**
	 *	@brief Called each time a player dies
	 */
	virtual void PlayerKilled(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) = 0;

	/**
	 *	@brief Call this from within a GameRules class to report an obituary.
	 */
	virtual void DeathNotice(CBasePlayer* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) = 0;

	virtual void MonsterKilled(CBaseMonster* pVictim, CBaseEntity* pKiller, CBaseEntity* inflictor) {}

	// Item retrieval
	/**
	 *	@brief is this player allowed to take this item?
	 */
	virtual bool CanHaveItem(CBasePlayer* player, CBaseItem* item);

	/**
	 *	@brief call each time a player picks up an item (battery, healthkit, longjump)
	 */
	virtual void PlayerGotItem(CBasePlayer* player, CBaseItem* item) = 0;

	// Item spawn/respawn control
	/**
	 *	@brief Should this item respawn?
	 */
	virtual bool ItemShouldRespawn(CBaseItem* item) = 0;

	/**
	 *	@brief when may this item respawn?
	 */
	virtual float ItemRespawnTime(CBaseItem* item) = 0;

	/**
	 *	@brief where in the world should this item respawn?
	 *	by default, everything spawns where placed by the designer.
	 *	Some game variations may choose to randomize spawn locations
	 */
	// Ammo retrieval
	/**
	 *	@brief can this player take more of this ammo?
	 */
	virtual bool CanHaveAmmo(CBasePlayer* pPlayer, const char* pszAmmoName);
	virtual Vector ItemRespawnSpot(CBaseItem* item) = 0;

	/**
	 *	@brief can i respawn now, and if not, when should i try again?
	 *	Returns 0 if the item can respawn now, otherwise it returns the time at which it can try to spawn again.
	 */
	virtual float ItemTryRespawn(CBaseItem* item) = 0;

	/**
	 *	@brief how long until a depleted HealthCharger recharges itself?
	 */
	virtual int HealthChargerRechargeTime() { return ChargerRechargeDelayNever; }

	/**
	 *	@brief how long until a depleted HEV Charger recharges itself?
	 */
	virtual int HEVChargerRechargeTime() { return ChargerRechargeDelayNever; }

	/**
	 *	@brief what do I do with a player's weapons when he's killed?
	 */
	virtual int DeadPlayerWeapons(CBasePlayer* pPlayer) = 0;

	/**
	 *	@brief Do I drop ammo when the player dies? How much?
	 */
	virtual int DeadPlayerAmmo(CBasePlayer* pPlayer) = 0;

	/**
	 *	@brief what team is this entity on?
	 */
	virtual const char* GetTeamID(CBaseEntity* pEntity) = 0;

	/**
	 *	@brief What is the player's relationship with this entity?
	 */
	virtual int PlayerRelationship(CBasePlayer* pPlayer, CBaseEntity* pTarget) = 0;

	virtual int GetTeamIndex(const char* pTeamName) { return -1; }
	virtual const char* GetIndexedTeamName(int teamIndex) { return ""; }
	virtual bool IsValidTeam(const char* pTeamName) { return true; }
	virtual void ChangePlayerTeam(CBasePlayer* pPlayer, const char* pTeamName, bool bKill, bool bGib) {}
	virtual const char* SetDefaultPlayerTeam(CBasePlayer* pPlayer) { return ""; }

	virtual const char* GetCharacterType(int iTeamNum, int iCharNum) { return ""; }
	virtual int GetNumTeams() { return 0; }
	virtual const char* TeamWithFewestPlayers() { return nullptr; }
	virtual bool TeamsBalanced() { return true; }

	// Sounds
	virtual bool PlayTextureSounds() { return true; }
	virtual bool PlayFootstepSounds(CBasePlayer* pl, float fvol) { return true; }

	// Monsters
	/**
	 *	@brief are monsters allowed
	 */
	virtual bool FAllowMonsters() = 0;

	/**
	 *	@brief Immediately end a multiplayer game
	 */
	virtual void EndMultiplayerGame() {}

protected:
	CBasePlayerWeapon* FindNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerWeapon* pCurrentWeapon);

	virtual void BecomeSpectator(CBasePlayer* player, const CommandArgs& args);

private:
	ScopedClientCommand m_SpectateCommand;
	ScopedClientCommand m_SpecModeCommand;
};

/**
 *	@brief instantiate the proper game rules object
 */
CGameRules* InstallGameRules(std::string_view gameModeName);

const char* GameModeIndexToString(int index);

void PrintMultiplayerGameModes();

inline CGameRules* g_pGameRules = nullptr;
inline bool g_fGameOver;

const char* GetTeamName(CBasePlayer* pEntity);

class GameRulesCanHaveItemVisitor : public IItemVisitor
{
public:
	explicit GameRulesCanHaveItemVisitor(CGameRules* gameRules, CBasePlayer* player)
		: GameRules(gameRules),
		  Player(player)
	{
	}

	void Visit(CBasePlayerAmmo* ammo) override {}

	void Visit(CBasePlayerWeapon* weapon) override;

	void Visit(CItem* pickupItem) override {}

	CGameRules* const GameRules;
	CBasePlayer* const Player;
	bool CanHaveItem = true;
};
