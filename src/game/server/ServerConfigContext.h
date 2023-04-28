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
#include <vector>

#include "EntityTemplateSystem.h"
#include "MapState.h"
#include "gamerules/PlayerInventory.h"
#include "ui/hud/HudReplacementSystem.h"

/**
 *	@brief Context used by config section parsers.
 */
struct ServerConfigContext
{
	MapState& State;

	std::vector<std::string> SentencesFiles;
	std::vector<std::string> MaterialsFiles;
	std::vector<std::string> SkillFiles;

	std::vector<std::string> GlobalModelReplacementFiles;
	std::vector<std::string> GlobalSentenceReplacementFiles;
	std::vector<std::string> GlobalSoundReplacementFiles;

	PlayerInventory SpawnInventory;

	EntityTemplateMap EntityTemplates;

	std::string EntityClassificationsFileName;

	std::string HudReplacementFile;

	WeaponHudReplacements WeaponHudReplacementFiles;
};
