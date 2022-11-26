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
#include "utils/GameSystem.h"

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

constexpr bool IsValidSkillLevel(int skillLevel)
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

	void NewMapStarted();

	void LoadSkillConfigFile();

	constexpr int GetSkillLevel() const { return m_SkillLevel; }

	void SetSkillLevel(int skillLevel);

	/**
	 *	@brief take the name of a variable, tack a digit for the skill level on, and return the value of that variable
	 */
	float GetValue(std::string_view name) const;

	void SetValue(std::string_view name, float value);

	void RemoveValue(std::string_view name);

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
	if (!IsValidSkillLevel(skillLevel))
	{
		m_Logger->error("SetSkillLevel: new level \"{}\" out of range", skillLevel);
		return;
	}

	m_SkillLevel = skillLevel;

	m_Logger->info("GAME SKILL LEVEL: {}", m_SkillLevel);
}
