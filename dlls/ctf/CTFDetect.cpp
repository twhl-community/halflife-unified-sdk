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
#include "ctf/CTFDetect.h"
#include "gamerules.h"
#include "ctf/ctfplay_gamerules.h"

LINK_ENTITY_TO_CLASS(info_ctfdetect, CTFDetect);

void CTFDetect::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("is_ctf", pkvd->szKeyName))
	{
		is_ctf = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("flagreturntime", pkvd->szKeyName))
	{
		g_flFlagReturnTime = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("basedefenddist", pkvd->szKeyName))
	{
		g_flBaseDefendDist = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("defendcarriertime", pkvd->szKeyName))
	{
		g_flDefendCarrierTime = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("captureassisttime", pkvd->szKeyName))
	{
		g_flCaptureAssistTime = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("poweruprespawntime", pkvd->szKeyName))
	{
		g_flPowerupRespawnTime = atof(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("score_icon_namebm", pkvd->szKeyName))
	{
		strcpy(g_szScoreIconNameBM, pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("score_icon_nameof", pkvd->szKeyName))
	{
		strcpy(g_szScoreIconNameOF, pkvd->szValue);
		pkvd->fHandled = true;
	}
	else if (FStrEq("map_score_max", pkvd->szKeyName))
	{
		g_iMapScoreMax = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		pkvd->fHandled = false;
	}
}
