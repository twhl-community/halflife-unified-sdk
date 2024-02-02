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

#include "cbase.h"
#include "UserMessages.h"
#include "config/CommandWhitelist.h"

class CLogicSetCVar : public CPointEntity
{
	DECLARE_CLASS(CLogicSetCVar, CPointEntity);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	string_t m_CVarName;
	string_t m_CVarValue;

	// Not saved, used to speed up repeated calls and log diagnostics.
	cvar_t* m_CVar{};
};

BEGIN_DATAMAP(CLogicSetCVar)
DEFINE_FIELD(m_CVarName, FIELD_STRING),
	DEFINE_FIELD(m_CVarValue, FIELD_STRING),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(logic_setcvar, CLogicSetCVar);

bool CLogicSetCVar::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "cvar_name"))
	{
		m_CVarName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "cvar_value"))
	{
		m_CVarValue = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CLogicSetCVar::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator, m_UseLocked))
	{
		return;
	}

	const auto cvarName = STRING(m_CVarName);
	const auto cvarValue = STRING(m_CVarValue);

	Logger->trace("{}:{}:{}: Attempting to set cvar", GetClassname(), entindex(), GetTargetname());

	// Commands should be evaluated on execution in case we've saved and restored and the whitelist has changed.
	if (!g_CommandWhitelist.contains(cvarName))
	{
		Logger->error(
			"The console variable \"{} {}\" cannot be changed because it is not listed in the command whitelist",
			cvarName, cvarValue);
		return;
	}

	if (!m_CVar)
	{
		m_CVar = CVAR_GET_POINTER(cvarName);

		if (!m_CVar)
		{
			Logger->error("The console variable \"{}\" does not exist");
			return;
		}
	}

	Logger->debug("Changing cvar \"{}\" from \"{}\" to \"{}\"", cvarName, m_CVar->string, cvarValue);
	g_engfuncs.pfnCvar_DirectSet(m_CVar, cvarValue);
}

constexpr std::string_view SkillKeyPrefix{"skill"sv};

// If the first skill level is not 1 then this code needs changing.
static_assert(int(SkillLevel::Easy) == 1);

static int ValidateSkillLevel(CBaseEntity* entity, int skillLevel)
{
	if (skillLevel < int(SkillLevel::Easy) || skillLevel > int(SkillLevel::Hard))
	{
		CBaseEntity::Logger->error("{}:{}: Invalid skill level \"{}\" specified",
			entity->GetClassname(), entity->entindex(), skillLevel);
		skillLevel = int(SkillLevel::Easy);
	}

	return skillLevel;
}

class CLogicIsSkill : public CPointEntity
{
	DECLARE_CLASS(CLogicIsSkill, CPointEntity);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	string_t m_Targets[SkillLevelCount];
};

BEGIN_DATAMAP(CLogicIsSkill)
DEFINE_ARRAY(m_Targets, FIELD_STRING, SkillLevelCount),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(logic_isskill, CLogicIsSkill);

bool CLogicIsSkill::KeyValue(KeyValueData* pkvd)
{
	if (std::string_view(pkvd->szKeyName).starts_with(SkillKeyPrefix))
	{
		int skillLevel = atoi(pkvd->szKeyName + SkillKeyPrefix.size());

		skillLevel = ValidateSkillLevel(this, skillLevel);

		auto& target = m_Targets[skillLevel - 1];

		if (FStringNull(target))
		{
			target = ALLOC_STRING(pkvd->szValue);
		}
		else
		{
			Logger->error("{}:{}: Multiple targets specified for skill level {}", GetClassname(), entindex(), skillLevel);
		}

		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CLogicIsSkill::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator, m_UseLocked))
	{
		return;
	}

	const int skillLevelIndex = int(g_Skill.GetSkillLevel()) - 1;

	assert(skillLevelIndex >= 0 && skillLevelIndex < SkillLevelCount);

	string_t target = m_Targets[skillLevelIndex];

	if (FStringNull(target))
	{
		return;
	}

	FireTargets(STRING(target), pActivator, this, USE_ON, 0);
}

class CLogicSetSkill : public CPointEntity
{
	DECLARE_CLASS(CLogicSetSkill, CPointEntity);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	int m_SkillLevel = 0;
};

BEGIN_DATAMAP(CLogicSetSkill)
DEFINE_FIELD(m_SkillLevel, FIELD_INTEGER),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(logic_setskill, CLogicSetSkill);

bool CLogicSetSkill::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "skill_level"))
	{
		m_SkillLevel = ValidateSkillLevel(this, atoi(pkvd->szValue));
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CLogicSetSkill::Spawn()
{
	// Check to make sure the skill level was specified at all.
	m_SkillLevel = ValidateSkillLevel(this, m_SkillLevel);
}

void CLogicSetSkill::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator, m_UseLocked))
	{
		return;
	}

	CVAR_SET_FLOAT("skill", m_SkillLevel);

	// Cached variables will be unaffected.
	g_Skill.SetSkillLevel(SkillLevel(m_SkillLevel));

	SUB_UseTargets(pActivator, USE_TOGGLE, 0);
}

class CLogicSetSkillVar : public CPointEntity
{
	DECLARE_CLASS(CLogicSetSkillVar, CPointEntity);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	string_t m_Name;
	float m_Value;
};

BEGIN_DATAMAP(CLogicSetSkillVar)
DEFINE_FIELD(m_Name, FIELD_STRING),
	DEFINE_FIELD(m_Value, FIELD_FLOAT),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(logic_setskillvar, CLogicSetSkillVar);

bool CLogicSetSkillVar::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "var_name"))
	{
		m_Name = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "var_value"))
	{
		m_Value = atof(pkvd->szValue);
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CLogicSetSkillVar::Spawn()
{
	if (FStringNull(m_Name))
	{
		Logger->error("{}:{}:{}: No variable name set", GetClassname(), entindex(), GetTargetname());
	}
}

void CLogicSetSkillVar::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator, m_UseLocked))
	{
		return;
	}

	g_Skill.SetValue(STRING(m_Name), m_Value);

	SUB_UseTargets(pActivator, USE_TOGGLE, 0);
}

class CLogicCampaignSelect : public CPointEntity
{
	DECLARE_CLASS(CLogicCampaignSelect, CPointEntity);

public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

LINK_ENTITY_TO_CLASS(logic_campaignselect, CLogicCampaignSelect);

void CLogicCampaignSelect::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (auto player = ToBasePlayer(pActivator); player)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgCampaignSelect, nullptr, player);
		MESSAGE_END();
	}
}

constexpr int MaxRandomTargets = 16;
constexpr std::string_view TargetKeyValuePrefix{"target"sv};

class CLogicRandom : public CBaseDelay
{
	DECLARE_CLASS(CLogicRandom, CBaseDelay);
	DECLARE_DATAMAP();

public:
	int ObjectCaps() override { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	bool KeyValue(KeyValueData* pkvd) override;

	void Spawn() override;

	void RandomUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

private:
	eastl::fixed_vector<std::uint8_t, MaxRandomTargets, false> BuildMap() const;

private:
	string_t m_iszTargets[MaxRandomTargets];
};

BEGIN_DATAMAP(CLogicRandom)
DEFINE_ARRAY(m_iszTargets, FIELD_STRING, MaxRandomTargets),
	DEFINE_FUNCTION(RandomUse),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(logic_random, CLogicRandom);

bool CLogicRandom::KeyValue(KeyValueData* pkvd)
{
	if (std::string_view{pkvd->szKeyName}.starts_with(TargetKeyValuePrefix))
	{
		const char* indexString = pkvd->szKeyName + TargetKeyValuePrefix.size();

		char* end;

		int index = strtol(indexString, &end, 10);

		// Must be a number and end after the number, e.g. "target1"
		// index is 1 based [1, MaxRandomTargets]
		if (end != indexString && *end == '\0')
		{
			--index;

			if (index >= 0 && index < MaxRandomTargets)
			{
				if (FStringNull(m_iszTargets[index]))
				{
					m_iszTargets[index] = ALLOC_STRING(pkvd->szValue);
				}
				else
				{
					Logger->error("{}: target \"{}\" already set to \"{}\"",
						GetClassname(), pkvd->szKeyName, STRING(m_iszTargets[index]));
				}
			}
			else
			{
				Logger->error("{}: invalid target index \"{}\", must be in range [1, {}]",
					GetClassname(), pkvd->szKeyName, MaxRandomTargets);
			}
		}
		else
		{
			Logger->error("{}: invalid target format \"{}\": must be {}[1, {}]",
				GetClassname(), pkvd->szKeyName, TargetKeyValuePrefix, MaxRandomTargets);
		}

		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CLogicRandom::Spawn()
{
	SetUse(&CLogicRandom::RandomUse);
}

void CLogicRandom::RandomUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (const auto map = BuildMap(); !map.empty())
	{
		const int index = RANDOM_LONG(0, map.size() - 1);

		const int targetIndex = map[index];

		const char* target = STRING(m_iszTargets[targetIndex]);

		FireTargets(target, pActivator, this, useType, 0);
	}
}

eastl::fixed_vector<std::uint8_t, MaxRandomTargets, false> CLogicRandom::BuildMap() const
{
	eastl::fixed_vector<std::uint8_t, MaxRandomTargets, false> map;

	for (int i = 0; i < MaxRandomTargets; ++i)
	{
		if (!FStringNull(m_iszTargets[i]))
		{
			map.push_back(i);
		}
	}

	return map;
}
