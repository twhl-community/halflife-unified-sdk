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

#include <memory>
#include <string>
#include <unordered_map>

#include <spdlog/logger.h>

#include "utils/GameSystem.h"
#include "utils/heterogeneous_lookup.h"
#include "utils/json_fwd.h"

class CBaseEntity;

using EntityTemplateMap = std::unordered_map<std::string, std::string, TransparentStringHash, TransparentEqual>;

class EntityTemplateSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "EntityTemplates"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override;

	void LoadTemplates(const EntityTemplateMap& templateMap);

	void MaybeApplyTemplate(CBaseEntity* entity);

private:
	std::unordered_map<std::string, std::string> LoadTemplate(const json& input);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_Templates;
};

inline EntityTemplateSystem g_EntityTemplates;
