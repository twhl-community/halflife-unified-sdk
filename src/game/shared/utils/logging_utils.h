#pragma once

#include <memory>
#include <string>

#include <spdlog/logger.h>

class CLogSystem
{
public:
	CLogSystem() = default;
	CLogSystem(const CLogSystem&) = delete;
	CLogSystem& operator=(const CLogSystem&) = delete;

	bool Initialize();
	void Shutdown();

	std::shared_ptr<spdlog::logger> CreateLogger(const std::string& name);

	spdlog::logger* GetGlobalLogger() { return m_GlobalLogger.get(); }

private:
	std::shared_ptr<spdlog::logger> m_GlobalLogger;
};

inline CLogSystem g_LogSystem;
