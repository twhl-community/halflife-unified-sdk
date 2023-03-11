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

#pragma once

#include <string>
#include <string_view>

#include "cbase.h"
#include "utils/GameSystem.h"

struct LibraryInfo
{
	int MajorVersion = -1;
	int MinorVersion = -1;
	int PatchVersion = -1;

	std::string ReleaseType;

	std::string BranchName;
	std::string TagName;
	std::string CommitHash;

	std::string BuildTimestamp;
};

/**
 *	@brief Provides access to local project info, stores received server project info.
 */
class ProjectInfoSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "ProjectInfo"; }

	bool Initialize() override;

	void PostInitialize() override {}
	void Shutdown() override {}

	const LibraryInfo* GetLocalInfo() const { return &m_LocalInfo; }

	const LibraryInfo* GetServerInfo() const { return &m_ServerInfo; }

	void SetServerInfo(LibraryInfo info)
	{
		m_ServerInfo = std::move(info);
	}

	static bool IsAlphaBuild(const LibraryInfo& info);

private:
	static void PrintLocalInfo();
	void PrintAllInfo();

	static void PrintLibraryInfo(std::string_view name, const LibraryInfo& info);

private:
	static const LibraryInfo m_LocalInfo;
	LibraryInfo m_ServerInfo{};
};

inline ProjectInfoSystem g_ProjectInfo;
