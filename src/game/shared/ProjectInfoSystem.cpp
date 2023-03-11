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

#include "cbase.h"
// This should be the only source file to include this header since it changes often.
#include "ProjectInfo.h"
#include "ProjectInfoSystem.h"

const LibraryInfo ProjectInfoSystem::m_LocalInfo{
	.MajorVersion = UnifiedSDKVersionMajor,
	.MinorVersion = UnifiedSDKVersionMinor,
	.PatchVersion = UnifiedSDKVersionPatch,

	.ReleaseType = UnifiedSDKReleaseType,

	.BranchName = UnifiedSDKGitBranchName,
	.TagName = UnifiedSDKGitTagName,
	.CommitHash = UnifiedSDKGitCommitHash,

	// This changes every time this file is built.
	// Since this file has to be rebuilt anyway whenever Git status changes
	// it will reflect the latest build even without forcing it to rebuild this file.
	.BuildTimestamp = __TIME__ " " __DATE__};

bool ProjectInfoSystem::Initialize()
{
	g_ConCommands.CreateCommand("projectinfo_print", [this](const auto&)
		{ PrintLocalInfo(); });

	#ifdef CLIENT_DLL
	g_ConCommands.CreateCommand("projectinfo_print_all", [this](const auto&)
		{ PrintAllInfo(); });
	#endif

	return true;
}

bool ProjectInfoSystem::IsAlphaBuild(const LibraryInfo& info)
{
	std::string releaseType{info.ReleaseType};

	ToLower(releaseType);

	return releaseType.find("alpha") != std::string::npos;
}

void ProjectInfoSystem::PrintLocalInfo()
{
	PrintLibraryInfo(GetLongLibraryPrefix(), m_LocalInfo);
}

void ProjectInfoSystem::PrintAllInfo()
{
	PrintLocalInfo();

	if (m_ServerInfo.MajorVersion != -1)
	{
		PrintLibraryInfo("server", m_ServerInfo);
	}
	else
	{
		Con_Printf("Project info for server: Not connected to a server\n");
	}
}

void ProjectInfoSystem::PrintLibraryInfo(std::string_view name, const LibraryInfo& info)
{
	Con_Printf("Project info for %s:\n", name.data());
	Con_Printf("Version: %d.%d.%d-%s\n", info.MajorVersion, info.MinorVersion, info.PatchVersion, info.ReleaseType.c_str());
	Con_Printf("Branch: %s\n", info.BranchName.c_str());
	Con_Printf("Tag: %s\n", info.TagName.c_str());
	Con_Printf("Commit Hash: %s\n", info.CommitHash.c_str());
	Con_Printf("Build Timestamp: %s\n", info.BuildTimestamp.c_str());
}
