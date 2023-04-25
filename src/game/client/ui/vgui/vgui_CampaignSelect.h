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

#pragma once

#include <vector>

#include "CampaignSelectSystem.h"
#include "vgui_TeamFortressViewport.h"

namespace vgui
{
class Button;
class ButtonGroup;
class CListBox;
class RadioButton;
class TextPanel;
}

class CCampaignSelectPanel : public CTransparentPanel
{
private:
	CListBox* m_CampaignList;

	ButtonGroup* m_MissionGroup;

	RadioButton* m_MissionButton;
	RadioButton* m_TrainingButton;

	ButtonGroup* m_DifficultyGroup;

	std::vector<RadioButton*> m_Difficulties;

	ScrollPanel* m_pScrollPanel;
	TextPanel* m_Description;

	Button* m_StartButton;

	int m_TextHeight = 0;

	std::vector<CampaignInfo> m_Campaigns;

public:
	CCampaignSelectPanel(int iTrans, int x, int y, int wide, int tall);

	virtual void Open();
	virtual void Update();
	virtual void Initialize();

	virtual void Reset()
	{
	}

	void CampaignChanged();
	void StartCurrentCampaign();

private:
	void SetCampaignByIndex(int index);
};
