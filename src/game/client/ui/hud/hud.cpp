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
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

#include "demo.h"
#include "demo_api.h"
#include "vgui_ScorePanel.h"

class CHLVoiceStatusHelper : public IVoiceStatusHelper
{
public:
	void GetPlayerTextColor(int entindex, int color[3]) override
	{
		color[0] = color[1] = color[2] = 255;

		if (entindex >= 0 && entindex < sizeof(g_PlayerExtraInfo) / sizeof(g_PlayerExtraInfo[0]))
		{
			int iTeam = g_PlayerExtraInfo[entindex].teamnumber;

			if (iTeam < 0)
			{
				iTeam = 0;
			}

			iTeam = iTeam % iNumberOfTeamColors;

			color[0] = iTeamColors[iTeam][0];
			color[1] = iTeamColors[iTeam][1];
			color[2] = iTeamColors[iTeam][2];
		}
	}

	void UpdateCursorState() override
	{
		gViewPort->UpdateCursorState();
	}

	int GetAckIconHeight() override
	{
		return ScreenHeight - gHUD.m_iFontHeight * 3 - 6;
	}

	bool CanShowSpeakerLabels() override
	{
		if (gViewPort && gViewPort->m_pScoreBoard)
			return !gViewPort->m_pScoreBoard->isVisible();
		else
			return false;
	}
};
static CHLVoiceStatusHelper g_VoiceStatusHelper;


extern client_sprite_t* GetSpriteList(client_sprite_t* pList, const char* psz, int iRes, int iCount);

extern cvar_t* sensitivity;
cvar_t* cl_lw = nullptr;
cvar_t* cl_rollangle = nullptr;
cvar_t* cl_rollspeed = nullptr;
cvar_t* cl_bobtilt = nullptr;

void ShutdownInput();

// TFFree Command Menu
void __CmdFunc_OpenCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->ShowCommandMenu(gViewPort->m_StandardMenu);
	}
}

// TFC "special" command
void __CmdFunc_InputPlayerSpecial()
{
	if (gViewPort)
	{
		gViewPort->InputPlayerSpecial();
	}
}

void __CmdFunc_CloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->InputSignalHideCommandMenu();
	}
}

void __CmdFunc_ForceCloseCommandMenu()
{
	if (gViewPort)
	{
		gViewPort->HideCommandMenu();
	}
}

// This is called every time the DLL is loaded
void CHud::Init()
{
	g_ClientUserMessages.RegisterHandler("HudColor", &CHud::MsgFunc_HudColor, this);
	g_ClientUserMessages.RegisterHandler("Logo", &CHud::MsgFunc_Logo, this);
	g_ClientUserMessages.RegisterHandler("ResetHUD", &CHud::MsgFunc_ResetHUD, this);
	g_ClientUserMessages.RegisterHandler("GameMode", &CHud::MsgFunc_GameMode, this);
	g_ClientUserMessages.RegisterHandler("InitHUD", &CHud::MsgFunc_InitHUD, this);
	g_ClientUserMessages.RegisterHandler("ViewMode", &CHud::MsgFunc_ViewMode, this);
	g_ClientUserMessages.RegisterHandler("SetFOV", &CHud::MsgFunc_SetFOV, this);
	g_ClientUserMessages.RegisterHandler("Concuss", &CHud::MsgFunc_Concuss, this);
	g_ClientUserMessages.RegisterHandler("Weapons", &CHud::MsgFunc_Weapons, this);

	// TFFree CommandMenu
	HOOK_COMMAND("+commandmenu", OpenCommandMenu);
	HOOK_COMMAND("-commandmenu", CloseCommandMenu);
	HOOK_COMMAND("ForceCloseCommandMenu", ForceCloseCommandMenu);
	HOOK_COMMAND("special", InputPlayerSpecial);

	CVAR_CREATE("hud_classautokill", "1", FCVAR_ARCHIVE | FCVAR_USERINFO); // controls whether or not to suicide immediately on TF class switch
	CVAR_CREATE("hud_takesshots", "0", FCVAR_ARCHIVE);					   // controls whether or not to automatically take screenshots at the end of a round

	CVAR_CREATE("zoom_sensitivity_ratio", "1.2", 0);
	CVAR_CREATE("cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO);
	default_fov = CVAR_CREATE("default_fov", "90", FCVAR_ARCHIVE);
	m_pCvarStealMouse = CVAR_CREATE("hud_capturemouse", "1", FCVAR_ARCHIVE);
	m_pCvarDraw = CVAR_CREATE("hud_draw", "1", FCVAR_ARCHIVE);
	m_pCvarCrosshair = gEngfuncs.pfnGetCvarPointer("crosshair");
	cl_lw = gEngfuncs.pfnGetCvarPointer("cl_lw");
	cl_rollangle = CVAR_CREATE("cl_rollangle", "2.0", FCVAR_ARCHIVE);
	cl_rollspeed = CVAR_CREATE("cl_rollspeed", "200", FCVAR_ARCHIVE);
	cl_bobtilt = CVAR_CREATE("cl_bobtilt", "0", FCVAR_ARCHIVE);

	// Clear any old HUD list
	m_HudList.clear();

	m_Ammo.Init();
	m_Health.Init();
	m_SayText.Init();
	m_Spectator.Init();
	m_Geiger.Init();
	m_Train.Init();
	m_Battery.Init();
	m_Flash.Init();
	m_Message.Init();
	m_Scoreboard.Init();
	m_StatusBar.Init();
	m_DeathNotice.Init();
	m_TextMessage.Init();
	m_StatusIcons.Init();
	m_FlagIcons.Init();
	m_PlayerBrowse.Init();
	m_ProjectInfo.Init();
	m_EntityInfo.Init();
	GetClientVoiceMgr()->Init(&g_VoiceStatusHelper, (vgui::Panel**)&gViewPort);

	m_Menu.Init();

	MsgFunc_ResetHUD(nullptr, 0, nullptr);
}

void CHud::Shutdown()
{
	GetClientVoiceMgr()->Shutdown();
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_Sprites list
// returns -1 if sprite not found
int CHud::GetSpriteIndex(const char* SpriteName)
{
	// look through the loaded sprite name list for SpriteName
	for (std::size_t i = 0; i < m_Sprites.size(); ++i)
	{
		if (SpriteName == m_Sprites[i].Name)
			return static_cast<int>(i);
	}

	return -1; // invalid sprite
}

void CHud::VidInit()
{
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	// ----------
	// Load Sprites
	// ---------
	//	m_hsprFont = LoadSprite("sprites/%d_font.spr");

	m_hsprLogo = 0;
	m_hsprCursor = 0;

	// Only load this once
	if (m_Sprites.empty())
	{
		// we need to load the hud.txt, and all sprites within
		int spriteCountAllRes;
		client_sprite_t* spriteList = SPR_GetList("sprites/hud.txt", &spriteCountAllRes);

		if (spriteList)
		{
			m_Sprites.reserve(spriteCountAllRes);

			client_sprite_t* p = spriteList;
			for (int j = 0; j < spriteCountAllRes; ++j)
			{
				if (p->iRes == m_iRes)
				{
					auto& hudSprite = m_Sprites.emplace_back();

					hudSprite.Name = p->szName;
					hudSprite.SpriteName = p->szSprite;
					hudSprite.Rectangle = p->rc;
				}

				++p;
			}

			m_Sprites.shrink_to_fit();

			gEngfuncs.COM_FreeFile(spriteList);
		}
	}

	// we have already have loaded the sprite reference from hud.txt, but
	// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
	for (auto& hudSprite : m_Sprites)
	{
		hudSprite.Handle = SPR_Load(fmt::format("sprites/{}.spr", hudSprite.SpriteName.c_str()).c_str());
	}

	// assumption: number_1, number_2, etc, are all listed and loaded sequentially
	m_HUD_number_0 = GetSpriteIndex("number_0");

	const auto& numberRect = m_Sprites[m_HUD_number_0].Rectangle;
	m_iFontHeight = numberRect.bottom - numberRect.top;

	// Reset to default on new map load
	m_HudColor = RGB_HUD_COLOR;
	m_HudItemColor = RGB_HUD_COLOR;

	for (auto hudElement : m_HudList)
	{
		hudElement->VidInit();
	}
}

void CHud::MsgFunc_HudColor(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_HudColor.Red = READ_BYTE();
	m_HudColor.Green = READ_BYTE();
	m_HudColor.Blue = READ_BYTE();

	// Sync item color up if we're not in NVG mode
	if (!m_NightVisionState)
	{
		m_HudItemColor = m_HudColor;
	}
}

void CHud::MsgFunc_Logo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	// update Train data
	m_ShowLogo = READ_BYTE() != 0;
}

float g_lastFOV = 0.0;

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase(const char* in, char* out)
{
	int len, start, end;

	len = strlen(in);

	// scan backward for '.'
	end = len - 1;
	while (0 != end && in[end] != '.' && in[end] != '/' && in[end] != '\\')
		end--;

	if (in[end] != '.') // no '.', copy to end
		end = len - 1;
	else
		end--; // Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len - 1;
	while (start >= 0 && in[start] != '/' && in[start] != '\\')
		start--;

	if (in[start] != '/' && in[start] != '\\')
		start = 0;
	else
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy(out, &in[start], len);
	// Terminate it
	out[len] = 0;
}

/*
=================
HUD_IsGame

=================
*/
int HUD_IsGame(const char* game)
{
	const char* gamedir;
	char gd[1024];

	gamedir = gEngfuncs.pfnGetGameDirectory();
	if (gamedir && '\0' != gamedir[0])
	{
		COM_FileBase(gamedir, gd);
		if (!stricmp(gd, game))
			return 1;
	}
	return 0;
}

/*
=====================
HUD_GetFOV

Returns last FOV
=====================
*/
float HUD_GetFOV()
{
	if (0 != gEngfuncs.pDemoAPI->IsRecording())
	{
		// Write it
		int i = 0;
		unsigned char buf[100];

		// Active
		*(float*)&buf[i] = g_lastFOV;
		i += sizeof(float);

		Demo_WriteBuffer(TYPE_ZOOM, i, buf);
	}

	if (0 != gEngfuncs.pDemoAPI->IsPlayingback())
	{
		g_lastFOV = g_demozoom;
	}
	return g_lastFOV;
}

void CHud::MsgFunc_SetFOV(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT("default_fov");

	// Weapon prediction already takes care of changing the fog. ( g_lastFOV ).
	// But it doesn't restore correctly so this still needs to be used
	/*
	if ( cl_lw && cl_lw->value )
		return 1;
		*/

	g_lastFOV = newfov;

	if (newfov == 0)
	{
		m_iFOV = def_fov;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if (m_iFOV == def_fov)
	{
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)def_fov) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}
}


void CHud::AddHudElem(CHudBase* phudelem)
{
	// phudelem->Think();

	if (!phudelem)
		return;

	m_HudList.push_back(phudelem);
}

float CHud::GetSensitivity()
{
	return m_flMouseSensitivity;
}

void CHud::SetNightVisionState(bool state)
{
	m_NightVisionState = state;

	if (state)
	{
		m_HudItemColor = RGB_WHITE;
	}
	else
	{
		m_HudItemColor = m_HudColor;
	}
}
