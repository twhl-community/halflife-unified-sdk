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
#include "MapCycleSystem.h"
#include "utils/JSONSystem.h"

// Can't use the engine's cvar because it forces the extension to .txt and it tries to parse the contents.
cvar_t mapcyclejsonfile = {"mapcyclejsonfile", "cfg/server/mapcycle.json", FCVAR_ISPATH};

constexpr std::string_view MapCycleSchemaName{"MapCycle"sv};

const MapCycleItem* MapCycle::GetNextMap(int currentPlayerCount)
{
	if (Items.empty())
	{
		return nullptr;
	}

	const auto itemIndex = [&, this]()
	{
		std::size_t nextItemIndex = Index;
		std::size_t itemsCount = Items.size();

		for (int cycle = 0; cycle < 2; ++cycle)
		{
			for (; nextItemIndex < itemsCount; ++nextItemIndex)
			{
				const auto& item = Items[nextItemIndex];

				if (item.MinPlayers != 0 && currentPlayerCount < item.MinPlayers)
				{
					continue;
				}

				if (item.MaxPlayers != 0 && currentPlayerCount > item.MaxPlayers)
				{
					continue;
				}

				return nextItemIndex;
			}

			// Start from beginning.
			nextItemIndex = 0;
			itemsCount = Index;
		}

		// No valid map found, use next in list.
		return Index;
	}();

	const auto item = &Items[itemIndex];

	Index = (itemIndex + 1) % Items.size();

	return item;
}

static std::string GetMapCycleSchema()
{
	return R"(
{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Map Cycle List",
	"type": "array",
	"items": {
		"oneOf": [
			{
				"title": "Map Name",
				"type": "string"
			},
			{
				"title": "Map Object",
				"type": "object",
				"properties": {
					"Name": {
						"title": "Map Name",
						"type": "string"
					},
					"MinPlayers": {
						"title": "Minimum Number Of Players",
						"type": "integer",
						"minimum": 0,
						"maximum": 32
					},
					"MaxPlayers": {
						"title": "Maximum Number Of Players",
						"type": "integer",
						"minimum": 0,
						"maximum": 32
					}
				}
			}
		]
	}
}
)";
}

bool MapCycleSystem::Initialize()
{
	g_engfuncs.pfnCVarRegister(&mapcyclejsonfile);
	m_Logger = g_Logging.CreateLogger("mapcycle");

	g_JSON.RegisterSchema(MapCycleSchemaName, &GetMapCycleSchema);

	return true;
}

void MapCycleSystem::Shutdown()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

MapCycle* MapCycleSystem::GetMapCycle()
{
	// find the map to change to
	const char* mapcfile = mapcyclejsonfile.string;
	ASSERT(mapcfile != nullptr);

	// Has the map cycle filename changed?
	if (m_PreviousMapCycleFile.comparei(mapcfile) != 0)
	{
		m_PreviousMapCycleFile = mapcfile;

		m_MapCycle = LoadMapCycle(mapcfile);

		if (m_MapCycle.Items.empty())
		{
			m_Logger->warn("Unable to load map cycle file {}", mapcfile);
		}
	}

	return &m_MapCycle;
}

MapCycle MapCycleSystem::LoadMapCycle(const char* fileName)
{
	return g_JSON.ParseJSONFile(fileName, {.SchemaName = MapCycleSchemaName, .PathID = "GAMECONFIG"},
					 [this](const auto& input)
					 { return ParseMapCycle(input); })
		.value_or(MapCycle{});
}

MapCycle MapCycleSystem::ParseMapCycle(const json& input)
{
	MapCycle mapCycle;

	mapCycle.Items.reserve(input.size());

	for (const auto& mapData : input)
	{
		std::string mapName;

		if (mapData.is_string())
		{
			mapName = mapData.get<std::string>();
		}
		else
		{
			mapName = mapData.value<std::string>("Name", "");
		}

		if (!IS_MAP_VALID(mapName.c_str()))
		{
			m_Logger->error("Skipping {} from mapcycle, not a valid map", mapName);
			continue;
		}

		auto& item = mapCycle.Items.emplace_back();

		item.MapName = mapName.c_str();

		if (mapData.is_object())
		{
			item.MinPlayers = std::clamp(mapData.value<int>("MinPlayers", 0), 0, gpGlobals->maxClients);
			item.MaxPlayers = std::clamp(mapData.value<int>("MaxPlayers", 0), 0, gpGlobals->maxClients);
		}
	}

	mapCycle.Items.shrink_to_fit();

	return mapCycle;
}
