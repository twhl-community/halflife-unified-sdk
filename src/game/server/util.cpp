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
/*

===== util.cpp ========================================================

  Utility code.  Really not optional after all.

*/

#include <bit>
#include <limits>

#include "cbase.h"
#include "shake.h"
#include "UserMessages.h"

float UTIL_WeaponTimeBase()
{
#if defined(CLIENT_WEAPONS)
	return 0.0;
#else
	return gpGlobals->time;
#endif
}

CBaseEntity* UTIL_FindEntityForward(CBaseEntity* pMe)
{
	TraceResult tr;

	UTIL_MakeVectors(pMe->pev->v_angle);
	UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs, pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters, pMe->edict(), &tr);
	if (tr.flFraction != 1.0 && !FNullEnt(tr.pHit))
	{
		CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
		return pHit;
	}
	return nullptr;
}

void UTIL_ParametricRocket(entvars_t* pev, Vector vecOrigin, Vector vecAngles, edict_t* owner)
{
	pev->startpos = vecOrigin;
	// Trace out line to end pos
	TraceResult tr;
	UTIL_MakeVectors(vecAngles);
	UTIL_TraceLine(pev->startpos, pev->startpos + gpGlobals->v_forward * 8192, ignore_monsters, owner, &tr);
	pev->endpos = tr.vecEndPos;

	// Now compute how long it will take based on current velocity
	Vector vecTravel = pev->endpos - pev->startpos;
	float travelTime = 0.0;
	if (pev->velocity.Length() > 0)
	{
		travelTime = vecTravel.Length() / pev->velocity.Length();
	}
	pev->starttime = gpGlobals->time;
	pev->impacttime = gpGlobals->time + travelTime;
}

// Normal overrides
void UTIL_SetGroupTrace(int groupmask, int op)
{
	g_groupmask = groupmask;
	g_groupop = op;

	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

void UTIL_UnsetGroupTrace()
{
	g_groupmask = 0;
	g_groupop = 0;

	ENGINE_SETGROUPMASK(0, 0);
}

// Smart version, it'll clean itself up when it pops off stack
UTIL_GroupTrace::UTIL_GroupTrace(int groupmask, int op)
{
	m_oldgroupmask = g_groupmask;
	m_oldgroupop = g_groupop;

	g_groupmask = groupmask;
	g_groupop = op;

	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

UTIL_GroupTrace::~UTIL_GroupTrace()
{
	g_groupmask = m_oldgroupmask;
	g_groupop = m_oldgroupop;

	ENGINE_SETGROUPMASK(g_groupmask, g_groupop);
}

TYPEDESCRIPTION gEntvarsDescription[] =
	{
		DEFINE_ENTITY_FIELD(classname, FIELD_STRING),
		DEFINE_ENTITY_GLOBAL_FIELD(globalname, FIELD_STRING),

		DEFINE_ENTITY_FIELD(origin, FIELD_POSITION_VECTOR),
		DEFINE_ENTITY_FIELD(oldorigin, FIELD_POSITION_VECTOR),
		DEFINE_ENTITY_FIELD(velocity, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(basevelocity, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(movedir, FIELD_VECTOR),

		DEFINE_ENTITY_FIELD(angles, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(avelocity, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(punchangle, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(v_angle, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(fixangle, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(idealpitch, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(pitch_speed, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(ideal_yaw, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(yaw_speed, FIELD_FLOAT),

		DEFINE_ENTITY_FIELD(modelindex, FIELD_INTEGER),
		DEFINE_ENTITY_GLOBAL_FIELD(model, FIELD_MODELNAME),

		DEFINE_ENTITY_FIELD(viewmodel, FIELD_MODELNAME),
		DEFINE_ENTITY_FIELD(weaponmodel, FIELD_MODELNAME),

		DEFINE_ENTITY_FIELD(absmin, FIELD_POSITION_VECTOR),
		DEFINE_ENTITY_FIELD(absmax, FIELD_POSITION_VECTOR),
		DEFINE_ENTITY_GLOBAL_FIELD(mins, FIELD_VECTOR),
		DEFINE_ENTITY_GLOBAL_FIELD(maxs, FIELD_VECTOR),
		DEFINE_ENTITY_GLOBAL_FIELD(size, FIELD_VECTOR),

		DEFINE_ENTITY_FIELD(ltime, FIELD_TIME),
		DEFINE_ENTITY_FIELD(nextthink, FIELD_TIME),

		DEFINE_ENTITY_FIELD(solid, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(movetype, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(skin, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(body, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(effects, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(gravity, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(friction, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(light_level, FIELD_FLOAT),

		DEFINE_ENTITY_FIELD(frame, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(scale, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(sequence, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(animtime, FIELD_TIME),
		DEFINE_ENTITY_FIELD(framerate, FIELD_FLOAT),
		DEFINE_ENTITY_ARRAY(controller, FIELD_CHARACTER, NUM_ENT_CONTROLLERS),
		DEFINE_ENTITY_ARRAY(blending, FIELD_CHARACTER, NUM_ENT_BLENDERS),

		DEFINE_ENTITY_FIELD(rendermode, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(renderamt, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(rendercolor, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(renderfx, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(health, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(frags, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(weapons, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(takedamage, FIELD_FLOAT),

		DEFINE_ENTITY_FIELD(deadflag, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(view_ofs, FIELD_VECTOR),
		DEFINE_ENTITY_FIELD(button, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(impulse, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(chain, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(dmg_inflictor, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(enemy, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(aiment, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(owner, FIELD_EDICT),
		DEFINE_ENTITY_FIELD(groundentity, FIELD_EDICT),

		DEFINE_ENTITY_FIELD(spawnflags, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(flags, FIELD_FLOAT),

		DEFINE_ENTITY_FIELD(colormap, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(team, FIELD_INTEGER),

		DEFINE_ENTITY_FIELD(max_health, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(teleport_time, FIELD_TIME),
		DEFINE_ENTITY_FIELD(armortype, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(armorvalue, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(waterlevel, FIELD_INTEGER),
		DEFINE_ENTITY_FIELD(watertype, FIELD_INTEGER),

		// Having these fields be local to the individual levels makes it easier to test those levels individually.
		DEFINE_ENTITY_GLOBAL_FIELD(target, FIELD_STRING),
		DEFINE_ENTITY_GLOBAL_FIELD(targetname, FIELD_STRING),
		DEFINE_ENTITY_FIELD(netname, FIELD_STRING),
		DEFINE_ENTITY_FIELD(message, FIELD_STRING),

		DEFINE_ENTITY_FIELD(dmg_take, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(dmg_save, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(dmg, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(dmgtime, FIELD_TIME),

		DEFINE_ENTITY_FIELD(noise, FIELD_SOUNDNAME),
		DEFINE_ENTITY_FIELD(noise1, FIELD_SOUNDNAME),
		DEFINE_ENTITY_FIELD(noise2, FIELD_SOUNDNAME),
		DEFINE_ENTITY_FIELD(noise3, FIELD_SOUNDNAME),
		DEFINE_ENTITY_FIELD(speed, FIELD_FLOAT),
		DEFINE_ENTITY_FIELD(air_finished, FIELD_TIME),
		DEFINE_ENTITY_FIELD(pain_finished, FIELD_TIME),
		DEFINE_ENTITY_FIELD(radsuit_finished, FIELD_TIME),
};

edict_t* UTIL_GetEntityList()
{
	return g_engfuncs.pfnPEntityOfEntOffset(0);
}

CBasePlayer* UTIL_GetLocalPlayer()
{
	return static_cast<CBasePlayer*>(UTIL_PlayerByIndex(1));
}

// ripped this out of the engine
float UTIL_AngleMod(float a)
{
	if (a < 0)
	{
		a = a + 360 * ((int)(a / 360) + 1);
	}
	else if (a >= 360)
	{
		a = a - 360 * ((int)(a / 360));
	}
	// a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}

float UTIL_AngleDiff(float destAngle, float srcAngle)
{
	float delta;

	delta = destAngle - srcAngle;
	if (destAngle > srcAngle)
	{
		if (delta >= 180)
			delta -= 360;
	}
	else
	{
		if (delta <= -180)
			delta += 360;
	}
	return delta;
}

Vector UTIL_VecToAngles(const Vector& vec)
{
	Vector out;
	VEC_TO_ANGLES(vec, out);
	return out;
}

//	float UTIL_MoveToOrigin( edict_t *pent, const Vector vecGoal, float flDist, int iMoveType )
void UTIL_MoveToOrigin(edict_t* pent, const Vector& vecGoal, float flDist, int iMoveType)
{
	MOVE_TO_ORIGIN(pent, vecGoal, flDist, iMoveType);
}


int UTIL_EntitiesInBox(CBaseEntity** pList, int listMax, const Vector& mins, const Vector& maxs, int flagMask)
{
	edict_t* pEdict = UTIL_GetEntityList();
	CBaseEntity* pEntity;
	int count;

	count = 0;

	if (!pEdict)
		return count;

	// Ignore world.
	++pEdict;

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (0 != pEdict->free) // Not in use
			continue;

		if (0 != flagMask && (pEdict->v.flags & flagMask) == 0) // Does it meet the criteria?
			continue;

		if (mins.x > pEdict->v.absmax.x ||
			mins.y > pEdict->v.absmax.y ||
			mins.z > pEdict->v.absmax.z ||
			maxs.x < pEdict->v.absmin.x ||
			maxs.y < pEdict->v.absmin.y ||
			maxs.z < pEdict->v.absmin.z)
			continue;

		pEntity = CBaseEntity::Instance(pEdict);
		if (!pEntity)
			continue;

		pList[count] = pEntity;
		count++;

		if (count >= listMax)
			return count;
	}

	return count;
}


int UTIL_MonstersInSphere(CBaseEntity** pList, int listMax, const Vector& center, float radius)
{
	edict_t* pEdict = UTIL_GetEntityList();
	CBaseEntity* pEntity;
	int count;
	float distance, delta;

	count = 0;
	float radiusSquared = radius * radius;

	if (!pEdict)
		return count;

	// Ignore world.
	++pEdict;

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (0 != pEdict->free) // Not in use
			continue;

		if ((pEdict->v.flags & (FL_CLIENT | FL_MONSTER)) == 0) // Not a client/monster ?
			continue;

		// Use origin for X & Y since they are centered for all monsters
		// Now X
		delta = center.x - pEdict->v.origin.x; //(pEdict->v.absmin.x + pEdict->v.absmax.x)*0.5;
		delta *= delta;

		if (delta > radiusSquared)
			continue;
		distance = delta;

		// Now Y
		delta = center.y - pEdict->v.origin.y; //(pEdict->v.absmin.y + pEdict->v.absmax.y)*0.5;
		delta *= delta;

		distance += delta;
		if (distance > radiusSquared)
			continue;

		// Now Z
		delta = center.z - (pEdict->v.absmin.z + pEdict->v.absmax.z) * 0.5;
		delta *= delta;

		distance += delta;
		if (distance > radiusSquared)
			continue;

		pEntity = CBaseEntity::Instance(pEdict);
		if (!pEntity)
			continue;

		pList[count] = pEntity;
		count++;

		if (count >= listMax)
			return count;
	}


	return count;
}

CBaseEntity* UTIL_FindEntityInSphere(CBaseEntity* pStartEntity, const Vector& vecCenter, float flRadius)
{
	edict_t* pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity->edict();
	else
		pentEntity = nullptr;

	pentEntity = FIND_ENTITY_IN_SPHERE(pentEntity, vecCenter, flRadius);

	if (!FNullEnt(pentEntity))
		return CBaseEntity::Instance(pentEntity);
	return nullptr;
}


CBaseEntity* UTIL_FindEntityByString(CBaseEntity* pStartEntity, const char* szKeyword, const char* szValue)
{
	edict_t* pentEntity;

	if (pStartEntity)
		pentEntity = pStartEntity->edict();
	else
		pentEntity = nullptr;

	pentEntity = g_engfuncs.pfnFindEntityByString(pentEntity, szKeyword, szValue);

	if (!FNullEnt(pentEntity))
		return CBaseEntity::Instance(pentEntity);
	return nullptr;
}

CBaseEntity* UTIL_FindEntityByClassname(CBaseEntity* pStartEntity, const char* szName)
{
	return UTIL_FindEntityByString(pStartEntity, "classname", szName);
}

CBaseEntity* UTIL_FindEntityByTargetname(CBaseEntity* pStartEntity, const char* szName)
{
	return UTIL_FindEntityByString(pStartEntity, "targetname", szName);
}

CBaseEntity* UTIL_FindEntityByTarget(CBaseEntity* pStartEntity, const char* szName)
{
	return UTIL_FindEntityByString(pStartEntity, "target", szName);
}

CBaseEntity* UTIL_FindEntityGeneric(const char* szWhatever, Vector& vecSrc, float flRadius)
{
	CBaseEntity* pEntity = nullptr;

	pEntity = UTIL_FindEntityByTargetname(nullptr, szWhatever);
	if (pEntity)
		return pEntity;

	CBaseEntity* pSearch = nullptr;
	float flMaxDist2 = flRadius * flRadius;
	while ((pSearch = UTIL_FindEntityByClassname(pSearch, szWhatever)) != nullptr)
	{
		float flDist2 = (pSearch->pev->origin - vecSrc).Length();
		flDist2 = flDist2 * flDist2;
		if (flMaxDist2 > flDist2)
		{
			pEntity = pSearch;
			flMaxDist2 = flDist2;
		}
	}
	return pEntity;
}

// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns nullptr
// Index is 1 based
CBasePlayer* UTIL_PlayerByIndex(int playerIndex)
{
	CBasePlayer* pPlayer = nullptr;

	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		edict_t* pPlayerEdict = INDEXENT(playerIndex);
		if (pPlayerEdict && 0 == pPlayerEdict->free)
		{
			pPlayer = static_cast<CBasePlayer*>(CBaseEntity::Instance(pPlayerEdict));
		}
	}

	return pPlayer;
}

CBasePlayer* UTIL_FindNearestPlayer(const Vector& origin)
{
	CBasePlayer* player = nullptr;

	float flMaxDist2 = std::numeric_limits<float>::max();

	for (auto search : UTIL_FindPlayers())
	{
		float flDist2 = (search->pev->origin - origin).Length();
		flDist2 = flDist2 * flDist2;

		if (flDist2 < flMaxDist2)
		{
			player = search;
			flMaxDist2 = flDist2;
		}
	}

	return player;
}

void UTIL_MakeVectors(const Vector& vecAngles)
{
	MAKE_VECTORS(vecAngles);
}


void UTIL_MakeAimVectors(const Vector& vecAngles)
{
	Vector rgflVec = vecAngles;
	rgflVec[0] = -rgflVec[0];
	MAKE_VECTORS(rgflVec);
}

void UTIL_MakeInvVectors(const Vector& vec, globalvars_t* pgv)
{
	MAKE_VECTORS(vec);

	pgv->v_right = pgv->v_right * -1;

	std::swap(pgv->v_forward.y, pgv->v_right.x);
	std::swap(pgv->v_forward.z, pgv->v_up.x);
	std::swap(pgv->v_right.z, pgv->v_up.y);
}

static unsigned short FixedUnsigned16(float value, float scale)
{
	int output;

	output = value * scale;
	if (output < 0)
		output = 0;
	if (output > 0xFFFF)
		output = 0xFFFF;

	return (unsigned short)output;
}

static short FixedSigned16(float value, float scale)
{
	int output;

	output = value * scale;

	if (output > 32767)
		output = 32767;

	if (output < -32768)
		output = -32768;

	return (short)output;
}

// Shake the screen of all clients within radius
// radius == 0, shake all clients
// UNDONE: Allow caller to shake clients not ONGROUND?
// UNDONE: Fix falloff model (disabled)?
// UNDONE: Affect user controls?
void UTIL_ScreenShake(const Vector& center, float amplitude, float frequency, float duration, float radius)
{
	int i;
	float localAmplitude;
	ScreenShake shake;

	shake.duration = FixedUnsigned16(duration, 1 << 12);  // 4.12 fixed
	shake.frequency = FixedUnsigned16(frequency, 1 << 8); // 8.8 fixed

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

		if (!pPlayer || (pPlayer->pev->flags & FL_ONGROUND) == 0) // Don't shake if not onground
			continue;

		localAmplitude = 0;

		if (radius <= 0)
			localAmplitude = amplitude;
		else
		{
			Vector delta = center - pPlayer->pev->origin;
			float distance = delta.Length();

			// Had to get rid of this falloff - it didn't work well
			if (distance < radius)
				localAmplitude = amplitude; // radius - distance;
		}
		if (0 != localAmplitude)
		{
			shake.amplitude = FixedUnsigned16(localAmplitude, 1 << 12); // 4.12 fixed

			MESSAGE_BEGIN(MSG_ONE, gmsgShake, nullptr, pPlayer->edict()); // use the magic #1 for "one client"

			WRITE_SHORT(shake.amplitude); // shake amount
			WRITE_SHORT(shake.duration);  // shake lasts this long
			WRITE_SHORT(shake.frequency); // shake noise frequency

			MESSAGE_END();
		}
	}
}



void UTIL_ScreenShakeAll(const Vector& center, float amplitude, float frequency, float duration)
{
	UTIL_ScreenShake(center, amplitude, frequency, duration, 0);
}


void UTIL_ScreenFadeBuild(ScreenFade& fade, const Vector& color, float fadeTime, float fadeHold, int alpha, int flags)
{
	fade.duration = FixedUnsigned16(fadeTime, 1 << 12); // 4.12 fixed
	fade.holdTime = FixedUnsigned16(fadeHold, 1 << 12); // 4.12 fixed
	fade.r = (int)color.x;
	fade.g = (int)color.y;
	fade.b = (int)color.z;
	fade.a = alpha;
	fade.fadeFlags = flags;
}


void UTIL_ScreenFadeWrite(const ScreenFade& fade, CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgFade, nullptr, pEntity->edict()); // use the magic #1 for "one client"

	WRITE_SHORT(fade.duration);	 // fade lasts this long
	WRITE_SHORT(fade.holdTime);	 // fade lasts this long
	WRITE_SHORT(fade.fadeFlags); // fade type (in / out)
	WRITE_BYTE(fade.r);			 // fade red
	WRITE_BYTE(fade.g);			 // fade green
	WRITE_BYTE(fade.b);			 // fade blue
	WRITE_BYTE(fade.a);			 // fade blue

	MESSAGE_END();
}


void UTIL_ScreenFadeAll(const Vector& color, float fadeTime, float fadeHold, int alpha, int flags)
{
	int i;
	ScreenFade fade;


	UTIL_ScreenFadeBuild(fade, color, fadeTime, fadeHold, alpha, flags);

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

		UTIL_ScreenFadeWrite(fade, pPlayer);
	}
}


void UTIL_ScreenFade(CBaseEntity* pEntity, const Vector& color, float fadeTime, float fadeHold, int alpha, int flags)
{
	ScreenFade fade;

	UTIL_ScreenFadeBuild(fade, color, fadeTime, fadeHold, alpha, flags);
	UTIL_ScreenFadeWrite(fade, pEntity);
}


void UTIL_HudMessage(CBaseEntity* pEntity, const hudtextparms_t& textparms, const char* pMessage)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, pEntity->edict());
	WRITE_BYTE(TE_TEXTMESSAGE);
	WRITE_BYTE(textparms.channel & 0xFF);

	WRITE_SHORT(FixedSigned16(textparms.x, 1 << 13));
	WRITE_SHORT(FixedSigned16(textparms.y, 1 << 13));
	WRITE_BYTE(textparms.effect);

	WRITE_BYTE(textparms.r1);
	WRITE_BYTE(textparms.g1);
	WRITE_BYTE(textparms.b1);
	WRITE_BYTE(textparms.a1);

	WRITE_BYTE(textparms.r2);
	WRITE_BYTE(textparms.g2);
	WRITE_BYTE(textparms.b2);
	WRITE_BYTE(textparms.a2);

	WRITE_SHORT(FixedUnsigned16(textparms.fadeinTime, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(textparms.fadeoutTime, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(textparms.holdTime, 1 << 8));

	if (textparms.effect == 2)
		WRITE_SHORT(FixedUnsigned16(textparms.fxTime, 1 << 8));

	if (strlen(pMessage) < 512)
	{
		WRITE_STRING(pMessage);
	}
	else
	{
		char tmp[512];
		strncpy(tmp, pMessage, 511);
		tmp[511] = 0;
		WRITE_STRING(tmp);
	}
	MESSAGE_END();
}

void UTIL_HudMessageAll(const hudtextparms_t& textparms, const char* pMessage)
{
	int i;

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);
		if (pPlayer)
			UTIL_HudMessage(pPlayer, textparms, pMessage);
	}
}

void UTIL_ClientPrintAll(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4)
{
	MESSAGE_BEGIN(MSG_ALL, gmsgTextMsg);
	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);

	if (param1)
		WRITE_STRING(param1);
	if (param2)
		WRITE_STRING(param2);
	if (param3)
		WRITE_STRING(param3);
	if (param4)
		WRITE_STRING(param4);

	MESSAGE_END();
}

void ClientPrint(entvars_t* client, int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgTextMsg, nullptr, client);
	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);

	if (param1)
		WRITE_STRING(param1);
	if (param2)
		WRITE_STRING(param2);
	if (param3)
		WRITE_STRING(param3);
	if (param4)
		WRITE_STRING(param4);

	MESSAGE_END();
}

void UTIL_SayText(const char* pText, CBaseEntity* pEntity)
{
	if (!pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, pEntity->edict());
	WRITE_BYTE(pEntity->entindex());
	WRITE_STRING(pText);
	MESSAGE_END();
}

void UTIL_SayTextAll(const char* pText, CBaseEntity* pEntity)
{
	MESSAGE_BEGIN(MSG_ALL, gmsgSayText, nullptr);
	WRITE_BYTE(pEntity->entindex());
	WRITE_STRING(pText);
	MESSAGE_END();
}

void UTIL_ShowMessage(const char* pString, CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->IsNetClient())
		return;

	MESSAGE_BEGIN(MSG_ONE, gmsgHudText, nullptr, pEntity->edict());
	WRITE_STRING(pString);
	MESSAGE_END();
}


void UTIL_ShowMessageAll(const char* pString)
{
	int i;

	// loop through all players

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);
		if (pPlayer)
			UTIL_ShowMessage(pString, pPlayer);
	}
}

// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t* pentIgnore, TraceResult* ptr)
{
	// TODO: define constants
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0) | (ignore_glass == ignoreGlass ? 0x100 : 0), pentIgnore, ptr);
}


void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, edict_t* pentIgnore, TraceResult* ptr)
{
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0), pentIgnore, ptr);
}


void UTIL_TraceHull(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t* pentIgnore, TraceResult* ptr)
{
	TRACE_HULL(vecStart, vecEnd, (igmon == ignore_monsters ? 1 : 0), hullNumber, pentIgnore, ptr);
}

void UTIL_TraceModel(const Vector& vecStart, const Vector& vecEnd, int hullNumber, edict_t* pentModel, TraceResult* ptr)
{
	g_engfuncs.pfnTraceModel(vecStart, vecEnd, hullNumber, pentModel, ptr);
}


TraceResult UTIL_GetGlobalTrace()
{
	TraceResult tr;

	tr.fAllSolid = gpGlobals->trace_allsolid;
	tr.fStartSolid = gpGlobals->trace_startsolid;
	tr.fInOpen = gpGlobals->trace_inopen;
	tr.fInWater = gpGlobals->trace_inwater;
	tr.flFraction = gpGlobals->trace_fraction;
	tr.flPlaneDist = gpGlobals->trace_plane_dist;
	tr.pHit = gpGlobals->trace_ent;
	tr.vecEndPos = gpGlobals->trace_endpos;
	tr.vecPlaneNormal = gpGlobals->trace_plane_normal;
	tr.iHitgroup = gpGlobals->trace_hitgroup;
	return tr;
}

float UTIL_VecToYaw(const Vector& vec)
{
	return VEC_TO_YAW(vec);
}


void UTIL_SetOrigin(entvars_t* pev, const Vector& vecOrigin)
{
	edict_t* ent = ENT(pev);
	if (ent)
		SET_ORIGIN(ent, vecOrigin);
}

void UTIL_ParticleEffect(const Vector& vecOrigin, const Vector& vecDirection, unsigned int ulColor, unsigned int ulCount)
{
	PARTICLE_EFFECT(vecOrigin, vecDirection, (float)ulColor, (float)ulCount);
}


float UTIL_Approach(float target, float value, float speed)
{
	float delta = target - value;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}


float UTIL_ApproachAngle(float target, float value, float speed)
{
	target = UTIL_AngleMod(target);
	value = UTIL_AngleMod(target);

	float delta = target - value;

	// Speed is assumed to be positive
	if (speed < 0)
		speed = -speed;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	if (delta > speed)
		value += speed;
	else if (delta < -speed)
		value -= speed;
	else
		value = target;

	return value;
}


float UTIL_AngleDistance(float next, float cur)
{
	float delta = next - cur;

	if (delta < -180)
		delta += 360;
	else if (delta > 180)
		delta -= 360;

	return delta;
}


float UTIL_SplineFraction(float value, float scale)
{
	value = scale * value;
	float valueSquared = value * value;

	// Nice little ease-in, ease-out spline-like curve
	return 3 * valueSquared - 2 * valueSquared * value;
}


char* UTIL_VarArgs(const char* format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

Vector UTIL_GetAimVector(edict_t* pent, float flSpeed)
{
	Vector tmp;
	GET_AIM_VECTOR(pent, flSpeed, tmp);
	return tmp;
}

bool UTIL_IsMasterTriggered(string_t sMaster, CBaseEntity* pActivator)
{
	if (!FStringNull(sMaster))
	{
		auto master = UTIL_FindEntityByTargetname(nullptr, STRING(sMaster));

		if (!FNullEnt(master))
		{
			if ((master->ObjectCaps() & FCAP_MASTER) != 0)
				return master->IsTriggered(pActivator);
		}

		CBaseEntity::IOLogger->debug("Master was null or not a master!");
	}

	// if this isn't a master entity, just say yes.
	return true;
}

bool UTIL_ShouldShowBlood(int color)
{
	if (color != DONT_BLEED)
	{
		if (color == BLOOD_COLOR_RED)
		{
			if (CVAR_GET_FLOAT("violence_hblood") != 0)
				return true;
		}
		else
		{
			if (CVAR_GET_FLOAT("violence_ablood") != 0)
				return true;
		}
	}
	return false;
}

int UTIL_PointContents(const Vector& vec)
{
	return POINT_CONTENTS(vec);
}

void UTIL_BloodStream(const Vector& origin, const Vector& direction, int color, int amount)
{
	if (!UTIL_ShouldShowBlood(color))
		return;

	if (g_Language == LANGUAGE_GERMAN && color == BLOOD_COLOR_RED)
		color = 0;


	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
	WRITE_BYTE(TE_BLOODSTREAM);
	WRITE_COORD(origin.x);
	WRITE_COORD(origin.y);
	WRITE_COORD(origin.z);
	WRITE_COORD(direction.x);
	WRITE_COORD(direction.y);
	WRITE_COORD(direction.z);
	WRITE_BYTE(color);
	WRITE_BYTE(V_min(amount, 255));
	MESSAGE_END();
}

void UTIL_BloodDrips(const Vector& origin, const Vector& direction, int color, int amount)
{
	if (!UTIL_ShouldShowBlood(color))
		return;

	if (color == DONT_BLEED || amount == 0)
		return;

	if (g_Language == LANGUAGE_GERMAN && color == BLOOD_COLOR_RED)
		color = 0;

	if (g_pGameRules->IsMultiplayer())
	{
		// scale up blood effect in multiplayer for better visibility
		amount *= 2;
	}

	if (amount > 255)
		amount = 255;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
	WRITE_BYTE(TE_BLOODSPRITE);
	WRITE_COORD(origin.x); // pos
	WRITE_COORD(origin.y);
	WRITE_COORD(origin.z);
	WRITE_SHORT(g_sModelIndexBloodSpray);		  // initial sprite model
	WRITE_SHORT(g_sModelIndexBloodDrop);		  // droplet sprite models
	WRITE_BYTE(color);							  // color index into host_basepal
	WRITE_BYTE(V_min(V_max(3, amount / 10), 16)); // size
	MESSAGE_END();
}

Vector UTIL_RandomBloodVector()
{
	Vector direction;

	direction.x = RANDOM_FLOAT(-1, 1);
	direction.y = RANDOM_FLOAT(-1, 1);
	direction.z = RANDOM_FLOAT(0, 1);

	return direction;
}


void UTIL_BloodDecalTrace(TraceResult* pTrace, int bloodColor)
{
	if (UTIL_ShouldShowBlood(bloodColor))
	{
		if (bloodColor == BLOOD_COLOR_RED)
			UTIL_DecalTrace(pTrace, DECAL_BLOOD1 + RANDOM_LONG(0, 5));
		else
			UTIL_DecalTrace(pTrace, DECAL_YBLOOD1 + RANDOM_LONG(0, 5));
	}
}


void UTIL_DecalTrace(TraceResult* pTrace, int decalNumber)
{
	short entityIndex;
	int index;
	int message;

	if (decalNumber < 0)
		return;

	index = gDecals[decalNumber].index;

	if (index < 0)
		return;

	if (pTrace->flFraction == 1.0)
		return;

	// Only decal BSP models
	if (pTrace->pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(pTrace->pHit);
		if (pEntity && !pEntity->IsBSPModel())
			return;
		entityIndex = ENTINDEX(pTrace->pHit);
	}
	else
		entityIndex = 0;

	message = TE_DECAL;
	if (entityIndex != 0)
	{
		if (index > 255)
		{
			message = TE_DECALHIGH;
			index -= 256;
		}
	}
	else
	{
		message = TE_WORLDDECAL;
		if (index > 255)
		{
			message = TE_WORLDDECALHIGH;
			index -= 256;
		}
	}

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(message);
	WRITE_COORD(pTrace->vecEndPos.x);
	WRITE_COORD(pTrace->vecEndPos.y);
	WRITE_COORD(pTrace->vecEndPos.z);
	WRITE_BYTE(index);
	if (0 != entityIndex)
		WRITE_SHORT(entityIndex);
	MESSAGE_END();
}

/*
==============
UTIL_PlayerDecalTrace

A player is trying to apply his custom decal for the spray can.
Tell connected clients to display it, or use the default spray can decal
if the custom can't be loaded.
==============
*/
void UTIL_PlayerDecalTrace(TraceResult* pTrace, int playernum, int decalNumber, bool bIsCustom)
{
	int index;

	if (!bIsCustom)
	{
		if (decalNumber < 0)
			return;

		index = gDecals[decalNumber].index;
		if (index < 0)
			return;
	}
	else
		index = decalNumber;

	if (pTrace->flFraction == 1.0)
		return;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_PLAYERDECAL);
	WRITE_BYTE(playernum);
	WRITE_COORD(pTrace->vecEndPos.x);
	WRITE_COORD(pTrace->vecEndPos.y);
	WRITE_COORD(pTrace->vecEndPos.z);
	WRITE_SHORT((short)ENTINDEX(pTrace->pHit));
	WRITE_BYTE(index);
	MESSAGE_END();
}

void UTIL_GunshotDecalTrace(TraceResult* pTrace, int decalNumber)
{
	if (decalNumber < 0)
		return;

	int index = gDecals[decalNumber].index;
	if (index < 0)
		return;

	if (pTrace->flFraction == 1.0)
		return;

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pTrace->vecEndPos);
	WRITE_BYTE(TE_GUNSHOTDECAL);
	WRITE_COORD(pTrace->vecEndPos.x);
	WRITE_COORD(pTrace->vecEndPos.y);
	WRITE_COORD(pTrace->vecEndPos.z);
	WRITE_SHORT((short)ENTINDEX(pTrace->pHit));
	WRITE_BYTE(index);
	MESSAGE_END();
}


void UTIL_Sparks(const Vector& position)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, position);
	WRITE_BYTE(TE_SPARKS);
	WRITE_COORD(position.x);
	WRITE_COORD(position.y);
	WRITE_COORD(position.z);
	MESSAGE_END();
}


void UTIL_Ricochet(const Vector& position, float scale)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, position);
	WRITE_BYTE(TE_ARMOR_RICOCHET);
	WRITE_COORD(position.x);
	WRITE_COORD(position.y);
	WRITE_COORD(position.z);
	WRITE_BYTE((int)(scale * 10));
	MESSAGE_END();
}


bool UTIL_TeamsMatch(const char* pTeamName1, const char* pTeamName2)
{
	// Everyone matches unless it's teamplay
	if (!g_pGameRules->IsTeamplay())
		return true;

	// Both on a team?
	if (*pTeamName1 != 0 && *pTeamName2 != 0)
	{
		if (!stricmp(pTeamName1, pTeamName2)) // Same Team?
			return true;
	}

	return false;
}

void UTIL_StringToIntArray(int* pVector, int count, const char* pString)
{
	char *pstr, *pfront, tempString[128];
	int j;

	strcpy(tempString, pString);
	pstr = pfront = tempString;

	for (j = 0; j < count; j++) // lifted from pr_edict.c
	{
		pVector[j] = atoi(pfront);

		while ('\0' != *pstr && *pstr != ' ')
			pstr++;
		if ('\0' == *pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	for (j++; j < count; j++)
	{
		pVector[j] = 0;
	}
}

Vector UTIL_ClampVectorToBox(const Vector& input, const Vector& clampSize)
{
	Vector sourceVector = input;

	if (sourceVector.x > clampSize.x)
		sourceVector.x -= clampSize.x;
	else if (sourceVector.x < -clampSize.x)
		sourceVector.x += clampSize.x;
	else
		sourceVector.x = 0;

	if (sourceVector.y > clampSize.y)
		sourceVector.y -= clampSize.y;
	else if (sourceVector.y < -clampSize.y)
		sourceVector.y += clampSize.y;
	else
		sourceVector.y = 0;

	if (sourceVector.z > clampSize.z)
		sourceVector.z -= clampSize.z;
	else if (sourceVector.z < -clampSize.z)
		sourceVector.z += clampSize.z;
	else
		sourceVector.z = 0;

	return sourceVector.Normalize();
}


float UTIL_WaterLevel(const Vector& position, float minz, float maxz)
{
	Vector midUp = position;
	midUp.z = minz;

	if (UTIL_PointContents(midUp) != CONTENTS_WATER)
		return minz;

	midUp.z = maxz;
	if (UTIL_PointContents(midUp) == CONTENTS_WATER)
		return maxz;

	float diff = maxz - minz;
	while (diff > 1.0)
	{
		midUp.z = minz + diff / 2.0;
		if (UTIL_PointContents(midUp) == CONTENTS_WATER)
		{
			minz = midUp.z;
		}
		else
		{
			maxz = midUp.z;
		}
		diff = maxz - minz;
	}

	return midUp.z;
}

void UTIL_Bubbles(Vector mins, Vector maxs, int count)
{
	Vector mid = (mins + maxs) * 0.5;

	float flHeight = UTIL_WaterLevel(mid, mid.z, mid.z + 1024);
	flHeight = flHeight - mins.z;

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, mid);
	WRITE_BYTE(TE_BUBBLES);
	WRITE_COORD(mins.x); // mins
	WRITE_COORD(mins.y);
	WRITE_COORD(mins.z);
	WRITE_COORD(maxs.x); // maxz
	WRITE_COORD(maxs.y);
	WRITE_COORD(maxs.z);
	WRITE_COORD(flHeight); // height
	WRITE_SHORT(g_sModelIndexBubbles);
	WRITE_BYTE(count); // count
	WRITE_COORD(8);	   // speed
	MESSAGE_END();
}

void UTIL_BubbleTrail(Vector from, Vector to, int count)
{
	float flHeight = UTIL_WaterLevel(from, from.z, from.z + 256);
	flHeight = flHeight - from.z;

	if (flHeight < 8)
	{
		flHeight = UTIL_WaterLevel(to, to.z, to.z + 256);
		flHeight = flHeight - to.z;
		if (flHeight < 8)
			return;

		// UNDONE: do a ploink sound
		flHeight = flHeight + to.z - from.z;
	}

	if (count > 255)
		count = 255;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BUBBLETRAIL);
	WRITE_COORD(from.x); // mins
	WRITE_COORD(from.y);
	WRITE_COORD(from.z);
	WRITE_COORD(to.x); // maxz
	WRITE_COORD(to.y);
	WRITE_COORD(to.z);
	WRITE_COORD(flHeight); // height
	WRITE_SHORT(g_sModelIndexBubbles);
	WRITE_BYTE(count); // count
	WRITE_COORD(8);	   // speed
	MESSAGE_END();
}


void UTIL_Remove(CBaseEntity* pEntity)
{
	if (!pEntity)
		return;

	pEntity->UpdateOnRemove();
	pEntity->pev->flags |= FL_KILLME;
	pEntity->pev->targetname = string_t::Null;
}


bool UTIL_IsValidEntity(edict_t* pent)
{
	if (!pent || 0 != pent->free || (pent->v.flags & FL_KILLME) != 0)
		return false;
	return true;
}


void UTIL_PrecacheOther(const char* szClassname)
{
	auto entity = g_EntityDictionary->Create(szClassname);
	if (FNullEnt(entity))
	{
		CBaseEntity::Logger->debug("nullptr Ent in UTIL_PrecacheOther");
		return;
	}

	entity->Precache();

	REMOVE_ENTITY(entity->edict());
}

//=========================================================
// UTIL_DotPoints - returns the dot product of a line from
// src to check and vecdir.
//=========================================================
float UTIL_DotPoints(const Vector& vecSrc, const Vector& vecCheck, const Vector& vecDir)
{
	Vector2D vec2LOS;

	vec2LOS = (vecCheck - vecSrc).Make2D();
	vec2LOS = vec2LOS.Normalize();

	return DotProduct(vec2LOS, (vecDir.Make2D()));
}


//=========================================================
// UTIL_StripToken - for redundant keynames
//=========================================================
void UTIL_StripToken(const char* pKey, char* pDest)
{
	int i = 0;

	while ('\0' != pKey[i] && pKey[i] != '#')
	{
		pDest[i] = pKey[i];
		i++;
	}
	pDest[i] = 0;
}


// --------------------------------------------------------------
//
// CSave
//
// --------------------------------------------------------------
static int gSizes[FIELD_TYPECOUNT] =
	{
		sizeof(float),	   // FIELD_FLOAT
		sizeof(int),	   // FIELD_STRING
		sizeof(int),	   // FIELD_ENTITY
		sizeof(int),	   // FIELD_CLASSPTR
		sizeof(int),	   // FIELD_EHANDLE
		sizeof(int),	   // FIELD_entvars_t
		sizeof(int),	   // FIELD_EDICT
		sizeof(float) * 3, // FIELD_VECTOR
		sizeof(float) * 3, // FIELD_POSITION_VECTOR
		sizeof(int*),	   // FIELD_POINTER
		sizeof(int),	   // FIELD_INTEGER
#ifdef GNUC
		sizeof(int*) * 2, // FIELD_FUNCTION
#else
		sizeof(int*), // FIELD_FUNCTION
#endif
		sizeof(byte),		   // FIELD_BOOLEAN
		sizeof(short),		   // FIELD_SHORT
		sizeof(char),		   // FIELD_CHARACTER
		sizeof(float),		   // FIELD_TIME
		sizeof(int),		   // FIELD_MODELNAME
		sizeof(int),		   // FIELD_SOUNDNAME
		sizeof(std::uint64_t), // FIELD_INT64
};


// Base class includes common SAVERESTOREDATA pointer, and manages the entity table
CSaveRestoreBuffer::CSaveRestoreBuffer(SAVERESTOREDATA& data)
	: m_data(data)
{
}

CSaveRestoreBuffer::~CSaveRestoreBuffer() = default;

int CSaveRestoreBuffer::EntityIndex(CBaseEntity* pEntity)
{
	if (pEntity == nullptr)
		return -1;
	return EntityIndex(pEntity->pev);
}


int CSaveRestoreBuffer::EntityIndex(entvars_t* pevLookup)
{
	if (pevLookup == nullptr)
		return -1;
	return EntityIndex(ENT(pevLookup));
}

int CSaveRestoreBuffer::EntityIndex(EOFFSET eoLookup)
{
	return EntityIndex(ENT(eoLookup));
}


int CSaveRestoreBuffer::EntityIndex(edict_t* pentLookup)
{
	if (pentLookup == nullptr)
		return -1;

	int i;
	ENTITYTABLE* pTable;

	for (i = 0; i < m_data.tableCount; i++)
	{
		pTable = m_data.pTable + i;
		if (pTable->pent == pentLookup)
			return i;
	}
	return -1;
}


edict_t* CSaveRestoreBuffer::EntityFromIndex(int entityIndex)
{
	if (entityIndex < 0)
		return nullptr;

	int i;
	ENTITYTABLE* pTable;

	for (i = 0; i < m_data.tableCount; i++)
	{
		pTable = m_data.pTable + i;
		if (pTable->id == entityIndex)
			return pTable->pent;
	}
	return nullptr;
}


int CSaveRestoreBuffer::EntityFlagsSet(int entityIndex, int flags)
{
	if (entityIndex < 0)
		return 0;
	if (entityIndex > m_data.tableCount)
		return 0;

	m_data.pTable[entityIndex].flags |= flags;

	return m_data.pTable[entityIndex].flags;
}


void CSaveRestoreBuffer::BufferRewind(int size)
{
	if (m_data.size < size)
		size = m_data.size;

	m_data.pCurrentData -= size;
	m_data.size -= size;
}

unsigned int CSaveRestoreBuffer::HashString(const char* pszToken)
{
	unsigned int hash = 0;

	while ('\0' != *pszToken)
		hash = std::rotr(hash, 4) ^ *pszToken++;

	return hash;
}

unsigned short CSaveRestoreBuffer::TokenHash(const char* pszToken)
{
#if _DEBUG
	static int tokensparsed = 0;
	tokensparsed++;
#endif
	if (0 == m_data.tokenCount || nullptr == m_data.pTokens)
	{
		// if we're here it means trigger_changelevel is trying to actually save something when it's not supposed to.
		Logger->error("No token table array in TokenHash()!");
		return 0;
	}

	const unsigned short hash = (unsigned short)(HashString(pszToken) % (unsigned)m_data.tokenCount);

	for (int i = 0; i < m_data.tokenCount; i++)
	{
#if _DEBUG
		static bool beentheredonethat = false;
		if (i > 50 && !beentheredonethat)
		{
			beentheredonethat = true;
			Logger->error("CSaveRestoreBuffer :: TokenHash() is getting too full!");
		}
#endif

		int index = hash + i;
		if (index >= m_data.tokenCount)
			index -= m_data.tokenCount;

		if (!m_data.pTokens[index] || strcmp(pszToken, m_data.pTokens[index]) == 0)
		{
			m_data.pTokens[index] = (char*)pszToken;
			return index;
		}
	}

	// Token hash table full!!!
	// [Consider doing overflow table(s) after the main table & limiting linear hash table search]
	Logger->error("CSaveRestoreBuffer :: TokenHash() is COMPLETELY FULL!");
	return 0;
}

void CSave::WriteData(const char* pname, int size, const char* pdata)
{
	BufferField(pname, size, pdata);
}


void CSave::WriteShort(const char* pname, const short* data, int count)
{
	BufferField(pname, sizeof(short) * count, (const char*)data);
}


void CSave::WriteInt(const char* pname, const int* data, int count)
{
	BufferField(pname, sizeof(int) * count, (const char*)data);
}


void CSave::WriteFloat(const char* pname, const float* data, int count)
{
	BufferField(pname, sizeof(float) * count, (const char*)data);
}


void CSave::WriteTime(const char* pname, const float* data, int count)
{
	int i;
	Vector tmp, input;

	BufferHeader(pname, sizeof(float) * count);
	for (i = 0; i < count; i++)
	{
		float tmp = data[0];

		// Always encode time as a delta from the current time so it can be re-based if loaded in a new level
		// Times of 0 are never written to the file, so they will be restored as 0, not a relative time
		tmp -= m_data.time;

		BufferData((const char*)&tmp, sizeof(float));
		data++;
	}
}


void CSave::WriteString(const char* pname, const char* pdata)
{
#ifdef TOKENIZE
	short token = (short)TokenHash(pdata);
	WriteShort(pname, &token, 1);
#else
	BufferField(pname, strlen(pdata) + 1, pdata);
#endif
}


void CSave::WriteString(const char* pname, const string_t* stringId, int count)
{
	int i, size;

#ifdef TOKENIZE
	short token = (short)TokenHash(STRING(*stringId));
	WriteShort(pname, &token, 1);
#else
#if 0
	if (count != 1)
		Logger->error("No string arrays!");
	WriteString(pname, STRING(*stringId));
#endif

	size = 0;
	for (i = 0; i < count; i++)
		size += strlen(STRING(stringId[i])) + 1;

	BufferHeader(pname, size);
	for (i = 0; i < count; i++)
	{
		const char* pString = STRING(stringId[i]);
		BufferData(pString, strlen(pString) + 1);
	}
#endif
}


void CSave::WriteVector(const char* pname, const Vector& value)
{
	WriteVector(pname, &value.x, 1);
}


void CSave::WriteVector(const char* pname, const float* value, int count)
{
	BufferHeader(pname, sizeof(float) * 3 * count);
	BufferData((const char*)value, sizeof(float) * 3 * count);
}



void CSave::WritePositionVector(const char* pname, const Vector& value)
{
	if (0 != m_data.fUseLandmark)
	{
		Vector tmp = value - m_data.vecLandmarkOffset;
		WriteVector(pname, tmp);
	}

	WriteVector(pname, value);
}


void CSave::WritePositionVector(const char* pname, const float* value, int count)
{
	int i;
	Vector tmp, input;

	BufferHeader(pname, sizeof(float) * 3 * count);
	for (i = 0; i < count; i++)
	{
		Vector tmp(value[0], value[1], value[2]);

		if (0 != m_data.fUseLandmark)
			tmp = tmp - m_data.vecLandmarkOffset;

		BufferData((const char*)&tmp.x, sizeof(float) * 3);
		value += 3;
	}
}


void CSave::WriteFunction(const char* pname, void** data, int count)
{
	const char* functionName;

	functionName = NAME_FOR_FUNCTION((uint32)*data);
	if (functionName)
		BufferField(pname, strlen(functionName) + 1, functionName);
	else
		Logger->error("Invalid function pointer in entity!");
}


void EntvarsKeyvalue(entvars_t* pev, KeyValueData* pkvd)
{
	TYPEDESCRIPTION* pField;

	for (std::size_t i = 0; i < std::size(gEntvarsDescription); ++i)
	{
		pField = &gEntvarsDescription[i];

		if (!stricmp(pField->fieldName, pkvd->szKeyName))
		{
			switch (pField->fieldType)
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				(*(string_t*)((char*)pev + pField->fieldOffset)) = ALLOC_STRING(pkvd->szValue);
				break;

			case FIELD_TIME:
			case FIELD_FLOAT:
				(*(float*)((char*)pev + pField->fieldOffset)) = atof(pkvd->szValue);
				break;

			case FIELD_INTEGER:
				(*(int*)((char*)pev + pField->fieldOffset)) = atoi(pkvd->szValue);
				break;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				UTIL_StringToVector((float*)((char*)pev + pField->fieldOffset), pkvd->szValue);
				break;

			default:
			case FIELD_EVARS:
			case FIELD_CLASSPTR:
			case FIELD_EDICT:
			case FIELD_ENTITY:
			case FIELD_POINTER:
				CBaseEntity::Logger->error("Bad field in entity!!");
				break;
			}
			pkvd->fHandled = 1;
			return;
		}
	}
}



bool CSave::WriteEntVars(const char* pname, entvars_t* pev)
{
	return WriteFields(pname, pev, gEntvarsDescription, std::size(gEntvarsDescription));
}



bool CSave::WriteFields(const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	int i, j, actualCount, emptyCount;
	TYPEDESCRIPTION* pTest;
	int entityArray[MAX_ENTITYARRAY];
	byte boolArray[MAX_ENTITYARRAY];

	// Precalculate the number of empty fields
	emptyCount = 0;
	for (i = 0; i < fieldCount; i++)
	{
		void* pOutputData;
		pOutputData = ((char*)pBaseData + pFields[i].fieldOffset);
		if (DataEmpty((const char*)pOutputData, pFields[i].fieldSize * gSizes[pFields[i].fieldType]))
			emptyCount++;
	}

	// Empty fields will not be written, write out the actual number of fields to be written
	actualCount = fieldCount - emptyCount;
	WriteInt(pname, &actualCount, 1);

	for (i = 0; i < fieldCount; i++)
	{
		void* pOutputData;
		pTest = &pFields[i];
		pOutputData = ((char*)pBaseData + pTest->fieldOffset);

		// UNDONE: Must we do this twice?
		if (DataEmpty((const char*)pOutputData, pTest->fieldSize * gSizes[pTest->fieldType]))
			continue;

		switch (pTest->fieldType)
		{
		case FIELD_FLOAT:
			WriteFloat(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_TIME:
			WriteTime(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			WriteString(pTest->fieldName, (string_t*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_CLASSPTR:
		case FIELD_EVARS:
		case FIELD_EDICT:
		case FIELD_ENTITY:
		case FIELD_EHANDLE:
			if (pTest->fieldSize > MAX_ENTITYARRAY)
				Logger->error("Can't save more than {} entities in an array!!!", MAX_ENTITYARRAY);
			for (j = 0; j < pTest->fieldSize; j++)
			{
				switch (pTest->fieldType)
				{
				case FIELD_EVARS:
					entityArray[j] = EntityIndex(((entvars_t**)pOutputData)[j]);
					break;
				case FIELD_CLASSPTR:
					entityArray[j] = EntityIndex(((CBaseEntity**)pOutputData)[j]);
					break;
				case FIELD_EDICT:
					entityArray[j] = EntityIndex(((edict_t**)pOutputData)[j]);
					break;
				case FIELD_ENTITY:
					entityArray[j] = EntityIndex(((EOFFSET*)pOutputData)[j]);
					break;
				case FIELD_EHANDLE:
					entityArray[j] = EntityIndex((CBaseEntity*)(((EHANDLE*)pOutputData)[j]));
					break;
				}
			}
			WriteInt(pTest->fieldName, entityArray, pTest->fieldSize);
			break;
		case FIELD_POSITION_VECTOR:
			WritePositionVector(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;
		case FIELD_VECTOR:
			WriteVector(pTest->fieldName, (float*)pOutputData, pTest->fieldSize);
			break;

		case FIELD_BOOLEAN:
		{
			// TODO: need to refactor save game stuff to make this cleaner and reusable
			// Convert booleans to bytes
			for (j = 0; j < pTest->fieldSize; j++)
			{
				boolArray[j] = ((bool*)pOutputData)[j] ? 1 : 0;
			}

			WriteData(pTest->fieldName, pTest->fieldSize, (char*)boolArray);
		}
		break;

		case FIELD_INTEGER:
			WriteInt(pTest->fieldName, (int*)pOutputData, pTest->fieldSize);
			break;

		case FIELD_INT64:
			WriteData(pTest->fieldName, sizeof(std::uint64_t) * pTest->fieldSize, ((char*)pOutputData));
			break;

		case FIELD_SHORT:
			WriteData(pTest->fieldName, 2 * pTest->fieldSize, ((char*)pOutputData));
			break;

		case FIELD_CHARACTER:
			WriteData(pTest->fieldName, pTest->fieldSize, ((char*)pOutputData));
			break;

			// For now, just write the address out, we're not going to change memory while doing this yet!
		case FIELD_POINTER:
			WriteInt(pTest->fieldName, (int*)(char*)pOutputData, pTest->fieldSize);
			break;

		case FIELD_FUNCTION:
			WriteFunction(pTest->fieldName, (void**)pOutputData, pTest->fieldSize);
			break;
		default:
			Logger->error("Bad field type");
		}
	}

	return true;
}


void CSave::BufferString(char* pdata, int len)
{
	char c = 0;

	BufferData(pdata, len); // Write the string
	BufferData(&c, 1);		// Write a null terminator
}


bool CSave::DataEmpty(const char* pdata, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (0 != pdata[i])
			return false;
	}
	return true;
}


void CSave::BufferField(const char* pname, int size, const char* pdata)
{
	BufferHeader(pname, size);
	BufferData(pdata, size);
}


void CSave::BufferHeader(const char* pname, int size)
{
	short hashvalue = TokenHash(pname);
	if (size > 1 << (sizeof(short) * 8))
		Logger->error("CSave :: BufferHeader() size parameter exceeds 'short'!");
	BufferData((const char*)&size, sizeof(short));
	BufferData((const char*)&hashvalue, sizeof(short));
}


void CSave::BufferData(const char* pdata, int size)
{
	if (m_data.size + size > m_data.bufferSize)
	{
		Logger->error("Save/Restore overflow!");
		m_data.size = m_data.bufferSize;
		return;
	}

	memcpy(m_data.pCurrentData, pdata, size);
	m_data.pCurrentData += size;
	m_data.size += size;
}



// --------------------------------------------------------------
//
// CRestore
//
// --------------------------------------------------------------

int CRestore::ReadField(void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount, int startField, int size, char* pName, void* pData)
{
	int i, j, stringCount, fieldNumber, entityIndex;
	TYPEDESCRIPTION* pTest;
	float timeData;
	Vector position;
	edict_t* pent;
	char* pString;

	position = Vector(0, 0, 0);

	if (0 != m_data.fUseLandmark)
		position = m_data.vecLandmarkOffset;

	for (i = 0; i < fieldCount; i++)
	{
		fieldNumber = (i + startField) % fieldCount;
		pTest = &pFields[fieldNumber];
		if (!stricmp(pTest->fieldName, pName))
		{
			if (!m_global || (pTest->flags & FTYPEDESC_GLOBAL) == 0)
			{
				for (j = 0; j < pTest->fieldSize; j++)
				{
					void* pOutputData = ((char*)pBaseData + pTest->fieldOffset + (j * gSizes[pTest->fieldType]));
					void* pInputData = (char*)pData + j * gSizes[pTest->fieldType];

					switch (pTest->fieldType)
					{
					case FIELD_TIME:
						timeData = *(float*)pInputData;
						// Re-base time variables
						timeData += m_data.time;
						*((float*)pOutputData) = timeData;
						break;
					case FIELD_FLOAT:
						*((float*)pOutputData) = *(float*)pInputData;
						break;
					case FIELD_MODELNAME:
					case FIELD_SOUNDNAME:
					case FIELD_STRING:
						// Skip over j strings
						pString = (char*)pData;
						for (stringCount = 0; stringCount < j; stringCount++)
						{
							while ('\0' != *pString)
								pString++;
							pString++;
						}
						pInputData = pString;
						if (strlen((char*)pInputData) == 0)
							*((int*)pOutputData) = 0;
						else
						{
							string_t string = ALLOC_STRING((char*)pInputData);

							*((string_t*)pOutputData) = string;

							if (!FStringNull(string) && m_precache)
							{
								// Don't use UTIL_PrecacheModel/Sound here because we're restoring an already-replaced name.
								if (pTest->fieldType == FIELD_MODELNAME)
									UTIL_PrecacheModelDirect(STRING(string));
								else if (pTest->fieldType == FIELD_SOUNDNAME)
									UTIL_PrecacheSoundDirect(STRING(string));
							}
						}
						break;
					case FIELD_EVARS:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((entvars_t**)pOutputData) = VARS(pent);
						else
							*((entvars_t**)pOutputData) = nullptr;
						break;
					case FIELD_CLASSPTR:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((CBaseEntity**)pOutputData) = CBaseEntity::Instance(pent);
						else
							*((CBaseEntity**)pOutputData) = nullptr;
						break;
					case FIELD_EDICT:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						*((edict_t**)pOutputData) = pent;
						break;
					case FIELD_EHANDLE:
						// Input and Output sizes are different!
						pOutputData = (char*)pOutputData + j * (sizeof(EHANDLE) - gSizes[pTest->fieldType]);
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((EHANDLE*)pOutputData) = CBaseEntity::Instance(pent);
						else
							*((EHANDLE*)pOutputData) = nullptr;
						break;
					case FIELD_ENTITY:
						entityIndex = *(int*)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((EOFFSET*)pOutputData) = OFFSET(pent);
						else
							*((EOFFSET*)pOutputData) = 0;
						break;
					case FIELD_VECTOR:
						((float*)pOutputData)[0] = ((float*)pInputData)[0];
						((float*)pOutputData)[1] = ((float*)pInputData)[1];
						((float*)pOutputData)[2] = ((float*)pInputData)[2];
						break;
					case FIELD_POSITION_VECTOR:
						((float*)pOutputData)[0] = ((float*)pInputData)[0] + position.x;
						((float*)pOutputData)[1] = ((float*)pInputData)[1] + position.y;
						((float*)pOutputData)[2] = ((float*)pInputData)[2] + position.z;
						break;

					case FIELD_BOOLEAN:
					{
						// Input and Output sizes are different!
						pOutputData = (char*)pOutputData + j * (sizeof(bool) - gSizes[pTest->fieldType]);
						const bool value = *((byte*)pInputData) != 0;

						*((bool*)pOutputData) = value;
					}
					break;

					case FIELD_INTEGER:
						*((int*)pOutputData) = *(int*)pInputData;
						break;

					case FIELD_INT64:
						*((std::uint64_t*)pOutputData) = *(std::uint64_t*)pInputData;
						break;

					case FIELD_SHORT:
						*((short*)pOutputData) = *(short*)pInputData;
						break;

					case FIELD_CHARACTER:
						*((char*)pOutputData) = *(char*)pInputData;
						break;

					case FIELD_POINTER:
						*((int*)pOutputData) = *(int*)pInputData;
						break;
					case FIELD_FUNCTION:
						if (strlen((char*)pInputData) == 0)
							*((int*)pOutputData) = 0;
						else
							*((int*)pOutputData) = FUNCTION_FROM_NAME((char*)pInputData);
						break;

					default:
						Logger->error("Bad field type");
					}
				}
			}
#if 0
			else
			{
				Logger->debug( "Skipping global field {}", pName);
			}
#endif
			return fieldNumber;
		}
	}

	return -1;
}


bool CRestore::ReadEntVars(const char* pname, entvars_t* pev)
{
	return ReadFields(pname, pev, gEntvarsDescription, std::size(gEntvarsDescription));
}


bool CRestore::ReadFields(const char* pname, void* pBaseData, TYPEDESCRIPTION* pFields, int fieldCount)
{
	unsigned short i, token;
	int lastField, fileCount;
	HEADER header;

	i = ReadShort();
	ASSERT(i == sizeof(int)); // First entry should be an int

	token = ReadShort();

	// Check the struct name
	if (token != TokenHash(pname)) // Field Set marker
	{
		//		Logger->error("Expected {} found {}!", pname, BufferPointer() );
		BufferRewind(2 * sizeof(short));
		return false;
	}

	// Skip over the struct name
	fileCount = ReadInt(); // Read field count

	lastField = 0; // Make searches faster, most data is read/written in the same order

	// Clear out base data
	for (i = 0; i < fieldCount; i++)
	{
		// Don't clear global fields
		if (!m_global || (pFields[i].flags & FTYPEDESC_GLOBAL) == 0)
			memset(((char*)pBaseData + pFields[i].fieldOffset), 0, pFields[i].fieldSize * gSizes[pFields[i].fieldType]);
	}

	for (i = 0; i < fileCount; i++)
	{
		BufferReadHeader(&header);
		lastField = ReadField(pBaseData, pFields, fieldCount, lastField, header.size, m_data.pTokens[header.token], header.pData);
		lastField++;
	}

	return true;
}


void CRestore::BufferReadHeader(HEADER* pheader)
{
	ASSERT(pheader != nullptr);
	pheader->size = ReadShort();	  // Read field size
	pheader->token = ReadShort();	  // Read field name token
	pheader->pData = BufferPointer(); // Field Data is next
	BufferSkipBytes(pheader->size);	  // Advance to next field
}


short CRestore::ReadShort()
{
	short tmp = 0;

	BufferReadBytes((char*)&tmp, sizeof(short));

	return tmp;
}

int CRestore::ReadInt()
{
	int tmp = 0;

	BufferReadBytes((char*)&tmp, sizeof(int));

	return tmp;
}

int CRestore::ReadNamedInt(const char* pName)
{
	HEADER header;

	BufferReadHeader(&header);
	return ((int*)header.pData)[0];
}

char* CRestore::ReadNamedString(const char* pName)
{
	HEADER header;

	BufferReadHeader(&header);
#ifdef TOKENIZE
	return (char*)(m_pdata->pTokens[*(short*)header.pData]);
#else
	return (char*)header.pData;
#endif
}


char* CRestore::BufferPointer()
{
	return m_data.pCurrentData;
}

void CRestore::BufferReadBytes(char* pOutput, int size)
{
	if (Empty())
		return;

	if ((m_data.size + size) > m_data.bufferSize)
	{
		Logger->error("Restore overflow!");
		m_data.size = m_data.bufferSize;
		return;
	}

	if (pOutput)
		memcpy(pOutput, m_data.pCurrentData, size);
	m_data.pCurrentData += size;
	m_data.size += size;
}


void CRestore::BufferSkipBytes(int bytes)
{
	BufferReadBytes(nullptr, bytes);
}

int CRestore::BufferSkipZString()
{
	const int maxLen = m_data.bufferSize - m_data.size;

	int len = 0;
	char* pszSearch = m_data.pCurrentData;
	while ('\0' != *pszSearch++ && len < maxLen)
		len++;

	len++;

	BufferSkipBytes(len);

	return len;
}

bool CRestore::BufferCheckZString(const char* string)
{
	const int maxLen = m_data.bufferSize - m_data.size;
	const int len = strlen(string);
	if (len <= maxLen)
	{
		if (0 == strncmp(string, m_data.pCurrentData, len))
			return true;
	}
	return false;
}

bool UTIL_IsMultiplayer()
{
	return g_pGameRules->IsMultiplayer();
}

bool UTIL_IsCTF()
{
	return g_pGameRules->IsCTF();
}
