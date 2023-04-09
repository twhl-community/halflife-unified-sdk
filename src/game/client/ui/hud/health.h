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

#pragma once

#include "palette.h"

#define DMG_IMAGE_LIFE 2 // seconds that image is up
#define NUM_DMG_TYPES 12
// instant damage

struct DAMAGE_IMAGE
{
	int SpriteIndex{0};
	float fExpire;
	float fBaseline;
	int x, y;
};

//
//-----------------------------------------------------
//
class CHudHealth : public CHudBase
{
public:
	bool Init() override;
	bool VidInit() override;
	bool Draw(float fTime) override;
	void Reset() override;
	void MsgFunc_Health(const char* pszName, BufferReader& reader);
	void MsgFunc_Damage(const char* pszName, BufferReader& reader);
	int m_iHealth;
	int m_HUD_cross;
	float m_fAttackFront, m_fAttackRear, m_fAttackLeft, m_fAttackRight;
	RGB24 GetPainColor();
	float m_fFade;

private:
	HSPRITE m_hSprite;
	HSPRITE m_hDamage;

	DAMAGE_IMAGE m_dmg[NUM_DMG_TYPES];
	int m_bitsDamage;
	bool DrawPain(float fTime);
	bool DrawDamage(float fTime);
	void CalcDamageDirection(Vector vecFrom);
	void UpdateTiles(float fTime, long bits);
};
