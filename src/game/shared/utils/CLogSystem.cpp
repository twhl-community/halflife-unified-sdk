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

#include <algorithm>
#include <functional>
#include <iterator>
#include <sstream>
#include <string_view>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

#include "extdll.h"
#include "util.h"
#include "CLogSystem.h"
#include "command_utils.h"
#include "heterogeneous_lookup.h"
#include "json_utils.h"

using namespace std::literals;

template<typename Mutex>
class ConsoleLogSink : public spdlog::sinks::base_sink<Mutex>
{
protected:
	void sink_it_(const spdlog::details::log_msg& msg) override final
	{
		Con_Printf("[%.*s] [%s] %.*s\n",
			GetSafePayloadSize(msg.logger_name), msg.logger_name.data(),
			spdlog::level::to_string_view(msg.level).data(),
			GetSafePayloadSize(msg.payload), msg.payload.data());
	}

	void flush_() override final
	{
		//Nothing
	}

private:
	static constexpr int GetSafePayloadSize(spdlog::string_view_t payload) noexcept
	{
		//Truncate payload if it's too large
		return std::min(payload.size(), static_cast<std::size_t>(std::numeric_limits<int>::max()));
	}
};

static json GetLoggingConfigSchema()
{
	//Create json array contents matching spdlog log level names
	const auto levels = []()
	{
		std::ostringstream levels;

		{
			bool first = true;

			for (const auto& level : SPDLOG_LEVEL_NAMES)
			{
				if (!first)
				{
					levels << ',';
				}

				first = false;

				levels << '"' << ToStringView(level) << '"';
			}
		}

		return levels.str();
	}();

	auto schema = fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Logging System Configuration",
	"type": "object",
	"properties": {{
		"Defaults": {{
			"type": "object",
			"properties": {{
				"LogLevel": {{
					"title": "Log Level",
					"type": "string",
					"enum": [{}]
				}}
			}}
		}},
		"LoggerConfigurations": {{
			"type": "array",
			"items": {{
				"title": "Logger Configurations",
				"type": "object",
				"properties": {{
					"Name": {{
						"title": "Logger Name",
						"type": "string",
						"pattern": "^[\\w]+$"
					}},
					"LogLevel": {{
						"title": "Log Level",
						"type": "string",
						"enum": [{}]
					}}
				}},
				"required": [
					"Name"
				]
			}}
		}}
	}}
}}
)", levels, levels);

	return g_JSON.ParseJSONSchema(schema).value_or(json{});
}

CLogSystem::CLogSystem() = default;
CLogSystem::~CLogSystem() = default;

bool CLogSystem::Initialize()
{
	if (m_Initialized)
	{
		return true;
	}

	g_ConCommands.CreateCommand("log_listloglevels", [this](const auto&) { ListLogLevels(); });
	g_ConCommands.CreateCommand("log_listloggers", [this](const auto&) { ListLoggers(); });
	g_ConCommands.CreateCommand("log_setlevel", [this](const auto& args) { SetLogLevel(args); });

	g_JSON.RegisterSchema("LoggingConfig", &GetLoggingConfigSchema);

	m_Sinks.push_back(std::make_shared<ConsoleLogSink<spdlog::details::null_mutex>>());

	//Create a logger to use during startup
	m_GlobalLogger = CreateLoggerCore("startup");

	spdlog::level::level_enum startupLogLevel = spdlog::level::info;

	//Allow a custom level to be specified on the command line for debugging purposes
	if (const char* logLevel = SPDLOG_LEVEL_NAME_TRACE.data(); COM_GetParam("-log_startup_level", &logLevel))
	{
		startupLogLevel = spdlog::level::from_str(logLevel);
	}
	//The developer cvar will be > 0 at this point if the -dev parameter was passed on the command line
	else if (g_pDeveloper->value > 0)
	{
		startupLogLevel = spdlog::level::debug;
	}

	m_GlobalLogger->set_level(startupLogLevel);

	FinishCreateLogger(*m_GlobalLogger);

	spdlog::set_default_logger(m_GlobalLogger);

	return true;
}

bool CLogSystem::PostInitialize()
{
	json jsonSchema = GetLoggingConfigSchema();

	if (jsonSchema.is_null())
	{
		return false;
	}

	json_validator validator{};

	validator.set_root_schema(std::move(jsonSchema));

	m_Settings = g_JSON.ParseJSONFile(
		"cfg/logging.json",
		validator,
		[this](const json& input) { return LoadSettings(input); },
		"GAMECONFIG")
		.value_or(Settings{});

	//TODO: create sinks based on settings

	m_Initialized = true;

	//Recreate the global logger with configuration settings
	m_GlobalLogger = CreateLogger("global");

	//Replace the startup logger
	spdlog::set_default_logger(m_GlobalLogger);

	return true;
}

void CLogSystem::Shutdown()
{
	m_GlobalLogger.reset();
	m_Sinks.clear();
	spdlog::shutdown();

	m_Initialized = false;
}

std::shared_ptr<spdlog::logger> CLogSystem::CreateLogger(const std::string & name)
{
	if (!m_Initialized)
	{
		Con_Printf("CLogSystem::CreateLogger: Tried to create logger before log system was initialized!\n");
		return {};
	}

	auto logger = CreateLoggerCore(name);

	spdlog::register_logger(logger);

	ApplySettingsToLogger(*logger);

	FinishCreateLogger(*logger);

	return logger;
}

void CLogSystem::RemoveLogger(const std::shared_ptr<spdlog::logger>&logger)
{
	if (!logger || logger == m_GlobalLogger)
	{
		return;
	}

	spdlog::drop(logger->name());
}

CLogSystem::Settings CLogSystem::LoadSettings(const json & input)
{
	auto parseLoggerSettings = [](const json& obj, std::optional<spdlog::level::level_enum> defaultLogLevel)
	{
		LoggerConfigurationSettings settings;

		if (const auto logLevel = obj.value("LogLevel", ""sv); !logLevel.empty())
		{
			settings.Level = spdlog::level::from_str(std::string{logLevel});
		}
		else
		{
			settings.Level = defaultLogLevel;
		}

		return settings;
	};

	Settings settings;

	if (auto defaults = input.find("Defaults"); defaults != input.end())
	{
		settings.Defaults = parseLoggerSettings(*defaults, DefaultLogLevel);
	}

	if (auto configs = input.find("LoggerConfigurations"); configs != input.end() && configs->is_array())
	{
		std::unordered_map<std::string, LoggerConfiguration, TransparentStringHash, TransparentEqual> configurations;

		configurations.reserve(configs->size());

		for (const auto& config : *configs)
		{
			if (!config.is_object())
			{
				continue;
			}

			auto name = config.value("Name", ""sv);

			if (name.empty())
			{
				continue;
			}

			//Can't be validated by schema
			if (configurations.contains(name))
			{
				m_GlobalLogger->error("Duplicate logger configuration \"{}\", ignoring", name);
				continue;
			}

			LoggerConfiguration configuration;

			configuration.Name = name;
			configuration.Settings = parseLoggerSettings(config, {});

			configurations.emplace(name, std::move(configuration));
		}

		//Flatten map into vector
		settings.Configurations.reserve(configurations.size());

		std::transform(
			std::make_move_iterator(configurations.begin()),
			std::make_move_iterator(configurations.end()),
			std::back_inserter(settings.Configurations),
			[](auto&& configuration)
			{
				return std::move(configuration.second);
			});
	}

	return settings;
}

std::shared_ptr<spdlog::logger> CLogSystem::CreateLoggerCore(const std::string & name)
{
	auto logger = std::make_shared<spdlog::logger>(name, m_Sinks.begin(), m_Sinks.end());

	return logger;
}

void CLogSystem::FinishCreateLogger(spdlog::logger& logger)
{
	logger.trace("Logger initialized");
}

void CLogSystem::ApplySettingsToLogger(spdlog::logger& logger)
{
	const LoggerConfiguration emptyConfiguration;

	const LoggerConfiguration* configuration = &emptyConfiguration;

	if (auto it = std::find_if(m_Settings.Configurations.begin(), m_Settings.Configurations.end(), [&](const auto& configuration)
		{
			return configuration.Name == logger.name();
		}); it != m_Settings.Configurations.end())
	{
		configuration = &(*it);
	}

	logger.set_level(configuration->Settings.Level.value_or(m_Settings.Defaults.Level.value_or(DefaultLogLevel)));
}

void CLogSystem::ListLogLevels()
{
	for (auto level : SPDLOG_LEVEL_NAMES)
	{
		Con_Printf("%s\n", level.data());
	}
}

void CLogSystem::ListLoggers()
{
	spdlog::apply_all([](auto logger)
		{
			Con_Printf("%s\n", logger->name().c_str());
		});
}

void CLogSystem::SetLogLevel(const CCommandArgs& args)
{
	const int count = args.Count();

	if (count != 2 && count != 3)
	{
		Con_Printf("Usage: log_setlevel logger_name [log_level]\n");
		return;
	}

	if (auto logger = spdlog::get(args.Argument(1)); logger)
	{
		if (count == 2)
		{
			Con_Printf("Log level is %s\n", spdlog::level::to_string_view(logger->level()).data());
		}
		else
		{
			const auto level = spdlog::level::from_str(args.Argument(2));

			logger->set_level(level);

			Con_Printf("Set log level to %s\n", spdlog::level::to_string_view(level).data());
		}
	}
	else
	{
		Con_Printf("No such logger\n");
	}
}
