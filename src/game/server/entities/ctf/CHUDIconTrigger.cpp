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
#include "CHUDIconTrigger.h"

LINK_ENTITY_TO_CLASS(ctf_hudicon, CHUDIconTrigger);

void CHUDIconTrigger::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	SetModel(STRING(pev->model));
	pev->movetype = MOVETYPE_NONE;
	m_fIsActive = false;
}

void CHUDIconTrigger::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (m_flNextActiveTime <= gpGlobals->time && UTIL_IsMasterTriggered(m_sMaster, pActivator, m_UseLocked))
	{
		if (!FStringNull(pev->noise))
			EmitSound(CHAN_VOICE, STRING(pev->noise), VOL_NORM, ATTN_NORM);

		m_hActivator = pActivator;

		SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);

		if (!FStringNull(pev->message))
		{
			if (auto player = ToBasePlayer(pActivator); player)
			{
				UTIL_ShowMessage(STRING(pev->message), player);
			}
		}

		switch (useType)
		{
		case USE_OFF:
			m_fIsActive = false;
			break;

		case USE_ON:
			m_fIsActive = true;
			break;

		default:
			m_fIsActive = !m_fIsActive;
			break;
		}

		MESSAGE_BEGIN(MSG_ALL, gmsgCustomIcon);
		g_engfuncs.pfnWriteByte(static_cast<int>(m_fIsActive));
		g_engfuncs.pfnWriteByte(m_nCustomIndex);

		if (m_fIsActive)
		{
			g_engfuncs.pfnWriteString(STRING(m_iszCustomName));
			g_engfuncs.pfnWriteByte(pev->rendercolor.x);
			g_engfuncs.pfnWriteByte(pev->rendercolor.y);
			g_engfuncs.pfnWriteByte(pev->rendercolor.z);
			g_engfuncs.pfnWriteByte(m_nLeft);
			g_engfuncs.pfnWriteByte(m_nTop);
			g_engfuncs.pfnWriteByte(m_nWidth + m_nLeft);
			g_engfuncs.pfnWriteByte(m_nHeight + m_nTop);
		}

		g_engfuncs.pfnMessageEnd();

		m_flNextActiveTime = gpGlobals->time + m_flWait;

		if (m_fIsActive)
		{
			for (auto entity : UTIL_FindEntitiesByClassname<CHUDIconTrigger>("ctf_hudicon"))
			{
				if (this != entity && m_nCustomIndex == entity->m_nCustomIndex)
				{
					entity->m_fIsActive = false;
				}
			}
		}
	}
}
bool CHUDIconTrigger::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("icon_name", pkvd->szKeyName))
	{
		m_iszCustomName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq("icon_index", pkvd->szKeyName))
	{
		m_nCustomIndex = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq("icon_left", pkvd->szKeyName))
	{
		m_nLeft = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq("icon_top", pkvd->szKeyName))
	{
		m_nTop = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq("icon_width", pkvd->szKeyName))
	{
		m_nWidth = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq("icon_height", pkvd->szKeyName))
	{
		m_nHeight = atoi(pkvd->szValue);
		return true;
	}

	return false;
}

void CHUDIconTrigger::UpdateUser(CBasePlayer* pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgCustomIcon, nullptr, pPlayer);
	g_engfuncs.pfnWriteByte(static_cast<int>(m_fIsActive));
	g_engfuncs.pfnWriteByte(m_nCustomIndex);

	if (m_fIsActive)
	{
		g_engfuncs.pfnWriteString(STRING(m_iszCustomName));
		g_engfuncs.pfnWriteByte(pev->rendercolor.x);
		g_engfuncs.pfnWriteByte(pev->rendercolor.y);
		g_engfuncs.pfnWriteByte(pev->rendercolor.z);
		g_engfuncs.pfnWriteByte(m_nLeft);
		g_engfuncs.pfnWriteByte(m_nTop);
		g_engfuncs.pfnWriteByte(m_nWidth + m_nLeft);
		g_engfuncs.pfnWriteByte(m_nHeight + m_nTop);
	}

	g_engfuncs.pfnMessageEnd();
}

void RefreshCustomHUD(CBasePlayer* pPlayer)
{
	// TODO: this will break when an index is larger than 31 or a negative value
	int activeIcons = 0;

	for (auto entity : UTIL_FindEntitiesByClassname<CHUDIconTrigger>("ctf_hudicon"))
	{
		if ((activeIcons & (1 << entity->m_nCustomIndex)) == 0)
		{
			if (entity->m_fIsActive)
			{
				activeIcons |= 1 << entity->m_nCustomIndex;
			}

			entity->UpdateUser(pPlayer);
		}
	}
}
