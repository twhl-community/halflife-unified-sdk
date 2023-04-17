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

#include <chrono>
#include <regex>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include "cbase.h"
#include "client.h"
#include "nodes.h"
#include "ProjectInfoSystem.h"
#include "scripted.h"
#include "ServerLibrary.h"
#include "skill.h"
#include "UserMessages.h"
#include "voice_gamemgr.h"

#include "config/CommandWhitelist.h"
#include "config/ConditionEvaluator.h"
#include "config/GameConfig.h"
#include "config/sections/CommandsSection.h"
#include "config/sections/EchoSection.h"
#include "config/sections/GameDataFilesSections.h"
#include "config/sections/GlobalReplacementFilesSections.h"
#include "config/sections/HudColorSection.h"
#include "config/sections/SpawnInventorySection.h"
#include "config/sections/SuitLightTypeSection.h"

#include "gamerules/MapCycleSystem.h"
#include "gamerules/PersistentInventorySystem.h"
#include "gamerules/SpawnInventorySystem.h"

#include "models/BspLoader.h"

#include "networking/NetworkDataSystem.h"

#include "sound/MaterialSystem.h"
#include "sound/SentencesSystem.h"
#include "sound/ServerSoundSystem.h"

cvar_t servercfgfile = {"sv_servercfgfile", "cfg/server/server.json", FCVAR_NOEXTRAWHITEPACE | FCVAR_ISPATH};
cvar_t mp_gamemode = {"mp_gamemode", "", FCVAR_SERVER};
cvar_t mp_createserver_gamemode = {"mp_createserver_gamemode", "", FCVAR_SERVER};

ServerLibrary::ServerLibrary() = default;
ServerLibrary::~ServerLibrary() = default;

bool ServerLibrary::Initialize()
{
	// Make sure both use the same info on the server.
	g_ProjectInfo.SetServerInfo(*g_ProjectInfo.GetLocalInfo());

	if (!GameLibrary::Initialize())
	{
		return false;
	}

	m_AllowDownload = g_ConCommands.GetCVar("sv_allowdownload");
	m_SendResources = g_ConCommands.GetCVar("sv_send_resources");
	m_AllowDLFile = g_ConCommands.GetCVar("sv_allow_dlfile");

	g_PrecacheLogger = g_Logging.CreateLogger("precache");
	CBaseEntity::IOLogger = g_Logging.CreateLogger("ent.io");
	CBaseMonster::AILogger = g_Logging.CreateLogger("ent.ai");
	CCineMonster::AIScriptLogger = g_Logging.CreateLogger("ent.ai.script");
	CGraph::Logger = g_Logging.CreateLogger("nodegraph");
	CSaveRestoreBuffer::Logger = g_Logging.CreateLogger("saverestore");
	CGameRules::Logger = g_Logging.CreateLogger("gamerules");
	CVoiceGameMgr::Logger = g_Logging.CreateLogger("voice");

	UTIL_CreatePrecacheLists();

	SV_CreateClientCommands();

	g_engfuncs.pfnCVarRegister(&servercfgfile);
	g_engfuncs.pfnCVarRegister(&mp_gamemode);
	g_engfuncs.pfnCVarRegister(&mp_createserver_gamemode);

	g_ConCommands.CreateCommand(
		"mp_list_gamemodes", [](const auto&)
		{ PrintMultiplayerGameModes(); },
		CommandLibraryPrefix::No);

	RegisterCommandWhitelistSchema();

	LoadCommandWhitelist();

	CreateConfigDefinitions();
	DefineSkillVariables();

	return true;
}

void ServerLibrary::Shutdown()
{
	m_MapConfigDefinition.reset();
	m_ServerConfigDefinition.reset();

	CVoiceGameMgr::Logger.reset();
	CGameRules::Logger.reset();
	CSaveRestoreBuffer::Logger.reset();
	CGraph::Logger.reset();
	CCineMonster::AIScriptLogger.reset();
	CBaseMonster::AILogger.reset();
	CBaseEntity::IOLogger.reset();
	g_PrecacheLogger.reset();

	GameLibrary::Shutdown();
}

static void ForceCvarToValue(cvar_t* cvar, float value)
{
	if (cvar->value != value)
	{
		// So server operators know what's going on since these cvars aren't normally logged.
		g_GameLogger->warn("Forcing server cvar \"{}\" to \"{}\" to ensure network data file is transferred",
			cvar->name, value);
		g_engfuncs.pfnCvar_DirectSet(cvar, std::to_string(value).c_str());
	}
}

void ServerLibrary::RunFrame()
{
	// Force the download cvars to enabled so we can download network data.
	ForceCvarToValue(m_AllowDownload, 1);
	ForceCvarToValue(m_SendResources, 1);
	ForceCvarToValue(m_AllowDLFile, 1);
}

// Note: don't return after calling this to ensure that server state is still mostly correct.
// Otherwise the game may crash.
template <typename... Args>
void ServerLibrary::ShutdownServer(spdlog::format_string_t<Args...> fmt, Args&&... args)
{
	g_GameLogger->critical(fmt, std::forward<Args>(args)...);
	SERVER_COMMAND("shutdownserver\n");
	// Don't do this; if done at certain points during the map spawn phase
	// this will cause a fatal error because the engine tries to write to
	// an uninitialized network message buffer.
	// SERVER_EXECUTE();
}

void ServerLibrary::NewMapStarted(bool loadGame)
{
	m_IsCurrentMapLoadedFromSaveGame = loadGame;

	++m_SpawnCount;

	// Need to check if we're getting multiple map start commands in the same frame.
	m_IsStartingNewMap = true;
	++m_InNewMapStartedCount;

	// Execute any commands still queued up so cvars have the correct value.
	SERVER_EXECUTE();
	// These extra executions are needed to overcome the engine's inserting of wait commands that
	// delay server settings configured in the Create Server dialog from being set.
	// We need those settings to configure our gamerules so we're brute forcing the additional executions.
	SERVER_EXECUTE();
	SERVER_EXECUTE();

	m_IsStartingNewMap = false;
	--m_InNewMapStartedCount;

	// If multiple map change commands are executed in the same frame then this will break the server's internal state.

	// Note that this will not happen the first time you load multiple maps after launching the client.
	// Console commands are processed differently when the server dll is loaded so
	// it will load the maps in reverse order.

	// This is because when the server dll is about to load it first executes remaining console commands.
	// If there are more map commands those will also try to load the server dll,
	// executing remaining commands in the process,
	// followed by the remaining map commands executing in reverse order.
	// Thus the second command is executed first, then control returns to the first map load command
	// which then continues loading.
	if (m_InNewMapStartedCount > 0)
	{
		// Reset so we clear to a good state.
		m_IsStartingNewMap = true;
		m_InNewMapStartedCount = 0;

		// This engine function triggers a Host_Error when the first character has
		// a value <= to the whitespace character ' '.
		// It prints the entire string, so we can use this as a poor man's Host_Error.
		g_engfuncs.pfnForceUnmodified(force_exactfile, nullptr, nullptr,
			" \nERROR: Cannot execute multiple map load commands at the same time\n");
		return;
	}

	g_GameLogger->trace("Starting new map");

	// Log some useful game info.
	g_GameLogger->info("Maximum number of edicts: {}", gpGlobals->maxEntities);

	g_LastPlayerJoinTime = 0;

	for (auto list : {g_ModelPrecache.get(), g_SoundPrecache.get(), g_GenericPrecache.get()})
	{
		list->Clear();
	}

	ClearStringPool();

	// Initialize map state to its default state
	m_MapState = MapState{};

	g_ReplacementMaps.Clear();

	// Add BSP models to precache list.
	const auto completeMapName = fmt::format("maps/{}.bsp", STRING(gpGlobals->mapname));

	if (auto bspData = BspLoader::Load(completeMapName.c_str()); bspData)
	{
		g_ModelPrecache->AddUnchecked(STRING(ALLOC_STRING(completeMapName.c_str())));

		// Submodel 0 is the world so skip it.
		for (std::size_t subModel = 1; subModel < bspData->SubModelCount; ++subModel)
		{
			g_ModelPrecache->AddUnchecked(STRING(ALLOC_STRING(fmt::format("*{}", subModel).c_str())));
		}
	}
	else
	{
		ShutdownServer("Shutting down server due to error loading BSP data");
	}

	// Load the config files, which will initialize the map state as needed
	LoadServerConfigFiles();

	// This must be done before any entities are created, and after gamerules have been installed.
	Weapon_RegisterWeaponData();

	g_PersistentInventory.NewMapStarted();

	sentences::g_Sentences.NewMapStarted();

	// Reset sky name to its default value. If the map specifies its own sky
	// it will be set in CWorld::KeyValue or restored by the engine on save game load.
	CVAR_SET_STRING("sv_skyname", DefaultSkyName);
}

void ServerLibrary::PreMapActivate()
{
}

void ServerLibrary::PostMapActivate()
{
	if (!g_NetworkData.GenerateNetworkDataFile())
	{
		ShutdownServer("Shutting down server due to fatal error writing network data file");
	}

	if (g_PrecacheLogger->should_log(spdlog::level::debug))
	{
		for (auto list : {g_ModelPrecache.get(), g_SoundPrecache.get(), g_GenericPrecache.get()})
		{
			// Don't count the invalid string.
			g_PrecacheLogger->debug("[{}] {} precached total", list->GetType(), list->GetCount() - 1);
		}
	}
}

void ServerLibrary::PlayerActivating(CBasePlayer* player)
{
	// Override the hud color.
	if (m_MapState.m_HudColor)
	{
		player->SetHudColor(*m_MapState.m_HudColor);
	}

	// Override the light type.
	if (m_MapState.m_LightType)
	{
		player->SetSuitLightType(*m_MapState.m_LightType);
	}
}

void ServerLibrary::AddGameSystems()
{
	GameLibrary::AddGameSystems();
	g_GameSystems.Add(&sound::g_ServerSound);
	g_GameSystems.Add(&sentences::g_Sentences);
	g_GameSystems.Add(&g_MapCycleSystem);
}

void ServerLibrary::SetEntLogLevels(spdlog::level::level_enum level)
{
	GameLibrary::SetEntLogLevels(level);

	const auto& levelName = spdlog::level::to_string_view(level);

	for (auto& logger : {
			 CBaseEntity::IOLogger,
			 CBaseMonster::AILogger,
			 CCineMonster::AIScriptLogger,
			 CGraph::Logger,
			 CSaveRestoreBuffer::Logger,
			 CGameRules::Logger,
			 CVoiceGameMgr::Logger})
	{
		logger->set_level(level);
		Con_Printf("Set \"%s\" log level to %s\n", logger->name().c_str(), levelName.data());
	}
}

void ServerLibrary::CreateConfigDefinitions()
{
	m_ServerConfigDefinition = g_GameConfigSystem.CreateDefinition("ServerGameConfig", false, [&]()
		{
			std::vector<std::unique_ptr<const GameConfigSection<ServerConfigContext>>> sections;

			// Server configs only get common and command sections. All other configuration is handled elsewhere.
			sections.push_back(std::make_unique<EchoSection<ServerConfigContext>>());
			sections.push_back(std::make_unique<CommandsSection<ServerConfigContext>>());

			return sections; }());

	m_MapConfigDefinition = g_GameConfigSystem.CreateDefinition("MapGameConfig", true, [&, this]()
		{
			std::vector<std::unique_ptr<const GameConfigSection<ServerConfigContext>>> sections;

			sections.push_back(std::make_unique<EchoSection<ServerConfigContext>>());
			sections.push_back(std::make_unique<CommandsSection<ServerConfigContext>>(&g_CommandWhitelist));
			sections.push_back(std::make_unique<SentencesSection>());
			sections.push_back(std::make_unique<MaterialsSection>());
			sections.push_back(std::make_unique<SkillSection>());
			sections.push_back(std::make_unique<GlobalModelReplacementSection>());
			sections.push_back(std::make_unique<GlobalSentenceReplacementSection>());
			sections.push_back(std::make_unique<GlobalSoundReplacementSection>());
			sections.push_back(std::make_unique<HudColorSection>());
			sections.push_back(std::make_unique<SuitLightTypeSection>());
			sections.push_back(std::make_unique<SpawnInventorySection>());

			return sections; }());
}

void ServerLibrary::DefineSkillVariables()
{
	// Gamemode variables
	g_Skill.DefineVariable("coop_persistent_inventory_grace_period", 60, {.Minimum = -1});

	// Weapon variables
	g_Skill.DefineVariable("revolver_laser_sight", 0, {.Networked = true});
	g_Skill.DefineVariable("smg_wide_spread", 0, {.Networked = true});
	g_Skill.DefineVariable("shotgun_single_tight_spread", 0, {.Networked = true});
	g_Skill.DefineVariable("shotgun_double_wide_spread", 0, {.Networked = true});
	g_Skill.DefineVariable("crossbow_sniper_bolt", 0, {.Networked = true});
	g_Skill.DefineVariable("gauss_charge_time", 4, {.Minimum = 0.1f, .Maximum = 10.f, .Networked = true});
	g_Skill.DefineVariable("gauss_fast_ammo_use", 0, {.Networked = true});
	g_Skill.DefineVariable("gauss_damage_radius", 2.5f, {.Minimum = 0});
	g_Skill.DefineVariable("egon_narrow_ammo_per_second", 6, {.Minimum = 0});
	g_Skill.DefineVariable("egon_wide_ammo_per_second", 10, {.Minimum = 0});
	g_Skill.DefineVariable("grapple_fast", 0, {.Networked = true});
	g_Skill.DefineVariable("m249_wide_spread", 0, {.Networked = true});
	g_Skill.DefineVariable("shockrifle_fast", 0, {.Networked = true});
}

void ServerLibrary::LoadServerConfigFiles()
{
	const auto start = std::chrono::high_resolution_clock::now();

	std::optional<GameConfig<ServerConfigContext>> mapConfig;

	// Check if the file exists so we don't get errors about it during loading
	if (const auto mapCfgFileName = fmt::format("cfg/maps/{}.json", STRING(gpGlobals->mapname));
		g_pFileSystem->FileExists(mapCfgFileName.c_str()))
	{
		g_GameLogger->trace("Loading map config file");
		mapConfig = m_MapConfigDefinition->TryLoad(mapCfgFileName.c_str());
	}

	GameModeConfiguration gameModeConfig;

	if (mapConfig)
	{
		gameModeConfig = mapConfig->GetGameModeConfiguration();
	}

	// The Create Server dialog only accepts lists with numeric values so we need to remap it to the game mode name.
	if (mp_createserver_gamemode.string[0] != '\0')
	{
		g_engfuncs.pfnCvar_DirectSet(&mp_gamemode, GameModeIndexToString(int(mp_createserver_gamemode.value)));
		g_engfuncs.pfnCvar_DirectSet(&mp_createserver_gamemode, "");
	}

	if (gameModeConfig.AllowOverride && mp_gamemode.string[0] != '\0')
	{
		if (gameModeConfig.GameMode.empty())
		{
			CGameRules::Logger->trace("Setting server configured game mode {}", mp_gamemode.string);
		}
		else
		{
			CGameRules::Logger->trace("Overriding map configured game mode {} with server configured game mode {}",
				gameModeConfig.GameMode, mp_gamemode.string);
		}

		gameModeConfig.GameMode = mp_gamemode.string;
	}
	else if (!gameModeConfig.GameMode.empty())
	{
		CGameRules::Logger->trace("Using map configured game mode {}", gameModeConfig.GameMode);
	}
	else
	{
		CGameRules::Logger->trace("Using autodetected game mode");
	}

	delete g_pGameRules;
	g_pGameRules = InstallGameRules(gameModeConfig.GameMode);

	assert(g_pGameRules);

	ServerConfigContext context{.State = m_MapState};

	// Initialize file lists to their defaults.
	context.SentencesFiles.push_back("sound/sentences.txt");
	context.MaterialsFiles.push_back("sound/materials.json");
	context.SkillFiles.push_back("cfg/skill.json");

	if (g_pGameRules->IsMultiplayer())
	{
		context.SkillFiles.push_back("cfg/skill_multiplayer.json");
	}

	if (const auto cfgFile = servercfgfile.string; cfgFile && '\0' != cfgFile[0])
	{
		g_GameLogger->trace("Loading server config file");
			
		if (auto config = m_ServerConfigDefinition->TryLoad(cfgFile, "GAMECONFIG"); config)
		{
			config->Parse(context);
		}
	}

	if (mapConfig)
	{
		mapConfig->Parse(context);
	}

	sentences::g_Sentences.LoadSentences(context.SentencesFiles);
	g_MaterialSystem.LoadMaterials(context.MaterialsFiles);
	g_Skill.LoadSkillConfigFiles(context.SkillFiles);

	m_MapState.m_GlobalModelReplacement = g_ReplacementMaps.LoadMultiple(
		context.GlobalModelReplacementFiles, {.CaseSensitive = false});
	m_MapState.m_GlobalSentenceReplacement = g_ReplacementMaps.LoadMultiple(
		context.GlobalSentenceReplacementFiles, {.CaseSensitive = true});
	m_MapState.m_GlobalSoundReplacement = g_ReplacementMaps.LoadMultiple(
		context.GlobalSoundReplacementFiles, {.CaseSensitive = false});

	g_SpawnInventory.SetInventory(std::move(context.SpawnInventory));

	const auto timeElapsed = std::chrono::high_resolution_clock::now() - start;

	g_GameLogger->trace("Server configurations loaded in {}ms",
		std::chrono::duration_cast<std::chrono::milliseconds>(timeElapsed).count());
}
