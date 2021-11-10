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

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ILogSystem.h"
#include "json_fwd.h"

class CCommandArgs;

class CLogSystem final : public ILogSystem
{
private:
	static constexpr spdlog::level::level_enum DefaultLogLevel = spdlog::level::info;

	struct LoggerConfigurationSettings
	{
		std::optional<spdlog::level::level_enum> Level;
	};

	struct LoggerConfiguration
	{
		std::string Name;
		LoggerConfigurationSettings Settings;
	};

	struct Settings
	{
		LoggerConfigurationSettings Defaults{.Level = DefaultLogLevel};

		std::vector<LoggerConfiguration> Configurations;
	};

public:
	CLogSystem();
	~CLogSystem();
	CLogSystem(const CLogSystem&) = delete;
	CLogSystem& operator=(const CLogSystem&) = delete;

	bool Initialize() override;
	bool PostInitialize() override;
	void Shutdown() override;

	std::shared_ptr<spdlog::logger> CreateLogger(const std::string& name) override;

	void RemoveLogger(const std::shared_ptr<spdlog::logger>& logger) override;

	std::shared_ptr<spdlog::logger> GetGlobalLogger() override { return m_GlobalLogger; }

private:
	Settings LoadSettings(const json& input);

	std::shared_ptr<spdlog::logger> CreateLoggerCore(const std::string& name);

	static void FinishCreateLogger(spdlog::logger& logger);

	void ApplySettingsToLogger(spdlog::logger& logger);

	void ListLogLevels();

	void ListLoggers();

	void SetLogLevel(const CCommandArgs& args);

private:
	bool m_Initialized = false;

	std::vector<std::shared_ptr<spdlog::sinks::sink>> m_Sinks;

	std::shared_ptr<spdlog::logger> m_GlobalLogger;

	Settings m_Settings;
};
