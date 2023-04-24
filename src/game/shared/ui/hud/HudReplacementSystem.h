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

#include <string>
#include <unordered_map>

#include "networking/NetworkDataSystem.h"
#include "utils/GameSystem.h"

using WeaponHudReplacements = std::unordered_map<std::string, std::string>;

class HudReplacementSystem final : public IGameSystem, public INetworkDataBlockHandler
{
public:
	const char* GetName() const override { return "HudReplacements"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override {}

	void HandleNetworkDataBlock(NetworkDataBlock& block) override;

	void SetWeaponHudReplacementFiles(WeaponHudReplacements&& fileNames);

	std::string HudReplacementFileName;
};

inline HudReplacementSystem g_HudReplacements;
