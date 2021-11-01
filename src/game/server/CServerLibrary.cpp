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
#include "com_model.h"
#include "CServerLibrary.h"

bool CServerLibrary::Initialize()
{
	return true;
}

void CServerLibrary::Shutdown()
{
}

void CServerLibrary::NewMapStarted(bool loadGame)
{
	//Initialize map state to its default state
	m_MapState = CMapState{};
}

void CServerLibrary::MapActivate()
{
}
