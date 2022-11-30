//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

// Types of demo messages we can write/parse
enum
{
	TYPE_SNIPERDOT = 0,
	TYPE_ZOOM
};

void Demo_WriteBuffer(int type, int size, unsigned char* buffer);

extern int g_demosniper;
extern int g_demosniperdamage;
extern Vector g_demosniperorg;
extern Vector g_demosniperangles;
extern float g_demozoom;
