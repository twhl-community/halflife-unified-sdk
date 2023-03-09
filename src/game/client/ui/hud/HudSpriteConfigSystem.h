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
#include <vector>

#include "hud.h"
#include "utils/GameSystem.h"

class HudSpriteConfigSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "HudSpriteConfig"; }
	bool Initialize() override;
	void PostInitialize() override {}
	void Shutdown() override {}

	std::vector<HudSprite> Load(const char* fileName);
};

inline HudSpriteConfigSystem g_HudSpriteConfig;

const HudSprite* GetSpriteList(const std::vector<HudSprite>& sprites, const char* spriteName);
