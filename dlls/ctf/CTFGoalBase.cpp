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
#include "ctf/CTFGoalBase.h"

LINK_ENTITY_TO_CLASS(item_ctfbase, CTFGoalBase);

void CTFGoalBase::BaseThink()
{
    Vector vecLightPos, vecLightAng;
    GetAttachment(0, vecLightPos, vecLightAng);

    g_engfuncs.pfnMessageBegin(MSG_BROADCAST, SVC_TEMPENTITY, 0, 0);
    g_engfuncs.pfnWriteByte(TE_ELIGHT);
    g_engfuncs.pfnWriteShort(entindex() + 0x1000);
    g_engfuncs.pfnWriteCoord(vecLightPos.x);
    g_engfuncs.pfnWriteCoord(vecLightPos.y);
    g_engfuncs.pfnWriteCoord(vecLightPos.z);
    g_engfuncs.pfnWriteCoord(128);

    if (m_iGoalNum == 1)
    {
        g_engfuncs.pfnWriteByte(255);
        g_engfuncs.pfnWriteByte(128);
        g_engfuncs.pfnWriteByte(128);
    }
    else
    {
        g_engfuncs.pfnWriteByte(128);
        g_engfuncs.pfnWriteByte(255);
        g_engfuncs.pfnWriteByte(128);
    }

    g_engfuncs.pfnWriteByte(255);
    g_engfuncs.pfnWriteCoord(0);
    g_engfuncs.pfnMessageEnd();
    pev->nextthink = gpGlobals->time + 20.0;
}

void CTFGoalBase::Spawn()
{
    pev->movetype = MOVETYPE_TOSS;
    pev->solid = SOLID_NOT;
    UTIL_SetOrigin(pev, pev->origin);

    Vector vecMin;
    Vector vecMax;

    vecMax.x = 16;
    vecMax.y = 16;
    vecMax.z = 16;
    vecMin.x = -16;
    vecMin.y = -16;
    vecMin.z = 0;
    UTIL_SetSize(pev, vecMin, vecMax);

    if (!g_engfuncs.pfnDropToFloor(edict()))
    {
        ALERT(
            at_error,
            "Item %s fell out of level at %f,%f,%f",
            STRING(pev->classname),
            pev->origin.x,
            pev->origin.y,
            pev->origin.z);

        UTIL_Remove(this);
    }
    else
    {
        if (pev->model)
        {
            g_engfuncs.pfnPrecacheModel((char*)STRING(pev->model));
            g_engfuncs.pfnSetModel(edict(), STRING(pev->model));

            pev->sequence = LookupSequence("on_ground");

            if (pev->sequence != -1)
            {
                ResetSequenceInfo();
                pev->frame = 0;
            }
        }

        SetThink(&CTFGoalBase::BaseThink);
        pev->nextthink = gpGlobals->time + 0.1;
    }
}

void CTFGoalBase::TurnOnLight(CBasePlayer* pPlayer)
{
    Vector vecLightPos, vecLightAng;
    GetAttachment(0, vecLightPos, vecLightAng);
    g_engfuncs.pfnMessageBegin(MSG_ONE, SVC_TEMPENTITY, 0, pPlayer->edict());
    g_engfuncs.pfnWriteByte(TE_ELIGHT);

    g_engfuncs.pfnWriteShort(entindex() + 0x1000);
    g_engfuncs.pfnWriteCoord(vecLightPos.x);
    g_engfuncs.pfnWriteCoord(vecLightPos.y);
    g_engfuncs.pfnWriteCoord(vecLightPos.z);
    g_engfuncs.pfnWriteCoord(128);

    if (m_iGoalNum == 1)
    {
        g_engfuncs.pfnWriteByte(255);
        g_engfuncs.pfnWriteByte(128);
        g_engfuncs.pfnWriteByte(128);
    }
    else
    {
        g_engfuncs.pfnWriteByte(128);
        g_engfuncs.pfnWriteByte(255);
        g_engfuncs.pfnWriteByte(128);
    }
    g_engfuncs.pfnWriteByte(255);
    g_engfuncs.pfnWriteCoord(0);
    g_engfuncs.pfnMessageEnd();
}
