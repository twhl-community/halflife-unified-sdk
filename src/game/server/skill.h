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
//=========================================================
// skill.h - skill level concerns
//=========================================================

#pragma once

#include <array>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>

#include <spdlog/logger.h>

#include "json_fwd.h"
#include "utils/GameSystem.h"

enum class SkillLevel
{
	Easy = 1,
	Medium = 2,
	Hard = 3
};

constexpr auto format_as(SkillLevel s) { return fmt::underlying(s); }

constexpr int SkillLevelCount = static_cast<int>(SkillLevel::Hard);

constexpr bool IsValidSkillLevel(SkillLevel skillLevel)
{
	return skillLevel >= SkillLevel::Easy && skillLevel <= SkillLevel::Hard;
}

class SkillSystem final : public IGameSystem
{
private:
	struct SkillVariable
	{
		std::string Name;
		float Value = 0;
	};

public:
	const char* GetName() const override { return "Skill"; }

	bool Initialize() override;
	void PostInitialize() override {}
	void Shutdown() override;

	void LoadSkillConfigFiles(std::span<const std::string> fileNames);

	constexpr SkillLevel GetSkillLevel() const { return m_SkillLevel; }

	void SetSkillLevel(SkillLevel skillLevel);

	/**
	 *	@brief Gets the value for a given skill variable.
	 */
	float GetValue(std::string_view name) const;

	void SetValue(std::string_view name, float value);

	void RemoveValue(std::string_view name);

private:
	bool ParseConfiguration(const json& input);

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	SkillLevel m_SkillLevel = SkillLevel::Easy;

	std::vector<SkillVariable> m_SkillVariables;
};

inline SkillSystem g_Skill;

inline void SkillSystem::SetSkillLevel(SkillLevel skillLevel)
{
	if (!IsValidSkillLevel(skillLevel))
	{
		m_Logger->error("SetSkillLevel: new level \"{}\" out of range", skillLevel);
		return;
	}

	m_SkillLevel = skillLevel;

	m_Logger->info("GAME SKILL LEVEL: {}", m_SkillLevel);
}
