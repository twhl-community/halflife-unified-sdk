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

#include <spdlog/logger.h>

class CLogSystem
{
public:
	CLogSystem() = default;
	CLogSystem(const CLogSystem&) = delete;
	CLogSystem& operator=(const CLogSystem&) = delete;

	bool Initialize();
	void Shutdown();

	std::shared_ptr<spdlog::logger> CreateLogger(const std::string& name);

	spdlog::logger* GetGlobalLogger() { return m_GlobalLogger.get(); }

private:
	std::shared_ptr<spdlog::logger> m_GlobalLogger;
};

inline CLogSystem g_LogSystem;
