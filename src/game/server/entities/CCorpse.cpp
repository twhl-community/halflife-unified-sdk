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

#include "cbase.h"
#include "CCorpse.h"

LINK_ENTITY_TO_CLASS(bodyque, CCorpse);

void InitBodyQue()
{
	const std::string_view className{"bodyque"sv};

	g_pBodyQueueHead = g_EntityDictionary->Create(className);
	auto body = g_pBodyQueueHead;

	// Reserve 3 more slots for dead bodies
	for (int i = 0; i < 3; i++)
	{
		body->SetOwner(g_EntityDictionary->Create(className));
		body = body->GetOwner();
	}

	body->SetOwner(g_pBodyQueueHead);
}

void CopyToBodyQue(CBaseEntity* entity)
{
	if ((entity->pev->effects & EF_NODRAW) != 0)
		return;

	auto body = g_pBodyQueueHead;

	body->pev->angles = entity->pev->angles;
	body->pev->model = entity->pev->model;
	body->pev->modelindex = entity->pev->modelindex;
	body->pev->frame = entity->pev->frame;
	body->pev->colormap = entity->pev->colormap;
	body->pev->movetype = MOVETYPE_TOSS;
	body->pev->velocity = entity->pev->velocity;
	body->pev->flags = 0;
	body->pev->deadflag = entity->pev->deadflag;
	body->pev->renderfx = kRenderFxDeadPlayer;
	body->pev->renderamt = entity->entindex();

	body->pev->effects = entity->pev->effects | EF_NOINTERP;
	// body->pev->goalstarttime = entity->pev->goalstarttime;
	// body->pev->goalframe	= entity->pev->goalframe;
	// body->pev->goalendtime = entity->pev->goalendtime;

	body->pev->sequence = entity->pev->sequence;
	body->pev->animtime = entity->pev->animtime;

	body->SetOrigin(entity->pev->origin);
	body->SetSize(entity->pev->mins, entity->pev->maxs);
	g_pBodyQueueHead = body->GetOwner();
}
