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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "basemonster.h"
#include "player.h"

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

TYPEDESCRIPTION	PlayerSetHudColor::m_SaveData[] =
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
		case Action::Set: return RGB24{
			static_cast<std::uint8_t>(m_HudColor.x),
			static_cast<std::uint8_t>(m_HudColor.y),
			static_cast<std::uint8_t>(m_HudColor.z)
		};

		default:
		case Action::Reset: return RGB_HUD_COLOR;
		}
	}();

	player->SetHudColor(color);
}
