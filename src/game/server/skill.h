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

struct skilldata_t
{
	// Monster Health & Damage
	float agruntHealth;
	float agruntDmgPunch;

	float apacheHealth;

	float barneyHealth;

	float otisHealth;

	float bigmommaHealthFactor; // Multiply each node's health by this
	float bigmommaDmgSlash;		// melee attack damage
	float bigmommaDmgBlast;		// mortar attack damage
	float bigmommaRadiusBlast;	// mortar attack radius

	float bullsquidHealth;
	float bullsquidDmgBite;
	float bullsquidDmgWhip;
	float bullsquidDmgSpit;

	float pitdroneHealth;
	float pitdroneDmgBite;
	float pitdroneDmgWhip;
	float pitdroneDmgSpit;

	float gargantuaHealth;
	float gargantuaDmgSlash;
	float gargantuaDmgFire;
	float gargantuaDmgStomp;

	float hassassinHealth;

	float headcrabHealth;
	float headcrabDmgBite;

	float shockroachHealth;
	float shockroachDmgBite;
	float shockroachLifespan;

	float hgruntHealth;
	float hgruntDmgKick;
	float hgruntShotgunPellets;
	float hgruntGrenadeSpeed;

	float hgruntAllyHealth;
	float hgruntAllyDmgKick;
	float hgruntAllyShotgunPellets;
	float hgruntAllyGrenadeSpeed;

	float medicAllyHealth;
	float medicAllyDmgKick;
	float medicAllyGrenadeSpeed;
	float medicAllyHeal;

	float torchAllyHealth;
	float torchAllyDmgKick;
	float torchAllyGrenadeSpeed;

	float massassinHealth;
	float massassinDmgKick;
	float massassinGrenadeSpeed;

	float shocktrooperHealth;
	float shocktrooperDmgKick;
	float shocktrooperGrenadeSpeed;
	float shocktrooperMaxCharge;
	float shocktrooperRechargeSpeed;

	float houndeyeHealth;
	float houndeyeDmgBlast;

	float slaveHealth;
	float slaveDmgClaw;
	float slaveDmgClawrake;
	float slaveDmgZap;

	float ichthyosaurHealth;
	float ichthyosaurDmgShake;

	float leechHealth;
	float leechDmgBite;

	float controllerHealth;
	float controllerDmgZap;
	float controllerSpeedBall;
	float controllerDmgBall;

	float nihilanthHealth;
	float nihilanthZap;

	float scientistHealth;

	float cleansuitScientistHealth;

	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;

	float voltigoreHealth;
	float voltigoreDmgBeam;
	float voltigoreDmgPunch;

	float babyvoltigoreHealth;
	float babyvoltigoreDmgPunch;

	float pitWormHealth;
	float pitWormDmgSwipe;
	float pitWormDmgBeam;

	float geneWormHealth;
	float geneWormDmgSpit;
	float geneWormDmgHit;

	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;

	float zombieBarneyHealth;
	float zombieBarneyDmgOneSlash;
	float zombieBarneyDmgBothSlash;

	float zombieSoldierHealth;
	float zombieSoldierDmgOneSlash;
	float zombieSoldierDmgBothSlash;

	float gonomeDmgGuts;
	float gonomeHealth;
	float gonomeDmgOneSlash;
	float gonomeDmgOneBite;

	float turretHealth;
	float miniturretHealth;
	float sentryHealth;


	// Player Weapons
	float plrDmgCrowbar;
	float plrDmg9MM;
	float plrDmg357;
	float plrDmgMP5;
	float plrDmgM203Grenade;
	float plrDmgBuckshot;
	float plrDmgCrossbowClient;
	float plrDmgCrossbowMonster;
	float plrDmgRPG;
	float plrDmgGauss;
	float plrDmgEgonNarrow;
	float plrDmgEgonWide;
	float plrDmgHornet;
	float plrDmgHandGrenade;
	float plrDmgSatchel;
	float plrDmgTripmine;

	float plrDmgPipewrench;
	float plrDmgKnife;
	float plrDmgGrapple;
	float plrDmgEagle;
	float plrDmg762;
	float plrDmg556;
	float plrDmgDisplacerSelf;
	float plrDmgDisplacerOther;
	float plrRadiusDisplacer;
	float plrDmgShockRoachS;
	float plrDmgShockRoachM;
	float plrDmgSpore;

	// weapons shared by monsters
	float monDmg9MM;
	float monDmgMP5;
	float monDmg12MM;
	float monDmgHornet;

	// health/suit charge
	float suitchargerCapacity;
	float batteryCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
	float scientistHeal;
	float cleansuitScientistHeal;

	// monster damage adj
	float monHead;
	float monChest;
	float monStomach;
	float monLeg;
	float monArm;

	// player damage adj
	float plrHead;
	float plrChest;
	float plrStomach;
	float plrLeg;
	float plrArm;
};

inline DLL_GLOBAL skilldata_t gSkillData;

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
