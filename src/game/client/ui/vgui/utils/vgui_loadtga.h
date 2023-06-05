//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

#include "VGUI_BitmapTGA.h"

vgui::BitmapTGA* vgui_LoadTGA(char const* pFilename, bool invertAlpha = true);

vgui::BitmapTGA* vgui_LoadTGAWithDirectory(const char* pImageName,
	bool addResolutionPrefix = false, bool invertAlpha = true);
