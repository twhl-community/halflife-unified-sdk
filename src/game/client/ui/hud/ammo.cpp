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
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "HudSpriteConfigSystem.h"
#include "pm_shared.h"
#include "triangleapi.h"
#include "com_model.h"
#include "r_studioint.h"

#include "ammohistory.h"
#include "AmmoTypeSystem.h"
#include "vgui_TeamFortressViewport.h"
#include "view.h"
#include "WeaponDataSystem.h"

#include "sound/ISoundSystem.h"

extern engine_studio_api_t IEngineStudio;

WEAPON* gpActiveSel; // nullptr means off, 1 means just the menu bar, otherwise
					 // this points to the active weapon menu item
WEAPON* gpLastSel;	 // Last weapon menu selection

int g_weaponselect = 0;

void WeaponsResource::InitializeWeapons()
{
	// Reload weapons based on current level's weapon data.
	rgWeapons = {};

	for (int i = 0; i < g_WeaponData.GetCount(); ++i)
	{
		const auto info = g_WeaponData.GetByIndex(i + 1);

		if (!info || info->Id == WEAPON_NONE)
		{
			continue;
		}

		auto& weapon = rgWeapons[i + 1];

		weapon.Info = info;
		weapon.AmmoType1 = g_AmmoTypes.GetByName(info->AmmoType1.c_str());
		weapon.AmmoType2 = g_AmmoTypes.GetByName(info->AmmoType2.c_str());

		LoadWeaponSprites(&weapon);
	}
}

void WeaponsResource::PickupWeapon(WEAPON* wp)
{
	rgSlots[wp->Info->Slot][wp->Info->Position] = wp;
}

void WeaponsResource::DropWeapon(WEAPON* wp)
{
	rgSlots[wp->Info->Slot][wp->Info->Position] = nullptr;
}

void WeaponsResource::DropAllWeapons()
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (rgWeapons[i].Info && rgWeapons[i].Info->Id != WEAPON_NONE)
			DropWeapon(&rgWeapons[i]);
	}
}

int WeaponsResource::CountAmmo(const AmmoType* type)
{
	if (!type)
		return 0;

	return riAmmo[type->Id];
}

bool WeaponsResource::HasAmmo(WEAPON* p)
{
	if (!p)
		return false;

	// weapons with no primary ammo or unlimited ammo can always be selected
	if (!p->AmmoType1 || p->AmmoType1->MaximumCapacity == WEAPON_NOCLIP)
		return true;

	return p->AmmoInMagazine > 0 ||
		   0 != CountAmmo(p->AmmoType1) ||
		   0 != CountAmmo(p->AmmoType2) ||
		   (p->Info->Flags & ITEM_FLAG_SELECTONEMPTY) != 0;
}


void WeaponsResource::LoadWeaponSprites(WEAPON* pWeapon)
{
	if (!pWeapon)
		return;

	pWeapon->rcCrosshair = Rect{};
	pWeapon->rcAutoaim = Rect{};
	pWeapon->rcZoomedCrosshair = Rect{};
	pWeapon->rcZoomedAutoaim = Rect{};
	pWeapon->rcInactive = Rect{};
	pWeapon->rcActive = Rect{};
	pWeapon->rcAmmo = Rect{};
	pWeapon->rcAmmo2 = Rect{};

	pWeapon->hCrosshair = 0;
	pWeapon->hAutoaim = 0;
	pWeapon->hZoomedCrosshair = 0;
	pWeapon->hZoomedAutoaim = 0;
	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;

	const auto sprites = g_HudSpriteConfig.Load(fmt::format("sprites/{}.json", pWeapon->Info->Name.c_str()).c_str());

	if (sprites.empty())
		return;

	const HudSprite* p;

	if (p = GetSpriteList(sprites, "crosshair"); p)
	{
		pWeapon->hCrosshair = SPR_Load(fmt::format("sprites/{}.spr", p->SpriteName.c_str()));
		pWeapon->rcCrosshair = p->Rectangle;
	}

	if (p = GetSpriteList(sprites, "autoaim"); p)
	{
		pWeapon->hAutoaim = SPR_Load(fmt::format("sprites/{}.spr", p->SpriteName.c_str()));
		pWeapon->rcAutoaim = p->Rectangle;
	}

	if (p = GetSpriteList(sprites, "zoom"); p)
	{
		pWeapon->hZoomedCrosshair = SPR_Load(fmt::format("sprites/{}.spr", p->SpriteName.c_str()));
		pWeapon->rcZoomedCrosshair = p->Rectangle;
	}
	else
	{
		pWeapon->hZoomedCrosshair = pWeapon->hCrosshair; // default to non-zoomed crosshair
		pWeapon->rcZoomedCrosshair = pWeapon->rcCrosshair;
	}

	if (p = GetSpriteList(sprites, "zoom_autoaim"); p)
	{
		pWeapon->hZoomedAutoaim = SPR_Load(fmt::format("sprites/{}.spr", p->SpriteName.c_str()));
		pWeapon->rcZoomedAutoaim = p->Rectangle;
	}
	else
	{
		// TODO: should this be using hAutoaim instead?
		pWeapon->hZoomedAutoaim = pWeapon->hZoomedCrosshair; // default to zoomed crosshair
		pWeapon->rcZoomedAutoaim = pWeapon->rcZoomedCrosshair;
	}

	if (p = GetSpriteList(sprites, "weapon"); p)
	{
		pWeapon->hInactive = SPR_Load(fmt::format("sprites/{}.spr", p->SpriteName.c_str()));
		pWeapon->rcInactive = p->Rectangle;

		gHR.iHistoryGap = std::max(gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top);
	}

	if (p = GetSpriteList(sprites, "weapon_s"); p)
	{
		pWeapon->hActive = SPR_Load(fmt::format("sprites/{}.spr", p->SpriteName.c_str()));
		pWeapon->rcActive = p->Rectangle;
	}

	if (p = GetSpriteList(sprites, "ammo"); p)
	{
		pWeapon->hAmmo = SPR_Load(fmt::format("sprites/{}.spr", p->SpriteName.c_str()));
		pWeapon->rcAmmo = p->Rectangle;

		gHR.iHistoryGap = std::max(gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top);
	}

	if (p = GetSpriteList(sprites, "ammo2"); p)
	{
		pWeapon->hAmmo2 = SPR_Load(fmt::format("sprites/{}.spr", p->SpriteName.c_str()));
		pWeapon->rcAmmo2 = p->Rectangle;

		gHR.iHistoryGap = std::max(gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top);
	}
}

// Returns the first weapon for a given slot.
WEAPON* WeaponsResource::GetFirstPos(int iSlot)
{
	WEAPON* pret = nullptr;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if (rgSlots[iSlot][i] && HasAmmo(rgSlots[iSlot][i]))
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}


WEAPON* WeaponsResource::GetNextActivePos(int iSlot, int iSlotPos)
{
	if (iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS)
		return nullptr;

	WEAPON* p = gWR.rgSlots[iSlot][iSlotPos + 1];

	if (!p || !gWR.HasAmmo(p))
		return GetNextActivePos(iSlot, iSlotPos + 1);

	return p;
}


int giBucketHeight, giBucketWidth;

// Ammo Bar width and height
const int giABHeight = 4;
const int giABWidth = 20;

HSPRITE ghsprBuckets; // Sprite for top row of weapons menu

DECLARE_COMMAND(m_Ammo, Slot1);
DECLARE_COMMAND(m_Ammo, Slot2);
DECLARE_COMMAND(m_Ammo, Slot3);
DECLARE_COMMAND(m_Ammo, Slot4);
DECLARE_COMMAND(m_Ammo, Slot5);
DECLARE_COMMAND(m_Ammo, Slot6);
DECLARE_COMMAND(m_Ammo, Slot7);
DECLARE_COMMAND(m_Ammo, Slot8);
DECLARE_COMMAND(m_Ammo, Slot9);
DECLARE_COMMAND(m_Ammo, Slot10);
DECLARE_COMMAND(m_Ammo, Close);
DECLARE_COMMAND(m_Ammo, NextWeapon);
DECLARE_COMMAND(m_Ammo, PrevWeapon);

// width of ammo fonts
#define AMMO_SMALL_WIDTH 10
#define AMMO_LARGE_WIDTH 20

#define HISTORY_DRAW_TIME "5"

void SendWeaponSelectCommand(const char* weaponName)
{
	char command[512];

	const int result = snprintf(command, sizeof(command), "selectweapon %s", weaponName);

	if (result >= 0 && result < sizeof(command))
	{
		ServerCmd(command);
	}
	else
	{
		gEngfuncs.Con_Printf("Error formatting selectweapon command\n");
	}
}

bool CHudAmmo::Init()
{
	gHUD.AddHudElem(this);

	g_ClientUserMessages.RegisterHandler("CurWeapon", &CHudAmmo::MsgFunc_CurWeapon, this);	 // Current weapon and clip
	g_ClientUserMessages.RegisterHandler("AmmoPickup", &CHudAmmo::MsgFunc_AmmoPickup, this); // flashes an ammo pickup record
	g_ClientUserMessages.RegisterHandler("WeapPickup", &CHudAmmo::MsgFunc_WeapPickup, this); // flashes a weapon pickup record
	g_ClientUserMessages.RegisterHandler("ItemPickup", &CHudAmmo::MsgFunc_ItemPickup, this);
	g_ClientUserMessages.RegisterHandler("HideWeapon", &CHudAmmo::MsgFunc_HideWeapon, this); // hides the weapon, ammo, and crosshair displays temporarily
	g_ClientUserMessages.RegisterHandler("AmmoX", &CHudAmmo::MsgFunc_AmmoX, this);			 // update known ammo type's count

	HOOK_COMMAND("slot1", Slot1);
	HOOK_COMMAND("slot2", Slot2);
	HOOK_COMMAND("slot3", Slot3);
	HOOK_COMMAND("slot4", Slot4);
	HOOK_COMMAND("slot5", Slot5);
	HOOK_COMMAND("slot6", Slot6);
	HOOK_COMMAND("slot7", Slot7);
	HOOK_COMMAND("slot8", Slot8);
	HOOK_COMMAND("slot9", Slot9);
	HOOK_COMMAND("slot10", Slot10);
	HOOK_COMMAND("cancelselect", Close);
	HOOK_COMMAND("invnext", NextWeapon);
	HOOK_COMMAND("invprev", PrevWeapon);

	Reset();

	CVAR_CREATE("hud_drawhistory_time", HISTORY_DRAW_TIME, 0);
	CVAR_CREATE("hud_fastswitch", "0", FCVAR_ARCHIVE); // controls whether or not weapons can be selected in one keypress
	m_pCvarCrosshairScale = CVAR_CREATE("crosshair_scale", "4", FCVAR_ARCHIVE);

	m_iFlags |= HUD_ACTIVE; //!!!

	gWR.Init();
	gHR.Init();

	return true;
}

void CHudAmmo::Reset()
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	gpActiveSel = nullptr;
	gHUD.m_iHideHUDDisplay = 0;

	gWR.Reset();
	gHR.Reset();
}

bool CHudAmmo::VidInit()
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_selection = gHUD.GetSpriteIndex("selection");

	for (int i = 0; i < MAX_WEAPON_SLOTS; ++i)
	{
		m_BucketSprites[i] = gHUD.GetSpriteIndex(fmt::format("bucket{}", i + 1).c_str());
	}

	ghsprBuckets = gHUD.GetSprite(m_BucketSprites[0]);
	const Rect bucketRect = gHUD.GetSpriteRect(m_BucketSprites[0]);
	giBucketWidth = bucketRect.right - bucketRect.left;
	giBucketHeight = bucketRect.bottom - bucketRect.top;

	gHR.iHistoryGap = std::max(gHR.iHistoryGap, bucketRect.bottom - bucketRect.top);

	// Get weapon and ammo info from server info, load weapon sprites.
	gWR.InitializeWeapons();

	return true;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think()
{
	if (gHUD.m_fPlayerDead)
		return;

	if (gHUD.m_iWeaponBits != gWR.iOldWeaponBits)
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = MAX_WEAPONS - 1; i > 0; i--)
		{
			WEAPON* p = gWR.GetWeapon(i);

			if (p && p->Info)
			{
				if (gHUD.HasWeapon(p->Info->Id))
					gWR.PickupWeapon(p);
				else
					gWR.DropWeapon(p);
			}
		}
	}

	if (!gpActiveSel)
		return;

	// has the player selected one?
	if ((gHUD.m_iKeyBits & IN_ATTACK) != 0)
	{
		if (gpActiveSel != (WEAPON*)1)
		{
			SendWeaponSelectCommand(gpActiveSel->Info->Name.c_str());
			g_weaponselect = gpActiveSel->Info->Id;
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = nullptr;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound("common/wpn_select.wav", 1);
	}
}

//
// Helper function to return a Ammo pointer from id
//

HSPRITE* WeaponsResource::GetAmmoPicFromWeapon(int iAmmoId, Rect& rect)
{
	for (auto& weapon : rgWeapons)
	{
		if (!weapon.Info)
		{
			continue;
		}

		if (weapon.AmmoType1 && weapon.AmmoType1->Id == iAmmoId)
		{
			rect = weapon.rcAmmo;
			return &weapon.hAmmo;
		}
		else if (weapon.AmmoType2 && weapon.AmmoType2->Id == iAmmoId)
		{
			rect = weapon.rcAmmo2;
			return &weapon.hAmmo2;
		}
	}

	return nullptr;
}


// Menu Selection Code

void WeaponsResource::SelectSlot(int iSlot, bool fAdvance, int iDirection)
{
	if (gHUD.m_Menu.m_fMenuDisplayed && (!fAdvance) && (iDirection == 1))
	{										   // menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem(iSlot + 1); // slots are one off the key numbers
		return;
	}

	if (iSlot > MAX_WEAPON_SLOTS)
		return;

	if (gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
		return;

	if (!gHUD.HasSuit())
		return;

	if (!gHUD.HasAnyWeapons())
		return;

	WEAPON* p = nullptr;
	bool fastSwitch = CVAR_GET_FLOAT("hud_fastswitch") != 0;

	if ((gpActiveSel == nullptr) || (gpActiveSel == (WEAPON*)1) || (iSlot != gpActiveSel->Info->Slot))
	{
		PlaySound("common/wpn_hudon.wav", 1);
		p = GetFirstPos(iSlot);

		if (p && fastSwitch) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress
			// but only if there is only one item in the bucket
			WEAPON* p2 = GetNextActivePos(p->Info->Slot, p->Info->Position);
			if (!p2)
			{ // only one active item in bucket, so change directly to weapon
				SendWeaponSelectCommand(p->Info->Name.c_str());
				g_weaponselect = p->Info->Id;
				return;
			}
		}
	}
	else
	{
		PlaySound("common/wpn_moveselect.wav", 1);
		if (gpActiveSel)
			p = GetNextActivePos(gpActiveSel->Info->Slot, gpActiveSel->Info->Position);
		if (!p)
			p = GetFirstPos(iSlot);
	}


	if (!p) // no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if (!fastSwitch)
			gpActiveSel = (WEAPON*)1;
		else
			gpActiveSel = nullptr;
	}
	else
		gpActiveSel = p;
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//
// AmmoX  -- Update the count of a known type of ammo
//
void CHudAmmo::MsgFunc_AmmoX(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	gWR.SetAmmo(iIndex, abs(iCount));
}

void CHudAmmo::MsgFunc_AmmoPickup(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	// Add ammo to the history
	gHR.AddToHistory(HISTSLOT_AMMO, iIndex, abs(iCount));
}

void CHudAmmo::MsgFunc_WeapPickup(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int iIndex = READ_BYTE();

	// Add the weapon to the history
	gHR.AddToHistory(HISTSLOT_WEAP, iIndex);
}

void CHudAmmo::MsgFunc_ItemPickup(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	const char* szName = READ_STRING();

	// Add the weapon to the history
	gHR.AddToHistory(HISTSLOT_ITEM, szName);
}


void CHudAmmo::MsgFunc_HideWeapon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	gHUD.m_iHideHUDDisplay = READ_BYTE();

	if (0 != gEngfuncs.IsSpectateOnly())
		return;

	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
	{
		static Rect nullrc;
		gpActiveSel = nullptr;
		SetDrawCrosshair(false);
		SetCrosshair(0, nullrc);
	}
	else
	{
		if (m_pWeapon)
		{
			SetDrawCrosshair(true);
			SetCrosshair(m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair);
		}
	}
}

//
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type
//  numbers match a real ammo type.
//
void CHudAmmo::MsgFunc_CurWeapon(const char* pszName, int iSize, void* pbuf)
{
	static Rect nullrc;

	BEGIN_READ(pbuf, iSize);

	const WeaponState iState = static_cast<WeaponState>(READ_BYTE());
	int iId = READ_CHAR();
	int iClip = READ_CHAR();

	// detect if we're also on target
	const bool onTarget = iState > WeaponState::Active;

	if (iId < 1)
	{
		SetDrawCrosshair(false);
		SetCrosshair(0, nullrc);
		m_pWeapon = nullptr;
		return;
	}

	if (g_iUser1 != OBS_IN_EYE)
	{
		// Is player dead???
		if ((iId == -1) && (iClip == -1))
		{
			gHUD.m_fPlayerDead = true;
			gpActiveSel = nullptr;
			return;
		}
		gHUD.m_fPlayerDead = false;
	}

	WEAPON* pWeapon = gWR.GetWeapon(iId);

	if (!pWeapon)
		return;

	if (iClip < -1)
		pWeapon->AmmoInMagazine = abs(iClip);
	else
		pWeapon->AmmoInMagazine = iClip;


	if (iState == WeaponState::Inactive) // we're not the current weapon, so update no more
		return;

	m_pWeapon = pWeapon;

	SetDrawCrosshair(true);

	if (gHUD.m_iFOV >= 90)
	{ // normal crosshairs
		if (onTarget && 0 != m_pWeapon->hAutoaim)
			SetAutoaimCrosshair(m_pWeapon->hAutoaim, m_pWeapon->rcAutoaim);
		else
			SetAutoaimCrosshair(0, {});

		SetCrosshair(m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair);
	}
	else
	{ // zoomed crosshairs
		if (onTarget && 0 != m_pWeapon->hZoomedAutoaim)
			SetAutoaimCrosshair(m_pWeapon->hZoomedAutoaim, m_pWeapon->rcZoomedAutoaim);
		else
			SetAutoaimCrosshair(0, {});

		SetCrosshair(m_pWeapon->hZoomedCrosshair, m_pWeapon->rcZoomedCrosshair);
	}

	m_fFade = 200.0f; //!!!
	m_iFlags |= HUD_ACTIVE;
}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput(int iSlot)
{
	if (gViewPort && gViewPort->SlotInput(iSlot))
		return;

	gWR.SelectSlot(iSlot, false, 1);
}

void CHudAmmo::UserCmd_Slot1()
{
	SlotInput(0);
}

void CHudAmmo::UserCmd_Slot2()
{
	SlotInput(1);
}

void CHudAmmo::UserCmd_Slot3()
{
	SlotInput(2);
}

void CHudAmmo::UserCmd_Slot4()
{
	SlotInput(3);
}

void CHudAmmo::UserCmd_Slot5()
{
	SlotInput(4);
}

void CHudAmmo::UserCmd_Slot6()
{
	SlotInput(5);
}

void CHudAmmo::UserCmd_Slot7()
{
	SlotInput(6);
}

void CHudAmmo::UserCmd_Slot8()
{
	SlotInput(7);
}

void CHudAmmo::UserCmd_Slot9()
{
	SlotInput(8);
}

void CHudAmmo::UserCmd_Slot10()
{
	SlotInput(9);
}

void CHudAmmo::UserCmd_Close()
{
	if (gpActiveSel)
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = nullptr;
		PlaySound("common/wpn_hudoff.wav", 1);
	}
	else
		EngineClientCmd("escape");
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon()
{
	if (gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
		return;

	if (!gpActiveSel || gpActiveSel == (WEAPON*)1)
		gpActiveSel = m_pWeapon;

	int pos = 0;
	int slot = 0;
	if (gpActiveSel)
	{
		pos = gpActiveSel->Info->Position + 1;
		slot = gpActiveSel->Info->Slot;
	}

	for (int loop = 0; loop <= 1; loop++)
	{
		for (; slot < MAX_WEAPON_SLOTS; slot++)
		{
			for (; pos < MAX_WEAPON_POSITIONS; pos++)
			{
				WEAPON* wsp = gWR.GetWeaponSlot(slot, pos);

				if (wsp && gWR.HasAmmo(wsp))
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = 0;
		}

		slot = 0; // start looking from the first slot again
	}

	gpActiveSel = nullptr;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon()
{
	if (gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
		return;

	if (!gpActiveSel || gpActiveSel == (WEAPON*)1)
		gpActiveSel = m_pWeapon;

	int pos = MAX_WEAPON_POSITIONS - 1;
	int slot = MAX_WEAPON_SLOTS - 1;
	if (gpActiveSel)
	{
		pos = gpActiveSel->Info->Position - 1;
		slot = gpActiveSel->Info->Slot;
	}

	for (int loop = 0; loop <= 1; loop++)
	{
		for (; slot >= 0; slot--)
		{
			for (; pos >= 0; pos--)
			{
				WEAPON* wsp = gWR.GetWeaponSlot(slot, pos);

				if (wsp && gWR.HasAmmo(wsp))
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS - 1;
		}

		slot = MAX_WEAPON_SLOTS - 1;
	}

	gpActiveSel = nullptr;
}

void CHudAmmo::SetCrosshair(HSPRITE sprite, Rect rect)
{
	m_Crosshair.sprite = sprite;
	m_Crosshair.rect = rect;
}

void CHudAmmo::SetAutoaimCrosshair(HSPRITE sprite, Rect rect)
{
	m_AutoaimCrosshair.sprite = sprite;
	m_AutoaimCrosshair.rect = rect;
}

void AdjustSubRect(const int iWidth, const int iHeight, float& left, float& right, float& top, float& bottom, int& w, int& h, const Rect& rcSubRect)
{
	if (rcSubRect.left >= rcSubRect.right || rcSubRect.top >= rcSubRect.bottom)
	{
		return;
	}

	const int iLeft = std::max(0, rcSubRect.left);
	const int iRight = std::min(w, rcSubRect.right);

	if (iLeft >= iRight)
	{
		return;
	}

	const int iTop = std::max(0, rcSubRect.top);
	const int iBottom = std::min(h, rcSubRect.bottom);

	if (iTop >= iBottom)
	{
		return;
	}

	w = iRight - iLeft;
	h = iBottom - iTop;

	const double flWidth = 1.0 / iWidth;

	left = (iLeft + 0.5) * flWidth;
	right = (iRight - 0.5) * flWidth;

	const double flHeight = 1.0 / iHeight;

	top = (iTop + 0.5) * flHeight;
	bottom = (iBottom - 0.5) * flHeight;
}

void CHudAmmo::DrawCrosshair(int x, int y)
{
	auto renderer = [this](int x, int y, const Crosshair& crosshair, RGB24 color)
	{
		if (0 != crosshair.sprite)
		{
			if (0 == IEngineStudio.IsHardware())
			{
				// Fall back to the regular render path for software
				x -= (crosshair.rect.right - crosshair.rect.left) / 2;
				y -= (crosshair.rect.bottom - crosshair.rect.top) / 2;

				SPR_Set(crosshair.sprite, color);
				gEngfuncs.pfnSPR_DrawHoles(0, x, y, &crosshair.rect);
				return;
			}

			const int iOrigWidth = gEngfuncs.pfnSPR_Width(crosshair.sprite, 0);
			const int iOrigHeight = gEngfuncs.pfnSPR_Height(crosshair.sprite, 0);

			const float flScale = std::max(1.f, m_pCvarCrosshairScale->value);

			Rect rect;

			// Trim a pixel border around it, since it blends.
			rect.left = crosshair.rect.left * flScale + (flScale - 1);
			rect.top = crosshair.rect.top * flScale + (flScale - 1);
			rect.right = crosshair.rect.right * flScale - (flScale - 1);
			rect.bottom = crosshair.rect.bottom * flScale - (flScale - 1);

			const int iWidth = iOrigWidth * flScale;
			const int iHeight = iOrigHeight * flScale;

			x -= (rect.right - rect.left) / 2;
			y -= (rect.bottom - rect.top) / 2;

			auto pCrosshair = const_cast<model_t*>(gEngfuncs.GetSpritePointer(crosshair.sprite));

			auto TriAPI = gEngfuncs.pTriAPI;

			TriAPI->SpriteTexture(pCrosshair, 0);

			TriAPI->Color4fRendermode(color.Red / 255.0f, color.Green / 255.0f, color.Blue / 255.0f, 255 / 255.0f, kRenderTransAlpha);
			TriAPI->RenderMode(kRenderTransAlpha);

			float flLeft = 0;
			float flTop = 0;
			float flRight = 1.0;
			float flBottom = 1.0f;

			int iImgWidth = iWidth;
			int iImgHeight = iHeight;

			AdjustSubRect(iWidth, iHeight, flLeft, flRight, flTop, flBottom, iImgWidth, iImgHeight, rect);

			TriAPI->Begin(TRI_QUADS);

			TriAPI->TexCoord2f(flLeft, flTop);
			TriAPI->Vertex3f(x, y, 0);

			TriAPI->TexCoord2f(flRight, flTop);
			TriAPI->Vertex3f(x + iImgWidth, y, 0);

			TriAPI->TexCoord2f(flRight, flBottom);
			TriAPI->Vertex3f(x + iImgWidth, y + iImgHeight, 0);

			TriAPI->TexCoord2f(flLeft, flBottom);
			TriAPI->Vertex3f(x, y + iImgHeight, 0);

			TriAPI->End();
		}
	};

	renderer(x, y, m_Crosshair, gHUD.m_HudItemColor);
	renderer(x, y, m_AutoaimCrosshair, RGB_REDISH);
}


//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

bool CHudAmmo::Draw(float flTime)
{
	int x, y;
	int AmmoWidth;

	if (!gHUD.HasSuit())
		return true;

	if ((gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) != 0)
		return true;

	// Draw Weapon Menu
	DrawWList(flTime);

	// Draw ammo pickup history
	gHR.DrawAmmoHistory(flTime);

	// Draw crosshair here so original engine behavior is mimicked pretty closely
	{
		if (0 != gHUD.m_pCvarCrosshair->value && m_DrawCrosshair)
		{
			const Vector angles = v_angles + v_crosshairangle;
			Vector forward;
			AngleVectors(angles, &forward, nullptr, nullptr);

			Vector point = v_origin + forward;
			Vector screen;
			gEngfuncs.pTriAPI->WorldToScreen(point, screen);

			// Round the value so the crosshair doesn't jitter
			screen = screen * 1000;

			for (int i = 0; i < 3; ++i)
			{
				screen[i] = std::floor(i);
			}

			screen = screen / 1000;

			const int adjustedX = XPROJECT(screen[0]);
			const int adjustedY = YPROJECT(screen[1]);

			DrawCrosshair(adjustedX, adjustedY);
		}
	}

	if ((m_iFlags & HUD_ACTIVE) == 0)
		return false;

	if (!m_pWeapon)
		return false;

	WEAPON* pw = m_pWeapon; // shorthand

	// SPR_Draw Ammo
	if (!m_pWeapon->AmmoType1 && !m_pWeapon->AmmoType2)
		return false;


	int iFlags = DHN_DRAWZERO; // draw 0 values

	AmmoWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;

	const int a = std::max(MIN_ALPHA, static_cast<int>(m_fFade));

	if (m_fFade > 0)
		m_fFade -= (gHUD.m_flTimeDelta * 20);

	const auto color = gHUD.GetHudItemColor(gHUD.m_HudItemColor.Scale(a));

	// Does this weapon have a clip?
	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

	// Does weapon have any ammo at all?
	if (m_pWeapon->AmmoType1)
	{
		int iIconWidth = m_pWeapon->rcAmmo.right - m_pWeapon->rcAmmo.left;

		if (m_pWeapon->AmmoInMagazine >= 0)
		{
			// room for the number and the '|' and the current ammo

			x = ScreenWidth - (8 * AmmoWidth) - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, m_pWeapon->AmmoInMagazine, color);

			Rect rc;
			rc.top = 0;
			rc.left = 0;
			rc.right = AmmoWidth;
			rc.bottom = 100;

			int iBarWidth = AmmoWidth / 10;

			x += AmmoWidth / 2;

			// draw the | bar
			FillRGBA(x, y, iBarWidth, gHUD.m_iFontHeight, gHUD.m_HudItemColor, a);

			x += iBarWidth + AmmoWidth / 2;

			// GL Seems to need the color to be scaled
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(m_pWeapon->AmmoType1), color);
		}
		else
		{
			// SPR_Draw a bullets only line
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(m_pWeapon->AmmoType1), color);
		}

		// Draw the ammo Icon
		int iOffset = (m_pWeapon->rcAmmo.bottom - m_pWeapon->rcAmmo.top) / 8;
		SPR_Set(m_pWeapon->hAmmo, color);
		SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo);
	}

	// Does weapon have seconday ammo?
	if (m_pWeapon->AmmoType2)
	{
		int iIconWidth = m_pWeapon->rcAmmo2.right - m_pWeapon->rcAmmo2.left;

		// Do we have secondary ammo?
		if (gWR.CountAmmo(m_pWeapon->AmmoType2) > 0)
		{
			y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight / 4;
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(m_pWeapon->AmmoType2), color);

			// Draw the ammo Icon
			SPR_Set(m_pWeapon->hAmmo2, color);
			int iOffset = (m_pWeapon->rcAmmo2.bottom - m_pWeapon->rcAmmo2.top) / 8;
			SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo2);
		}
	}
	return true;
}


//
// Draws the ammo bar on the hud
//
int DrawBar(int x, int y, int width, int height, float f)
{
	if (f < 0)
		f = 0;
	if (f > 1)
		f = 1;

	if (0 != f)
	{
		int w = f * width;

		// Always show at least one pixel if we have ammo.
		if (w <= 0)
			w = 1;
		FillRGBA(x, y, w, height, RGB_GREENISH, 255);
		x += w;
		width -= w;
	}

	FillRGBA(x, y, width, height, gHUD.m_HudItemColor, 128);

	return (x + width);
}



void DrawAmmoBar(WEAPON* p, int x, int y, int width, int height)
{
	if (!p)
		return;

	if (p->AmmoType1)
	{
		if (0 == gWR.CountAmmo(p->AmmoType1))
			return;

		float f = (float)gWR.CountAmmo(p->AmmoType1) / (float)p->AmmoType1->MaximumCapacity;

		x = DrawBar(x, y, width, height, f);


		// Do we have secondary ammo too?

		if (p->AmmoType2)
		{
			f = (float)gWR.CountAmmo(p->AmmoType2) / (float)p->AmmoType2->MaximumCapacity;

			x += 5; //!!!

			DrawBar(x, y, width, height, f);
		}
	}
}




//
// Draw Weapon Menu
//
bool CHudAmmo::DrawWList(float flTime)
{
	int x, y, i;

	if (!gpActiveSel)
		return false;

	int iActiveSlot;

	if (gpActiveSel == (WEAPON*)1)
		iActiveSlot = -1; // current slot has no weapons
	else
		iActiveSlot = gpActiveSel->Info->Slot;

	x = 10; //!!!
	y = 10; //!!!


	// Ensure that there are available choices in the active slot
	if (iActiveSlot > 0)
	{
		if (!gWR.GetFirstPos(iActiveSlot))
		{
			gpActiveSel = (WEAPON*)1;
			iActiveSlot = -1;
		}
	}

	// Draw top line
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		int iWidth;

		/*
		if (iActiveSlot == i)
			a = 255;
		else
			a = 192;
		*/

		SPR_Set(gHUD.GetSprite(m_BucketSprites[i]), gHUD.m_HudItemColor);

		// make active slot wide enough to accomodate gun pictures
		if (i == iActiveSlot)
		{
			WEAPON* p = gWR.GetFirstPos(iActiveSlot);
			if (p)
				iWidth = p->rcActive.right - p->rcActive.left;
			else
				iWidth = giBucketWidth;
		}
		else
			iWidth = giBucketWidth;

		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_BucketSprites[i]));

		x += iWidth + 5;
	}


	x = 10;

	// Draw all of the buckets
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		y = giBucketHeight + 10;

		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if (i == iActiveSlot)
		{
			WEAPON* p = gWR.GetFirstPos(i);
			int iWidth = giBucketWidth;
			if (p)
				iWidth = p->rcActive.right - p->rcActive.left;

			for (int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++)
			{
				p = gWR.GetWeaponSlot(i, iPos);

				if (!p || !p->Info)
					continue;

				const auto color = gHUD.m_HudItemColor;

				// if active, then we must have ammo.

				if (gpActiveSel == p)
				{
					SPR_Set(p->hActive, color);
					SPR_DrawAdditive(0, x, y, &p->rcActive);

					SPR_Set(gHUD.GetSprite(m_HUD_selection), color);
					SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_selection));
				}
				else
				{
					// Draw Weapon if Red if no ammo

					const auto inactiveColor = gWR.HasAmmo(p) ? gHUD.GetHudItemColor(color.Scale(192)) : RGB_REDISH.Scale(128);

					SPR_Set(p->hInactive, inactiveColor);
					SPR_DrawAdditive(0, x, y, &p->rcInactive);
				}

				// Draw Ammo Bar

				DrawAmmoBar(p, x + giABWidth / 2, y, giABWidth, giABHeight);

				y += p->rcActive.bottom - p->rcActive.top + 5;
			}

			x += iWidth + 5;
		}
		else
		{
			// Draw Row of weapons.

			for (int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++)
			{
				WEAPON* p = gWR.GetWeaponSlot(i, iPos);

				if (!p || !p->Info)
					continue;

				RGB24 color;
				int a;

				if (gWR.HasAmmo(p))
				{
					color = gHUD.m_HudItemColor;
					a = 128;
				}
				else
				{
					color = RGB_REDISH;
					a = 96;
				}

				FillRGBA(x, y, giBucketWidth, giBucketHeight, color, a);

				y += giBucketHeight + 5;
			}

			x += giBucketWidth + 5;
		}
	}

	return true;
}
