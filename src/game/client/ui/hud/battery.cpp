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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"

bool CHudBattery::Init()
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	g_ClientUserMessages.RegisterHandler("Battery", &CHudBattery::MsgFunc_Battery, this);

	gHUD.AddHudElem(this);

	return true;
}


bool CHudBattery::VidInit()
{
	const int HUD_suit_empty = gHUD.GetSpriteIndex("suit_empty");
	const int HUD_suit_full = gHUD.GetSpriteIndex("suit_full");

	m_hSprite1 = gHUD.GetSprite(HUD_suit_empty);
	m_hSprite2 = gHUD.GetSprite(HUD_suit_full);

	m_prc1 = &gHUD.GetSpriteRect(HUD_suit_empty);
	m_prc2 = &gHUD.GetSpriteRect(HUD_suit_full);
	m_fFade = 0;
	return true;
}

void CHudBattery::MsgFunc_Battery(const char* pszName, BufferReader& reader)
{
	m_iFlags |= HUD_ACTIVE;

	int x = reader.ReadShort();

	if (x != m_iBat)
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}
}


bool CHudBattery::Draw(float flTime)
{
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) != 0)
		return true;

	if (!gHUD.HasSuit())
		return true;

	int a;

	// Has health changed? Flash the health #
	if (0 != m_fFade)
	{
		if (m_fFade > FADE_TIME)
			m_fFade = FADE_TIME;

		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if (m_fFade <= 0)
		{
			a = 128;
			m_fFade = 0;
		}

		// Fade the health number back to dim

		a = MIN_ALPHA + (m_fFade / FADE_TIME) * 128;
	}
	else
		a = MIN_ALPHA;

	const auto color = gHUD.m_HudItemColor.Scale(a);

	const int iOffset = (m_prc1->bottom - m_prc1->top) / 6;

	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	int x = ScreenWidth / 4;

	SPR_Set(m_hSprite1, color);
	SPR_DrawAdditive(0, x, y - iOffset, m_prc1);

	// Note: this assumes empty and full have the same size.
	Rect rc = *m_prc2;

	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1
	rc.top += (rc.bottom - rc.top) * (100 - std::min(100, m_iBat)) * 0.01f;

	if (rc.bottom > rc.top)
	{
		SPR_Set(m_hSprite2, color);
		SPR_DrawAdditive(0, x, y - iOffset + (rc.top - m_prc2->top), &rc);
	}

	x += (m_prc1->right - m_prc1->left);
	x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, color);

	return true;
}
