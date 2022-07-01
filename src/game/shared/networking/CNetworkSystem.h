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

#include <charconv>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <spdlog/logger.h>

#include <steam/steamnetworkingsockets.h>

#include "GameSystem.h"

#include "utils/ConCommandSystem.h"

namespace net
{
inline const SteamNetworkingIPAddr EmptyIPAddress = []()
{
	SteamNetworkingIPAddr addr{};
	addr.Clear();
	return addr;
}();

constexpr uint32 LoopbackIPAddress = 0x7F000001;

constexpr char GNSPortKeyName[] = "gns_port";

inline std::optional<uint16> ParseGNSPort(std::string_view port)
{
	unsigned int portValue = 0;
	if (const auto result = std::from_chars(port.data(), port.data() + port.size(), portValue);
		result.ec != std::errc() || result.ptr != (port.data() + port.size()) || portValue < 0 || portValue > std::numeric_limits<uint16>::max())
	{
		return {};
	}

	return static_cast<uint16>(portValue);
}

/**
 *	@brief Amount of time before clients are automatically kicked/disconnected for not finishing connection setup.
 *	Starts counting from the moment the engine tells us the client has connected.
 *	We may already have the connection incoming by that point.
 *	This has to account for a complete round trip and some leeway to avoid false negative connection handling.
 */
constexpr std::chrono::milliseconds NetworkConnectTimeout{2000};

/**
 *	@brief Short descriptions for each connection end reason.
 *	@see ESteamNetConnectionEnd
 */
constexpr std::string_view ReasonCategories[] =
	{
		"Invalid",
		"Normal",
		"Exception",
		"Local",
		"Remote",
		"Misc"};

constexpr std::string_view GetReasonCategory(int errorCategory)
{
	if (errorCategory >= 0 && static_cast<size_t>(errorCategory))
	{
		return ReasonCategories[errorCategory];
	}

	return "Unknown";
}
}

/**
 *	@brief Handles intialization and shutdown of the networking system.
 */
class CNetworkSystem : public IGameSystem
{
public:
	const char* GetName() const override { return "Networking"; }

	bool Initialize() override;

	void PostInitialize() override;

	void Shutdown() override;

	void RunFrame();

	spdlog::logger* GetLogger() const { return m_NetworkLogger.get(); }

private:
	spdlog::logger* GetGNSLogger() const { return m_GNSLogger.get(); }

	static void NetworkDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg);

	void FlushThreadedMessages();

private:
	std::shared_ptr<spdlog::logger> m_NetworkLogger;
	std::shared_ptr<spdlog::logger> m_GNSLogger;

	std::thread::id m_MainThreadId;
	std::mutex m_LogMutex;
	std::vector<std::pair<ESteamNetworkingSocketsDebugOutputType, std::string>> m_ThreadedGNSLogMessages;
};

inline CNetworkSystem g_Networking;
