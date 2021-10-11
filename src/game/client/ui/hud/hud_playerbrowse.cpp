/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//
// hud_playerbrowse.cpp
//
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "event_api.h"

#include "ctf/CTFDefs.h"

DECLARE_MESSAGE(m_PlayerBrowse, PlyrBrowse);

int CHudPlayerBrowse::Init()
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(PlyrBrowse);

	Reset();

	return 1;
}

int CHudPlayerBrowse::VidInit()
{
	return 1;
}

void CHudPlayerBrowse::InitHUDData()
{
	memset(m_szLineBuffer, 0, sizeof(m_szLineBuffer));
	memset(&m_PowerupSprite, 0, sizeof(m_PowerupSprite));
	m_flDelayFade = 0;
}

int CHudPlayerBrowse::Draw(float flTime)
{
	int r, g, b;

	if (m_iTeamNum == static_cast<int>(CTFTeam::BlackMesa))
	{
		UnpackRGB(r, g, b, RGB_YELLOWISH);
	}
	else if (m_iTeamNum == static_cast<int>(CTFTeam::OpposingForce))
	{
		UnpackRGB(r, g, b, RGB_HUD_COLOR);
	}
	else
	{
		r = 192;
		g = 192;
		b = 192;
	}

	if (m_flDelayFade > 0)
	{
		if (m_flDelayFade < 40)
		{
			m_flDelayFade = 0;
			m_szLineBuffer[0] = '\0';
			m_iTeamNum = m_iNewTeamNum;

			memset(&m_PowerupSprite, 0, sizeof(m_PowerupSprite));

			m_iFlags &= ~HUD_ACTIVE;
		}
		else
		{
			ScaleColors(r, g, b, m_flDelayFade);
			m_flDelayFade -= 85 * gHUD.m_flTimeDelta;
		}
	}

	if (m_szLineBuffer[0])
	{
		if (m_PowerupSprite.spr)
		{
			if (m_flDelayFadeSprite > 0)
			{
				ScaleColors(m_PowerupSprite.r, m_PowerupSprite.g, m_PowerupSprite.b, V_max(40, m_flDelayFadeSprite));
				m_flDelayFadeSprite -= 10 * gHUD.m_flTimeDelta;
			}

			gEngfuncs.pfnSPR_Set(m_PowerupSprite.spr, m_PowerupSprite.r, m_PowerupSprite.g, m_PowerupSprite.b);
			gEngfuncs.pfnSPR_DrawAdditive(
				0,
				ScreenWidth / 2 + (m_PowerupSprite.rc.left - m_PowerupSprite.rc.right - 105),
				ScreenHeight * 0.75,
				&m_PowerupSprite.rc);
		}

		gHUD.DrawHudString(ScreenWidth / 2 - 100, ScreenHeight * 0.75, ScreenWidth, m_szLineBuffer, r, g, b);
	}

	return 1;
}

int CHudPlayerBrowse::MsgFunc_PlyrBrowse(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_fFriendly = READ_BYTE() != 0;
	m_iNewTeamNum = READ_BYTE();
	m_szNewLineBuffer[0] = 0;

	if (m_iNewTeamNum > 0)
	{
		const char* identifier = "(Friendly) ";

		//TODO: is the leading space supposed to be here?
		if (!m_fFriendly)
			identifier = " (Enemy) ";

		strcpy(m_szNewLineBuffer, identifier);
	}

	//TODO: unsafe use of strncat count parameter
	strncat(m_szNewLineBuffer, READ_STRING(), iSize - 3);

	if (m_szNewLineBuffer[0])
	{
		const int items = READ_BYTE();

		if (items)
		{
			if (items & CTFItem::LongJump)
			{
				const int spriteIndex = gHUD.GetSpriteIndex("score_ctfljump");
				m_PowerupSprite.spr = gHUD.GetSprite(spriteIndex);
				m_PowerupSprite.rc = gHUD.GetSpriteRect(spriteIndex);
				m_PowerupSprite.r = 255;
				m_PowerupSprite.g = 160;
				m_PowerupSprite.b = 0;
			}
			else if (items & CTFItem::PortableHEV)
			{
				const int spriteIndex = gHUD.GetSpriteIndex("score_ctfphev");
				m_PowerupSprite.spr = gHUD.GetSprite(spriteIndex);
				m_PowerupSprite.rc = gHUD.GetSpriteRect(spriteIndex);
				m_PowerupSprite.r = 128;
				m_PowerupSprite.g = 160;
				m_PowerupSprite.b = 255;
			}
			else if (items & CTFItem::Regeneration)
			{
				const int spriteIndex = gHUD.GetSpriteIndex("score_ctfregen");
				m_PowerupSprite.spr = gHUD.GetSprite(spriteIndex);
				m_PowerupSprite.rc = gHUD.GetSpriteRect(spriteIndex);
				m_PowerupSprite.r = 0;
				m_PowerupSprite.g = 255;
				m_PowerupSprite.b = 0;
			}
			else if (items & CTFItem::Acceleration)
			{
				const int spriteIndex = gHUD.GetSpriteIndex("score_ctfaccel");
				m_PowerupSprite.spr = gHUD.GetSprite(spriteIndex);
				m_PowerupSprite.rc = gHUD.GetSpriteRect(spriteIndex);
				m_PowerupSprite.r = 255;
				m_PowerupSprite.g = 0;
				m_PowerupSprite.b = 0;
			}
			else if (items & CTFItem::Backpack)
			{
				const int spriteIndex = gHUD.GetSpriteIndex("score_ctfbpack");
				m_PowerupSprite.spr = gHUD.GetSprite(spriteIndex);
				m_PowerupSprite.rc = gHUD.GetSpriteRect(spriteIndex);
				m_PowerupSprite.r = 255;
				m_PowerupSprite.g = 255;
				m_PowerupSprite.b = 0;
			}
		}
		else
		{
			memset(&m_PowerupSprite, 0, sizeof(m_PowerupSprite));
		}

		m_iHealth = READ_BYTE();
		m_iArmor = READ_BYTE();

		if (!(m_fFriendly == 0) || !m_iNewTeamNum)
		{
			//TODO: unsafe
			sprintf(&m_szNewLineBuffer[strlen(m_szNewLineBuffer)], " (%d/%d)", m_iHealth, m_iArmor);
		}

		strcpy(m_szLineBuffer, m_szNewLineBuffer);

		m_iFlags |= HUD_ACTIVE;
		m_flDelayFade = 0;
		m_iTeamNum = m_iNewTeamNum;
		m_flDelayFadeSprite = 0;
	}
	else
	{
		m_flDelayFade = 200;
		m_flDelayFadeSprite = 255;
	}

	return 1;
}
