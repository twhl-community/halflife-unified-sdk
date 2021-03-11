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

DECLARE_MESSAGE(m_FlagIcons, FlagIcon);
DECLARE_MESSAGE(m_FlagIcons, FlagTimer);

int CHudFlagIcons::Init()
{
    HOOK_MESSAGE(FlagIcon);
    HOOK_MESSAGE(FlagTimer);

    gHUD.AddHudElem(this);

    memset(m_FlagList, 0, sizeof(m_FlagList));
    m_flTimeStart = 0;
    m_flTimeLimit = 0;
    m_bIsTimer = false;
    m_bTimerReset = false;

    return 1;
}

int CHudFlagIcons::VidInit()
{
    return 1;
}

void CHudFlagIcons::InitHUDData()
{
    memset(m_FlagList, 0, sizeof(m_FlagList));
}

int CHudFlagIcons::Draw(float flTime)
{
    //TODO: can this ever return 2?
    if (gEngfuncs.IsSpectateOnly() != 2)
    {
        int y = ScreenHeight - 64;

        for (int i = 0; i < MAX_FLAGSPRITES; ++i)
        {
            const auto& flag = m_FlagList[i];

            if (flag.spr)
            {
                y += flag.rc.top - flag.rc.bottom - 5;

                gEngfuncs.pfnSPR_Set(flag.spr, flag.r, flag.g, flag.b);
                gEngfuncs.pfnSPR_DrawAdditive(0, 5, y, &flag.rc);
                gHUD.DrawHudNumber(
                    flag.rc.right - flag.rc.left + 10,
                    y + ((flag.rc.bottom - flag.rc.top) / 2) - 5,
                    DHN_DRAWZERO | DHN_2DIGITS,
                    flag.score, flag.r, flag.g, flag.b);
            }
        }

        if (m_bIsTimer)
        {
            if (m_bTimerReset)
            {
                m_flTimeStart = gHUD.m_flTime;
                m_bTimerReset = false;
            }

            const float totalSecondsLeft = m_flTimeLimit - (gHUD.m_flTime - m_flTimeStart);

            const int minutesLeft = V_max(0, totalSecondsLeft / 60.0);
            const int secondsLeft = V_max(0, totalSecondsLeft - (60 * minutesLeft));

            //TODO: this buffer is static in vanilla Op4
            char szBuf[40];
            sprintf(szBuf, "%s %d:%02d", CHudTextMessage::BufferedLocaliseTextString("#CTFTimeRemain"), minutesLeft, secondsLeft);
            gHUD.DrawHudString(5, ScreenHeight - 60, 200, szBuf, giR, giG, giB);
        }
    }

    return 1;
}

void CHudFlagIcons::EnableFlag(const char* pszFlagName, unsigned char team_idx, unsigned char red, unsigned char green, unsigned char blue, unsigned char score)
{
    if (team_idx < MAX_FLAGSPRITES)
    {
        auto& flag = m_FlagList[team_idx];

        const int spriteIndex = gHUD.GetSpriteIndex(pszFlagName);

        flag.spr = gHUD.GetSprite(spriteIndex);
        flag.rc = gHUD.GetSpriteRect(spriteIndex);
        flag.r = red;
        flag.g = green;
        flag.b = blue;
        flag.score = score;

        auto& teamInfo = g_TeamInfo[team_idx + 1];
        teamInfo.scores_overriden = 1;
        teamInfo.frags = score;

        strcpy(flag.szSpriteName, pszFlagName);
    }
}

void CHudFlagIcons::DisableFlag(const char* pszFlagName, unsigned char team_idx)
{
    if (team_idx < MAX_FLAGSPRITES)
    {
        memset(&m_FlagList[team_idx], 0, sizeof(m_FlagList[team_idx]));
    }
    else
    {
        m_iFlags &= ~HUD_ACTIVE;
    }
}

int CHudFlagIcons::MsgFunc_FlagIcon(const char* pszName, int iSize, void* pbuf)
{
    BEGIN_READ(pbuf, iSize);
    const int isActive = READ_BYTE();
    const char* flagName = READ_STRING();
    const int team_idx = READ_BYTE();

    if (isActive)
    {
        const int r = READ_BYTE();
        const int g = READ_BYTE();
        const int b = READ_BYTE();
        const int score = READ_BYTE();
        EnableFlag(flagName, team_idx, r, g, b, score);
        m_iFlags |= HUD_ACTIVE;
    }
    else
    {
        DisableFlag(flagName, team_idx);
    }

    return 1;
}

int CHudFlagIcons::MsgFunc_FlagTimer(const char* pszName, int iSize, void* pbuf)
{
    BEGIN_READ(pbuf, iSize);
    m_bIsTimer = READ_BYTE() != 0;

    if (m_bIsTimer)
    {
        m_flTimeLimit = READ_SHORT();
        m_bTimerReset = true;
    }

    return 1;
}
