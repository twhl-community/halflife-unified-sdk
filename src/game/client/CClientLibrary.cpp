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

#include "hud.h"
#include "cbase.h"
#include "CClientLibrary.h"
#include "view.h"
#include "sound/IMusicSystem.h"

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
	m_SoundSystem = CreateSoundSystem();
}

void CClientLibrary::Shutdown()
{
	m_SoundSystem.reset();

	ShutdownCommon();
}

void CClientLibrary::Frame()
{
	//Only do this on change to prevent problems from constantly calling a threaded function.
	if (m_PreviousPausedState != g_Paused)
	{
		m_PreviousPausedState = g_Paused;

		if (g_Paused)
		{
			m_SoundSystem->GetMusicSystem()->Pause();
		}
		else
		{
			m_SoundSystem->GetMusicSystem()->Resume();
		}
	}
}
