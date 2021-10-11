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
#include "player.h"
#include "UserMessages.h"
#include "CHUDIconTrigger.h"

LINK_ENTITY_TO_CLASS(ctf_hudicon, CHUDIconTrigger);

void CHUDIconTrigger::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	g_engfuncs.pfnSetModel(edict(), STRING(pev->model));
	pev->movetype = MOVETYPE_NONE;
	m_fIsActive = false;
}

void CHUDIconTrigger::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (m_flNextActiveTime <= gpGlobals->time && UTIL_IsMasterTriggered(m_sMaster, pActivator))
	{
		if (pev->noise)
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, STRING(pev->noise), VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

		m_hActivator = pActivator;

		SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);

		if (pev->message && pActivator->IsPlayer())
			UTIL_ShowMessage(STRING(pev->message), pActivator);

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

		g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgCustomIcon, 0, 0);
		g_engfuncs.pfnWriteByte(m_fIsActive);
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
void CHUDIconTrigger::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("icon_name", pkvd->szKeyName))
	{
		m_iszCustomName = g_engfuncs.pfnAllocString(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("icon_index", pkvd->szKeyName))
	{
		m_nCustomIndex = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("icon_left", pkvd->szKeyName))
	{
		m_nLeft = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("icon_top", pkvd->szKeyName))
	{
		m_nTop = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("icon_width", pkvd->szKeyName))
	{
		m_nWidth = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("icon_height", pkvd->szKeyName))
	{
		m_nHeight = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		pkvd->fHandled = false;
	}
}

void CHUDIconTrigger::UpdateUser(CBaseEntity* pPlayer)
{
	g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgCustomIcon, 0, pPlayer->edict());
	g_engfuncs.pfnWriteByte(m_fIsActive);
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
	//TODO: this will break when an index is larger than 31 or a negative value
	int activeIcons = 0;

	for (auto entity : UTIL_FindEntitiesByClassname<CHUDIconTrigger>("ctf_hudicon"))
	{
		if (!(activeIcons & (1 << entity->m_nCustomIndex)))
		{
			if (entity->m_fIsActive)
			{
				activeIcons |= 1 << entity->m_nCustomIndex;
			}

			entity->UpdateUser(pPlayer);
		}
	}
}
