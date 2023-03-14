//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

// TODO: could just make a copy of ref_params_t when it gets passed in instead.
inline bool g_Paused = false;
inline WaterLevel g_WaterLevel = WaterLevel::Dry;
inline int g_ViewEntity = 0;

inline Vector v_origin;
inline Vector v_angles;
inline Vector v_cl_angles;
inline Vector v_sim_org;
inline Vector v_client_aimangles;
inline Vector v_crosshairangle;

void V_StartPitchDrift();
void V_StopPitchDrift();

void V_GetInEyePos(int entity, Vector& origin, Vector& angles);
void V_ResetChaseCam();
void V_GetChasePos(int target, Vector* cl_angles, Vector& origin, Vector& angles);
