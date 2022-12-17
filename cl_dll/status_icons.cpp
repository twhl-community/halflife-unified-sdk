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
// status_icons.cpp
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

DECLARE_MESSAGE(m_StatusIcons, StatusIcon);
DECLARE_MESSAGE(m_StatusIcons, CustomIcon);

bool CHudStatusIcons::Init()
{
	HOOK_MESSAGE(StatusIcon);
	HOOK_MESSAGE(CustomIcon);

	gHUD.AddHudElem(this);

	Reset();

	return true;
}

bool CHudStatusIcons::VidInit()
{

	return true;
}

void CHudStatusIcons::Reset()
{
	memset(m_IconList, 0, sizeof m_IconList);
	memset(m_CustomList, 0, sizeof(m_CustomList));
	m_iFlags &= ~HUD_ACTIVE;
}

// Draw status icons along the left-hand side of the screen
bool CHudStatusIcons::Draw(float flTime)
{
	if (0 != gEngfuncs.IsSpectateOnly())
		return true;
	{
		// find starting position to draw from, along right-hand side of screen
		int x = 5;
		int y = ScreenHeight / 2;

		// loop through icon list, and draw any valid icons drawing up from the middle of screen
		for (int i = 0; i < MAX_ICONSPRITES; i++)
		{
			if (0 != m_IconList[i].spr)
			{
				y -= (m_IconList[i].rc.bottom - m_IconList[i].rc.top) + 5;

				SPR_Set(m_IconList[i].spr, m_IconList[i].r, m_IconList[i].g, m_IconList[i].b);
				SPR_DrawAdditive(0, x, y, &m_IconList[i].rc);
			}
		}
	}

	{
		int y = ScreenHeight / 2;

		for (int i = 0; i < MAX_CUSTOMSPRITES; ++i)
		{
			if (i == (MAX_CUSTOMSPRITES / 2))
			{
				y = ScreenHeight / 2;
			}

			const auto& icon = m_CustomList[i];

			if (0 != icon.spr)
			{
				const int x = (i < (MAX_CUSTOMSPRITES / 2)) ? 100 : (ScreenWidth - 100);

				gEngfuncs.pfnSPR_Set(icon.spr, icon.r, icon.g, icon.b);
				gEngfuncs.pfnSPR_DrawAdditive(0, x, y, &icon.rc);
				y += (icon.rc.bottom - icon.rc.top) + 5;
			}
		}
	}

	return true;
}

// Message handler for StatusIcon message
// accepts five values:
//		byte   : true = ENABLE icon, false = DISABLE icon
//		string : the sprite name to display
//		byte   : red
//		byte   : green
//		byte   : blue
bool CHudStatusIcons::MsgFunc_StatusIcon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	bool ShouldEnable = READ_BYTE() != 0;
	char* pszIconName = READ_STRING();
	if (ShouldEnable)
	{
		int r = READ_BYTE();
		int g = READ_BYTE();
		int b = READ_BYTE();
		EnableIcon(pszIconName, r, g, b);
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		DisableIcon(pszIconName);
	}

	return true;
}

bool CHudStatusIcons::MsgFunc_CustomIcon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	const bool ShouldEnable = READ_BYTE() != 0;
	int index = READ_BYTE();

	if (ShouldEnable)
	{
		char* pszIconName = READ_STRING();
		int r = READ_BYTE();
		int g = READ_BYTE();
		int b = READ_BYTE();

		Rect aRect;
		aRect.left = READ_BYTE();
		aRect.top = READ_BYTE();
		aRect.right = READ_BYTE();
		aRect.bottom = READ_BYTE();

		EnableCustomIcon(index, pszIconName, r, g, b, aRect);
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		DisableCustomIcon(index);
	}

	return true;
}

// add the icon to the icon list, and set it's drawing color
void CHudStatusIcons::EnableIcon(const char* pszIconName, unsigned char red, unsigned char green, unsigned char blue)
{
	int i;
	// check to see if the sprite is in the current list
	for (i = 0; i < MAX_ICONSPRITES; i++)
	{
		if (!stricmp(m_IconList[i].szSpriteName, pszIconName))
			break;
	}

	if (i == MAX_ICONSPRITES)
	{
		// icon not in list, so find an empty slot to add to
		for (i = 0; i < MAX_ICONSPRITES; i++)
		{
			if (0 == m_IconList[i].spr)
				break;
		}
	}

	// if we've run out of space in the list, overwrite the first icon
	if (i == MAX_ICONSPRITES)
	{
		i = 0;
	}

	// Load the sprite and add it to the list
	// the sprite must be listed in hud.txt
	int spr_index = gHUD.GetSpriteIndex(pszIconName);
	m_IconList[i].spr = gHUD.GetSprite(spr_index);
	m_IconList[i].rc = gHUD.GetSpriteRect(spr_index);
	m_IconList[i].r = red;
	m_IconList[i].g = green;
	m_IconList[i].b = blue;
	strcpy(m_IconList[i].szSpriteName, pszIconName);

	// Hack: Play Timer sound when a grenade icon is played (in 0.8 seconds)
	if (strstr(m_IconList[i].szSpriteName, "grenade"))
	{
		cl_entity_t* pthisplayer = gEngfuncs.GetLocalPlayer();
		gEngfuncs.pEventAPI->EV_PlaySound(pthisplayer->index, pthisplayer->origin, CHAN_STATIC, "weapons/timer.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
	}
}

void CHudStatusIcons::DisableIcon(const char* pszIconName)
{
	// find the sprite is in the current list
	for (int i = 0; i < MAX_ICONSPRITES; i++)
	{
		if (!stricmp(m_IconList[i].szSpriteName, pszIconName))
		{
			// clear the item from the list
			memset(&m_IconList[i], 0, sizeof(icon_sprite_t));
			return;
		}
	}
}

void CHudStatusIcons::EnableCustomIcon(int nIndex, char* pszIconName, unsigned char red, unsigned char green, unsigned char blue, const Rect& aRect)
{
	if (nIndex < MAX_CUSTOMSPRITES)
	{
		char szTemp[256];
		sprintf(szTemp, "sprites/%s.spr", pszIconName);

		auto& icon = m_CustomList[nIndex];

		icon.spr = gEngfuncs.pfnSPR_Load(szTemp);
		icon.rc = aRect;
		icon.r = red;
		icon.g = green;
		icon.b = blue;
		//TODO: potential overflow
		strcpy(icon.szSpriteName, pszIconName);
	}
}

void CHudStatusIcons::DisableCustomIcon(int nIndex)
{
	if (nIndex < MAX_CUSTOMSPRITES)
	{
		memset(m_CustomList + nIndex, 0, sizeof(icon_sprite_t));
	}
}
