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
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/logger.h>
#include <spdlog/sinks/daily_file_sink.h>

#include "GameSystem.h"
#include "json_fwd.h"

class CommandArgs;

class LogSystem final : public IGameSystem
{
private:
	static constexpr spdlog::level::level_enum DefaultLogLevel = spdlog::level::info;
	static const inline std::string DefaultBaseFileName{"L"};
	static constexpr std::size_t MaxBaseFileNameLength{16};
	static constexpr std::uint16_t DefaultMaxFiles{8}; //Have a finite limit for files by default.

	struct LoggerConfigurationSettings
	{
		std::optional<spdlog::level::level_enum> Level;
	};

	struct LoggerConfiguration
	{
		std::string Name;
		LoggerConfigurationSettings Settings;
	};

	struct LogFileSettings
	{
		bool Enabled = false;
		std::optional<std::string> BaseFileName;
		std::uint16_t MaxFiles = DefaultMaxFiles;
	};

	struct Settings
	{
		LoggerConfigurationSettings Defaults{.Level = DefaultLogLevel};

		std::vector<LoggerConfiguration> Configurations;

		LogFileSettings LogFile;
	};

public:
	LogSystem();
	~LogSystem();
	LogSystem(const LogSystem&) = delete;
	LogSystem& operator=(const LogSystem&) = delete;

	const char* GetName() const override { return "Logging"; }

	void PreInitialize();

	bool Initialize() override;
	void PostInitialize() override;
	void Shutdown() override;

	std::shared_ptr<spdlog::logger> CreateLogger(const std::string& name);

	void RemoveLogger(const std::shared_ptr<spdlog::logger>& logger);

private:
	Settings LoadSettings(const json& input);

	void ApplySettingsToLogger(spdlog::logger& logger);

	void SetFileLoggingEnabled(bool enable);

	void ListLogLevels();

	void ListLoggers();

	void SetLogLevel(const CommandArgs& args);

	void SetAllLogLevels(const CommandArgs& args);

	void FileCommand(const CommandArgs& args);

private:
	std::vector<std::shared_ptr<spdlog::sinks::sink>> m_Sinks;

	std::shared_ptr<spdlog::sinks::daily_file_sink_st> m_FileSink;

	std::shared_ptr<spdlog::logger> m_Logger;

	Settings m_Settings;
};

inline LogSystem g_Logging;
