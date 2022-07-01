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

#include <limits>

#include <steam/isteamnetworkingutils.h>

#include "cbase.h"
#include "CServerNetworkSystem.h"
#include "networking/CNetworkSystem.h"
#include "utils/ConCommandSystem.h"

bool CServerNetworkSystem::Initialize()
{
	// 27015 is the default internet port
	// 27020 is the default HLTV port
	// TODO: needs double checking to make sure this port is not in use.
	m_GNSPort = g_ConCommands.CreateCVar("gns_port", "27025");

	m_Interface = SteamNetworkingSockets();

	return true;
}

void CServerNetworkSystem::PostInitialize()
{
	// Nothing.
}

void CServerNetworkSystem::Shutdown()
{
	ShutdownServer();
	m_Interface = nullptr;
}

spdlog::logger* CServerNetworkSystem::GetLogger() const
{
	return g_Networking.GetLogger();
}

void CServerNetworkSystem::RunFrame()
{
	PollIncomingMessages();
	m_Interface->RunCallbacks();
	UpdateClients();
}

void CServerNetworkSystem::OnNewMapStarted()
{
	// The server is started on first map start, giving time to initialize cvars.
	// The server isn't stopped on map end because we may be performing a changelevel, so connections have to carry over.
	StartServer();
}

bool CServerNetworkSystem::OnClientConnect(edict_t* entity, std::string&& ipAddress)
{
	const int index = ENTINDEX(entity) - 1;

	auto& client = m_Clients[index];

	if (client.Connected)
	{
		// This should never happen. The engine tells the server about all client disconnects,
		// so unless there is some external tampering going on (e.g. AMXModX plugins refusing connections) this shouldn't happen.
		GetLogger()->error("Attempting to use client {} slot without properly clearing it first", index);
		DisconnectClient(client, k_ESteamNetConnectionEnd_AppException_Generic, "Closing unterminated connection");
	}

	SteamNetworkingIPAddr addr;

	// For listen servers the local player has this address.
	if (ipAddress == "loopback")
	{
		addr.SetIPv4(net::LoopbackIPAddress, 0);
	}
	else
	{
		// Chop off the port so we can compare the IP address on its own.
		if (const auto end = ipAddress.find(':'); end != ipAddress.npos)
		{
			ipAddress.erase(end);
		}

		if (!addr.ParseString(ipAddress.c_str()))
		{
			return false;
		}
	}

	client.Connected = true;
	client.IPAddress = addr;
	client.ConnectTime = std::chrono::high_resolution_clock::now();

	// Set the port on serverinfo now so the client can get it.
	auto serverinfo = g_engfuncs.pfnGetInfoKeyBuffer(INDEXENT(0));

	g_engfuncs.pfnSetKeyValue(serverinfo, net::GNSPortKeyName, std::to_string(m_GNSPortValue).c_str());

	// Connection is still invalid, waiting on client to initiate the connection to us when they're ready.

	return true;
}

void CServerNetworkSystem::OnClientDisconnect(edict_t* entity)
{
	const int index = ENTINDEX(entity) - 1;

	auto& client = m_Clients[index];

	if (!client.Connected)
	{
		// Shouldn't happen unless something is tampering with the game.
		GetLogger()->error("Attempting to disconnect client {} slot that was never in use", index);
		return;
	}

	DisconnectClient(client, k_ESteamNetConnectionEnd_App_Generic, "Closing connection");
}

CServerNetworkSystem::Client* CServerNetworkSystem::GetClientByConnection(HSteamNetConnection conn)
{
	if (auto it = std::find_if(m_Clients.begin(), m_Clients.end(), [&](const auto& client)
			{ return client.Connection == conn; });
		it != m_Clients.end())
	{
		return &(*it);
	}

	return nullptr;
}

void CServerNetworkSystem::KickClient(Client* client, const char* message)
{
	if (client->Kicked)
	{
		// Don't try to kick multiple times.
		return;
	}

	client->Kicked = true;

	const auto entityIndex = (client - m_Clients.data()) + 1;

	if (!UTIL_IsDedicatedServer() && entityIndex == 1)
	{
		// Can't kick the local player, so shut the server down instead.
		GetLogger()->error("Shutting server down: {}", message);
		UTIL_ShutdownServer();
		return;
	}

	auto entity = g_engfuncs.pfnPEntityOfEntIndex(entityIndex);

	if (entity != nullptr)
	{
		UTIL_KickPlayer(entity, message);
	}
}

bool CServerNetworkSystem::StartServer()
{
	if (IsActive())
	{
		return true;
	}

	const auto success = [&]()
	{
		const auto portValue = net::ParseGNSPort(m_GNSPort->string);

		if (!portValue)
		{
			GetLogger()->error("Port \"{}\" is invalid", m_GNSPort->string);
			return false;
		}

		// Cache it so cvar changes don't affect client connections.
		m_GNSPortValue = *portValue;

		SteamNetworkingIPAddr serverLocalAddr;
		// Setting Ipv4 0.0.0.0 should force the server into Ipv4 mode. We only have Ipv4 addresses to work with so this is necessary.
		serverLocalAddr.SetIPv4(0x00000000, m_GNSPortValue);

		SteamNetworkingConfigValue_t opt;
		opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, SteamNetConnectionStatusChangedCallback);

		m_Socket = m_Interface->CreateListenSocketIP(serverLocalAddr, 1, &opt);

		if (m_Socket == k_HSteamListenSocket_Invalid)
		{
			GetLogger()->error("Failed to listen on port {}", serverLocalAddr.m_port);
			return false;
		}

		m_PollGroup = m_Interface->CreatePollGroup();

		if (m_PollGroup == k_HSteamNetPollGroup_Invalid)
		{
			m_Interface->CloseListenSocket(m_Socket);
			m_Socket = k_HSteamListenSocket_Invalid;
			GetLogger()->error("Failed to listen on port {}", serverLocalAddr.m_port);
			return false;
		}

		return true;
	}();

	if (!success)
	{
		// Shut the server down so we don't try to continue in an invalid state.
		UTIL_ShutdownServer();
	}

	return success;
}

void CServerNetworkSystem::ShutdownServer()
{
	// Forcefully disconnect clients if they are still connected.
	// Should never happen since the engine drops all clients which should notify us.
	for (auto& client : m_Clients)
	{
		if (client.Connected)
		{
			DisconnectClient(client, k_ESteamNetConnectionEnd_App_Generic, "Server Shutdown");
		}
	}

	m_Interface->CloseListenSocket(m_Socket);
	m_Socket = k_HSteamListenSocket_Invalid;

	m_Interface->DestroyPollGroup(m_PollGroup);
	m_PollGroup = k_HSteamNetPollGroup_Invalid;
}

void CServerNetworkSystem::DisconnectClient(Client& client, int reason, const char* debug)
{
	// It's possible for a client to disconnect without having ever fully connected, so Connection can be invalid.
	m_Interface->CloseConnection(client.Connection, reason, debug, false);

	GetLogger()->trace("Disconnecting client with reason {}, debug message \"{}\"", reason, debug);

	client = {};
}

void CServerNetworkSystem::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	switch (pInfo->m_info.m_eState)
	{
	case k_ESteamNetworkingConnectionState_None:
		// NOTE: We will get callbacks here when we destroy connections. You can ignore these.
		break;

	case k_ESteamNetworkingConnectionState_Connecting:
	{
		// This must be a new connection
		assert(!GetClientByConnection(pInfo->m_hConn));

		GetLogger()->debug("Connection request from {}", pInfo->m_info.m_szConnectionDescription);

		// Find the connection's IP address and assign connection.
		const bool success = [&]()
		{
			SteamNetConnectionInfo_t info;
			if (!m_Interface->GetConnectionInfo(pInfo->m_hConn, &info))
			{
				return false;
			}

			auto addr = info.m_addrRemote;

			// The stored address has port 0 so we have to match that.
			addr.m_port = 0;

			for (auto& client : m_Clients)
			{
				if (client.IPAddress == addr)
				{
					client.Connection = pInfo->m_hConn;
					return true;
				}
			}

			return false;
		}();

		if (!success)
		{
			// This can happen if there is a race between the engine's and our connection, so this isn't an error.
			m_Interface->CloseConnection(pInfo->m_hConn, k_ESteamNetConnectionEnd_App_Generic, nullptr, false);
			GetLogger()->debug("Can't accept connection (Could not find client IP in list, engine is probably waiting on ConnectClient message)");
			break;
		}

		// A client is attempting to connect
		// Try to accept the connection.
		if (m_Interface->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
		{
			// This could fail. If the remote host tried to connect,
			// but then disconnected, the connection may already be half closed.
			// Just destroy whatever we have on our side.
			m_Interface->CloseConnection(pInfo->m_hConn, k_ESteamNetConnectionEnd_App_Generic, nullptr, false);
			GetLogger()->info("Can't accept connection (It was already closed)");
			break;
		}

		// Assign the poll group
		if (!m_Interface->SetConnectionPollGroup(pInfo->m_hConn, m_PollGroup))
		{
			m_Interface->CloseConnection(pInfo->m_hConn, k_ESteamNetConnectionEnd_App_Generic, nullptr, false);
			GetLogger()->info("Failed to set poll group");
			break;
		}

		break;
	}

	case k_ESteamNetworkingConnectionState_Connected:
	{
		// We will get a callback immediately after accepting the connection.
		// Since we are the server, we can ignore this, it's not news to us.

		//TODO: start sending data now.
		break;
	}

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		// Ignore if they were not previously connected.  (If they disconnected before we accepted the connection.)
		if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
		{
			auto client = GetClientByConnection(pInfo->m_hConn);

			if (!client)
			{
				// Client was disconnected by the server when they left.
			}
			else
			{
				// If we get here then the client is disconnecting by itself for some reason. It might be a race condition between the engine and us.

				// Select appropriate log messages
				const char* pszDebugLogAction;
				if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
				{
					pszDebugLogAction = "problem detected locally";
				}
				else
				{
					// Note that here we could check the reason code to see if it was a "usual" connection or an "unusual" one.
					pszDebugLogAction = "closed by peer";
				}

				GetLogger()->debug("Connection {} {}, reason {}: {}",
					pInfo->m_info.m_szConnectionDescription,
					pszDebugLogAction,
					pInfo->m_info.m_eEndReason,
					pInfo->m_info.m_szEndDebug);

				const char* message = "Kicked due to closed GNS connection";

				DisconnectClient(*client, k_ESteamNetConnectionEnd_App_Generic, message);

				// Also kick the player from the server, in case this is a bogus disconnect.
				KickClient(client, message);
				break;
			}
		}
		else
		{
			assert(pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting);
		}

		// Clean up the connection. This is important!
		// The connection is "closed" in the network sense, but it has not been destroyed.
		// We must close it on our end, too to finish up.
		// The reason information do not matter in this case,
		// and we cannot linger because it's already closed on the other end,
		// so we just pass 0's.
		m_Interface->CloseConnection(pInfo->m_hConn, k_ESteamNetConnectionEnd_App_Generic, nullptr, false);
		break;
	}

	default:
		// Silences -Wswitch
		break;
	}
}

void CServerNetworkSystem::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	g_ServerNetworking.OnSteamNetConnectionStatusChanged(pInfo);
}

void CServerNetworkSystem::PollIncomingMessages()
{
	while (true)
	{
		ISteamNetworkingMessage* pIncomingMsg = nullptr;
		int numMsgs = m_Interface->ReceiveMessagesOnPollGroup(m_PollGroup, &pIncomingMsg, 1);
		if (numMsgs == 0)
			break;
		if (numMsgs < 0)
		{
			GetLogger()->error("Error checking for messages");
			UTIL_ShutdownServer();
			break;
		}
		assert(numMsgs == 1 && pIncomingMsg);
		auto client = GetClientByConnection(pIncomingMsg->m_conn);
		assert(client != nullptr);

		// TODO: process incoming messages.

		// We don't need this anymore.
		pIncomingMsg->Release();
	}
}

void CServerNetworkSystem::UpdateClients()
{
	const auto now = std::chrono::high_resolution_clock::now();

	for (auto& client : m_Clients)
	{
		if (!client.Connected)
		{
			continue;
		}

		if (client.Connection == k_HSteamNetConnection_Invalid)
		{
			if ((now - client.ConnectTime) > net::NetworkConnectTimeout)
			{
				// Kick the player so they don't hang around with a broken connection.
				KickClient(&client, "Timed out GNS connection setup");
			}
		}
	}
}
