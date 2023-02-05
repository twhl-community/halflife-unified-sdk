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

	void PostInitialize();

	void Shutdown() override;

	void RunFrame() override;

private:
	SDL_Window* FindWindow();

private:
	bool m_IsConnected = false;
	float m_ConnectionTime = 0;
	netadr_t m_ServerAddress;
	Filename m_MapName;

	Uint32 m_WindowId = 0;
};

inline ClientLibrary g_Client;
