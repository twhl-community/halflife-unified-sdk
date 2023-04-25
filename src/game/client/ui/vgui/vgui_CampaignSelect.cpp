//=========== (C) Copyright 1996-2002 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Class Menu
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include <array>

#include "hud.h"
#include "CampaignSelectSystem.h"
#include "skill.h"
#include "vgui_CampaignSelect.h"
#include "vgui_listbox.h"

#include <VGUI_ButtonGroup.h>
#include <VGUI_RadioButton.h>
#include <VGUI_TextImage.h>

#define CAMPAIGNSELECT_TITLE_X XRES(40)
#define CAMPAIGNSELECT_TITLE_Y YRES(32)
#define CAMPAIGNSELECT_WINDOW_X XRES(64)
#define CAMPAIGNSELECT_WINDOW_Y YRES(64)
#define CAMPAIGNSELECT_WINDOW_SIZE_X XRES((640 - (64 * 2)))
#define CAMPAIGNSELECT_WINDOW_SIZE_Y YRES(200)

constexpr std::array<const char*, SkillLevelCount> DifficultyLabels =
	{
		"#CampaignSelect_Easy",
		"#CampaignSelect_Medium",
		"#CampaignSelect_Hard"};

class CampaignSelectionChangedButtonSignal : public ActionSignal
{
public:
	CCampaignSelectPanel* m_CampaignSelect;

public:
	CampaignSelectionChangedButtonSignal(CCampaignSelectPanel* campaignSelect)
		: m_CampaignSelect(campaignSelect)
	{
	}

public:
	void actionPerformed(Panel* panel) override
	{
		m_CampaignSelect->CampaignChanged();
	}
};

class CampaignStartButtonSignal : public ActionSignal
{
public:
	CCampaignSelectPanel* m_CampaignSelect;

public:
	CampaignStartButtonSignal(CCampaignSelectPanel* campaignSelect)
		: m_CampaignSelect(campaignSelect)
	{
	}

public:
	void actionPerformed(Panel* panel) override
	{
		m_CampaignSelect->StartCurrentCampaign();
	}
};

class CampaignRadioButton : public RadioButton
{
public:
	using RadioButton::RadioButton;

	void paintBackground() override
	{
		// Don't draw background.
	}
};

CCampaignSelectPanel::CCampaignSelectPanel(int iTrans, int x, int y, int wide, int tall)
	: CTransparentPanel(iTrans, x, y, wide, tall)
{
	// Get the scheme used for the Titles
	CSchemeManager* pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Title Font");
	SchemeHandle_t hClassWindowText = pSchemes->getSchemeHandle("Briefing Text");

	// color schemes
	int r, g, b, a;

	// Create the title
	Label* pLabel = new Label("", CAMPAIGNSELECT_TITLE_X, CAMPAIGNSELECT_TITLE_Y);
	pLabel->setParent(this);
	pLabel->setFont(pSchemes->getFont(hTitleScheme));
	pSchemes->getFgColor(hTitleScheme, r, g, b, a);
	pLabel->setFgColor(r, g, b, a);
	pSchemes->getBgColor(hTitleScheme, r, g, b, a);
	pLabel->setBgColor(r, g, b, a);
	pLabel->setContentAlignment(vgui::Label::a_west);
	pLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#CampaignSelect_Title"));

	int textWidth;
	pLabel->getTextSize(textWidth, m_TextHeight);

	int yOffset = 0;

	m_CampaignList = new CListBox();
	m_CampaignList->setParent(this);
	m_CampaignList->setBounds(CAMPAIGNSELECT_WINDOW_X, CAMPAIGNSELECT_WINDOW_Y + yOffset, CAMPAIGNSELECT_WINDOW_SIZE_X, m_TextHeight);

	m_CampaignList->GetScrollBar()->getButton(0)->addActionSignal(new CampaignSelectionChangedButtonSignal(this));
	m_CampaignList->GetScrollBar()->getButton(1)->addActionSignal(new CampaignSelectionChangedButtonSignal(this));

	yOffset += m_TextHeight;

	const int missionButtonWidth = 150;

	const int difficultyXPos = CAMPAIGNSELECT_WINDOW_X + missionButtonWidth + XRES(20);

	auto missionLabel = new Label(gHUD.m_TextMessage.BufferedLocaliseTextString("#CampaignSelect_Mission"),
		CAMPAIGNSELECT_WINDOW_X, CAMPAIGNSELECT_WINDOW_Y + yOffset);

	missionLabel->setParent(this);
	missionLabel->setContentAlignment(Label::a_west);

	missionLabel->setFgColor(Scheme::sc_primary1);
	missionLabel->setBgColor(0, 0, 0, 255);

	auto difficultyLabel = new Label(gHUD.m_TextMessage.BufferedLocaliseTextString("#CampaignSelect_Difficulty"),
		difficultyXPos, CAMPAIGNSELECT_WINDOW_Y + yOffset);

	difficultyLabel->setParent(this);
	difficultyLabel->setContentAlignment(Label::a_west);

	difficultyLabel->setFgColor(Scheme::sc_primary1);
	difficultyLabel->setBgColor(0, 0, 0, 255);

	yOffset += m_TextHeight;

	m_MissionGroup = new ButtonGroup();

	const int buttonsYStart = yOffset;

	m_MissionButton = new CampaignRadioButton(gHUD.m_TextMessage.BufferedLocaliseTextString("#CampaignSelect_Campaign"),
		CAMPAIGNSELECT_WINDOW_X, CAMPAIGNSELECT_WINDOW_Y + yOffset, missionButtonWidth, m_TextHeight);
	m_MissionButton->setParent(this);
	m_MissionButton->setButtonGroup(m_MissionGroup);
	m_MissionButton->setContentAlignment(Label::a_west);
	m_MissionButton->setFgColor(Scheme::sc_primary1);
	m_MissionButton->setBgColor(0, 0, 0, 0);

	yOffset += m_TextHeight;

	m_TrainingButton = new CampaignRadioButton(gHUD.m_TextMessage.BufferedLocaliseTextString("#CampaignSelect_Training"),
		CAMPAIGNSELECT_WINDOW_X, CAMPAIGNSELECT_WINDOW_Y + yOffset, missionButtonWidth, m_TextHeight);
	m_TrainingButton->setParent(this);
	m_TrainingButton->setButtonGroup(m_MissionGroup);
	m_TrainingButton->setContentAlignment(Label::a_west);
	m_TrainingButton->setFgColor(Scheme::sc_primary1);
	m_TrainingButton->setBgColor(0, 0, 0, 0);

	yOffset += m_TextHeight;

	m_DifficultyGroup = new ButtonGroup();

	const int difficultyButtonWidth = 200;
	int difficultyYOffset = buttonsYStart;

	for (auto label : DifficultyLabels)
	{
		auto button = new CampaignRadioButton(gHUD.m_TextMessage.BufferedLocaliseTextString(label),
			difficultyXPos,
			CAMPAIGNSELECT_WINDOW_Y + difficultyYOffset, difficultyButtonWidth, m_TextHeight);
		button->setParent(this);
		button->setButtonGroup(m_DifficultyGroup);
		button->setContentAlignment(Label::a_west);
		button->setFgColor(Scheme::sc_primary1);
		button->setBgColor(0, 0, 0, 0);

		m_Difficulties.push_back(button);

		difficultyYOffset += m_TextHeight;
	}

	m_Difficulties[0]->setSelected(true);

	yOffset = difficultyYOffset;

	// Create the Scroll panel
	m_pScrollPanel = new CTFScrollPanel(CAMPAIGNSELECT_WINDOW_X, CAMPAIGNSELECT_WINDOW_Y + yOffset,
		CAMPAIGNSELECT_WINDOW_SIZE_X, CAMPAIGNSELECT_WINDOW_SIZE_Y);
	m_pScrollPanel->setParent(this);
	// force the scrollbars on, so after the validate clientClip will be smaller
	m_pScrollPanel->setScrollBarAutoVisible(false, false);
	m_pScrollPanel->setScrollBarVisible(true, true);
	m_pScrollPanel->setBorder(new LineBorder(Color(0, 112, 0, 0)));
	m_pScrollPanel->validate();

	// turn scrollpanel back into auto show scrollbar mode and validate
	m_pScrollPanel->setScrollBarAutoVisible(false, true);
	m_pScrollPanel->setScrollBarVisible(false, false);
	m_pScrollPanel->validate();

	m_Description = new TextPanel("", 0, 0, CAMPAIGNSELECT_WINDOW_SIZE_X, CAMPAIGNSELECT_WINDOW_SIZE_Y);
	m_Description->setParent(m_pScrollPanel->getClient());
	m_Description->setFont(pSchemes->getFont(hClassWindowText));
	pSchemes->getFgColor(hClassWindowText, r, g, b, a);
	m_Description->setFgColor(r, g, b, a);
	pSchemes->getBgColor(hClassWindowText, r, g, b, a);
	m_Description->setBgColor(r, g, b, a);

	m_StartButton = new CommandButton(gHUD.m_TextMessage.BufferedLocaliseTextString("#CampaignSelect_Start"),
		CAMPAIGNSELECT_WINDOW_X, CAMPAIGNSELECT_WINDOW_Y + yOffset + CAMPAIGNSELECT_WINDOW_SIZE_Y + m_TextHeight,
		125, m_TextHeight);
	m_StartButton->setParent(this);
	m_StartButton->setFgColor(Scheme::sc_primary1);
	m_StartButton->setBgColor(0, 0, 0, 0);

	m_StartButton->addActionSignal(new CampaignStartButtonSignal(this));
}

void CCampaignSelectPanel::Update()
{
	m_CampaignList->SetScrollRange(m_CampaignList->GetNumItems());

	m_pScrollPanel->validate();
}

void CCampaignSelectPanel::Open()
{
	bool needsInit = false;

	if (m_CampaignList->GetNumItems() == 0)
	{
		needsInit = true;

		m_Campaigns = g_CampaignSelect.LoadCampaigns();

		for (const auto& campaign : m_Campaigns)
		{
			auto label = new Label(campaign.Label.c_str());

			int width, height;
			label->getSize(width, height);

			label->setSize(width, m_TextHeight);

			label->setContentAlignment(Label::a_west);

			label->setFgColor(Scheme::sc_primary1);
			label->setBgColor(0, 0, 0, 255);

			m_CampaignList->AddItem(label);
		}
	}

	if (needsInit)
	{
		SetCampaignByIndex(0);
	}

	Update();
	setVisible(true);
}

void CCampaignSelectPanel::Initialize()
{
	setVisible(false);
	m_pScrollPanel->setScrollValue(0, 0);
}

void CCampaignSelectPanel::CampaignChanged()
{
	SetCampaignByIndex(m_CampaignList->GetItemOffset());
}

void CCampaignSelectPanel::StartCurrentCampaign()
{
	const int index = m_CampaignList->GetItemOffset();

	if (index < 0 || std::size_t(index) >= m_Campaigns.size())
	{
		return;
	}

	int difficulty = 1;

	for (const auto button : m_Difficulties)
	{
		if (button->isSelected())
		{
			break;
		}

		++difficulty;
	}

	const auto& campaign = m_Campaigns[index];

	std::string_view mapName;

	if (m_MissionButton->isSelected())
	{
		mapName = campaign.CampaignMap;
	}
	else if (m_TrainingButton->isSelected())
	{
		mapName = campaign.TrainingMap;
	}
	else
	{
		Con_Printf("Failed to start campaign: no option selected\n");
		return;
	}

	// Matches the commands used in the New Game dialog.
	const auto commands = fmt::format("disconnect;maxplayers 1;deathmatch 0;skill {};map {};", difficulty, mapName);
	gEngfuncs.pfnClientCmd(commands.c_str());
}

void CCampaignSelectPanel::SetCampaignByIndex(int index)
{
	if (index < 0 || std::size_t(index) >= m_Campaigns.size())
	{
		m_MissionButton->setVisible(false);
		m_TrainingButton->setVisible(false);

		for (auto button : m_Difficulties)
		{
			button->setVisible(false);
		}

		m_Description->setText("No campaigns available.");
		m_StartButton->setVisible(false);
		return;
	}

	const auto& campaign = m_Campaigns[index];

	m_MissionButton->setVisible(!campaign.CampaignMap.empty());
	m_TrainingButton->setVisible(!campaign.TrainingMap.empty());

	if (!campaign.CampaignMap.empty())
	{
		m_MissionButton->setSelected(true);
	}
	else
	{
		m_TrainingButton->setSelected(true);
	}

	for (auto button : m_Difficulties)
	{
		button->setVisible(true);
	}

	m_Description->setText(campaign.Description.c_str());

	int iXSize, iYSize;
	m_Description->getTextImage()->getTextSize(iXSize, iYSize);
	m_Description->setSize(iXSize, iYSize);

	m_pScrollPanel->validate();

	m_StartButton->setVisible(true);
}
