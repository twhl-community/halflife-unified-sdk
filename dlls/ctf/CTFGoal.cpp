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
#include "basemonster.h"
#include "player.h"
#include "ctf/CTFGoal.h"

void CTFGoal::KeyValue(KeyValueData* pkvd)
{
    if (FStrEq("goal_no", pkvd->szKeyName))
    {
        m_iGoalNum = strtol(pkvd->szValue, 0, 10);
        pkvd->fHandled = true;
    }
    else if (FStrEq("goal_min", pkvd->szKeyName))
    {
        Vector tmp;
        UTIL_StringToVector(tmp, pkvd->szValue);
        if (tmp != g_vecZero)
            m_GoalMin = tmp;

        pkvd->fHandled = true;
    }
    else if (FStrEq("goal_max", pkvd->szKeyName))
    {
        Vector tmp;
        UTIL_StringToVector(tmp, pkvd->szValue);
        if (tmp != g_vecZero)
            m_GoalMax = tmp;

        pkvd->fHandled = true;
    }
    else
    {
        pkvd->fHandled = false;
    }
}

void CTFGoal::Spawn()
{
    if (pev->model)
    {
        const char* modelName = STRING(pev->model);

        if (*modelName == '*')
            pev->effects |= EF_NODRAW;

        g_engfuncs.pfnPrecacheModel((char*)modelName);
        g_engfuncs.pfnSetModel(edict(), STRING(pev->model));
    }

    pev->solid = SOLID_TRIGGER;

    if (!m_iGoalState)
        m_iGoalState = 1;

    UTIL_SetOrigin(pev, pev->origin);

    SetThink(&CTFGoal::PlaceGoal);
    pev->nextthink = gpGlobals->time + 0.2;
}

void CTFGoal::SetObjectCollisionBox()
{
    if (*STRING(pev->model) == '*')
    {
        float max = 0;
        for (int i = 0; i < 3; ++i)
        {
            float v = fabs(pev->mins[i]);
            if (v > max)
                max = v;
            v = fabs(pev->maxs[i]);
            if (v > max)
                max = v;
        }

        for (int i = 0; i < 3; ++i)
        {
            pev->absmin[i] = pev->origin[i] - max;
            pev->absmax[i] = pev->origin[i] + max;
        }

        pev->absmin.x -= 1.0;
        pev->absmin.y -= 1.0;
        pev->absmin.z -= 1.0;
        pev->absmax.x += 1.0;
        pev->absmax.y += 1.0;
        pev->absmax.z += 1.0;
    }
    else
    {
        pev->absmin = pev->origin + Vector(-24, -24, 0);
        pev->absmax = pev->origin + Vector(24, 24, 16);
    }
}

void CTFGoal::StartGoal()
{
    SetThink(&CTFGoal::PlaceGoal);
    pev->nextthink = gpGlobals->time + 0.2;
}

void CTFGoal::PlaceGoal()
{
    pev->movetype = MOVETYPE_NONE;
    pev->velocity = g_vecZero;
    pev->oldorigin = pev->origin;
}
