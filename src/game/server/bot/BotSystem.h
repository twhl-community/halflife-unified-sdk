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

#include <spdlog/logger.h>

#include "utils/GameSystem.h"

/**
 *	@brief Provides barebones bot spawning functionality for testing multiplayer.
 *	Bots are not capable of movement or actions, only spawning in the game.
 */
class BotSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "Bots"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override;

	void RunFrame();

private:
	void AddBot(const char* name);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	float m_LastUpdateTime = 0;
};

inline BotSystem g_Bots;
