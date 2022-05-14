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

/**
*	@brief A map of string to string for model replacements, stored as lowercase filenames.
*	@details Do not modify these maps after they've been loaded. Any modification will invalidate pointers to values.
*/
using ModelReplacementMap = std::unordered_map<std::string, std::string, TransparentStringHash, TransparentEqual>;

/**
*	@brief Handles the registration of the model replacement schema and the loading of files.
*/
class ModelReplacementSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "ModelReplacement"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override;

	ModelReplacementMap LoadReplacementMap(const std::string& fileName) const;

private:
	ModelReplacementMap ParseModelReplacement(const json& input) const;

private:
	std::shared_ptr<spdlog::logger> m_Logger;
};

inline ModelReplacementSystem g_ModelReplacement;
