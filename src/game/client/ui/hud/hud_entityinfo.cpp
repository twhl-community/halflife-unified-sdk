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

#include <tuple>

#include "hud.h"

bool CHudEntityInfo::Init()
{
	gHUD.AddHudElem(this);

	g_ClientUserMessages.RegisterHandler("EntityInfo", &CHudEntityInfo::MsgFunc_EntityInfo, this);

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
	const int labelXPos = static_cast<int>(ScreenWidth * 0.45);
	const int startingYPos = static_cast<int>(ScreenHeight * 0.55);

	const auto healthString = UTIL_ToString(m_EntityInfo.Health);

	// label, text pairs
	const std::tuple<const char*, const char*> infos[] =
		{
			// Include spaces in the label text.
			{"Classname: ", m_EntityInfo.Classname.c_str()},
			{"Health: ", healthString.c_str()}};

	// Draw labels first, calculate maximum width needed.
	int maximumWidth = -1;

	int yPos = startingYPos;

	for (const auto& info : infos)
	{
		maximumWidth = std::max(maximumWidth, gHUD.DrawHudString(labelXPos, yPos, ScreenWidth - labelXPos, std::get<0>(info), RGB_WHITE));
		yPos += lineHeight;
	}

	// Draw text starting at maximum width.
	yPos = startingYPos;

	for (const auto& info : infos)
	{
		gHUD.DrawHudString(maximumWidth, yPos, ScreenWidth - maximumWidth, std::get<1>(info), m_EntityInfo.Color);
		yPos += lineHeight;
	}

	return true;
}

void CHudEntityInfo::MsgFunc_EntityInfo(const char* pszName, BufferReader& reader)
{
	m_EntityInfo = {};

	m_EntityInfo.Classname = reader.ReadString();

	// Nothing to draw, skip rest of message.
	if (m_EntityInfo.Classname.empty())
	{
		return;
	}

	m_EntityInfo.Health = reader.ReadLong();
	m_EntityInfo.Color = reader.ReadRGB24();

	m_DrawEndTime = gHUD.m_flTime + EntityInfoDrawTime;
}
