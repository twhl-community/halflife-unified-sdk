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

#include <string_view>

#include <VGUI_TextImage.h>

#include "hud.h"
#include "ClientLibrary.h"
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


#define STATSMENU_TITLE_X XRES(40)
#define STATSMENU_TITLE_Y YRES(32)
#define STATSMENU_TOPLEFT_BUTTON_X XRES(40)
#define STATSMENU_TOPLEFT_BUTTON_Y YRES(400)
#define STATSMENU_BUTTON_SIZE_X XRES(124)
#define STATSMENU_BUTTON_SIZE_Y YRES(24)
#define STATSMENU_BUTTON_SPACER_Y YRES(8)
#define STATSMENU_WINDOW_X XRES(40)
#define STATSMENU_WINDOW_Y YRES(80)
#define STATSMENU_WINDOW_SIZE_X XRES(600)
#define STATSMENU_WINDOW_SIZE_Y YRES(312)
#define STATSMENU_WINDOW_TEXT_X XRES(150)
#define STATSMENU_WINDOW_TEXT_Y YRES(80)
#define STATSMENU_WINDOW_NAME_X XRES(150)
#define STATSMENU_WINDOW_NAME_Y YRES(8)

static int g_iWinningTeam = 0;
static team_stat_info_t g_TeamStatInfo[3];

constexpr std::string_view DefaultTeamStatInfoFormatString = R"(MVP: {} with {} points
Most Kills: {} with {} kills
Highest CTF Score: {} with {} points
Most Offense: {} with {} points
Most Defense: {} with {} points
Most Sniper Kills: {} with {} kills
Most Barnacle Kills: {} with {} kills
Most Deaths: {} died {} times
Suicide King: {} killed themself {} times
Most Damage: {} with {} damage
Accelerator Whore: {} carried for {}:{:02}
Most Backpack time: {} carried for {}:{:02}
Most Health time: {} carried for {}:{:02}
Most Shield time: {} carried for {}:{:02}
Most Jump Pack time: {} carried for {}:{:02}
)";

constexpr std::string_view DefaultPlayerStatInfoFormatString = R"(Total Score: {} points
Kills: {} kills
CTF Score: {} points
Offense: {} points
Defense: {} points
Sniper Kills: {} kills
Barnacle Kills: {} kills
Deaths: {} times
Suicides: {} times
Damage: {} damage
Accelerator time: {}:{:02}
Backpack time: {}:{:02}
Health time: {}:{:02}
Shield time: {}:{:02}
Jump Pack time: {}:{:02}
)";

CStatsMenuPanel::CStatsMenuPanel(int iTrans, bool iRemoveMe, int x, int y, int wide, int tall)
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
	// force the scrollbars on, so after the validate clientClip will be smaller
	m_pScrollPanel->setScrollBarAutoVisible(false, false);
	m_pScrollPanel->setScrollBarVisible(true, true);
	m_pScrollPanel->setBorder(new LineBorder(Color(0, 112, 0, 0)));
	m_pScrollPanel->validate();

	int clientWide = m_pScrollPanel->getClient()->getWide();

	// turn scrollpanel back into auto show scrollbar mode and validate
	m_pScrollPanel->setScrollBarAutoVisible(false, true);
	m_pScrollPanel->setScrollBarVisible(false, false);
	m_pScrollPanel->validate();

	for (int i = 0; i < StatsTeamsCount; ++i)
	{
		char sz[256];
		// TODO: this is using YRES for an X coord. Bugged in vanilla
		int iXPos = STATSMENU_TOPLEFT_BUTTON_X + ((STATSMENU_BUTTON_SIZE_X + STATSMENU_BUTTON_SPACER_Y) * i);

		strcpy(sz, CHudTextMessage::BufferedLocaliseTextString(sLocalisedStatsTeams[i]));
		m_pButtons[i] = new CommandButton(sz, iXPos, STATSMENU_TOPLEFT_BUTTON_Y, STATSMENU_BUTTON_SIZE_X, STATSMENU_BUTTON_SIZE_Y, true);

		sprintf(sz, "%d", i);
		m_pButtons[i]->setBoundKey(sz[0]);
		m_pButtons[i]->setContentAlignment(vgui::Label::a_west);
		m_pButtons[i]->addInputSignal(new CHandler_MenuButtonOver(this, i));
		m_pButtons[i]->setParent(this);

		// Create the Class Info Window
		// m_pClassInfoPanel[i] = new CTransparentPanel( 255, CLASSMENU_WINDOW_X, CLASSMENU_WINDOW_Y, CLASSMENU_WINDOW_SIZE_X, CLASSMENU_WINDOW_SIZE_Y );
		m_pClassInfoPanel[i] = new CTransparentPanel(255, 0, 0, clientWide, STATSMENU_WINDOW_SIZE_Y);
		m_pClassInfoPanel[i]->setParent(m_pScrollPanel->getClient());
		// m_pClassInfoPanel[i]->setVisible( false );

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
		// pNameLabel->setBorder(new LineBorder());
		pNameLabel->setText("%s", localName);

		if (ScreenWidth >= BASE_XRES)
		{
			int xOut, yOut;
			pNameLabel->getTextSize(xOut, yOut);

			if (i == 3)
				strncpy(sz, sCTFStatsSelection[1], sizeof(sz));
			else
				strncpy(sz, sCTFStatsSelection[i], sizeof(sz));

			sz[sizeof(sz) - 1] = '\0';

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

		// check to see if the image goes lower than the text
		if (m_pClassImages[i] != nullptr)
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
		image->LoadImage(szImage);
	}
}

void CStatsMenuPanel::MsgFunc_StatsInfo(const char* pszName, BufferReader& reader)
{
	const int teamNum = reader.ReadByte();
	g_iWinningTeam = reader.ReadByte();
	const int iNumPlayers = reader.ReadByte();
	const int chunkId = reader.ReadByte();

	// TODO: define constants for team counts and such
	if (teamNum < 0 || teamNum > 2)
	{
		return;
	}

	if (g_iWinningTeam < 0 || g_iWinningTeam > 2)
	{
		return;
	}

	if (0 == teamNum)
	{
		if (m_pClassImages[0])
		{
			m_pClassImages[0]->LoadImage(sCTFStatsSelection[g_iWinningTeam]);
		}
	}

	switch (chunkId)
	{
	case 0:
		strcpy(g_TeamStatInfo[teamNum].szMVP, reader.ReadString());
		g_TeamStatInfo[teamNum].iMVPVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostKills, reader.ReadString());
		g_TeamStatInfo[teamNum].iKillsVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostCTF, reader.ReadString());
		g_TeamStatInfo[teamNum].iCTFVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostOff, reader.ReadString());
		g_TeamStatInfo[teamNum].iOffVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostDef, reader.ReadString());
		g_TeamStatInfo[teamNum].iDefVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostSnipe, reader.ReadString());
		g_TeamStatInfo[teamNum].iSnipeVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostBarnacle, reader.ReadString());
		g_TeamStatInfo[teamNum].iBarnacleVal = reader.ReadShort();
		break;

	case 1:
		strcpy(g_TeamStatInfo[teamNum].szMostDeaths, reader.ReadString());
		g_TeamStatInfo[teamNum].iDeathsVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostSuicides, reader.ReadString());
		g_TeamStatInfo[teamNum].iSuicidesVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostDamage, reader.ReadString());
		g_TeamStatInfo[teamNum].iDamageVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostAccel, reader.ReadString());
		g_TeamStatInfo[teamNum].iAccelVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostBackpack, reader.ReadString());
		g_TeamStatInfo[teamNum].iBackpackVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostHealth, reader.ReadString());
		g_TeamStatInfo[teamNum].iHealthVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostShield, reader.ReadString());
		g_TeamStatInfo[teamNum].iShieldVal = reader.ReadShort();
		strcpy(g_TeamStatInfo[teamNum].szMostJump, reader.ReadString());
		g_TeamStatInfo[teamNum].iJumpVal = reader.ReadShort();
		break;
	}

	g_TeamStatInfo[teamNum].bChunksRead |= (1 << chunkId);

	if ((g_TeamStatInfo[teamNum].bChunksRead & 3) == 0)
	{
		return;
	}

	std::string text;

	if (iNumPlayers <= 0)
	{
		text = "No players on this team\n";
	}
	else
	{
		gViewPort->GetAllPlayersInfo();

		const auto fileName = fmt::format("stats/stats_{}.ost", sCTFStatsSelection[teamNum]);

		const auto fileContents = FileSystem_LoadFileIntoBuffer(fileName.c_str(), FileContentFormat::Text);

		const auto formatter = [&](std::string_view formatString) -> std::string
		{
			try
			{
				const auto& info = g_TeamStatInfo[teamNum];

				return fmt::format(fmt::runtime({formatString.data(), formatString.size()}),
					info.szMVP,
					info.iMVPVal,
					info.szMostKills,
					info.iKillsVal,
					info.szMostCTF,
					info.iCTFVal,
					info.szMostOff,
					info.iOffVal,
					info.szMostDef,
					info.iDefVal,
					info.szMostSnipe,
					info.iSnipeVal,
					info.szMostBarnacle,
					info.iBarnacleVal,
					info.szMostDeaths,
					info.iDeathsVal,
					info.szMostSuicides,
					info.iSuicidesVal,
					info.szMostDamage,
					info.iDamageVal,
					info.szMostAccel,
					info.iAccelVal / 60, info.iAccelVal % 60,
					info.szMostBackpack,
					info.iBackpackVal / 60, info.iBackpackVal % 60,
					info.szMostHealth,
					info.iHealthVal / 60, info.iHealthVal % 60,
					info.szMostShield, info.iShieldVal / 60,
					info.iShieldVal % 60, info.szMostJump,
					info.iJumpVal / 60, info.iJumpVal % 60);
			}
			catch (const fmt::format_error& e)
			{
				g_UILogger->error("CTF stat format string file \"{}\" has invalid contents: {}",
					fileName, e.what());
				return {};
			}
		};

		if (!fileContents.empty())
		{
			text = formatter(reinterpret_cast<const char*>(fileContents.data()));
		}

		// If no file exists or the format string is invalid, fall back to the default format string.
		if (text.empty())
		{
			text = formatter(DefaultTeamStatInfoFormatString);
		}
	}

	m_pStatsWindow[teamNum]->setText(text.c_str());

	int wide, tall;
	m_pStatsWindow[teamNum]->getTextImage()->getSize(wide, tall);
	m_pStatsWindow[teamNum]->setSize(wide, tall);
}

void CStatsMenuPanel::MsgFunc_StatsPlayer(const char* pszName, BufferReader& reader)
{
	const int playerIndex = reader.ReadByte();
	const int iMVPVal = reader.ReadShort();
	const int iKillsVal = reader.ReadShort();
	const int iCTFVal = reader.ReadShort();
	const int iOffVal = reader.ReadShort();
	const int iDefVal = reader.ReadShort();
	const int iSnipeVal = reader.ReadShort();
	const int iBarnacleVal = reader.ReadShort();
	const int iDeathsVal = reader.ReadShort();
	const int iSuicidesVal = reader.ReadShort();
	const int iDamageVal = reader.ReadShort();
	const int iAccelVal = reader.ReadShort();
	const int iBackpackVal = reader.ReadShort();
	const int iHealthVal = reader.ReadShort();
	const int iShieldTime = reader.ReadShort();
	const int iJumpVal = reader.ReadShort();

	if (playerIndex <= 0 || playerIndex > +MAX_PLAYERS_HUD)
	{
		return;
	}

	const auto fileName = fmt::format("stats/stats_{}.ost", sCTFStatsSelection[3]);

	const auto fileContents = FileSystem_LoadFileIntoBuffer(fileName.c_str(), FileContentFormat::Text);

	std::string text;

	const auto formatter = [&](std::string_view formatString) -> std::string
	{
		try
		{
			return fmt::format(fmt::runtime({formatString.data(), formatString.size()}),
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
				iAccelVal / 60, iAccelVal % 60,
				iBackpackVal / 60, iBackpackVal % 60,
				iHealthVal / 60, iHealthVal % 60,
				iShieldTime / 60, iShieldTime % 60,
				iJumpVal / 60, iJumpVal % 60);
		}
		catch (const fmt::format_error& e)
		{
			g_UILogger->error("CTF stat format string file \"{}\" has invalid contents: {}",
				fileName, e.what());
			return {};
		}
	};

	if (!fileContents.empty())
	{
		text = formatter(reinterpret_cast<const char*>(fileContents.data()));
	}

	// If no file exists or the format string is invalid, fall back to the default format string.
	if (text.empty())
	{
		text = formatter(DefaultPlayerStatInfoFormatString);
	}

	m_pStatsWindow[3]->setText(text.c_str());

	int wide, tall;
	m_pStatsWindow[3]->getTextImage()->getSize(wide, tall);
	m_pStatsWindow[3]->setSize(wide, tall);
}
