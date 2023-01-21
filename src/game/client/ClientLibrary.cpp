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

#include <SDL2/SDL_messagebox.h>

#include "hud.h"
#include "cbase.h"
#include "ClientLibrary.h"
#include "net_api.h"
#include "parsemsg.h"
#include "view.h"

#include "networking/ClientUserMessages.h"
#include "networking/NetworkDataSystem.h"

#include "sound/ClientSoundReplacement.h"
#include "sound/IGameSoundSystem.h"
#include "sound/IMusicSystem.h"
#include "sound/ISoundSystem.h"

#include "utils/ReplacementMaps.h"

void MsgFunc_SoundRpl(const char* pszName, int iSize, void* pbuf);

bool ClientLibrary::Initialize()
{
	if (!GameLibrary::Initialize())
	{
		return false;
	}

	gpGlobals->pStringBase = "";

	// Enable buffering for non-debug print output so it isn't ignored outright by the engine.
	Con_SetPrintBufferingEnabled(true);

	g_ClientUserMessages.RegisterHandler("SoundRpl", &MsgFunc_SoundRpl);

	return true;
}

void ClientLibrary::HudInit()
{
	m_AllowDownload = g_ConCommands.GetCVar("cl_allowdownload");

	// Has to be done here because music cvars don't exist at Initialize time.
	sound::CreateSoundSystem();

	sound::g_SoundSystem->GetGameSoundSystem()->LoadSentences();
}

void ClientLibrary::VidInit()
{
	// Remove downloaded files only. The server is responsible for removing generated files.
	// This has to happen now since not all cases are caught in RunFrame.
	g_NetworkData.RemoveNetworkDataFiles("GAMEDOWNLOAD");
}

void ClientLibrary::PostInitialize()
{
	Con_SetPrintBufferingEnabled(false);
}

void ClientLibrary::ClientActivated()
{
	if (m_Activated)
	{
		return;
	}

	m_Activated = true;

	CheckNetworkDataFile();
}

void ClientLibrary::Shutdown()
{
	sound::g_SoundSystem.reset();

	GameLibrary::Shutdown();

	// Clear handlers in case of restart.
	g_ClientUserMessages.Clear();
}

void ClientLibrary::RunFrame()
{
	// Force the download cvar to enabled so we can download network data.
	if (m_AllowDownload->value == 0)
	{
		gEngfuncs.Cvar_Set(m_AllowDownload->name, "1");
	}

	// Check connection status.
	net_status_t status;
	gEngfuncs.pNetAPI->Status(&status);

	const bool isConnected = status.connected != 0;

	auto mapName = gEngfuncs.pfnGetLevelName();

	if (!mapName)
	{
		mapName = "";
	}

	if (m_IsConnected != isConnected || m_ConnectionTime > status.connection_time || m_ServerAddress != status.remote_address || m_MapName.comparei(mapName) != 0)
	{
		m_Activated = false;
		m_NetworkDataFileLoaded = false;

		// If we're connecting to a dedicated server then remove the local config files as well
		// so the engine won't think the old locally generated one is what we need.
		// The map name changes to a valid name after the server address has been initialized so
		// this should wait long enough, but will run before the engine checks to see if precached files
		// are present on the client side.
		if (isConnected && mapName[0] != '\0' && status.remote_address.type != NA_LOOPBACK)
		{
			g_NetworkData.RemoveNetworkDataFiles("GAMECONFIG");
		}

		// Stop all sounds if we connect, disconnect, start a new map using "map" (resets connection time), change servers or change maps.
		sound::g_SoundSystem->GetGameSoundSystem()->StopAllSounds();
		sound::g_SoundSystem->GetGameSoundSystem()->ClearCaches();

		// Clear sound replacement map on any change.
		g_ReplacementMaps.Clear();
		sound::g_ClientSoundReplacement = &ReplacementMap::Empty;

		if (!isConnected || m_ConnectionTime > status.connection_time || m_ServerAddress != status.remote_address)
		{
			// Stop music playback if we disconnect or start a new map.
			sound::g_SoundSystem->GetMusicSystem()->Stop();
		}

		m_IsConnected = isConnected;
		m_ConnectionTime = status.connection_time;
		m_ServerAddress = status.remote_address;
		m_MapName = mapName;
		m_BaseMapName = m_MapName;

		if (const auto extensionStart = m_BaseMapName.find_last_of('.'); extensionStart != m_BaseMapName.npos)
		{
			m_BaseMapName.erase(extensionStart);
		}

		if (const auto lastDirEnd = m_BaseMapName.find_last_of('/'); lastDirEnd != m_BaseMapName.npos)
		{
			m_BaseMapName.erase(0, lastDirEnd + 1);
		}

		gpGlobals->mapname = MAKE_STRING(m_BaseMapName.c_str());
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

void ClientLibrary::OnUserMessageReceived()
{
	CheckNetworkDataFile();
}

void ClientLibrary::CheckNetworkDataFile()
{
	if (!m_NetworkDataFileLoaded)
	{
		// Always set this so we only show errors once.
		m_NetworkDataFileLoaded = true;

		// If the file failed to load for any reason then our client state will be invalid.
		if (!g_NetworkData.TryLoadNetworkDataFile())
		{
			g_GameLogger->critical("Could not load network data file; disconnecting from server");
			gEngfuncs.pfnClientCmd("disconnect\n");

			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Disconnected", R"(You have been disconnected from the server.

The reason given was:
Could not load network data file; needed to continue.
Check the console for more information.)",
				nullptr);

			return;
		}
	}
}

void ClientLibrary::AddGameSystems()
{
	GameLibrary::AddGameSystems();
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

void MsgFunc_SoundRpl(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const char* replacementFileName = READ_STRING();

	sound::g_ClientSoundReplacement = g_ReplacementMaps.Load(replacementFileName, {.CaseSensitive = false, .LoadFromAllPaths = true});
}
