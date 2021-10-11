#pragma once

#include "vgui_TeamFortressViewport.h"

class CStatsMenuPanel : public CMenuPanel
{
public:
	CStatsMenuPanel(int iTrans, int iRemoveMe, int x, int y, int wide, int tall);

	void Initialize();
	void Open() override;
	void Reset() override;
	void SetActiveInfo(int iInput) override;
	bool SlotInput(int iSlot) override;

	void SetPlayerImage(const char* szImage);

	virtual void Update() {}
	void SaveStats() {}

	int MsgFunc_StatsInfo(const char* pszName, int iSize, void* pbuf);
	int MsgFunc_StatsPlayer(const char* pszName, int iSize, void* pbuf);

public:
	CTransparentPanel* m_pClassInfoPanel[StatsTeamsCount];

	CommandButton* m_pButtons[StatsTeamsCount];
	CommandButton* m_pCancelButton;
	CommandButton* m_pScoreBoardButton;

	vgui::ScrollPanel* m_pScrollPanel;
	vgui::TextPanel* m_pStatsWindow[StatsTeamsCount];
	CImageLabel* m_pClassImages[StatsTeamsCount];

	int m_iCurrentInfo;

	CommandButton* m_pSaveButton;
};

class CMenuHandler_ScoreStatWindow : public vgui::ActionSignal
{
public:
	CMenuHandler_ScoreStatWindow(int state)
		: m_iState(state)
	{
	}

	void actionPerformed(vgui::Panel* panel) override
	{
		if (m_iState)
		{
			if (m_iState == MENU_STATSMENU)
			{
				gViewPort->SwitchToStatsMenu();
			}
			else if (m_iState == MENU_SCOREBOARD)
			{
				gViewPort->SwitchToScoreBoard();
			}
		}
		else
		{
			gViewPort->HideScoreStatsWindow();
		}
	}

private:
	int m_iState;
};

class CMenuHandler_SaveStats : public vgui::ActionSignal
{
public:
	CMenuHandler_SaveStats()
	{
	}

	void actionPerformed(vgui::Panel* panel) override
	{
		gViewPort->SaveStatsMenu();
	}

private:
};