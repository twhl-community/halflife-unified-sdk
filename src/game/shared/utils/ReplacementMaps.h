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

using Replacements = std::unordered_map<std::string, std::string, TransparentStringHash, TransparentEqual>;

/**
 *	@brief A map of string to string for replacements.
 */
struct ReplacementMap
{
	explicit ReplacementMap() = default;

	explicit ReplacementMap(Replacements&& replacements)
		: m_Replacements(std::move(replacements))
	{
	}

	ReplacementMap(const ReplacementMap&) = delete;
	ReplacementMap& operator=(const ReplacementMap&) = delete;
	ReplacementMap(ReplacementMap&&) = default;
	ReplacementMap& operator=(ReplacementMap&&) = default;

	bool empty() const noexcept { return m_Replacements.empty(); }

	const Replacements& GetAll() const noexcept { return m_Replacements; }

	const char* Lookup(const char* value, bool lowercase) const noexcept;

private:
	Replacements m_Replacements;
};

struct ReplacementMapOptions
{
	/**
	*	@brief If @c true, converts all strings to lowercase.
	*/
	bool ConvertToLowercase = false;

	/**
	*	@brief If @c true, searches all paths for the file.
	*/
	bool LoadFromAllPaths = false;
};

/**
*	@brief Handles the registration of the replacement map schema and the loading of files.
*/
class ReplacementMapSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "ReplacementMap"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override;

	ReplacementMap Load(const std::string& fileName, const ReplacementMapOptions& options = {}) const;

private:
	ReplacementMap Parse(const json& input, const ReplacementMapOptions& options) const;

private:
	std::shared_ptr<spdlog::logger> m_Logger;
};

inline ReplacementMapSystem g_ReplacementMaps;
