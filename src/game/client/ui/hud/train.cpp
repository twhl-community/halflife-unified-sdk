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
// Train.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"

bool CHudTrain::Init()
{
	g_ClientUserMessages.RegisterHandler("Train", &CHudTrain::MsgFunc_Train, this);

	m_iPos = 0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);

	return true;
}

bool CHudTrain::VidInit()
{
	m_hSprite = 0;

	return true;
}

bool CHudTrain::Draw(float fTime)
{
	if (0 == m_hSprite)
		m_hSprite = LoadSprite("sprites/%d_train.spr");

	if (0 != m_iPos)
	{
		int x, y;

		SPR_Set(m_hSprite, gHUD.m_HudColor);

		// This should show up to the right and part way up the armor number
		y = ScreenHeight - SPR_Height(m_hSprite, 0) - gHUD.m_iFontHeight;
		x = ScreenWidth / 3 + SPR_Width(m_hSprite, 0) / 4;

		SPR_DrawAdditive(m_iPos - 1, x, y, nullptr);
	}

	return true;
}


void CHudTrain::MsgFunc_Train(const char* pszName, BufferReader& reader)
{
	// update Train data
	m_iPos = reader.ReadByte();

	if (0 != m_iPos)
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;
}
