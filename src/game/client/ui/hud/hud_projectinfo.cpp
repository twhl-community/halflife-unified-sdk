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

bool CHudProjectInfo::Init()
{
	gHUD.AddHudElem(this);

	m_iFlags |= HUD_ACTIVE;

	m_IsAlphaBuild = g_ProjectInfo.IsAlphaBuild(*g_ProjectInfo.GetLocalInfo());

	// Turn this on by default if this is a (pre-)alpha build.
	// Users will easily know they're running a (pre-)alpha build and screenshots and videos will include important information.
	m_ShowProjectInfo = CVAR_CREATE("cl_projectinfo_show", m_IsAlphaBuild ? "1" : "0", 0);

	return true;
}

bool CHudProjectInfo::VidInit()
{
	return true;
}

bool CHudProjectInfo::Draw(float flTime)
{
	if (m_IsAlphaBuild || m_ShowProjectInfo->value > 0)
	{
		const int xPos = 20;

		int lineWidth, lineHeight;
		GetConsoleStringSize("", &lineWidth, &lineHeight);

		// Shrink line height a bit.
		lineHeight = static_cast<int>(lineHeight * 0.9f);

		int yPos = static_cast<int>(ScreenHeight * 0.05);

		const auto lineDrawer = [&](const std::string& text, const RGB24& color = {255, 255, 255})
		{
			gHUD.DrawHudString(xPos, yPos, ScreenWidth - xPos, text.c_str(), color);
			yPos += lineHeight;
		};

		const auto libraryDrawer = [&](const LibraryInfo& info, const char* libraryName, const RGB24& libraryNameColor)
		{
			lineDrawer(fmt::format("{}:", libraryName), libraryNameColor);
			lineDrawer(fmt::format("\tVersion: {}.{}.{}-{} | Branch: {} | Tag: {}",
				info.MajorVersion, info.MinorVersion, info.PatchVersion, info.ReleaseType, info.BranchName, info.TagName));
			lineDrawer(fmt::format("\tCommit Hash: {}", info.CommitHash));
			lineDrawer(fmt::format("\tBuild Timestamp: {}", info.BuildTimestamp));
		};

		const auto clientInfo = g_ProjectInfo.GetLocalInfo();
		const auto serverInfo = g_ProjectInfo.GetServerInfo();

		// The server's build type isn't important enough to send over.
		lineDrawer(fmt::format("Build type: {}", UNIFIED_SDK_CONFIG));

		libraryDrawer(*clientInfo, "Client", {128, 128, 255});
		libraryDrawer(*serverInfo, "Server", {255, 128, 128});

		yPos += lineHeight;

		if (m_IsAlphaBuild)
		{
			lineDrawer("Work In Progress build not suited for use (testing individual features by request only)");
		}

		if (clientInfo->CommitHash != serverInfo->CommitHash)
		{
			lineDrawer("Warning: Client and server builds do not match");
		}
	}

	return true;
}
