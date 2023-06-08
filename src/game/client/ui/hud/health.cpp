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
// Health.cpp
//
// implementation of CHudHealth class
//

#include "hud.h"

int giDmgHeight, giDmgWidth;

struct DamageType
{
	const char* HudSpriteName;
	int TypeFlags;
};

// TODO: add remaining damage types from TFC
const DamageType g_DamageTypes[NUM_DMG_TYPES] =
	{
		{"dmg_poison", DMG_POISON},
		{"dmg_chem", DMG_ACID},
		{"dmg_cold", DMG_FREEZE | DMG_SLOWFREEZE},
		{"dmg_drown", DMG_DROWN},
		{"dmg_heat", DMG_BURN | DMG_SLOWBURN},
		{"dmg_gas", DMG_NERVEGAS},
		{"dmg_rad", DMG_RADIATION},
		{"dmg_shock", DMG_SHOCK},
		{"", DMG_CALTROP},
		{"", DMG_TRANQ},
		{"", DMG_CONCUSS},
		{"", DMG_HALLUC}};

bool CHudHealth::Init()
{
	g_ClientUserMessages.RegisterHandler("Health", &CHudHealth::MsgFunc_Health, this);
	g_ClientUserMessages.RegisterHandler("Battery", &CHudHealth::MsgFunc_Battery, this);
	g_ClientUserMessages.RegisterHandler("Damage", &CHudHealth::MsgFunc_Damage, this);
	m_iHealth = 100;
	m_HealthFade = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	memset(m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES);

	m_iBat = 0;
	m_ArmorFade = 0;

	gHUD.AddHudElem(this);
	return true;
}

void CHudHealth::Reset()
{
	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;


	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		m_dmg[i].fExpire = 0;
	}
}

bool CHudHealth::VidInit()
{
	m_DamageDirectionsSprite = gHUD.GetSprite(gHUD.GetSpriteIndex("pain_directions"));

	m_HUD_cross = gHUD.GetSpriteIndex("cross");

	for (int i = 0; i < NUM_DMG_TYPES; ++i)
	{
		m_dmg[i].SpriteIndex = gHUD.GetSpriteIndex(g_DamageTypes[i].HudSpriteName);
	}

	const Rect damageRect = gHUD.GetSpriteRect(m_dmg[0].SpriteIndex);

	giDmgHeight = damageRect.right - damageRect.left;
	giDmgWidth = damageRect.bottom - damageRect.top;

	const int HUD_suit_empty = gHUD.GetSpriteIndex("suit_empty");
	const int HUD_suit_full = gHUD.GetSpriteIndex("suit_full");

	m_ArmorSprite1 = gHUD.GetSprite(HUD_suit_empty);
	m_ArmorSprite2 = gHUD.GetSprite(HUD_suit_full);

	m_ArmorSprite1Rect = &gHUD.GetSpriteRect(HUD_suit_empty);
	m_ArmorSprite2Rect = &gHUD.GetSpriteRect(HUD_suit_full);
	m_ArmorFade = 0;

	return true;
}

void CHudHealth::MsgFunc_Health(const char* pszName, BufferReader& reader)
{
	// TODO: update local health data
	int x = reader.ReadShort();

	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if (x != m_iHealth)
	{
		m_HealthFade = FADE_TIME;
		m_iHealth = x;
	}
}

void CHudHealth::MsgFunc_Battery(const char* pszName, BufferReader& reader)
{
	m_iFlags |= HUD_ACTIVE;

	int x = reader.ReadShort();

	if (x != m_iBat)
	{
		m_ArmorFade = FADE_TIME;
		m_iBat = x;
	}
}

void CHudHealth::MsgFunc_Damage(const char* pszName, BufferReader& reader)
{
	int armor = reader.ReadByte();		 // armor
	int damageTaken = reader.ReadByte(); // health
	long bitsDamage = reader.ReadLong(); // damage bits

	Vector vecFrom;

	for (int i = 0; i < 3; i++)
		vecFrom[i] = reader.ReadCoord();

	UpdateTiles(gHUD.m_flTime, bitsDamage);

	// Actually took damage?
	if (damageTaken > 0 || armor > 0)
		CalcDamageDirection(vecFrom);
}

// Returns back a color from the
// Green <-> Yellow <-> Red ramp
RGB24 CHudHealth::GetPainColor()
{
	int iHealth = m_iHealth;

	if (iHealth > 25)
		iHealth -= 25;
	else if (iHealth < 0)
		iHealth = 0;

	RGB24 color;

#if 0
	const std::uint8_t g = iHealth * 255 / 100;

	color = {static_cast<std::uint8_t>(255 - g), g, 0};
#else
	if (m_iHealth > 25)
	{
		color = gHUD.m_HudItemColor;
	}
	else
	{
		color = {250, 0, 0};
	}
#endif

	return gHUD.GetHudItemColor(color);
}

bool CHudHealth::Draw(float flTime)
{
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) != 0 || 0 != gEngfuncs.IsSpectateOnly())
		return true;

	// Only draw health and armor if we have the suit.
	if (gHUD.HasSuit())
	{
		const int armorStartX = DrawHealth();
		DrawArmor(armorStartX);
	}

	DrawDamage(flTime);
	return DrawPain(flTime);
}

void CHudHealth::CalcDamageDirection(Vector vecFrom)
{
	Vector forward, right, up;
	float side, front;
	Vector vecOrigin, vecAngles;

	if (vecFrom == g_vecZero)
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		return;
	}


	memcpy(vecOrigin, gHUD.m_vecOrigin, sizeof(Vector));
	memcpy(vecAngles, gHUD.m_vecAngles, sizeof(Vector));


	vecFrom = vecFrom - vecOrigin;

	float flDistToTarget = vecFrom.Length();

	vecFrom = vecFrom.Normalize();
	AngleVectors(vecAngles, forward, right, up);

	front = DotProduct(vecFrom, right);
	side = DotProduct(vecFrom, forward);

	if (flDistToTarget <= 50)
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
	}
	else
	{
		if (side > 0)
		{
			if (side > 0.3)
				m_fAttackFront = std::max(m_fAttackFront, side);
		}
		else
		{
			float f = fabs(side);
			if (f > 0.3)
				m_fAttackRear = std::max(m_fAttackRear, f);
		}

		if (front > 0)
		{
			if (front > 0.3)
				m_fAttackRight = std::max(m_fAttackRight, front);
		}
		else
		{
			float f = fabs(front);
			if (f > 0.3)
				m_fAttackLeft = std::max(m_fAttackLeft, f);
		}
	}
}

int CHudHealth::DrawHealth()
{
	int a = MIN_ALPHA;

	// Has health changed? Flash the health #
	if (0 != m_HealthFade)
	{
		m_HealthFade -= (gHUD.m_flTimeDelta * 20);
		if (m_HealthFade <= 0)
		{
			m_HealthFade = 0;
		}

		// Fade the health number back to dim

		a += (m_HealthFade / FADE_TIME) * 128;
	}

	// If health is getting low, make it bright red
	if (m_iHealth <= 15)
		a = 255;

	const auto painColor = gHUD.GetHudItemColor(GetPainColor().Scale(a));

	const int HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
	int CrossWidth = gHUD.GetSpriteRect(m_HUD_cross).right - gHUD.GetSpriteRect(m_HUD_cross).left;

	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	int x = CrossWidth / 2;

	SPR_Set(gHUD.GetSprite(m_HUD_cross), painColor);
	SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_cross));

	x = CrossWidth + HealthWidth / 2;

	// Reserve space for 3 digits by default, but allow it to expand
	x += gHUD.GetHudNumberWidth(m_iHealth, 3, DHN_DRAWZERO);

	gHUD.DrawHudNumberReverse(x, y, m_iHealth, DHN_DRAWZERO, painColor);

	// x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iHealth, painColor);

	x += HealthWidth / 2;

	int iHeight = gHUD.m_iFontHeight;
	int iWidth = HealthWidth / 10;
	FillRGBA(x, y, iWidth, iHeight, gHUD.m_HudItemColor, a);

	return x + HealthWidth;
}

void CHudHealth::DrawArmor(int startX)
{
	int a = MIN_ALPHA;

	// Has health changed? Flash the health #
	if (0 != m_ArmorFade)
	{
		if (m_ArmorFade > FADE_TIME)
			m_ArmorFade = FADE_TIME;

		m_ArmorFade -= (gHUD.m_flTimeDelta * 20);
		if (m_ArmorFade <= 0)
		{
			m_ArmorFade = 0;
		}

		// Fade the health number back to dim

		a += (m_ArmorFade / FADE_TIME) * 128;
	}

	const auto color = gHUD.m_HudItemColor.Scale(a);

	const int iOffset = (m_ArmorSprite1Rect->bottom - m_ArmorSprite1Rect->top) / 6;

	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	int x = startX; // ScreenWidth / 4;

	SPR_Set(m_ArmorSprite1, color);
	SPR_DrawAdditive(0, x, y - iOffset, m_ArmorSprite1Rect);

	// Note: this assumes empty and full have the same size.
	Rect rc = *m_ArmorSprite2Rect;

	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1
	rc.top += (rc.bottom - rc.top) * (100 - std::min(100, m_iBat)) * 0.01f;

	if (rc.bottom > rc.top)
	{
		SPR_Set(m_ArmorSprite2, color);
		SPR_DrawAdditive(0, x, y - iOffset + (rc.top - m_ArmorSprite2Rect->top), &rc);
	}

	x += (m_ArmorSprite1Rect->right - m_ArmorSprite1Rect->left);
	x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, color);
}

bool CHudHealth::DrawPain(float flTime)
{
	if (!(0 != m_fAttackFront || 0 != m_fAttackRear || 0 != m_fAttackLeft || 0 != m_fAttackRight))
		return true;

	int x, y, shade;

	// TODO:  get the shift value of the health
	const int a = 255; // max brightness until then

	float fFade = gHUD.m_flTimeDelta * 2;

	// TODO: can probably rework this into an array and a for loop
	//  SPR_Draw top
	if (m_fAttackFront > 0.4)
	{
		shade = a * std::max(m_fAttackFront, 0.5f);
		const auto painColor = gHUD.GetHudItemColor(GetPainColor()).Scale(shade);
		SPR_Set(m_DamageDirectionsSprite, painColor);

		x = ScreenWidth / 2 - SPR_Width(m_DamageDirectionsSprite, 0) / 2;
		y = ScreenHeight / 2 - SPR_Height(m_DamageDirectionsSprite, 0) * 3;
		SPR_DrawAdditive(0, x, y, nullptr);
		m_fAttackFront = std::max(0.f, m_fAttackFront - fFade);
	}
	else
		m_fAttackFront = 0;

	if (m_fAttackRight > 0.4)
	{
		shade = a * std::max(m_fAttackRight, 0.5f);
		const auto painColor = gHUD.GetHudItemColor(GetPainColor()).Scale(shade);
		SPR_Set(m_DamageDirectionsSprite, painColor);

		x = ScreenWidth / 2 + SPR_Width(m_DamageDirectionsSprite, 1) * 2;
		y = ScreenHeight / 2 - SPR_Height(m_DamageDirectionsSprite, 1) / 2;
		SPR_DrawAdditive(1, x, y, nullptr);
		m_fAttackRight = std::max(0.f, m_fAttackRight - fFade);
	}
	else
		m_fAttackRight = 0;

	if (m_fAttackRear > 0.4)
	{
		shade = a * std::max(m_fAttackRear, 0.5f);
		const auto painColor = gHUD.GetHudItemColor(GetPainColor()).Scale(shade);
		SPR_Set(m_DamageDirectionsSprite, painColor);

		x = ScreenWidth / 2 - SPR_Width(m_DamageDirectionsSprite, 2) / 2;
		y = ScreenHeight / 2 + SPR_Height(m_DamageDirectionsSprite, 2) * 2;
		SPR_DrawAdditive(2, x, y, nullptr);
		m_fAttackRear = std::max(0.f, m_fAttackRear - fFade);
	}
	else
		m_fAttackRear = 0;

	if (m_fAttackLeft > 0.4)
	{
		shade = a * std::max(m_fAttackLeft, 0.5f);
		const auto painColor = gHUD.GetHudItemColor(GetPainColor()).Scale(shade);
		SPR_Set(m_DamageDirectionsSprite, painColor);

		x = ScreenWidth / 2 - SPR_Width(m_DamageDirectionsSprite, 3) * 3;
		y = ScreenHeight / 2 - SPR_Height(m_DamageDirectionsSprite, 3) / 2;
		SPR_DrawAdditive(3, x, y, nullptr);

		m_fAttackLeft = std::max(0.f, m_fAttackLeft - fFade);
	}
	else
		m_fAttackLeft = 0;

	return true;
}

bool CHudHealth::DrawDamage(float flTime)
{
	if (0 == m_bitsDamage)
		return true;

	const int a = (int)(fabs(sin(flTime * 2)) * 256.0);

	const auto color = gHUD.m_HudColor.Scale(a);

	// Draw all the items
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		if ((m_bitsDamage & g_DamageTypes[i].TypeFlags) != 0)
		{
			DAMAGE_IMAGE* pdmg = &m_dmg[i];
			SPR_Set(gHUD.GetSprite(pdmg->SpriteIndex), color);
			SPR_DrawAdditive(0, pdmg->x, pdmg->y, &gHUD.GetSpriteRect(pdmg->SpriteIndex));
		}
	}


	// check for bits that should be expired
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		DAMAGE_IMAGE* pdmg = &m_dmg[i];

		if ((m_bitsDamage & g_DamageTypes[i].TypeFlags) != 0)
		{
			pdmg->fExpire = std::min(flTime + DMG_IMAGE_LIFE, pdmg->fExpire);

			if (pdmg->fExpire <= flTime // when the time has expired
				&& a < 40)				// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for (int j = 0; j < NUM_DMG_TYPES; j++)
				{
					pdmg = &m_dmg[j];
					if (0 != pdmg->y && (pdmg->y < y))
						pdmg->y += giDmgHeight;
				}

				m_bitsDamage &= ~g_DamageTypes[i].TypeFlags; // clear the bits
			}
		}
	}

	return true;
}

void CHudHealth::UpdateTiles(float flTime, long bitsDamage)
{
	DAMAGE_IMAGE* pdmg;

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;

	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		pdmg = &m_dmg[i];

		// Is this one already on?
		if ((m_bitsDamage & g_DamageTypes[i].TypeFlags) != 0)
		{
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if (0 == pdmg->fBaseline)
				pdmg->fBaseline = flTime;
		}

		// Are we just turning it on?
		if ((bitsOn & g_DamageTypes[i].TypeFlags) != 0)
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth / 8;
			pdmg->y = ScreenHeight - giDmgHeight * 2;
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE;

			// move everyone else up
			for (int j = 0; j < NUM_DMG_TYPES; j++)
			{
				if (j == i)
					continue;

				pdmg = &m_dmg[j];
				if (0 != pdmg->y)
					pdmg->y -= giDmgHeight;
			}
			pdmg = &m_dmg[i];
		}
	}

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
}
