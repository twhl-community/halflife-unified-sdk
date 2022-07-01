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

#include <chrono>
#include <thread>

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include "cbase.h"
#include "CNetworkSystem.h"

static spdlog::level::level_enum GNSLevelToSpdlog(ESteamNetworkingSocketsDebugOutputType type)
{
	switch (type)
	{
	case k_ESteamNetworkingSocketsDebugOutputType_None: return spdlog::level::off;
	case k_ESteamNetworkingSocketsDebugOutputType_Bug: return spdlog::level::critical;
	case k_ESteamNetworkingSocketsDebugOutputType_Error: return spdlog::level::err;
	case k_ESteamNetworkingSocketsDebugOutputType_Important:
		[[fallthrough]];
	case k_ESteamNetworkingSocketsDebugOutputType_Warning: return spdlog::level::warn;
	case k_ESteamNetworkingSocketsDebugOutputType_Msg: return spdlog::level::info;
	case k_ESteamNetworkingSocketsDebugOutputType_Verbose:
		[[fallthrough]];
	case k_ESteamNetworkingSocketsDebugOutputType_Debug: return spdlog::level::debug;
	case k_ESteamNetworkingSocketsDebugOutputType_Everything: return spdlog::level::trace;
	}

	[[unlikely]]

	g_Networking.GetLogger()
		->critical("Invalid GNS log level \"{}\" received", type);

	// When in doubt, log it.
	return spdlog::level::critical;
}

void CNetworkSystem::NetworkDebugOutput(ESteamNetworkingSocketsDebugOutputType eType, const char* pszMsg)
{
	// If we're not on the main thread then log messages should be queued for logging on that thread.
	// Otherwise the console will sometimes end up corrupting the heap due to a lack of synchronized access to its buffer reallocation code.
	// This is pretty expensive compared to logging on the main thread,
	// so in release builds the log level needs to be low enough to not have this happen too much.
	// This shouldn't happen too often so it's worth the extra overhead.
	if (std::this_thread::get_id() != g_Networking.m_MainThreadId)
	{
		const std::lock_guard guard{g_Networking.m_LogMutex};
		g_Networking.m_ThreadedGNSLogMessages.emplace_back(eType, pszMsg);
		return;
	}

	g_Networking.GetGNSLogger()->log(GNSLevelToSpdlog(eType), pszMsg);
}

bool CNetworkSystem::Initialize()
{
	m_NetworkLogger = g_Logging.CreateLogger("networking");
	m_GNSLogger = g_Logging.CreateLogger("gns");

	SteamDatagramErrMsg errMsg;
	if (!GameNetworkingSockets_Init(nullptr, errMsg))
	{
		m_NetworkLogger->error("GameNetworkingSockets_Init failed: {}", errMsg);
		return false;
	}

	m_MainThreadId = std::this_thread::get_id();

	const auto detailLevel = []()
	{
#ifdef DEBUG
		return k_ESteamNetworkingSocketsDebugOutputType_Debug;
#else
		if (g_pDeveloper->value > 0)
		{
			return k_ESteamNetworkingSocketsDebugOutputType_Msg;
		}
		else
		{
			return k_ESteamNetworkingSocketsDebugOutputType_Warning;
		}
#endif
	}();

	SteamNetworkingUtils()->SetDebugOutputFunction(detailLevel, NetworkDebugOutput);

	m_GNSLogger->debug("GameNetworkingSockets (GNS) initialized");

	return true;
}

void CNetworkSystem::PostInitialize()
{
	// Nothing.
}

void CNetworkSystem::Shutdown()
{
	// TODO: make sure this is correct.
	// Only wait if the library was initialized. If it failed to init then waiting is pointless.
	// Only wait if we're a dedicated server. If this is a listen server then the client has already waited,
	// so pending connections have had time to close.
	if (SteamNetworkingSockets() != nullptr && UTIL_IsDedicatedServer())
	{
		const std::chrono::milliseconds waitDelay{500};
		m_NetworkLogger->debug("Waiting {} for GNS connections to close", waitDelay);
		std::this_thread::sleep_for(waitDelay);
	}

	GameNetworkingSockets_Kill();

	// Avoid null pointer dereferences if anything does get logged.
	SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_None, nullptr);

	// In case we have anything left in the queue. Nothing should be logging at this point, so the lock should be acquired.
	FlushThreadedMessages();

	m_GNSLogger.reset();
	m_NetworkLogger.reset();

	m_MainThreadId = {};
}

void CNetworkSystem::RunFrame()
{
	FlushThreadedMessages();
}

void CNetworkSystem::FlushThreadedMessages()
{
	if (const std::unique_lock lock{m_LogMutex, std::try_to_lock}; lock)
	{
		for (const auto& [eType, msg] : m_ThreadedGNSLogMessages)
		{
			// Mark these as coming from the worker thread. This makes it easier to tell if a log message is out-of-order.
			m_GNSLogger->log(GNSLevelToSpdlog(eType), "[thread] {}", msg);
		}

		m_ThreadedGNSLogMessages.clear();
	}
}
