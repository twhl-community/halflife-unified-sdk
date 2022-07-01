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

#include <steam/isteamnetworkingutils.h>

#include "hud.h"
#include "CClientNetworkSystem.h"
#include "networking/CNetworkSystem.h"

#include "net_api.h"

bool CClientNetworkSystem::Initialize()
{
	m_Interface = SteamNetworkingSockets();

	return true;
}

void CClientNetworkSystem::PostInitialize()
{
	// Nothing.
}

void CClientNetworkSystem::Shutdown()
{
	m_Interface = nullptr;
}

spdlog::logger* CClientNetworkSystem::GetLogger() const
{
	return g_Networking.GetLogger();
}

void CClientNetworkSystem::RunFrame()
{
	CheckForServerConnection();

	PollIncomingMessages();
	m_Interface->RunCallbacks();
}

void CClientNetworkSystem::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	switch (pInfo->m_info.m_eState)
	{
	case k_ESteamNetworkingConnectionState_None:
		// NOTE: We will get callbacks here when we destroy connections. You can ignore these.
		break;

	case k_ESteamNetworkingConnectionState_Connecting:
	{
		// We initiate connections so this can be ignored.
		break;
	}

	case k_ESteamNetworkingConnectionState_Connected:
		GetLogger()->debug("GNS connection established");
		break;

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		// Print an appropriate message
		if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
		{
			const int errorCategory = pInfo->m_info.m_eEndReason / 1000;

			GetLogger()->error("GNS connection closed (category: {}) ({})", net::GetReasonCategory(errorCategory), pInfo->m_info.m_szEndDebug);
		}
		else if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
		{
			GetLogger()->error("GNS connection closed due to error ({})", pInfo->m_info.m_szEndDebug);
		}
		else
		{
			GetLogger()->debug("GNS connection closed ({})", pInfo->m_info.m_szEndDebug);
		}

		// Clean up the connection.  This is important!
		// The connection is "closed" in the network sense, but
		// it has not been destroyed.  We must close it on our end, too
		// to finish up.  The reason information do not matter in this case,
		// and we cannot linger because it's already closed on the other end,
		// so we just pass 0's.
		m_Interface->CloseConnection(pInfo->m_hConn, k_ESteamNetConnectionEnd_App_Generic, nullptr, false);

		m_Connection = k_HSteamNetConnection_Invalid;
		break;
	}

	default:
		// Silences -Wswitch
		break;
	}
}

void CClientNetworkSystem::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	g_ClientNetworking.OnSteamNetConnectionStatusChanged(pInfo);
}

void CClientNetworkSystem::CheckForServerConnection()
{
	net_status_t status;
	gEngfuncs.pNetAPI->Status(&status);

	if (!status.connected)
	{
		// Reset in two cases: we're not currently connected or we got a port string.
		// If the string is invalid we'll end up disconnecting anyway,
		// but players could connect to another server while connected so it needs to be reset in both locations to work.
		m_ConnectStartTime = {};
		m_CurrentServerAddress = {};

		CloseConnection("Closing connection because we are no longer connected to a server");
	}

	// If we're still connected to the same server then we don't need to do anything.
	// If we have an active connection to a server then we cache the connection time for map command execution detection.
	// The map command restarts the server, so for listen server hosts it counts as a new connection.
	const bool isNewConnection = status.connected && (status.remote_address != m_CurrentServerAddress || status.connection_time < m_ConnectionTime);

	// Attempt to connect to the server if necessary.
	// The server may not have received our engine level ClientConnect message yet, so this could take a few frames.
	// The initial timeout is pretty generous so this isn't likely to be a problem.
	if (isNewConnection)
	{
		CloseConnection("Closing connection because we are connecting to another server");

		const std::string_view port{gEngfuncs.ServerInfo_ValueForKey(net::GNSPortKeyName)};

		if (port.empty())
		{
			// Wait a bit before deciding the port value is unavailable.
			if (m_ConnectStartTime == std::chrono::high_resolution_clock::time_point{})
			{
				m_ConnectStartTime = std::chrono::high_resolution_clock::now();
			}
			else if ((std::chrono::high_resolution_clock::now() - m_ConnectStartTime) > net::NetworkConnectTimeout)
			{
				// We'll get here if the serverinfo buffer doesn't contain the port keyvalue.
				GetLogger()->error("Timed out attempting to set up GNS connection");
				UTIL_DisconnectFromServer();
				return;
			}

			// Haven't parsed the fullserverinfo command yet or the buffer doesn't contain the keyvalue.
			return;
		}

		// No longer need to track this.
		m_ConnectStartTime = {};

		const auto portValue = net::ParseGNSPort(port);

		if (!portValue)
		{
			GetLogger()->error("GNS port is not valid");
			UTIL_DisconnectFromServer();
			return;
		}

		const auto adr = status.remote_address;

		// Sanity check to prevent weird edge cases. The engine should never give us an address that isn't one of these.
		if (adr.type != NA_LOOPBACK && adr.type != NA_IP)
		{
			GetLogger()->error("Cannot connect to addresses of type {}", adr.type);
			UTIL_DisconnectFromServer();
			return;
		}

		m_CurrentServerAddress = status.remote_address;

		SteamNetworkingIPAddr serverAddr;

		if (adr.type == NA_LOOPBACK)
		{
			serverAddr.SetIPv4(net::LoopbackIPAddress, *portValue);
		}
		else
		{
			serverAddr.SetIPv4(adr.ip[0] << 24 | adr.ip[1] << 16 | adr.ip[2] << 8 | adr.ip[3], *portValue);
		}

		char addressString[SteamNetworkingIPAddr::k_cchMaxString];
		serverAddr.ToString(addressString, std::size(addressString), true);
		GetLogger()->debug("Connecting to server at {}", addressString);

		SteamNetworkingConfigValue_t opt;
		opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, SteamNetConnectionStatusChangedCallback);

		m_Connection = m_Interface->ConnectByIPAddress(serverAddr, 1, &opt);

		if (m_Connection == k_HSteamNetConnection_Invalid)
		{
			GetLogger()->error("Couldn't create GNS connection");
			UTIL_DisconnectFromServer();
			return;
		}
	}

	// We'll only get here if we're not connected, if we successfully initiated a connection or if we're fully connected.
	if (status.connected && status.remote_address == m_CurrentServerAddress)
	{
		m_ConnectionTime = status.connection_time;
	}
	else
	{
		m_ConnectionTime = 0;
	}
}

void CClientNetworkSystem::CloseConnection(const char* reason)
{
	if (m_Connection != k_HSteamNetConnection_Invalid)
	{
		m_Interface->CloseConnection(
			m_Connection, k_ESteamNetConnectionEnd_App_Generic, reason, false);
		m_Connection = k_HSteamNetConnection_Invalid;
	}
}

void CClientNetworkSystem::PollIncomingMessages()
{
	if (m_Connection == k_HSteamNetConnection_Invalid)
	{
		return;
	}

	while (true)
	{
		ISteamNetworkingMessage* pIncomingMsg = nullptr;
		int numMsgs = m_Interface->ReceiveMessagesOnConnection(m_Connection, &pIncomingMsg, 1);
		if (numMsgs == 0)
			break;
		if (numMsgs < 0)
		{
			GetLogger()->error("Error checking for messages");
			UTIL_DisconnectFromServer();
			break;
		}
		assert(numMsgs == 1 && pIncomingMsg);

		// TODO: process incoming messages.

		// We don't need this anymore.
		pIncomingMsg->Release();
	}
}
