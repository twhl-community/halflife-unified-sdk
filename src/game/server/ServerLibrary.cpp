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
#include "config/sections/SuitLightTypeSection.h"

#include "models/BspLoader.h"

#include "networking/NetworkDataSystem.h"

#include "sound/MaterialSystem.h"
#include "sound/SentencesSystem.h"
#include "sound/ServerSoundSystem.h"

cvar_t servercfgfile = {"sv_servercfgfile", "cfg/server/server.json", FCVAR_NOEXTRAWHITEPACE | FCVAR_ISPATH};
cvar_t mapchangecfgfile = {"sv_mapchangecfgfile", "", FCVAR_NOEXTRAWHITEPACE | FCVAR_ISPATH};

template <typename DataContext>
static void AddCommonConfigSections(std::vector<std::unique_ptr<const GameConfigSection<DataContext>>>& sections)
{
	// Always add this
	sections.push_back(std::make_unique<EchoSection<DataContext>>());
}

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
	g_engfuncs.pfnCVarRegister(&mapchangecfgfile);

	RegisterCommandWhitelistSchema();

	LoadCommandWhitelist();

	CreateConfigDefinitions();

	return true;
}

void ServerLibrary::Shutdown()
{
	m_MapConfigDefinition.reset();
	m_MapChangeConfigDefinition.reset();
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

	// We're loading a save game so we will always use singleplayer gamerules.
	// Install it now so entities can access it during restore.
	if (IsCurrentMapLoadedFromSaveGame())
	{
		delete g_pGameRules;
		g_pGameRules = InstallSinglePlayerGameRules();
	}

	// This must be done before any entities are created, and after gamerules have been installed.
	Weapon_RegisterWeaponData();

	// Load the config files, which will initialize the map state as needed
	LoadServerConfigFiles();

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
	LoadMapChangeConfigFile();

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
	g_GameSystems.Add(&g_Skill);
	g_GameSystems.Add(&sound::g_ServerSound);
	g_GameSystems.Add(&sentences::g_Sentences);
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
	const auto addMapAndServerSections = [](std::vector<std::unique_ptr<const GameConfigSection<ServerConfigContext>>>& sections)
	{
		sections.push_back(std::make_unique<SentencesSection>());
		sections.push_back(std::make_unique<MaterialsSection>());
		sections.push_back(std::make_unique<SkillSection>());
		sections.push_back(std::make_unique<GlobalModelReplacementSection>());
		sections.push_back(std::make_unique<GlobalSentenceReplacementSection>());
		sections.push_back(std::make_unique<GlobalSoundReplacementSection>());
		sections.push_back(std::make_unique<HudColorSection>());
		sections.push_back(std::make_unique<SuitLightTypeSection>());
	};

	m_ServerConfigDefinition = g_GameConfigSystem.CreateDefinition("ServerGameConfig", [&]()
		{
			std::vector<std::unique_ptr<const GameConfigSection<ServerConfigContext>>> sections;

			AddCommonConfigSections(sections);
			sections.push_back(std::make_unique<CommandsSection<ServerConfigContext>>());
			addMapAndServerSections(sections);

			return sections; }());

	m_MapConfigDefinition = g_GameConfigSystem.CreateDefinition("MapGameConfig", [&, this]()
		{
			std::vector<std::unique_ptr<const GameConfigSection<ServerConfigContext>>> sections;

			AddCommonConfigSections(sections);
			sections.push_back(std::make_unique<CommandsSection<ServerConfigContext>>(&g_CommandWhitelist));
			addMapAndServerSections(sections);

			return sections; }());

	m_MapChangeConfigDefinition = g_GameConfigSystem.CreateDefinition("MapChangeGameConfig", []()
		{
			std::vector<std::unique_ptr<const GameConfigSection<ServerConfigContext>>> sections;

			//Limit the map change config to commands only, configuration should be handled by other cfg files
			AddCommonConfigSections(sections);
			sections.push_back(std::make_unique<CommandsSection<ServerConfigContext>>());

			return sections; }());
}

void ServerLibrary::LoadServerConfigFiles()
{
	const auto start = std::chrono::high_resolution_clock::now();

	ServerConfigContext context{.State = m_MapState};

	if (const auto cfgFile = servercfgfile.string; cfgFile && '\0' != cfgFile[0])
	{
		g_GameLogger->trace("Loading server config file");
		m_ServerConfigDefinition->TryLoad(cfgFile, {.Data = context, .PathID = "GAMECONFIG"});
	}

	// Check if the file exists so we don't get errors about it during loading
	if (const auto mapCfgFileName = fmt::format("cfg/maps/{}.json", STRING(gpGlobals->mapname));
		g_pFileSystem->FileExists(mapCfgFileName.c_str()))
	{
		g_GameLogger->trace("Loading map config file");
		m_MapConfigDefinition->TryLoad(mapCfgFileName.c_str(), {.Data = context});
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

	const auto timeElapsed = std::chrono::high_resolution_clock::now() - start;

	g_GameLogger->trace("Server configurations loaded in {}ms",
		std::chrono::duration_cast<std::chrono::milliseconds>(timeElapsed).count());
}

void ServerLibrary::LoadMapChangeConfigFile()
{
	ServerConfigContext context{.State = m_MapState};

	if (const auto cfgFile = mapchangecfgfile.string; cfgFile && '\0' != cfgFile[0])
	{
		g_GameLogger->trace("Loading map change config file");
		m_MapChangeConfigDefinition->TryLoad(cfgFile, {.Data = context, .PathID = "GAMECONFIG"});
	}
}
