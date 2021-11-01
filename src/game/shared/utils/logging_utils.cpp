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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

#include "extdll.h"
#include "util.h"
#include "logging_utils.h"

template<typename Mutex>
class ConsoleLogSink : public spdlog::sinks::base_sink<Mutex>
{
protected:
	void sink_it_(const spdlog::details::log_msg& msg) override final
	{
		static char buffer[8192];

		const int result = snprintf(buffer, std::size(buffer), "[%.*s] %.*s\n",
			GetSafePayloadSize(msg.logger_name), msg.logger_name.data(),
			GetSafePayloadSize(msg.payload), msg.payload.data());

		if (result < 0 || 0 >= std::size(buffer))
		{
			g_engfuncs.pfnServerPrint("Error logging message\n");
			return;
		}

		g_engfuncs.pfnServerPrint(buffer);
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

bool CLogSystem::Initialize()
{
	//TODO: load settings from file

	m_GlobalLogger = CreateLogger("game");

	m_GlobalLogger->info("Global logger initialized");

	return true;
}

void CLogSystem::Shutdown()
{
	m_GlobalLogger.reset();
	spdlog::shutdown();
}

std::shared_ptr<spdlog::logger> CLogSystem::CreateLogger(const std::string& name)
{
	auto sink = std::make_shared<ConsoleLogSink<spdlog::details::null_mutex>>();

	auto logger = std::make_shared<spdlog::logger>(name, std::move(sink));

	spdlog::register_logger(logger);

	//TODO: use setting provided by user
	logger->set_level(spdlog::level::trace);

	return logger;
}
