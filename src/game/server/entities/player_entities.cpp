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
#include "basemonster.h"

class PlayerSetHudColor : public CPointEntity
{
public:
	enum class Action
	{
		Set = 0,
		Reset = 1
	};

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool KeyValue(KeyValueData* pkvd) override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	Vector m_HudColor{vec3_origin};
	Action m_Action{Action::Set};
};

LINK_ENTITY_TO_CLASS(player_sethudcolor, PlayerSetHudColor);

TYPEDESCRIPTION PlayerSetHudColor::m_SaveData[] =
	{
		DEFINE_FIELD(PlayerSetHudColor, m_HudColor, FIELD_VECTOR),
		DEFINE_FIELD(PlayerSetHudColor, m_Action, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(PlayerSetHudColor, CPointEntity);

bool PlayerSetHudColor::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "hud_color"))
	{
		UTIL_StringToVector(m_HudColor, pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "action"))
	{
		m_Action = static_cast<Action>(atoi(pkvd->szValue));
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void PlayerSetHudColor::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!pActivator || !pActivator->IsPlayer())
	{
		return;
	}

	auto player = static_cast<CBasePlayer*>(pActivator);

	if (!player->IsNetClient())
	{
		return;
	}

	const RGB24 color = [this]()
	{
		switch (m_Action)
		{
		case Action::Set:
			return RGB24{
				static_cast<std::uint8_t>(m_HudColor.x),
				static_cast<std::uint8_t>(m_HudColor.y),
				static_cast<std::uint8_t>(m_HudColor.z)};

		default:
		case Action::Reset:
			return RGB_HUD_COLOR;
		}
	}();

	player->SetHudColor(color);
}

class CStripWeapons : public CPointEntity
{
public:
	static constexpr int StripFlagAllPlayers = 1 << 0;
	static constexpr int StripFlagRemoveWeapons = 1 << 1;
	static constexpr int StripFlagRemoveSuit = 1 << 2;
	static constexpr int StripFlagRemoveLongJump = 1 << 3;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool KeyValue(KeyValueData* pkvd) override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	int m_Flags = StripFlagRemoveWeapons;
};

LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

TYPEDESCRIPTION CStripWeapons::m_SaveData[] =
	{
		DEFINE_FIELD(CStripWeapons, m_Flags, FIELD_INTEGER)};

IMPLEMENT_SAVERESTORE(CStripWeapons, CPointEntity);

bool CStripWeapons::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "all_players"))
	{
		if (atoi(pkvd->szValue) != 0)
		{
			SetBits(m_Flags, StripFlagAllPlayers);
		}
		else
		{
			ClearBits(m_Flags, StripFlagAllPlayers);
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "strip_weapons"))
	{
		if (atoi(pkvd->szValue) != 0)
		{
			SetBits(m_Flags, StripFlagRemoveWeapons);
		}
		else
		{
			ClearBits(m_Flags, StripFlagRemoveWeapons);
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "strip_suit"))
	{
		if (atoi(pkvd->szValue) != 0)
		{
			SetBits(m_Flags, StripFlagRemoveSuit);
		}
		else
		{
			ClearBits(m_Flags, StripFlagRemoveSuit);
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "strip_longjump"))
	{
		if (atoi(pkvd->szValue) != 0)
		{
			SetBits(m_Flags, StripFlagRemoveLongJump);
		}
		else
		{
			ClearBits(m_Flags, StripFlagRemoveLongJump);
		}

		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CStripWeapons::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	const auto executor = [this](CBasePlayer* player)
	{
		if ((m_Flags & StripFlagRemoveWeapons) != 0)
		{
			player->RemoveAllItems(false);
		}

		if ((m_Flags & StripFlagRemoveSuit) != 0)
		{
			player->SetHasSuit(false);
		}

		if ((m_Flags & StripFlagRemoveLongJump) != 0)
		{
			player->SetHasLongJump(false);
		}
	};

	if ((m_Flags & StripFlagAllPlayers) != 0)
	{
		for (auto player : UTIL_FindPlayers())
		{
			executor(player);
		}
	}
	else
	{
		CBasePlayer* player = ToBasePlayer(pActivator);

		if (!player && !g_pGameRules->IsDeathmatch())
		{
			player = UTIL_GetLocalPlayer();
		}

		if (player)
		{
			executor(player);
		}
	}
}

/**
 *	@brief Sets the player's health and/or armor to a mapper-specified value
 */
class CPlayerSetHealth : public CPointEntity
{
public:
	static constexpr int FlagAllPlayers = 1 << 0;
	static constexpr int FlagSetHealth = 1 << 1;
	static constexpr int FlagSetArmor = 1 << 2;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool KeyValue(KeyValueData* pkvd) override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	int m_Flags = 0;
	int m_Health = 100;
	int m_Armor = 0;
};

LINK_ENTITY_TO_CLASS(player_sethealth, CPlayerSetHealth);

TYPEDESCRIPTION CPlayerSetHealth::m_SaveData[] =
	{
		DEFINE_FIELD(CPlayerSetHealth, m_Flags, FIELD_INTEGER),
		DEFINE_FIELD(CPlayerSetHealth, m_Health, FIELD_INTEGER),
		DEFINE_FIELD(CPlayerSetHealth, m_Armor, FIELD_INTEGER)};

IMPLEMENT_SAVERESTORE(CPlayerSetHealth, CPointEntity);

bool CPlayerSetHealth::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "all_players"))
	{
		if (atoi(pkvd->szValue) != 0)
		{
			SetBits(m_Flags, FlagAllPlayers);
		}
		else
		{
			ClearBits(m_Flags, FlagAllPlayers);
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "set_health"))
	{
		if (atoi(pkvd->szValue) != 0)
		{
			SetBits(m_Flags, FlagSetHealth);
		}
		else
		{
			ClearBits(m_Flags, FlagSetHealth);
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "set_armor"))
	{
		if (atoi(pkvd->szValue) != 0)
		{
			SetBits(m_Flags, FlagSetArmor);
		}
		else
		{
			ClearBits(m_Flags, FlagSetArmor);
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "health_to_set"))
	{
		// Clamp to 1 so players don't end up in an invalid state
		m_Health = std::max(atoi(pkvd->szValue), 1);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "armor_to_set"))
	{
		m_Armor = std::max(atoi(pkvd->szValue), 0);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CPlayerSetHealth::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	const auto executor = [this](CBasePlayer* player)
	{
		if ((m_Flags & FlagSetHealth) != 0)
		{
			// Clamp it to the current max health
			player->pev->health = std::min(static_cast<int>(std::floor(player->pev->max_health)), m_Health);
		}

		if ((m_Flags & FlagSetArmor) != 0)
		{
			// Clamp it now, in case future changes allow for custom armor maximum
			player->pev->armorvalue = std::min(MAX_NORMAL_BATTERY, m_Armor);
		}
	};

	if ((m_Flags & FlagAllPlayers) != 0)
	{
		for (auto player : UTIL_FindPlayers())
		{
			executor(player);
		}
	}
	else
	{
		CBasePlayer* player = ToBasePlayer(pActivator);

		if (!player && !g_pGameRules->IsMultiplayer())
		{
			player = UTIL_GetLocalPlayer();
		}

		if (player)
		{
			executor(player);
		}
	}
}
