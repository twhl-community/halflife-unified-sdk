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

#include "GameSystem.h"
#include "json_fwd.h"

class CCommandArgs;

class CLogSystem final : public IGameSystem
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

	void ListLogLevels();

	void ListLoggers();

	void SetLogLevel(const CCommandArgs& args);

private:
	std::vector<std::shared_ptr<spdlog::sinks::sink>> m_Sinks;

	std::shared_ptr<spdlog::logger> m_Logger;

	Settings m_Settings;
};
