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
#include "ProjectInfo.h"

DECLARE_MESSAGE(m_ProjectInfo, ProjectInfo);

bool CHudProjectInfo::Init()
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(ProjectInfo);

	m_iFlags |= HUD_ACTIVE;

	const char* showProjectInfoDefault = "0";

	//Turn this on by default if this is an alpha build.
	//Users will easily know they're running an alpha build and screenshots and videos will include important information.
	if (0 == stricmp(UnifiedSDKReleaseType, "Alpha"))
	{
		showProjectInfoDefault = "1";
	}

	m_ShowProjectInfo = CVAR_CREATE("cl_showprojectinfo", showProjectInfoDefault, 0);

	m_ClientInfo.MajorVersion = UnifiedSDKVersionMajor;
	m_ClientInfo.MinorVersion = UnifiedSDKVersionMinor;
	m_ClientInfo.PatchVersion = UnifiedSDKVersionPatch;

	m_ClientInfo.BranchName = UnifiedSDKGitBranchName;
	m_ClientInfo.TagName = UnifiedSDKGitTagName;
	m_ClientInfo.CommitHash = UnifiedSDKGitCommitHash;

	return true;
}

bool CHudProjectInfo::VidInit()
{
	//New server, reset info.
	m_ServerInfo = {};
	return true;
}

bool CHudProjectInfo::Draw(float flTime)
{
	if (m_ShowProjectInfo->value > 0)
	{
		const int xPos = 20;
		const int lineHeight = 20;

		int yPos = static_cast<int>(ScreenHeight * 0.05);

		const auto lineDrawer = [&](const std::string& text, const RGB24& color = {255, 255, 255})
		{
			gHUD.DrawHudString(xPos, yPos, ScreenWidth - xPos, text.c_str(), color);
			yPos += lineHeight;
		};

		const auto libraryDrawer = [&](const LibraryInfo& info, const char* libraryName, const RGB24& libraryNameColor)
		{
			lineDrawer(fmt::format("{}:", libraryName), libraryNameColor);

			if (info.MajorVersion != -1)
			{
				lineDrawer(fmt::format("\tVersion: {}.{}.{}", info.MajorVersion, info.MinorVersion, info.PatchVersion));
				lineDrawer(fmt::format("\tBranch: {}", info.BranchName));
				lineDrawer(fmt::format("\tTag: {}", info.TagName));
				lineDrawer(fmt::format("\tCommit Hash: {}", info.CommitHash));
			}
			else
			{
				lineDrawer("\tUnknown (incompatible server, or branch and/or tag names may be too long)");
			}
		};

		//The server's build type isn't important enough to send over.
		lineDrawer(fmt::format("Build type: {}", UNIFIED_SDK_CONFIG));
		lineDrawer(fmt::format("Release type: {}", UnifiedSDKReleaseType));

		libraryDrawer(m_ClientInfo, "Client", {128, 128, 255});
		libraryDrawer(m_ServerInfo, "Server", {255, 128, 128});

		yPos += lineHeight;

		if (m_ClientInfo.MajorVersion != m_ServerInfo.MajorVersion || m_ClientInfo.MinorVersion != m_ServerInfo.MinorVersion || m_ClientInfo.PatchVersion != m_ServerInfo.PatchVersion)
		{
			lineDrawer("Warning: Client and server versions do not match");
		}

		if (m_ClientInfo.CommitHash != m_ServerInfo.CommitHash)
		{
			lineDrawer("Warning: Client and server builds do not match");
		}
	}

	return true;
}

bool CHudProjectInfo::MsgFunc_ProjectInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_ServerInfo.MajorVersion = READ_LONG();
	m_ServerInfo.MinorVersion = READ_LONG();
	m_ServerInfo.PatchVersion = READ_LONG();

	m_ServerInfo.BranchName = READ_STRING();
	m_ServerInfo.TagName = READ_STRING();
	m_ServerInfo.CommitHash = READ_STRING();

	m_ServerInfo.MajorVersion = 2;
	m_ServerInfo.CommitHash[0] = 'f';

	return true;
}
