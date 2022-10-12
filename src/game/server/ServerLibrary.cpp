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

#include <fmt/format.h>

#include "cbase.h"
#include "client.h"
#include "nodes.h"
#include "ProjectInfo.h"
#include "ServerLibrary.h"
#include "skill.h"
#include "UserMessages.h"

#include "config/ConditionEvaluator.h"
#include "config/GameConfig.h"
#include "config/sections/CommandsSection.h"
#include "config/sections/EchoSection.h"
#include "config/sections/GlobalModelReplacementSection.h"
#include "config/sections/GlobalSentenceReplacementSection.h"
#include "config/sections/GlobalSoundReplacementSection.h"
#include "config/sections/HudColorSection.h"
#include "config/sections/SuitLightTypeSection.h"

#include "sound/SentencesSystem.h"
#include "sound/ServerSoundSystem.h"

using namespace std::literals;

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

template<typename DataContext>
static void AddCommonConfigSections(std::vector<std::unique_ptr<const GameConfigSection<DataContext>>>& sections)
{
	//Always add this
	sections.push_back(std::make_unique<EchoSection<DataContext>>());
}

ServerLibrary::ServerLibrary() = default;
ServerLibrary::~ServerLibrary() = default;

bool ServerLibrary::Initialize()
{
	if (!InitializeCommon())
	{
		return false;
	}

	CGraph::Logger = g_Logging.CreateLogger("nodegraph");
	CSaveRestoreBuffer::Logger = g_Logging.CreateLogger("saverestore");

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

	CSaveRestoreBuffer::Logger.reset();
	CGraph::Logger.reset();
	ShutdownCommon();
}

void ServerLibrary::NewMapStarted(bool loadGame)
{
	ClearStringPool();

	//Initialize map state to its default state
	m_MapState = MapState{};

	g_ReplacementMaps.Clear();

	//Load the config files, which will initialize the map state as needed
	LoadServerConfigFiles();

	g_Skill.NewMapStarted();
	sentences::g_Sentences.NewMapStarted();

	if (!m_MapState.m_GlobalSoundReplacementFileName.empty())
	{
		g_engfuncs.pfnPrecacheGeneric(m_MapState.m_GlobalSoundReplacementFileName.c_str());
		g_engfuncs.pfnForceUnmodified(force_exactfile, nullptr, nullptr, m_MapState.m_GlobalSoundReplacementFileName.c_str());
	}
}

void ServerLibrary::PreMapActivate()
{
}

void ServerLibrary::PostMapActivate()
{
	LoadMapChangeConfigFile();
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

	if (!m_MapState.m_GlobalSoundReplacementFileName.empty())
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgSoundRpl, nullptr, player->edict());
		WRITE_STRING(m_MapState.m_GlobalSoundReplacementFileName.c_str());
		MESSAGE_END();
	}

	//Override the hud color.
	if (m_MapState.m_HudColor)
	{
		player->SetHudColor(*m_MapState.m_HudColor);
	}

	//Override the light type.
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

void ServerLibrary::CreateConfigDefinitions()
{
	m_ServerConfigDefinition = g_GameConfigSystem.CreateDefinition("ServerGameConfig", []()
		{
			std::vector<std::unique_ptr<const GameConfigSection<MapState>>> sections;

			AddCommonConfigSections(sections);
			sections.push_back(std::make_unique<CommandsSection<MapState>>());
			sections.push_back(std::make_unique<GlobalModelReplacementSection>());
			sections.push_back(std::make_unique<GlobalSentenceReplacementSection>());
			sections.push_back(std::make_unique<GlobalSoundReplacementSection>());
			sections.push_back(std::make_unique<HudColorSection>());
			sections.push_back(std::make_unique<SuitLightTypeSection>());

			return sections;
		}());

	m_MapConfigDefinition = g_GameConfigSystem.CreateDefinition("MapGameConfig", [this]()
		{
			std::vector<std::unique_ptr<const GameConfigSection<MapState>>> sections;

			AddCommonConfigSections(sections);
			sections.push_back(std::make_unique<CommandsSection<MapState>>(GetMapConfigCommandWhitelist()));
			sections.push_back(std::make_unique<GlobalModelReplacementSection>());
			sections.push_back(std::make_unique<GlobalSentenceReplacementSection>());
			sections.push_back(std::make_unique<GlobalSoundReplacementSection>());
			sections.push_back(std::make_unique<HudColorSection>());
			sections.push_back(std::make_unique<SuitLightTypeSection>());

			return sections;
		}());

	m_MapChangeConfigDefinition = g_GameConfigSystem.CreateDefinition("MapChangeGameConfig", []()
		{
			std::vector<std::unique_ptr<const GameConfigSection<MapState>>> sections;

			//Limit the map change config to commands only, configuration should be handled by other cfg files
			AddCommonConfigSections(sections);
			sections.push_back(std::make_unique<CommandsSection<MapState>>());

			return sections;
		}());
}

void ServerLibrary::LoadServerConfigFiles()
{
	if (const auto cfgFile = servercfgfile.string; cfgFile && '\0' != cfgFile[0])
	{
		m_ServerConfigDefinition->TryLoad(cfgFile, {.Data = m_MapState, .PathID = "GAMECONFIG"});
	}

	//Check if the file exists so we don't get errors about it during loading
	if (const auto mapCfgFileName = fmt::format("cfg/maps/{}.json", STRING(gpGlobals->mapname));
		g_pFileSystem->FileExists(mapCfgFileName.c_str()))
	{
		m_MapConfigDefinition->TryLoad(mapCfgFileName.c_str(), {.Data = m_MapState});
	}
}

void ServerLibrary::LoadMapChangeConfigFile()
{
	if (const auto cfgFile = mapchangecfgfile.string; cfgFile && '\0' != cfgFile[0])
	{
		m_MapChangeConfigDefinition->TryLoad(cfgFile, {.Data = m_MapState, .PathID = "GAMECONFIG"});
	}
}

std::unordered_set<std::string> ServerLibrary::GetMapConfigCommandWhitelist()
{
	//Load the whitelist from a file
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
