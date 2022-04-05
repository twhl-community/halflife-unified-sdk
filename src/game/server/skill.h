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
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/logger.h>

#include "json_fwd.h"

namespace SkillLevel
{
enum SkillLevel
{
	Easy = 1,
	Medium = 2,
	Hard = 3
};
}

constexpr int SkillLevelCount = SkillLevel::Hard;

class SkillSystem final
{
private:
	struct SkillVariable
	{
		std::string Name;
		std::array<float, SkillLevelCount> Values;
	};

public:

	bool Initialize();
	void Shutdown();

	void NewMapStarted();

	void LoadSkillConfigFile();

	constexpr int GetSkillLevel() const { return m_SkillLevel; }

	void SetSkillLevel(int skillLevel);

	/**
	*	@brief take the name of a variable, tack a digit for the skill level on, and return the value of that variable
	*/
	float GetValue(std::string_view name) const;

	void SetValue(std::string_view name, int skillLevel, float value);

	void SetValue(std::string_view name, float value);

private:
	bool ParseConfiguration(const json& input);

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	int m_SkillLevel = SkillLevel::Easy;

	std::vector<SkillVariable> m_SkillVariables;
};

inline SkillSystem g_Skill;

inline void SkillSystem::SetSkillLevel(int skillLevel)
{
	if (skillLevel < SkillLevel::Easy || skillLevel > SkillLevel::Hard)
	{
		m_Logger->error("SetSkillLevel: new level \"{}\" out of range", skillLevel);
		return;
	}

	m_SkillLevel = skillLevel;

	m_Logger->info("GAME SKILL LEVEL: {}", m_SkillLevel);
}

inline void SkillSystem::SetValue(std::string_view name, float value)
{
	SetValue(name, m_SkillLevel, value);
}
