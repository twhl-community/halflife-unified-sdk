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

#include <functional>
#include <iterator>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

#include "cbase.h"
#include "LogSystem.h"
#include "ConCommandSystem.h"
#include "heterogeneous_lookup.h"
#include "JSONSystem.h"

using namespace std::literals;

constexpr std::string_view LoggingConfigSchemaName{"LoggingConfig"sv};

template <typename Mutex>
class ConsoleLogSink : public spdlog::sinks::base_sink<Mutex>
{
protected:
	void sink_it_(const spdlog::details::log_msg& msg) override final
	{
		Con_Printf("[%s] [%.*s] [%s] %.*s\n",
			GetShortLibraryPrefix().data(),
			GetSafePayloadSize(msg.logger_name), msg.logger_name.data(),
			spdlog::level::to_string_view(msg.level).data(),
			GetSafePayloadSize(msg.payload), msg.payload.data());
	}

	void flush_() override final
	{
		// Nothing
	}

private:
	static constexpr int GetSafePayloadSize(spdlog::string_view_t payload) noexcept
	{
		// Truncate payload if it's too large
		return std::min(payload.size(), static_cast<std::size_t>(std::numeric_limits<int>::max()));
	}
};

static std::string GetLoggingConfigSchema()
{
	// Create json array contents matching spdlog log level names
	const auto levels = []()
	{
		std::string levels;

		{
			bool first = true;

			for (const auto& level : SPDLOG_LEVEL_NAMES)
			{
				if (!first)
				{
					levels += ',';
				}
				else
				{
					first = false;
				}

				levels += '"';
				levels += ToStringView(level);
				levels += '"';
			}
		}

		return levels;
	}();

	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Logging System Configuration",
	"type": "object",
	"properties": {{
		"Reset": {{
			"description": "Whether to reset all logging settings before applying the current settings",
			"type": "boolean"
		}},
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
		}},
		"LogFile": {{
			"type": "object",
			"properties": {{
				"Enabled": {{
					"type": "boolean"
				}},
				"BaseFileName": {{
					"type": "string"
				}},
				"MaxFiles": {{
					"type": "integer",
					"pattern": "^\\d+$"
				}}
			}},
			"required": [
				"Enabled"
			]
		}}
	}}
}}
)",
		levels, levels);
}

LogSystem::LogSystem() = default;
LogSystem::~LogSystem() = default;

static void LogErrorHandler(const std::string& msg)
{
	assert(!"A error occurred during logging");
	Con_Printf("[%s] [spdlog] [error]: %s\n", GetShortLibraryPrefix().data(), msg.c_str());
}

void LogSystem::PreInitialize()
{
	spdlog::set_error_handler(&LogErrorHandler);

	m_Sinks.push_back(std::make_shared<ConsoleLogSink<spdlog::details::null_mutex>>());

	spdlog::level::level_enum startupLogLevel = DefaultLogLevel;

	// Allow a custom level to be specified on the command line for debugging purposes
	if (const char* logLevel = SPDLOG_LEVEL_NAME_TRACE.data(); COM_GetParam("-log_startup_level", &logLevel))
	{
		startupLogLevel = spdlog::level::from_str(logLevel);
	}
	// The developer cvar will be > 0 at this point if the -dev parameter was passed on the command line
	else if (g_pDeveloper->value > 0)
	{
		startupLogLevel = spdlog::level::debug;
	}

	m_Settings.Defaults.Level = startupLogLevel;

	m_Settings.LogFile.Enabled = COM_HasParam("-log_file_on");

	m_Logger = CreateLogger("logging");

	spdlog::set_default_logger(m_Logger);
}

bool LogSystem::Initialize()
{
	g_ConCommands.CreateCommand("log_listloglevels", [this](const auto&)
		{ ListLogLevels(); });
	g_ConCommands.CreateCommand("log_listloggers", [this](const auto&)
		{ ListLoggers(); });
	g_ConCommands.CreateCommand("log_setlevel", [this](const auto& args)
		{ SetLogLevel(args); });
	g_ConCommands.CreateCommand("log_setalllevels", [this](const auto& args)
		{ SetAllLogLevels(args); });
	g_ConCommands.CreateCommand("log_file", [this](const auto& args)
		{ FileCommand(args); });

	g_JSON.RegisterSchema(LoggingConfigSchemaName, &GetLoggingConfigSchema);

	if (auto settings = g_JSON.ParseJSONFile(
			"cfg/logging.json",
			{.SchemaName = LoggingConfigSchemaName, .PathID = "GAMECONFIG"},
			[this](const json& input)
			{ return LoadSettings(input); });
		settings.has_value())
	{
		m_Settings = settings.value();
	}

	SetFileLoggingEnabled(m_Settings.LogFile.Enabled);

	m_Logger->trace("Updating loggers with configuration settings");

	spdlog::apply_all([this](std::shared_ptr<spdlog::logger> logger)
		{ ApplySettingsToLogger(*logger); });

	return true;
}

void LogSystem::PostInitialize()
{
	// Nothing.
}

void LogSystem::Shutdown()
{
	// Close log file if it's open.
	SetFileLoggingEnabled(false);

	m_Logger.reset();
	m_Sinks.clear();
	spdlog::shutdown();
	spdlog::set_error_handler(nullptr);
}

std::shared_ptr<spdlog::logger> LogSystem::CreateLogger(const std::string& name)
{
	auto logger = std::make_shared<spdlog::logger>(name, m_Sinks.begin(), m_Sinks.end());

	logger->set_error_handler(&LogErrorHandler);

	spdlog::register_logger(logger);

	ApplySettingsToLogger(*logger);

	logger->trace("Logger initialized");

	return logger;
}

void LogSystem::RemoveLogger(const std::shared_ptr<spdlog::logger>& logger)
{
	if (!logger)
	{
		return;
	}

	spdlog::drop(logger->name());
}

LogSystem::Settings LogSystem::LoadSettings(const json& input)
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

	if (auto reset = input.find("Reset"); !(reset != input.end() && reset->is_boolean() && reset->get<bool>()))
	{
		settings = m_Settings;
	}

	if (auto defaults = input.find("Defaults"); defaults != input.end())
	{
		settings.Defaults = parseLoggerSettings(*defaults, settings.Defaults.Level);
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

			// Can't be validated by schema
			if (configurations.contains(name))
			{
				m_Logger->error("Duplicate logger configuration \"{}\", ignoring", name);
				continue;
			}

			LoggerConfiguration configuration;

			configuration.Name = name;
			configuration.Settings = parseLoggerSettings(config, {});

			configurations.emplace(name, std::move(configuration));
		}

		// Flatten map into vector
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

	if (auto logFile = input.find("LogFile"); logFile != input.end() && logFile->is_object())
	{
		if (auto enabled = logFile->find("Enabled"); enabled != logFile->end() && enabled->is_boolean())
		{
			settings.LogFile.Enabled = enabled->get<bool>();
		}
		else
		{
			// Leave setting as-is, in case it was enabled through command line.
			// The setting is marked as required so it should always be there if there is a LogFile object at all.
		}

		if (auto enabled = logFile->find("BaseFileName"); enabled != logFile->end() && enabled->is_string())
		{
			settings.LogFile.BaseFileName = enabled->get<std::string>();

			if (settings.LogFile.BaseFileName->size() > MaxBaseFileNameLength)
			{
				settings.LogFile.BaseFileName->resize(MaxBaseFileNameLength);
			}
		}
		else
		{
			settings.LogFile.BaseFileName.reset();
		}

		if (auto enabled = logFile->find("MaxFiles"); enabled != logFile->end() && enabled->is_number_integer())
		{
			settings.LogFile.MaxFiles = static_cast<std::uint16_t>(std::max(0, enabled->get<int>()));
		}
		else
		{
			settings.LogFile.MaxFiles = DefaultMaxFiles;
		}
	}

	return settings;
}

void LogSystem::ApplySettingsToLogger(spdlog::logger& logger)
{
	const LoggerConfiguration emptyConfiguration;

	const LoggerConfiguration* configuration = &emptyConfiguration;

	if (auto it = std::find_if(m_Settings.Configurations.begin(), m_Settings.Configurations.end(), [&](const auto& configuration)
			{ return configuration.Name == logger.name(); });
		it != m_Settings.Configurations.end())
	{
		configuration = &(*it);
	}

	logger.set_level(configuration->Settings.Level.value_or(m_Settings.Defaults.Level.value_or(DefaultLogLevel)));
}

void LogSystem::SetFileLoggingEnabled(bool enable)
{
	if (enable && !m_FileSink)
	{
		// Separate log files into client and server files to avoid races between the two.
		const std::string& gameDir = FileSystem_GetModDirectoryName();
		const std::string baseFileName = fmt::format("{}/logs/{}/{}.log",
			gameDir, GetShortLibraryPrefix(), m_Settings.LogFile.BaseFileName.value_or(DefaultBaseFileName));

		m_FileSink = std::make_shared<spdlog::sinks::daily_file_sink_st>(baseFileName, 0, 0, false, m_Settings.LogFile.MaxFiles);

		m_Sinks.push_back(m_FileSink);

		const auto fileName = m_FileSink->filename();

		spdlog::apply_all([this](std::shared_ptr<spdlog::logger> logger)
			{ logger->sinks().push_back(m_FileSink); });

		Con_Printf("Logging data to file %s\n", fileName.c_str());

		m_Logger->info("Log file started (file \"{}\") (game \"{}\")", fileName, gameDir);
	}
	else if (!enable && m_FileSink)
	{
		m_Logger->info("Log file closed.");

		Con_Printf("Logging disabled\n");

		// Flush any pending data before removing the sink to avoid race conditions.
		m_FileSink->flush();

		m_Sinks.erase(std::find(m_Sinks.begin(), m_Sinks.end(), m_FileSink));

		spdlog::apply_all([this](std::shared_ptr<spdlog::logger> logger)
			{
				auto& sinks = logger->sinks();
				sinks.erase(std::find(sinks.begin(), sinks.end(), m_FileSink)); });

		m_FileSink.reset();
	}
}

void LogSystem::ListLogLevels()
{
	for (auto level : SPDLOG_LEVEL_NAMES)
	{
		Con_Printf("%s\n", level.data());
	}
}

void LogSystem::ListLoggers()
{
	std::vector<spdlog::logger*> loggers;

	spdlog::apply_all([&](auto logger)
		{ loggers.push_back(logger.get()); });

	std::sort(loggers.begin(), loggers.end(), [](const auto lhs, const auto& rhs)
		{ return lhs->name() < rhs->name(); });

	for (auto logger : loggers)
	{
		Con_Printf("%s\n", logger->name().c_str());
	}
}

void LogSystem::SetLogLevel(const CommandArgs& args)
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

			Con_Printf("Set \"%s\" log level to %s\n", logger->name().c_str(), spdlog::level::to_string_view(level).data());
		}
	}
	else
	{
		Con_Printf("No such logger\n");
	}
}

void LogSystem::SetAllLogLevels(const CommandArgs& args)
{
	if (args.Count() != 2)
	{
		Con_Printf("Usage: log_setalllevels log_level\n");
		return;
	}

	const auto level = spdlog::level::from_str(args.Argument(1));
	const auto& levelName = spdlog::level::to_string_view(level);

	spdlog::apply_all([&](auto logger)
		{
			logger->set_level(level);
			Con_Printf("Set \"%s\" log level to %s\n", logger->name().c_str(), levelName.data()); });
}

void LogSystem::FileCommand(const CommandArgs& args)
{
	if (args.Count() == 2)
	{
		if (0 == stricmp("off", args.Argument(1)))
		{
			SetFileLoggingEnabled(false);
		}
		else if (0 == stricmp("on", args.Argument(1)))
		{
			SetFileLoggingEnabled(true);
		}
		else
		{
			Con_Printf("log_file: unknown parameter %s, 'on' and 'off' are valid\n", args.Argument(1));
			return;
		}
	}
	else
	{
		Con_Printf("usage: log_file < on | off >\n");

		if (m_Settings.LogFile.Enabled)
		{
			Con_Printf("currently logging\n");
		}
		else
		{
			Con_Printf("not currently logging\n");
		}
	}
}
