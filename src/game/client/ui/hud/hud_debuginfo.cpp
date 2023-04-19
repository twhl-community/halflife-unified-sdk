/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "ProjectInfoSystem.h"
#include "view.h"

bool CHudDebugInfo::Init()
{
	gHUD.AddHudElem(this);

	m_iFlags |= HUD_ACTIVE;

	m_ShowDebugInfo = CVAR_CREATE("cl_debuginfo_show", "0", 0);

	return true;
}

bool CHudDebugInfo::VidInit()
{
	m_GameMode = gEngfuncs.ServerInfo_ValueForKey("gm");

	// If the game mode string is empty then we've loaded a save game or moved through a level transition.
	if (m_GameMode.empty())
	{
		m_GameMode = "singleplayer";
	}

	return true;
}

bool CHudDebugInfo::Draw(float flTime)
{
	if (m_ShowDebugInfo->value > 0)
	{
		const int xPos = 20;

		int lineWidth, lineHeight;
		GetConsoleStringSize("", &lineWidth, &lineHeight);

		// Shrink line height a bit.
		lineHeight = static_cast<int>(lineHeight * 0.9f);

		int yPos = static_cast<int>(ScreenHeight * 0.5f);

		const auto lineDrawer = [&](const std::string& text, const RGB24& color = {255, 255, 255})
		{
			gHUD.DrawHudString(xPos, yPos, ScreenWidth - xPos, text.c_str(), color);
			yPos += lineHeight;
		};

		const auto levelName = gEngfuncs.pfnGetLevelName();
		const auto localPlayer = gEngfuncs.GetLocalPlayer();

		if (const auto levelName = gEngfuncs.pfnGetLevelName(); levelName && '\0' != levelName[0] && localPlayer)
		{
			lineDrawer("Current game time:", {128, 64, 255});
			lineDrawer(fmt::format("  {}", SecondsToTime(int(gHUD.m_flTime))));

			lineDrawer("Map name:", {255, 128, 0});
			lineDrawer(fmt::format("  {}", levelName));

			lineDrawer("Game mode:", {255, 0, 255});
			lineDrawer(fmt::format("  {}", m_GameMode));

			lineDrawer("Player Origin:", {0, 255, 0});

			lineDrawer(fmt::format("  X: {:+09.2f}", localPlayer->curstate.origin.x));
			lineDrawer(fmt::format("  Y: {:+09.2f}", localPlayer->curstate.origin.y));
			lineDrawer(fmt::format("  Z: {:+09.2f}", localPlayer->curstate.origin.z));

			lineDrawer("Player Angles:", {0, 128, 255});
			// Angles are modified in the view code so we need to use the original angles.
			// Pitch is inverted for players so it needs to be inverted again.
			// Roll is always 0 so don't show it.
			lineDrawer(fmt::format("  Pitch: {:+07.2f}", -v_client_aimangles.x));
			lineDrawer(fmt::format("  Yaw: {:+07.2f}", v_client_aimangles.y));
		}
	}

	return true;
}
