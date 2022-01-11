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
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>


DECLARE_MESSAGE(m_Flash, FlashBat)
DECLARE_MESSAGE(m_Flash, Flashlight)

#define BAT_NAME "sprites/%d_Flashlight.spr"

bool CHudFlashlight::Init()
{
	m_fFade = 0;
	m_fOn = false;

	HOOK_MESSAGE(Flashlight);
	HOOK_MESSAGE(FlashBat);

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return true;
};

void CHudFlashlight::Reset()
{
	m_fFade = 0;
	m_fOn = false;
}

bool CHudFlashlight::VidInit()
{
	auto setup = [](const char* empty, const char* full, const char* beam) {
		const int HUD_flash_empty = gHUD.GetSpriteIndex(empty);
		const int HUD_flash_full = gHUD.GetSpriteIndex(full);
		const int HUD_flash_beam = gHUD.GetSpriteIndex(beam);

		LightData data;

		data.m_hSprite1 = gHUD.GetSprite(HUD_flash_empty);
		data.m_hSprite2 = gHUD.GetSprite(HUD_flash_full);
		data.m_hBeam = gHUD.GetSprite(HUD_flash_beam);
		data.m_prc1 = &gHUD.GetSpriteRect(HUD_flash_empty);
		data.m_prc2 = &gHUD.GetSpriteRect(HUD_flash_full);
		data.m_prcBeam = &gHUD.GetSpriteRect(HUD_flash_beam);
		data.m_iWidth = data.m_prc2->right - data.m_prc2->left;

		return data;
	};

	m_Flashlight = setup("flash_empty", "flash_full", "flash_beam");
	m_Nightvision = setup("nvg_empty", "nvg_full", "nvg_beam");

	m_nvSprite = LoadSprite("sprites/of_nv_b.spr");

	return true;
};

bool CHudFlashlight::MsgFunc_FlashBat(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ((float)x) / 100.0;

	return true;
}

bool CHudFlashlight::MsgFunc_Flashlight(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_SuitLightType = static_cast<SuitLightType>(READ_BYTE());
	m_fOn = 0 != READ_BYTE();
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ((float)x) / 100.0;

	//Always update this, so that changing to flashlight type disables NVG effects
	gHUD.SetNightVisionState(m_SuitLightType == SuitLightType::Nightvision && m_fOn);

	return true;
}

bool CHudFlashlight::Draw(float flTime)
{
	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_FLASHLIGHT | HIDEHUD_ALL)) != 0)
		return true;

	int x, y;
	Rect rc;

	if (!gHUD.HasSuit())
		return true;

	const int a = m_fOn ? 225 : MIN_ALPHA;

	const auto& originalColor = m_flBat < 0.20 ? RGB_REDISH : gHUD.m_HudItemColor;

	const auto color = originalColor.Scale(a);

	auto data = GetLightData();

	y = (data->m_prc1->bottom - data->m_prc2->top) / 2;
	x = ScreenWidth - data->m_iWidth - data->m_iWidth / 2;

	// Draw the flashlight casing
	SPR_Set(data->m_hSprite1, color);
	SPR_DrawAdditive(0, x, y, data->m_prc1);

	if (m_fOn)
	{ // draw the flashlight beam
		x = ScreenWidth - data->m_iWidth / 2;

		SPR_Set(data->m_hBeam, color);
		SPR_DrawAdditive(0, x, y, data->m_prcBeam);

		if (m_SuitLightType == SuitLightType::Nightvision)
		{
			DrawNightVision();
		}
	}

	// draw the flashlight energy level
	x = ScreenWidth - data->m_iWidth - data->m_iWidth / 2;
	int iOffset = data->m_iWidth * (1.0 - m_flBat);
	if (iOffset < data->m_iWidth)
	{
		rc = *data->m_prc2;
		rc.left += iOffset;

		SPR_Set(data->m_hSprite2, color);
		SPR_DrawAdditive(0, x + iOffset, y, &rc);
	}


	return true;
}

void CHudFlashlight::DrawNightVision()
{
	static int lastFrame = 0;

	auto frameIndex = rand() % gEngfuncs.pfnSPR_Frames(m_nvSprite);

	if (frameIndex == lastFrame)
		frameIndex = (frameIndex + 1) % gEngfuncs.pfnSPR_Frames(m_nvSprite);

	lastFrame = frameIndex;

	if (0 != m_nvSprite)
	{
		const auto width = gEngfuncs.pfnSPR_Width(m_nvSprite, 0);
		const auto height = gEngfuncs.pfnSPR_Height(m_nvSprite, 0);

		gEngfuncs.pfnSPR_Set(m_nvSprite, 0, 170, 0);

		Rect drawingRect;

		for (auto x = 0; x < gHUD.m_scrinfo.iWidth; x += width)
		{
			drawingRect.left = 0;
			drawingRect.right = x + width >= gHUD.m_scrinfo.iWidth ? gHUD.m_scrinfo.iWidth - x : width;

			for (auto y = 0; y < gHUD.m_scrinfo.iHeight; y += height)
			{
				drawingRect.top = 0;
				drawingRect.bottom = y + height >= gHUD.m_scrinfo.iHeight ? gHUD.m_scrinfo.iHeight - y : height;

				gEngfuncs.pfnSPR_DrawAdditive(frameIndex, x, y, &drawingRect);
			}
		}
	}
}
