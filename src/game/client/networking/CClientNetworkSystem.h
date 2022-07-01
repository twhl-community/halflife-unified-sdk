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
#include "netadr.h"
#include "networking/CNetworkSystem.h"

/**
 *	@brief On client connect, receives a set of immutable data from the server.
 */
class CClientNetworkSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "ClientNetworking"; }

	bool Initialize() override;

	void PostInitialize() override;

	void Shutdown() override;

	spdlog::logger* GetLogger() const;

	void RunFrame();

private:
	void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

	static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

	void CheckForServerConnection();

	void CloseConnection(const char* reason);

	void PollIncomingMessages();

private:
	ISteamNetworkingSockets* m_Interface = nullptr;

	HSteamNetConnection m_Connection = k_HSteamNetConnection_Invalid;

	std::chrono::high_resolution_clock::time_point m_ConnectStartTime;

	netadr_t m_CurrentServerAddress{};
	double m_ConnectionTime = 0;
};

inline CClientNetworkSystem g_ClientNetworking;
