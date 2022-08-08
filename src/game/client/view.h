//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

//TODO: could just make a copy of ref_params_t when it gets passed in instead.
inline bool g_Paused = false;
inline int g_MaxEntities = 0;
inline int g_WaterLevel = 0;

void V_StartPitchDrift();
void V_StopPitchDrift();
