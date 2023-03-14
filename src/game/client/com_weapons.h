//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// com_weapons.h
// Shared weapons common function prototypes

#pragma once

#include "Exports.h"

struct edict_t;
struct local_state_t;

bool CL_IsDead();

int HUD_GetWeaponAnim();
void HUD_SendWeaponAnim(int iAnim, int body, bool force);
void HUD_PlaybackEvent(int flags, const edict_t* pInvoker, unsigned short eventindex, float delay, const float* origin, const float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
void HUD_SetMaxSpeed(const edict_t* ed, float speed);

Vector HUD_GetLastOrg();
void HUD_SetLastOrg();

/**
 *	@brief Set up functions needed to run weapons code client-side.
 */
void HUD_SetupServerEngineInterface();

unsigned short stub_PrecacheEvent(int type, const char* s);
const char* stub_NameForFunction(uint32 function);
void stub_SetModel(edict_t* e, const char* m);


extern cvar_t* cl_lw;

// g_runfuncs is true if this is the first time we've "predicated" a particular movement/firing
//  command.  If it is 1, then we should play events/sounds etc., otherwise, we just will be
//  updating state info, but not firing events
inline bool g_runfuncs = false;

extern float g_lastFOV;

// During our weapon prediction processing, we'll need to reference some data that is part of
//  the final state passed into the postthink functionality.  We'll set this pointer and then
//  reset it to nullptr as appropriate
inline local_state_t* g_finalstate = nullptr;
