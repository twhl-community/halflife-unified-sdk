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

#include "CGameLibrary.h"
#include "CMapState.h"

#include "config/GameConfigLoader.h"

#include "utils/json_utils.h"

class GameConfigDefinition;

/**
*	@brief Handles core server actions
*/
class CServerLibrary final : public CGameLibrary
{
public:
	CServerLibrary();
	~CServerLibrary();

	CServerLibrary(const CServerLibrary&) = delete;
	CServerLibrary& operator=(const CServerLibrary&) = delete;
	CServerLibrary(CServerLibrary&&) = delete;
	CServerLibrary& operator=(CServerLibrary&&) = delete;

	CMapState* GetMapState() { return &m_MapState; }

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
		m_isStartingNewMap = true;
	}

	bool CheckForNewMapStart(bool loadGame)
	{
		if (m_isStartingNewMap)
		{
			m_isStartingNewMap = false;
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

private:
	/**
	*	@brief Called when a new map has started
	*	@param loadGame Whether this is a save game being loaded or a new map being started
	*/
	void NewMapStarted(bool loadGame);

	void CreateConfigDefinitions();

	void LoadConfigFile(const char* fileName, const GameConfigDefinition& definition, const GameConfigLoadParameters& parameters = {});

	void LoadServerConfigFiles();

	void LoadMapChangeConfigFile();

	static json GetMapConfigCommandWhitelistSchema();

	std::unordered_set<std::string> GetMapConfigCommandWhitelist();

private:
	json_validator m_MapConfigCommandWhitelistValidator;

	std::shared_ptr<const GameConfigDefinition> m_ServerConfigDefinition;
	std::shared_ptr<const GameConfigDefinition> m_MapConfigDefinition;
	std::shared_ptr<const GameConfigDefinition> m_MapChangeConfigDefinition;

	bool m_isStartingNewMap = true;

	CMapState m_MapState;
};

inline CServerLibrary g_Server;
