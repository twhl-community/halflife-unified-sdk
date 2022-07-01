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

#include <array>
#include <chrono>
#include <string>

#include <spdlog/logger.h>

#include <steam/steamnetworkingsockets.h>

#include "GameSystem.h"
#include "networking/CNetworkSystem.h"

/**
 *	@brief On client connect, sends a set of immutable data to the client.
 */
class CServerNetworkSystem final : public IGameSystem
{
private:
	struct Client
	{
		/**
		 *	@brief Whether this client is currently connected to the server.
		 *	A client that is timing out is still considered to be connected.
		 */
		bool Connected = false;

		/**
		 *	@brief @c true if this client has been kicked and is awaiting disconnect processing.
		 */
		bool Kicked = false;

		/**
		 *	@brief The IPv4 address of this client, as given by the engine on connect.
		 */
		SteamNetworkingIPAddr IPAddress = net::EmptyIPAddress;

		/**
		 *	@brief The GNS connection.
		 */
		HSteamNetConnection Connection = k_HSteamNetConnection_Invalid;

		/**
		 *	@brief When the client connected. Only valid if @c Connected is true.
		 */
		std::chrono::high_resolution_clock::time_point ConnectTime;
	};

public:
	const char* GetName() const override { return "ServerNetworking"; }

	bool Initialize() override;

	void PostInitialize() override;

	void Shutdown() override;

	spdlog::logger* GetLogger() const;

	bool IsActive() const { return m_Socket != k_HSteamListenSocket_Invalid; }

	void RunFrame();

	void OnNewMapStarted();

	bool OnClientConnect(edict_t* entity, std::string&& ipAddress);

	void OnClientDisconnect(edict_t* entity);

private:
	Client* GetClientByConnection(HSteamNetConnection conn);

	void KickClient(Client* client, const char* message);

	bool StartServer();

	void ShutdownServer();

	void DisconnectClient(Client& client, int reason, const char* debug);

	void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

	void PollIncomingMessages();
	void UpdateClients();

private:
	cvar_t* m_GNSPort = nullptr;

	ISteamNetworkingSockets* m_Interface = nullptr;

	HSteamListenSocket m_Socket = k_HSteamListenSocket_Invalid;
	HSteamNetPollGroup m_PollGroup = k_HSteamNetPollGroup_Invalid;

	uint16 m_GNSPortValue = 0;

	std::array<Client, MAX_PLAYERS> m_Clients;
};

inline CServerNetworkSystem g_ServerNetworking;
