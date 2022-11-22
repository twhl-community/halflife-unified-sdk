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

#include <spdlog/common.h>
#include <spdlog/logger.h>

#include "utils/ConCommandSystem.h"

/**
 *	@brief Handles core game actions
 */
class GameLibrary
{
protected:
	GameLibrary() = default;

public:
	virtual ~GameLibrary() = default;

	GameLibrary(const GameLibrary&) = delete;
	GameLibrary& operator=(const GameLibrary&) = delete;
	GameLibrary(GameLibrary&&) = delete;
	GameLibrary& operator=(GameLibrary&&) = delete;

	/**
	 *	@details Derived classes must call the base implementation first.
	 */
	virtual bool Initialize();

	/**
	 *	@details Derived classes must call the base implementation first.
	 *	Called even if @see Initialize returned false.
	 */
	virtual void Shutdown();

	/**
	 *	@brief Called every frame.
	 */
	virtual void RunFrame() = 0;

protected:
	virtual void AddGameSystems();

	virtual void SetEntLogLevels(spdlog::level::level_enum level);

private:
	void SetEntLogLevels(const CommandArgs& args);
};

/**
*	@brief Logger for general game events.
*/
inline std::shared_ptr<spdlog::logger> g_GameLogger;
