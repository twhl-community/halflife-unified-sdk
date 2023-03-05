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

#include <regex>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include "cbase.h"
#include "client.h"
#include "gamerules.h"
#include "nodes.h"
#include "ProjectInfo.h"
#include "scripted.h"
#include "ServerLibrary.h"
#include "skill.h"
#include "UserMessages.h"
#include "voice_gamemgr.h"

#include "config/ConditionEvaluator.h"
#include "config/GameConfig.h"
#include "config/sections/CommandsSection.h"
#include "config/sections/EchoSection.h"
#include "config/sections/GameDataFilesSections.h"
#include "config/sections/GlobalReplacementFilesSections.h"
#include "config/sections/HudColorSection.h"
#include "config/sections/SuitLightTypeSection.h"

#include "networking/NetworkDataSystem.h"

#include "sound/MaterialSystem.h"
#include "sound/SentencesSystem.h"
#include "sound/ServerSoundSystem.h"

constexpr const char* const MapConfigCommandWhitelistFileName = "cfg/MapConfigCommandWhitelist.json";
const std::regex MapConfigCommandWhitelistRegex{"^[\\w]+$"};
constexpr std::string_view MapConfigCommandWhitelistSchemaName{"MapConfigCommandWhitelist"sv};

cvar_t servercfgfile = {"sv_servercfgfile", "cfg/server/server.json", FCVAR_NOEXTRAWHITEPACE | FCVAR_ISPATH};
cvar_t mapchangecfgfile = {"sv_mapchangecfgfile", "", FCVAR_NOEXTRAWHITEPACE | FCVAR_ISPATH};

static std::string GetMapConfigCommandWhitelistSchema()
{
	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Map Configuration Command Whitelist",
	"type": "array",
	"items": {{
		"title": "Command Name",
		"type": "string",
		"pattern": "^[\\w]+$"
	}}
}}
)");
}

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
	if (!GameLibrary::Initialize())
	{
		return false;
	}

	m_AllowDownload = g_ConCommands.GetCVar("sv_allowdownload");
	m_SendResources = g_ConCommands.GetCVar("sv_send_resources");
	m_AllowDLFile = g_ConCommands.GetCVar("sv_allow_dlfile");

	CBaseEntity::IOLogger = g_Logging.CreateLogger("ent.io");
	CBaseMonster::AILogger = g_Logging.CreateLogger("ent.ai");
	CCineMonster::AIScriptLogger = g_Logging.CreateLogger("ent.ai.script");
	CGraph::Logger = g_Logging.CreateLogger("nodegraph");
	CSaveRestoreBuffer::Logger = g_Logging.CreateLogger("saverestore");
	CGameRules::Logger = g_Logging.CreateLogger("gamerules");
	CVoiceGameMgr::Logger = g_Logging.CreateLogger("voice");

	SV_CreateClientCommands();

	g_engfuncs.pfnCVarRegister(&servercfgfile);
	g_engfuncs.pfnCVarRegister(&mapchangecfgfile);

	g_JSON.RegisterSchema(MapConfigCommandWhitelistSchemaName, &GetMapConfigCommandWhitelistSchema);

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
	g_GameLogger->trace("Starting new map");

	// Log some useful game info.
	g_GameLogger->info("Maximum number of edicts: {}", gpGlobals->maxEntities);

	g_LastPlayerJoinTime = 0;

	ClearStringPool();

	// Initialize map state to its default state
	m_MapState = MapState{};

	g_ReplacementMaps.Clear();

	// Load the config files, which will initialize the map state as needed
	LoadServerConfigFiles();

	sentences::g_Sentences.NewMapStarted();
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
}

void ServerLibrary::PlayerActivating(CBasePlayer* player)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgProjectInfo, nullptr, player->edict());
	WRITE_LONG(UnifiedSDKVersionMajor);
	WRITE_LONG(UnifiedSDKVersionMinor);
	WRITE_LONG(UnifiedSDKVersionPatch);

	WRITE_STRING(UnifiedSDKGitBranchName);
	WRITE_STRING(UnifiedSDKGitTagName);
	WRITE_STRING(UnifiedSDKGitCommitHash);
	MESSAGE_END();

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
			sections.push_back(std::make_unique<CommandsSection<ServerConfigContext>>(GetMapConfigCommandWhitelist()));
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

	m_MapState.m_GlobalModelReplacement = g_ReplacementMaps.LoadMultiple(context.GlobalModelReplacementFiles, {.CaseSensitive = false});
	m_MapState.m_GlobalSentenceReplacement = g_ReplacementMaps.LoadMultiple(context.GlobalSentenceReplacementFiles, {.CaseSensitive = true});
	m_MapState.m_GlobalSoundReplacement = g_ReplacementMaps.LoadMultiple(context.GlobalSoundReplacementFiles, {.CaseSensitive = false});
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

std::unordered_set<std::string> ServerLibrary::GetMapConfigCommandWhitelist()
{
	// Load the whitelist from a file
	auto whitelist = g_JSON.ParseJSONFile(
		MapConfigCommandWhitelistFileName,
		{.SchemaName = MapConfigCommandWhitelistSchemaName, .PathID = "GAMECONFIG"},
		[](const json& input)
		{
			std::unordered_set<std::string> list;

			if (input.is_array())
			{
				list.reserve(input.size());

				for (const auto& element : input)
				{
					auto command = element.get<std::string>();

					if (std::regex_match(command, MapConfigCommandWhitelistRegex))
					{
						if (!list.insert(std::move(command)).second)
						{
							g_GameConfigSystem.GetLogger()->debug("Whitelist command \"{}\" encountered more than once", command);
						}
					}
					else
					{
						g_GameConfigSystem.GetLogger()->warn("Whitelist command \"{}\" has invalid syntax, ignoring", command);
					}
				}
			}

			return list;
		});

	return whitelist.value_or(std::unordered_set<std::string>{});
}
