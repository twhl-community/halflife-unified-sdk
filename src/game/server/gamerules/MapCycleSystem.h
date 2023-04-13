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

#include <cstddef>
#include <memory>
#include <vector>

#include <spdlog/logger.h>

#include <EASTL/fixed_string.h>

#include "cbase.h"
#include "utils/GameSystem.h"
#include "utils/json_fwd.h"

struct MapCycleItem
{
	eastl::fixed_string<char, cchMapNameMost> MapName;

	int MinPlayers{0};
	int MaxPlayers{0};
};

class MapCycle final
{
public:
	std::vector<MapCycleItem> Items;

	std::size_t Index{0};

	const MapCycleItem* GetNextMap(int currentPlayerCount);
};

class MapCycleSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "MapCycle"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override;

	MapCycle* GetMapCycle();

private:
	MapCycle LoadMapCycle(const char* fileName);
	MapCycle ParseMapCycle(const json& input);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	eastl::fixed_string<char, MAX_PATH_LENGTH> m_PreviousMapCycleFile;

	MapCycle m_MapCycle;
};

inline MapCycleSystem g_MapCycleSystem;
