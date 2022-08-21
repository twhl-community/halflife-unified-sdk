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

#include "hud.h"
#include "cbase.h"
#include "ClientLibrary.h"
#include "net_api.h"
#include "parsemsg.h"
#include "view.h"
#include "sound/ClientSoundReplacement.h"
#include "sound/IGameSoundSystem.h"
#include "sound/IMusicSystem.h"
#include "sound/ISoundSystem.h"
#include "utils/ReplacementMaps.h"

int MsgFunc_SoundRpl(const char* pszName, int iSize, void* pbuf);

bool ClientLibrary::Initialize()
{
	if (!InitializeCommon())
	{
		return false;
	}

	gEngfuncs.pfnHookUserMsg("SoundRpl", &MsgFunc_SoundRpl);

	return true;
}

void ClientLibrary::HudInit()
{
	//Has to be done here because music cvars don't exist at Initialize time.
	sound::CreateSoundSystem();

	sound::g_SoundSystem->GetGameSoundSystem()->LoadSentences();
}

void ClientLibrary::Shutdown()
{
	sound::g_SoundSystem.reset();

	ShutdownCommon();
}

void ClientLibrary::Frame()
{
	// Check connection status.
	net_status_t status;
	gEngfuncs.pNetAPI->Status(&status);

	const bool isConnected = status.connected != 0;

	auto mapName = gEngfuncs.pfnGetLevelName();

	if (!mapName)
	{
		mapName = "";
	}

	if (m_IsConnected != isConnected
		|| m_ConnectionTime > status.connection_time
		|| m_ServerAddress != status.remote_address
		|| m_MapName.comparei(mapName) != 0)
	{
		// Stop all sounds if we connect, disconnect, start a new map using "map" (resets connection time), change servers or change maps.
		sound::g_SoundSystem->GetGameSoundSystem()->StopAllSounds();
		sound::g_SoundSystem->GetGameSoundSystem()->ClearCaches();

		// Clear sound replacement map on any change.
		sound::g_ClientSoundReplacement = ReplacementMap{};

		if (!isConnected
			|| m_ConnectionTime > status.connection_time
			|| m_ServerAddress != status.remote_address)
		{
			// Stop music playback if we disconnect or start a new map.
			sound::g_SoundSystem->GetMusicSystem()->Stop();
		}

		m_IsConnected = isConnected;
		m_ConnectionTime = status.connection_time;
		m_ServerAddress = status.remote_address;
		m_MapName = mapName;
	}

	// Unblock audio if we can't find the window.
	if (auto window = FindWindow(); !window || (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0)
	{
		sound::g_SoundSystem->Unblock();
	}
	else
	{
		sound::g_SoundSystem->Block();
	}

	if (g_Paused)
	{
		sound::g_SoundSystem->Pause();
	}
	else
	{
		sound::g_SoundSystem->Resume();
	}

	sound::g_SoundSystem->Update();
}

SDL_Window* ClientLibrary::FindWindow()
{
	// Find the game window. The window id is a unique identifier that increments starting from 1, so we can cache the value to speed up lookup.
	while (m_WindowId < std::numeric_limits<Uint32>::max())
	{
		if (auto window = SDL_GetWindowFromID(m_WindowId); window)
		{
			return window;
		}

		++m_WindowId;
	}

	return nullptr;
}

int MsgFunc_SoundRpl(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const char* replacementFileName = READ_STRING();

	sound::g_ClientSoundReplacement = g_ReplacementMaps.Load(replacementFileName, {.ConvertToLowercase = true, .LoadFromAllPaths = true});

	return 1;
}
