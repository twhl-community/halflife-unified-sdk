//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// eventscripts.h

#pragma once

struct event_args_t;

// Some of these are HL/TFC specific?
void EV_EjectBrass(float* origin, float* velocity, float rotation, int model, int soundtype);
void EV_GetGunPosition(event_args_t* args, float* pos, float* origin);
void EV_GetDefaultShellInfo(event_args_t* args, float* origin, float* velocity, float* ShellVelocity, float* ShellOrigin, float* forward, float* right, float* up, float forwardScale, float upScale, float rightScale);
bool EV_IsLocal(int idx);
bool EV_IsPlayer(int idx);
void EV_CreateTracer(float* start, float* end);

cl_entity_t* GetEntity(int idx);
cl_entity_t* GetViewEntity();
void EV_MuzzleFlash();
