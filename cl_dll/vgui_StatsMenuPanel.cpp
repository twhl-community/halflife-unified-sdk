//=========== (C) Copyright 1996-2002 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Stats Menu 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include <VGUI_TextImage.h>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_StatsMenuPanel.h"

struct team_stat_info_t
{
	byte bChunksRead;
	char szMVP[32];
	char szMostKills[32];
	char szMostCTF[32];
	char szMostOff[32];
	char szMostDef[32];
	char szMostSnipe[32];
	char szMostBarnacle[32];
	char szMostDeaths[32];
	char szMostSuicides[32];
	char szMostDamage[32];
	char szMostAccel[32];
	char szMostBackpack[32];
	char szMostHealth[32];
	char szMostShield[32];
	char szMostJump[32];
	int iMVPVal;
	int iKillsVal;
	int iCTFVal;
	int iOffVal;
	int iDefVal;
	int iSnipeVal;
	int iBarnacleVal;
	int iDeathsVal;
	int iSuicidesVal;
	int iDamageVal;
	int iAccelVal;
	int iBackpackVal;
	int iHealthVal;
	int iShieldVal;
	int iJumpVal;
};


#define STATSMENU_TITLE_X				XRES(40)
#define STATSMENU_TITLE_Y				YRES(32)
#define STATSMENU_TOPLEFT_BUTTON_X		XRES(40)
#define STATSMENU_TOPLEFT_BUTTON_Y		YRES(400)
#define STATSMENU_BUTTON_SIZE_X			XRES(124)
#define STATSMENU_BUTTON_SIZE_Y			YRES(24)
#define STATSMENU_BUTTON_SPACER_Y		YRES(8)
#define STATSMENU_WINDOW_X				XRES(40)
#define STATSMENU_WINDOW_Y				YRES(80)
#define STATSMENU_WINDOW_SIZE_X			XRES(600)
#define STATSMENU_WINDOW_SIZE_Y			YRES(312)
#define STATSMENU_WINDOW_TEXT_X			XRES(150)
#define STATSMENU_WINDOW_TEXT_Y			YRES(80)
#define STATSMENU_WINDOW_NAME_X			XRES(150)
#define STATSMENU_WINDOW_NAME_Y			YRES(8)

static char szStatsBuf[StatsTeamsCount][1024];
static int g_iWinningTeam = 0;
static team_stat_info_t g_TeamStatInfo[3];

CStatsMenuPanel::CStatsMenuPanel(int iTrans, int iRemoveMe, int x, int y, int wide, int tall)
	: CMenuPanel(iTrans, iRemoveMe, x, y, wide, tall)
{
	memset(m_pClassImages, 0, sizeof(m_pClassImages));

	// Get the scheme used for the Titles
	CSchemeManager* pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Title Font");
	SchemeHandle_t hClassWindowText = pSchemes->getSchemeHandle("Briefing Text");
	SchemeHandle_t hStatsWindowText = pSchemes->getSchemeHandle("CommandMenu Text");

	// color schemes
	int r, g, b, a;

	// Create the title
	Label* pLabel = new Label("", STATSMENU_TITLE_X, STATSMENU_TITLE_Y);
	pLabel->setParent(this);
	pLabel->setFont(pSchemes->getFont(hTitleScheme));
	pSchemes->getFgColor(hTitleScheme, r, g, b, a);
	pLabel->setFgColor(r, g, b, a);
	pSchemes->getBgColor(hTitleScheme, r, g, b, a);
	pLabel->setBgColor(r, g, b, a);
	pLabel->setContentAlignment(vgui::Label::a_west);
	pLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#CTFTitle_EndGameStats"));

	// Create the Scroll panel
	m_pScrollPanel = new CTFScrollPanel(STATSMENU_WINDOW_X, STATSMENU_WINDOW_Y, STATSMENU_WINDOW_SIZE_X, STATSMENU_WINDOW_SIZE_Y);
	m_pScrollPanel->setParent(this);
	//force the scrollbars on, so after the validate clientClip will be smaller
	m_pScrollPanel->setScrollBarAutoVisible(false, false);
	m_pScrollPanel->setScrollBarVisible(true, true);
	m_pScrollPanel->setBorder(new LineBorder(Color(0, 112, 0, 0)));
	m_pScrollPanel->validate();

	int clientWide = m_pScrollPanel->getClient()->getWide();

	//turn scrollpanel back into auto show scrollbar mode and validate
	m_pScrollPanel->setScrollBarAutoVisible(false, true);
	m_pScrollPanel->setScrollBarVisible(false, false);
	m_pScrollPanel->validate();

	for (int i = 0; i < StatsTeamsCount; ++i)
	{
		char sz[256];
		//TODO: this is using YRES for an X coord. Bugged in vanilla
		int iXPos = STATSMENU_TOPLEFT_BUTTON_X + ((STATSMENU_BUTTON_SIZE_X + STATSMENU_BUTTON_SPACER_Y) * i);

		strcpy(sz, CHudTextMessage::BufferedLocaliseTextString(sLocalisedStatsTeams[i]));
		m_pButtons[i] = new CommandButton(sz, iXPos, STATSMENU_TOPLEFT_BUTTON_Y, STATSMENU_BUTTON_SIZE_X, STATSMENU_BUTTON_SIZE_Y, true);

		sprintf(sz, "%d", i);
		m_pButtons[i]->setBoundKey(sz[0]);
		m_pButtons[i]->setContentAlignment(vgui::Label::a_west);
		m_pButtons[i]->addInputSignal(new CHandler_MenuButtonOver(this, i));
		m_pButtons[i]->setParent(this);

		// Create the Class Info Window
		//m_pClassInfoPanel[i] = new CTransparentPanel( 255, CLASSMENU_WINDOW_X, CLASSMENU_WINDOW_Y, CLASSMENU_WINDOW_SIZE_X, CLASSMENU_WINDOW_SIZE_Y );
		m_pClassInfoPanel[i] = new CTransparentPanel(255, 0, 0, clientWide, STATSMENU_WINDOW_SIZE_Y);
		m_pClassInfoPanel[i]->setParent(m_pScrollPanel->getClient());
		//m_pClassInfoPanel[i]->setVisible( false );

		// don't show class pic in lower resolutions
		int textOffs = XRES(8);

		// Create the Class Name Label
		sprintf(sz, "#CTFTitleStats_%s", sCTFStatsSelection[i]);
		char* localName = CHudTextMessage::BufferedLocaliseTextString(sz);
		Label* pNameLabel = new Label("", textOffs, STATSMENU_WINDOW_NAME_Y);
		pNameLabel->setFont(pSchemes->getFont(hTitleScheme));
		pNameLabel->setParent(m_pClassInfoPanel[i]);
		pSchemes->getFgColor(hTitleScheme, r, g, b, a);
		pNameLabel->setFgColor(r, g, b, a);
		pSchemes->getBgColor(hTitleScheme, r, g, b, a);
		pNameLabel->setBgColor(r, g, b, a);
		pNameLabel->setContentAlignment(vgui::Label::a_west);
		//pNameLabel->setBorder(new LineBorder());
		pNameLabel->setText("%s", localName);

		if (ScreenWidth > 639)
		{
			int xOut, yOut;
			pNameLabel->getTextSize(xOut, yOut);

			if (i == 3)
				sprintf(sz, sCTFStatsSelection[1]);
			else
				sprintf(sz, sCTFStatsSelection[i]);

			m_pClassImages[i] = new CImageLabel(sz, textOffs, 2 * yOut, XRES(250), YRES(80));
			m_pClassImages[i]->setParent(m_pClassInfoPanel[i]);

			textOffs += XRES(8) + m_pClassImages[i]->getImageWide();
		}

		const char* cText = "No players on this team";

		if (i == 3)
			cText = "Spectator";

		// Create the Text info window
		m_pStatsWindow[i] = new TextPanel(cText, textOffs, STATSMENU_WINDOW_TEXT_Y, XRES(250), STATSMENU_WINDOW_TEXT_Y);
		m_pStatsWindow[i]->setParent(m_pClassInfoPanel[i]);
		m_pStatsWindow[i]->setFont(pSchemes->getFont(hStatsWindowText));
		pSchemes->getFgColor(hStatsWindowText, r, g, b, a);
		m_pStatsWindow[i]->setFgColor(r, g, b, a);
		pSchemes->getBgColor(hStatsWindowText, r, g, b, a);
		m_pStatsWindow[i]->setBgColor(r, g, b, a);

		// Resize the Info panel to fit it all
		int xx, yy;
		m_pStatsWindow[i]->getPos(xx, yy);
		int maxX = xx + wide;
		int maxY = yy + tall;

		//check to see if the image goes lower than the text
		if (m_pClassImages[i] != null)
		{
			m_pClassImages[i]->getPos(xx, yy);
			if ((yy + m_pClassImages[i]->getTall()) > maxY)
			{
				maxY = yy + m_pClassImages[i]->getTall();
			}
		}

		m_pClassInfoPanel[i]->setSize(maxX, maxY);
	}

	// Create the Cancel button
	m_pCancelButton = new CommandButton(gHUD.m_TextMessage.BufferedLocaliseTextString("#CTFMenu_Cancel"), XRES(500), YRES(40), STATSMENU_BUTTON_SIZE_X, STATSMENU_BUTTON_SIZE_Y);
	m_pCancelButton->setParent(this);
	m_pCancelButton->addActionSignal(new CMenuHandler_ScoreStatWindow(0));

	m_pScoreBoardButton = new CommandButton(CHudTextMessage::BufferedLocaliseTextString("#CTFMenu_ScoreBoard"), XRES(500) - (YRES(8) + XRES(124)), YRES(40), XRES(124), YRES(24));
	m_pScoreBoardButton->setParent(this);
	m_pScoreBoardButton->addActionSignal(new CMenuHandler_ScoreStatWindow(MENU_SCOREBOARD));

	m_pSaveButton = new CommandButton(CHudTextMessage::BufferedLocaliseTextString("#CTFMenu_SaveStats"), XRES(500) - ((XRES(124) + YRES(8)) * 2), YRES(40), XRES(124), YRES(24));
	m_pSaveButton->setParent(this);
	m_pSaveButton->addActionSignal(new CMenuHandler_SaveStats());

	SetActiveInfo(0);
}

void CStatsMenuPanel::Initialize()
{
	setVisible(false);
	m_pScrollPanel->setScrollValue(0, 0);
}

void CStatsMenuPanel::Open()
{
	Update();
	CMenuPanel::Open();
}

void CStatsMenuPanel::Reset()
{
	CMenuPanel::Reset();
	m_iCurrentInfo = 0;
}

void CStatsMenuPanel::SetActiveInfo(int iInput)
{
	for (int i = 0; i < StatsTeamsCount; ++i)
	{
		m_pButtons[i]->setArmed(false);
		m_pClassInfoPanel[i]->setVisible(false);
	}

	if (iInput < 0 || iInput >= StatsTeamsCount)
		iInput = 0;

	m_pButtons[iInput]->setArmed(true);
	m_pClassInfoPanel[iInput]->setVisible(true);

	m_iCurrentInfo = iInput;

	m_pScrollPanel->setScrollValue(0, 0);
	m_pScrollPanel->validate();
}

bool CStatsMenuPanel::SlotInput(int iSlot)
{
	const int buttonIndex = iSlot - 1;

	if (buttonIndex >= 0 && buttonIndex < 3)
	{
		auto button = m_pButtons[buttonIndex];

		if (button && !button->IsNotValid())
		{
			button->fireActionSignal();
			return true;
		}
	}

	return false;
}

void CStatsMenuPanel::SetPlayerImage(const char* szImage)
{
	CImageLabel* image = m_pClassImages[3];

	if (image)
	{
		image->setImage(szImage);
	}
}

int CStatsMenuPanel::MsgFunc_StatsInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int teamNum = READ_BYTE();
	g_iWinningTeam = READ_BYTE();
	const int iNumPlayers = READ_BYTE();
	const int chunkId = READ_BYTE();

	if (!teamNum)
	{
		if (m_pClassImages[0])
		{
			m_pClassImages[0]->setImage(sCTFStatsSelection[g_iWinningTeam]);
		}
	}

	switch (chunkId)
	{
	case 0:
		strcpy(g_TeamStatInfo[teamNum].szMVP, READ_STRING());
		g_TeamStatInfo[teamNum].iMVPVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostKills, READ_STRING());
		g_TeamStatInfo[teamNum].iKillsVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostCTF, READ_STRING());
		g_TeamStatInfo[teamNum].iCTFVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostOff, READ_STRING());
		g_TeamStatInfo[teamNum].iOffVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostDef, READ_STRING());
		g_TeamStatInfo[teamNum].iDefVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostSnipe, READ_STRING());
		g_TeamStatInfo[teamNum].iSnipeVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostBarnacle, READ_STRING());
		g_TeamStatInfo[teamNum].iBarnacleVal = READ_SHORT();
		break;

	case 1:
		strcpy(g_TeamStatInfo[teamNum].szMostDeaths, READ_STRING());
		g_TeamStatInfo[teamNum].iDeathsVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostSuicides, READ_STRING());
		g_TeamStatInfo[teamNum].iSuicidesVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostDamage, READ_STRING());
		g_TeamStatInfo[teamNum].iDamageVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostAccel, READ_STRING());
		g_TeamStatInfo[teamNum].iAccelVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostBackpack, READ_STRING());
		g_TeamStatInfo[teamNum].iBackpackVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostHealth, READ_STRING());
		g_TeamStatInfo[teamNum].iHealthVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostShield, READ_STRING());
		g_TeamStatInfo[teamNum].iShieldVal = READ_SHORT();
		strcpy(g_TeamStatInfo[teamNum].szMostJump, READ_STRING());
		g_TeamStatInfo[teamNum].iJumpVal = READ_SHORT();
		break;
	}

	g_TeamStatInfo[teamNum].bChunksRead |= (1 << chunkId);

	if (g_TeamStatInfo[teamNum].bChunksRead & 3)
	{
		char sz[64];
		sprintf(sz, "stats/stats_%s.ost", sCTFStatsSelection[teamNum]);

		char* pfile = (char*)gEngfuncs.COM_LoadFile(sz, 5, nullptr);

		if (pfile)
		{
			if (iNumPlayers <= 0)
			{
				strncpy(szStatsBuf[teamNum], "No players on this team\n", sizeof(szStatsBuf) - 1);
			}
			else
			{
				gViewPort->GetAllPlayersInfo();

				//TODO: this passes an arbitrary string as the format string which is incredibly dangerous (also in vanilla)
				//Overcomplicated time calculations, can just use modulo instead for seconds
				sprintf(
					szStatsBuf[teamNum],
					pfile,
					g_TeamStatInfo[teamNum].szMVP,
					g_TeamStatInfo[teamNum].iMVPVal,
					g_TeamStatInfo[teamNum].szMostKills,
					g_TeamStatInfo[teamNum].iKillsVal,
					g_TeamStatInfo[teamNum].szMostCTF,
					g_TeamStatInfo[teamNum].iCTFVal,
					g_TeamStatInfo[teamNum].szMostOff,
					g_TeamStatInfo[teamNum].iOffVal,
					g_TeamStatInfo[teamNum].szMostDef,
					g_TeamStatInfo[teamNum].iDefVal,
					g_TeamStatInfo[teamNum].szMostSnipe,
					g_TeamStatInfo[teamNum].iSnipeVal,
					g_TeamStatInfo[teamNum].szMostBarnacle,
					g_TeamStatInfo[teamNum].iBarnacleVal,
					g_TeamStatInfo[teamNum].szMostDeaths,
					g_TeamStatInfo[teamNum].iDeathsVal,
					g_TeamStatInfo[teamNum].szMostSuicides,
					g_TeamStatInfo[teamNum].iSuicidesVal,
					g_TeamStatInfo[teamNum].szMostDamage,
					g_TeamStatInfo[teamNum].iDamageVal,
					g_TeamStatInfo[teamNum].szMostAccel,
					g_TeamStatInfo[teamNum].iAccelVal / 60,
					g_TeamStatInfo[teamNum].iAccelVal
					+ 4 * (g_TeamStatInfo[teamNum].iAccelVal / 60)
					- (g_TeamStatInfo[teamNum].iAccelVal / 60 << 6),
					g_TeamStatInfo[teamNum].szMostBackpack,
					g_TeamStatInfo[teamNum].iBackpackVal / 60,
					g_TeamStatInfo[teamNum].iBackpackVal
					+ 4 * (g_TeamStatInfo[teamNum].iBackpackVal / 60)
					- (g_TeamStatInfo[teamNum].iBackpackVal / 60 << 6),
					g_TeamStatInfo[teamNum].szMostHealth,
					g_TeamStatInfo[teamNum].iHealthVal / 60,
					g_TeamStatInfo[teamNum].iHealthVal
					+ 4 * (g_TeamStatInfo[teamNum].iHealthVal / 60)
					- (g_TeamStatInfo[teamNum].iHealthVal / 60 << 6),
					g_TeamStatInfo[teamNum].szMostShield,
					g_TeamStatInfo[teamNum].iShieldVal / 60,
					g_TeamStatInfo[teamNum].iShieldVal
					+ 4 * (g_TeamStatInfo[teamNum].iShieldVal / 60)
					- (g_TeamStatInfo[teamNum].iShieldVal / 60 << 6),
					g_TeamStatInfo[teamNum].szMostJump,
					g_TeamStatInfo[teamNum].iJumpVal / 60,
					g_TeamStatInfo[teamNum].iJumpVal
					+ 4 * (g_TeamStatInfo[teamNum].iJumpVal / 60)
					- (g_TeamStatInfo[teamNum].iJumpVal / 60 << 6));
			}

			m_pStatsWindow[teamNum]->setText(szStatsBuf[teamNum]);

			int wide, tall;
			m_pStatsWindow[teamNum]->getTextImage()->getSize(wide, tall);
			m_pStatsWindow[teamNum]->setSize(wide, tall);

			//TODO: missing from vanilla
			gEngfuncs.COM_FreeFile(pfile);
		}
	}

	return 0;
}

int CStatsMenuPanel::MsgFunc_StatsPlayer(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const int playerIndex = READ_BYTE();
	const int iMVPVal = READ_SHORT();
	const int iKillsVal = READ_SHORT();
	const int iCTFVal = READ_SHORT();
	const int iOffVal = READ_SHORT();
	const int iDefVal = READ_SHORT();
	const int iSnipeVal = READ_SHORT();
	const int iBarnacleVal = READ_SHORT();
	const int iDeathsVal = READ_SHORT();
	const int iSuicidesVal = READ_SHORT();
	const int iDamageVal = READ_SHORT();
	const int iAccelVal = READ_SHORT();
	const int iBackpackVal = READ_SHORT();
	const int iHealthVal = READ_SHORT();
	const int iShieldTime = READ_SHORT();
	const int iJumpVal = READ_SHORT();

	if (playerIndex > 0 && playerIndex < MAX_PLAYERS)
	{
		char sz[64];
		sprintf(sz, "stats/stats_%s.ost", sCTFStatsSelection[3]);

		char* pfile = (char*)gEngfuncs.COM_LoadFile(sz, 5, nullptr);

		if (pfile)
		{
			//TODO: this passes an arbitrary string as the format string which is incredibly dangerous (also in vanilla)
			//Overcomplicated time calculations, can just use modulo instead for seconds
			sprintf(
				szStatsBuf[3],
				pfile,
				iMVPVal,
				iKillsVal,
				iCTFVal,
				iOffVal,
				iDefVal,
				iSnipeVal,
				iBarnacleVal,
				iDeathsVal,
				iSuicidesVal,
				iDamageVal,
				iAccelVal / 60,
				iAccelVal + 4 * (iAccelVal / 60) - (iAccelVal / 60 * 64),
				iBackpackVal / 60,
				iBackpackVal + 4 * (iBackpackVal / 60) - (iBackpackVal / 60 << 6),
				iHealthVal / 60,
				iHealthVal + 4 * (iHealthVal / 60) - (iHealthVal / 60 << 6),
				iShieldTime / 60,
				iShieldTime + 4 * (iShieldTime / 60) - (iShieldTime / 60 << 6),
				iJumpVal / 60,
				iJumpVal + 4 * (iJumpVal / 60) - (iJumpVal / 60 << 6));

			m_pStatsWindow[3]->setText(szStatsBuf[3]);

			int wide, tall;
			m_pStatsWindow[3]->getTextImage()->getSize(wide, tall);
			m_pStatsWindow[3]->setSize(wide, tall);

			//TODO: missing from vanilla
			gEngfuncs.COM_FreeFile(pfile);
		}
	}

	return 0;
}
