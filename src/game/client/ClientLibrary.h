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

#include <SDL2/SDL_video.h>

#include "GameLibrary.h"

#include "netadr.h"

struct cvar_t;

/**
 *	@brief Handles core client actions
 */
class ClientLibrary final : public GameLibrary
{
public:
	ClientLibrary() = default;
	~ClientLibrary() = default;

	ClientLibrary(const ClientLibrary&) = delete;
	ClientLibrary& operator=(const ClientLibrary&) = delete;
	ClientLibrary(ClientLibrary&&) = delete;
	ClientLibrary& operator=(ClientLibrary&&) = delete;

	bool Initialize() override;

	/**
	 *	@brief Called when the engine finishes up client initialization.
	 */
	void HudInit();

	/**
	 *	@brief Called whenever the client connects to a server.
	 */
	void VidInit();

	void PostInitialize();

	void ClientActivated();

	void Shutdown() override;

	void RunFrame() override;

	void OnUserMessageReceived();

	/**
	 *	@brief Checks if the network data file needs loading.
	 *	@details Should be called if it is possible for networked data to be accessed before the file is loaded.
	 *	In practice this means user messages that use such data.
	 */
	void CheckNetworkDataFile();

protected:
	void AddGameSystems() override;

private:
	SDL_Window* FindWindow();

private:
	bool m_IsConnected = false;
	float m_ConnectionTime = 0;
	netadr_t m_ServerAddress;
	Filename m_MapName;
	Filename m_BaseMapName;

	Uint32 m_WindowId = 0;

	cvar_t* m_AllowDownload{};

	bool m_Activated{false};
	bool m_NetworkDataFileLoaded{false};
};

inline ClientLibrary g_Client;

inline cvar_t* r_decals = nullptr;
