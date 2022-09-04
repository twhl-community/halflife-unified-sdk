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

#include "hud.h"

DECLARE_MESSAGE(m_EntityInfo, EntityInfo);

bool CHudEntityInfo::Init()
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(EntityInfo);

	m_iFlags |= HUD_ACTIVE;

	return true;
}

bool CHudEntityInfo::VidInit()
{
	// New map, reset info.
	m_EntityInfo = {};
	m_DrawEndTime = 0;
	return true;
}

bool CHudEntityInfo::Draw(float flTime)
{
	if (m_EntityInfo.Classname.empty())
	{
		return true;
	}

	if (m_DrawEndTime <= gHUD.m_flTime)
	{
		m_EntityInfo = {};
		m_DrawEndTime = 0;
		return true;
	}

	const int lineHeight = 20;

	// Start drawing under the crosshair.
	// Align the fields vertically. This may need adjusting if you add more lines.
	const int labelXPos = static_cast<int>(ScreenWidth * 0.45);
	const int fieldXPos = labelXPos + 90;
	int yPos = static_cast<int>(ScreenHeight * 0.55);

	const auto lineDrawer = [&](const char* label, const char* text)
	{
		gHUD.DrawHudString(labelXPos, yPos, ScreenWidth - labelXPos, label, RGB_WHITE);
		gHUD.DrawHudString(fieldXPos, yPos, ScreenWidth - fieldXPos, text, m_EntityInfo.Color);
		yPos += lineHeight;
	};

	lineDrawer("Classname:", m_EntityInfo.Classname.c_str());
	lineDrawer("Health:", UTIL_ToString(m_EntityInfo.Health).c_str());

	return true;
}

bool CHudEntityInfo::MsgFunc_EntityInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_EntityInfo = {};

	m_EntityInfo.Classname = READ_STRING();

	// Nothing to draw, skip rest of message.
	if (m_EntityInfo.Classname.empty())
	{
		return true;
	}

	m_EntityInfo.Health = READ_LONG();
	m_EntityInfo.Color = READ_RGB24();

	m_DrawEndTime = gHUD.m_flTime + EntityInfoDrawTime;

	return true;
}
