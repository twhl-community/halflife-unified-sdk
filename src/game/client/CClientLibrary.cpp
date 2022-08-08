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
#include "CClientLibrary.h"
#include "view.h"
#include "sound/IMusicSystem.h"
#include "sound/ISoundSystem.h"

bool CClientLibrary::Initialize()
{
	if (!InitializeCommon())
	{
		return false;
	}

	return true;
}

void CClientLibrary::HudInit()
{
	//Has to be done here because music cvars don't exist at Initialize time.
	CreateSoundSystem();
}

void CClientLibrary::Shutdown()
{
	g_SoundSystem.reset();

	ShutdownCommon();
}

void CClientLibrary::Frame()
{
	// Unblock audio if we can't find the window.
	if (auto window = FindWindow(); !window || (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0)
	{
		g_SoundSystem->Unblock();
	}
	else
	{
		g_SoundSystem->Block();
	}

	if (g_Paused)
	{
		g_SoundSystem->Pause();
	}
	else
	{
		g_SoundSystem->Resume();
	}

}

SDL_Window* CClientLibrary::FindWindow()
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
