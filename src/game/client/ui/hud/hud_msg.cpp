/***
 *
 *	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//  hud_msg.cpp
//

#include "hud.h"
#include "r_efx.h"

#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"

#include "particleman.h"

extern int giTeamplay;

extern BEAM* pBeam;
extern BEAM* pBeam2;
extern TEMPENTITY* pFlare; // Vit_amiN


/// USER-DEFINED SERVER MESSAGE HANDLERS

void CHud::MsgFunc_ResetHUD(const char* pszName, BufferReader& reader)
{
	// clear all hud data
	for (auto hudElement : m_HudList)
	{
		hudElement->Reset();
	}

	// Reset weapon bits.
	m_iWeaponBits = 0ULL;

	m_HudFlags = 0U;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;
}

void CAM_ToFirstPerson();

void CHud::MsgFunc_ViewMode(const char* pszName, BufferReader& reader)
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD(const char* pszName, BufferReader& reader)
{
	// prepare all hud data
	for (auto hudElement : m_HudList)
	{
		hudElement->InitHUDData();
	}

	// TODO: needs to be called on every map change, not just when starting a new game
	if (g_pParticleMan)
		g_pParticleMan->ResetParticles();

	// Probably not a good place to put this.
	pBeam = pBeam2 = nullptr;
	pFlare = nullptr; // Vit_amiN: clear egon's beam flare
}


void CHud::MsgFunc_GameMode(const char* pszName, BufferReader& reader)
{
	m_Teamplay = giTeamplay = reader.ReadByte();

	if (gViewPort && !gViewPort->m_pScoreBoard)
	{
		gViewPort->CreateScoreBoard();
		gViewPort->m_pScoreBoard->Initialize();

		if (!gHUD.m_iIntermission)
		{
			gViewPort->HideScoreBoard();
		}
	}
}


void CHud::MsgFunc_Damage(const char* pszName, BufferReader& reader)
{
	int armor, blood;
	Vector from;
	int i;
	float count;

	armor = reader.ReadByte();
	blood = reader.ReadByte();

	for (i = 0; i < 3; i++)
		from[i] = reader.ReadCoord();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually
}

void CHud::MsgFunc_Concuss(const char* pszName, BufferReader& reader)
{
	m_iConcussionEffect = reader.ReadByte();
	if (0 != m_iConcussionEffect)
	{
		this->m_StatusIcons.EnableIcon("dmg_concuss", gHUD.m_HudColor);
	}
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
}

void CHud::MsgFunc_Weapons(const char* pszName, BufferReader& reader)
{
	const std::uint64_t lowerBits = reader.ReadLong();
	const std::uint64_t upperBits = reader.ReadLong();

	m_iWeaponBits = (lowerBits & 0XFFFFFFFF) | ((upperBits & 0XFFFFFFFF) << 32ULL);
}
