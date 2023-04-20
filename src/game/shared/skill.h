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

#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>

#include <spdlog/logger.h>

#include "networking/NetworkDataSystem.h"
#include "utils/json_fwd.h"
#include "utils/GameSystem.h"

class BufferReader;

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

enum class SkillVarType
{
	Float = 0,
	Integer
};

struct SkillVarConstraints
{
	std::optional<float> Minimum{std::nullopt};
	std::optional<float> Maximum{std::nullopt};
	bool Networked{false};
	SkillVarType Type{SkillVarType::Float};
};

/**
 *	@brief Loads skill variables from files and provides a means of looking them up.
 */
class SkillSystem final : public IGameSystem, public INetworkDataBlockHandler
{
private:
	// Change to std::uint16_t and change WRITE_BYTE to WRITE_SHORT to have more networked variables.
	// Note: must make sure values > 32767 are sent correctly in that case!
	using MessageIndex = std::uint8_t;

	// 0 is a valid variable index.
	static constexpr int MaxNetworkedVariables = std::numeric_limits<MessageIndex>::max() + 1;

	static constexpr int NotNetworkedIndex = -1;
	static constexpr int LargestNetworkedIndex = std::numeric_limits<MessageIndex>::max();

	static constexpr int SingleMessageSize = sizeof(MessageIndex) + sizeof(float);

	enum VarFlag
	{
		VarFlag_IsExplicitlyDefined = 1 << 0,
	};

	struct SkillVariable
	{
		std::string Name;
		float CurrentValue = 0;
		float InitialValue = 0;
		float NetworkedValue = 0;
		SkillVarConstraints Constraints;
		int NetworkIndex = NotNetworkedIndex;
		int Flags = 0;
	};

public:
	const char* GetName() const override { return "Skill"; }

	bool Initialize() override;
	void PostInitialize() override {}
	void Shutdown() override;

	void HandleNetworkDataBlock(NetworkDataBlock& block) override;

	void LoadSkillConfigFiles(std::span<const std::string> fileNames);

	constexpr SkillLevel GetSkillLevel() const { return m_SkillLevel; }

	void SetSkillLevel(SkillLevel skillLevel);

	void DefineVariable(std::string name, float initialValue, const SkillVarConstraints& constraints = {});

	/**
	 *	@brief Gets the value for a given skill variable.
	 */
	float GetValue(std::string_view name, float defaultValue = 0.f) const;

	void SetValue(std::string_view name, float value);

#ifndef CLIENT_DLL
	void SendAllNetworkedSkillVars(CBasePlayer* player);
#endif

private:
	static float ClampValue(float value, const SkillVarConstraints& constraints);

	bool ParseConfiguration(const json& input);

#ifdef CLIENT_DLL
	void MsgFunc_SkillVars(BufferReader& reader);
#endif

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	SkillLevel m_SkillLevel = SkillLevel::Easy;

	std::vector<SkillVariable> m_SkillVariables;

	int m_NextNetworkedIndex = 0;

	bool m_LoadingSkillFiles = false;
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
