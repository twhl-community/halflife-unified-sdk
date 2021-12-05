/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "shake.h"
#include "UserMessages.h"

int gmsgShake = 0;
int gmsgFade = 0;
int gmsgSelAmmo = 0;
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgShowGameTitle = 0;
int gmsgCurWeapon = 0;
int gmsgHealth = 0;
int gmsgDamage = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgWeaponList = 0;
int gmsgAmmoX = 0;
int gmsgHudText = 0;
int gmsgDeathMsg = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgMOTD = 0;
int gmsgServerName = 0;
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgGeigerRange = 0;
int gmsgTeamNames = 0;

int gmsgStatusText = 0;
int gmsgStatusValue = 0;

int gmsgSpectator = 0;
int gmsgStatusIcon = 0;
int gmsgPlayerBrowse = 0;
int gmsgHudColor = 0;
int gmsgFlagIcon = 0;
int gmsgFlagTimer = 0;
int gmsgPlayerIcon = 0;
int gmsgVGUIMenu = 0;
int gmsgAllowSpec = 0;
int gmsgSetMenuTeam = 0;
int gmsgCTFScore = 0;
int gmsgStatsInfo = 0;
int gmsgStatsPlayer = 0;
int gmsgTeamFull = 0;
int gmsgOldWeapon = 0;
int gmsgCustomIcon = 0;

void LinkUserMessages()
{
	// Already taken care of?
	if (0 != gmsgSelAmmo)
	{
		return;
	}

	gmsgSelAmmo = REG_USER_MSG("SelAmmo", sizeof(SelAmmo));
	gmsgCurWeapon = REG_USER_MSG("CurWeapon", 3);
	gmsgGeigerRange = REG_USER_MSG("Geiger", 1);
	gmsgFlashlight = REG_USER_MSG("Flashlight", 2);
	gmsgFlashBattery = REG_USER_MSG("FlashBat", 1);
	gmsgHealth = REG_USER_MSG("Health", 2);
	gmsgDamage = REG_USER_MSG("Damage", 12);
	gmsgBattery = REG_USER_MSG("Battery", 2);
	gmsgTrain = REG_USER_MSG("Train", 1);
	//gmsgHudText = REG_USER_MSG( "HudTextPro", -1 );
	gmsgHudText = REG_USER_MSG("HudText", -1); // we don't use the message but 3rd party addons may!
	gmsgSayText = REG_USER_MSG("SayText", -1);
	gmsgTextMsg = REG_USER_MSG("TextMsg", -1);
	gmsgWeaponList = REG_USER_MSG("WeaponList", -1);
	gmsgResetHUD = REG_USER_MSG("ResetHUD", 1); // called every respawn
	gmsgInitHUD = REG_USER_MSG("InitHUD", 0);	// called every time a new player joins the server
	gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
	gmsgDeathMsg = REG_USER_MSG("DeathMsg", -1);
	gmsgScoreInfo = REG_USER_MSG("ScoreInfo", 5);
	gmsgTeamInfo = REG_USER_MSG("TeamInfo", -1);   // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG("TeamScore", -1); // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG("GameMode", 1);
	gmsgMOTD = REG_USER_MSG("MOTD", -1);
	gmsgServerName = REG_USER_MSG("ServerName", -1);
	gmsgAmmoPickup = REG_USER_MSG("AmmoPickup", 2);
	gmsgWeapPickup = REG_USER_MSG("WeapPickup", 1);
	gmsgItemPickup = REG_USER_MSG("ItemPickup", -1);
	gmsgHideWeapon = REG_USER_MSG("HideWeapon", 1);
	gmsgSetFOV = REG_USER_MSG("SetFOV", 1);
	gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);
	gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
	gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));
	gmsgAmmoX = REG_USER_MSG("AmmoX", 2);
	gmsgTeamNames = REG_USER_MSG("TeamNames", -1);

	gmsgStatusText = REG_USER_MSG("StatusText", -1);
	gmsgStatusValue = REG_USER_MSG("StatusValue", 3);

	gmsgSpectator = g_engfuncs.pfnRegUserMsg("Spectator", 2);
	gmsgStatusIcon = g_engfuncs.pfnRegUserMsg("StatusIcon", -1);
	gmsgPlayerBrowse = g_engfuncs.pfnRegUserMsg("PlyrBrowse", -1);
	gmsgHudColor = g_engfuncs.pfnRegUserMsg("HudColor", 3);
	gmsgFlagIcon = g_engfuncs.pfnRegUserMsg("FlagIcon", -1);
	gmsgFlagTimer = g_engfuncs.pfnRegUserMsg("FlagTimer", -1);
	gmsgPlayerIcon = g_engfuncs.pfnRegUserMsg("PlayerIcon", -1);
	gmsgVGUIMenu = g_engfuncs.pfnRegUserMsg("VGUIMenu", -1);
	gmsgAllowSpec = g_engfuncs.pfnRegUserMsg("AllowSpec", 1);
	gmsgSetMenuTeam = g_engfuncs.pfnRegUserMsg("SetMenuTeam", 1);
	gmsgCTFScore = g_engfuncs.pfnRegUserMsg("CTFScore", 2);
	gmsgStatsInfo = g_engfuncs.pfnRegUserMsg("StatsInfo", -1);
	gmsgStatsPlayer = g_engfuncs.pfnRegUserMsg("StatsPlayer", 31);
	gmsgTeamFull = g_engfuncs.pfnRegUserMsg("TeamFull", 1);
	gmsgOldWeapon = g_engfuncs.pfnRegUserMsg("OldWeapon", 1);
	gmsgCustomIcon = g_engfuncs.pfnRegUserMsg("CustomIcon", -1);
}
