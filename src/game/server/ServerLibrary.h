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
#include <string>
#include <unordered_set>

#include "GameLibrary.h"
#include "MapState.h"

#include "config/GameConfig.h"

class CBasePlayer;

/**
*	@brief Handles core server actions
*/
class ServerLibrary final : public GameLibrary
{
public:
	ServerLibrary();
	~ServerLibrary();

	ServerLibrary(const ServerLibrary&) = delete;
	ServerLibrary& operator=(const ServerLibrary&) = delete;
	ServerLibrary(ServerLibrary&&) = delete;
	ServerLibrary& operator=(ServerLibrary&&) = delete;

	MapState* GetMapState() { return &m_MapState; }

	/**
	*	@brief Handles server-side initialization
	*	@return Whether initialization succeeded
	*/
	bool Initialize();

	/**
	*	@brief Handles server-side shutdown
	*	@details Called even if @see Initialize returned false
	*/
	void Shutdown();

	/**
	*	@brief Should only ever be called by worldspawn's destructor
	*/
	void MapIsEnding()
	{
		m_IsStartingNewMap = true;
	}

	bool CheckForNewMapStart(bool loadGame)
	{
		if (m_IsStartingNewMap)
		{
			m_IsStartingNewMap = false;
			NewMapStarted(loadGame);
			return true;
		}

		return false;
	}

	/**
	*	@brief Called right before entities are activated
	*/
	void PreMapActivate();

	/**
	*	@brief Called right after entities are activated
	*/
	void PostMapActivate();

	/**
	*	@brief Called when the player activates (first UpdateClientData call after ClientPutInServer or Restore).
	*/
	void PlayerActivating(CBasePlayer* player);

protected:
	void AddGameSystems() override;

private:
	/**
	*	@brief Called when a new map has started
	*	@param loadGame Whether this is a save game being loaded or a new map being started
	*/
	void NewMapStarted(bool loadGame);

	void CreateConfigDefinitions();

	void LoadConfigFile(const char* fileName, const GameConfigDefinition& definition, GameConfigLoadParameters parameters = {});

	void LoadServerConfigFiles();

	void LoadMapChangeConfigFile();

	std::unordered_set<std::string> GetMapConfigCommandWhitelist();

private:
	std::shared_ptr<const GameConfigDefinition> m_ServerConfigDefinition;
	std::shared_ptr<const GameConfigDefinition> m_MapConfigDefinition;
	std::shared_ptr<const GameConfigDefinition> m_MapChangeConfigDefinition;

	bool m_IsStartingNewMap = true;

	MapState m_MapState;
};

inline ServerLibrary g_Server;
