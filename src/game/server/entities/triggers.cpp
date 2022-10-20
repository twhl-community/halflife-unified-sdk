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

===== triggers.cpp ========================================================

  spawn and use functions for editor-placed triggers

*/

#include "cbase.h"
#include "trains.h" // trigger_camera has train functionality
#include "ctfplay_gamerules.h"
#include "ctf/CTFGoalFlag.h"
#include "UserMessages.h"

#define SF_TRIGGER_PUSH_START_OFF 2		   //spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_TARGETONCE 1	   // Only fire hurt target once
#define SF_TRIGGER_HURT_START_OFF 2		   //spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_NO_CLIENTS 8	   //spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE 16  // trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32 // only clients may touch this trigger.

void SetMovedir(entvars_t* pev);
Vector VecBModelOrigin(entvars_t* pevBModel);

class CFrictionModifier : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT ChangeFriction(CBaseEntity* pOther);
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static TYPEDESCRIPTION m_SaveData[];

	float m_frictionFraction; // Sorry, couldn't resist this name :)
};

LINK_ENTITY_TO_CLASS(func_friction, CFrictionModifier);

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION CFrictionModifier::m_SaveData[] =
	{
		DEFINE_FIELD(CFrictionModifier, m_frictionFraction, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CFrictionModifier, CBaseEntity);


// Modify an entity's friction
void CFrictionModifier::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	SetModel(STRING(pev->model)); // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch(&CFrictionModifier::ChangeFriction);
}


// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier::ChangeFriction(CBaseEntity* pOther)
{
	if (pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE)
		pOther->pev->friction = m_frictionFraction;
}



// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
bool CFrictionModifier::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "modifier"))
	{
		m_frictionFraction = atof(pkvd->szValue) / 100.0;
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}


// This trigger will fire when the level spawns (or respawns if not fire once)
// It will check a global state before firing.  It supports delay and killtargets

#define SF_AUTO_FIREONCE 0x0001

class CAutoTrigger : public CBaseDelay
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Precache() override;
	void Think() override;

	int ObjectCaps() override { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

private:
	string_t m_globalstate;
	USE_TYPE triggerType;
};
LINK_ENTITY_TO_CLASS(trigger_auto, CAutoTrigger);

TYPEDESCRIPTION CAutoTrigger::m_SaveData[] =
	{
		DEFINE_FIELD(CAutoTrigger, m_globalstate, FIELD_STRING),
		DEFINE_FIELD(CAutoTrigger, triggerType, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CAutoTrigger, CBaseDelay);

bool CAutoTrigger::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);
		switch (type)
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		return true;
	}

	return CBaseDelay::KeyValue(pkvd);
}


void CAutoTrigger::Spawn()
{
	Precache();
}


void CAutoTrigger::Precache()
{
	pev->nextthink = gpGlobals->time + 0.1;
}


void CAutoTrigger::Think()
{
	if (FStringNull(m_globalstate) || gGlobalState.EntityGetState(m_globalstate) == GLOBAL_ON)
	{
		SUB_UseTargets(this, triggerType, 0);
		if ((pev->spawnflags & SF_AUTO_FIREONCE) != 0)
			UTIL_Remove(this);
	}
}



#define SF_RELAY_FIREONCE 0x0001

class CTriggerRelay : public CBaseDelay
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

private:
	USE_TYPE triggerType;
};
LINK_ENTITY_TO_CLASS(trigger_relay, CTriggerRelay);

TYPEDESCRIPTION CTriggerRelay::m_SaveData[] =
	{
		DEFINE_FIELD(CTriggerRelay, triggerType, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CTriggerRelay, CBaseDelay);

bool CTriggerRelay::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);
		switch (type)
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		return true;
	}

	return CBaseDelay::KeyValue(pkvd);
}


void CTriggerRelay::Spawn()
{
}




void CTriggerRelay::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SUB_UseTargets(this, triggerType, 0);
	if ((pev->spawnflags & SF_RELAY_FIREONCE) != 0)
		UTIL_Remove(this);
}


//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE 0x80000000
#define SF_MULTIMAN_THREAD 0x00000001

class CMultiManager : public CBaseToggle
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void EXPORT ManagerThink();
	void EXPORT ManagerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

#if _DEBUG
	void EXPORT ManagerReport();
#endif

	bool HasTarget(string_t targetname) override;

	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	int m_cTargets;								// the total number of targets in this manager's fire list.
	int m_index;								// Current target
	float m_startTime;							// Time we started firing
	string_t m_iTargetName[MAX_MULTI_TARGETS];	// list if indexes into global string array
	float m_flTargetDelay[MAX_MULTI_TARGETS];	// delay (in seconds) from time of manager fire to target fire
private:
	inline bool IsClone() { return (pev->spawnflags & SF_MULTIMAN_CLONE) != 0; }
	inline bool ShouldClone()
	{
		if (IsClone())
			return false;

		return (pev->spawnflags & SF_MULTIMAN_THREAD) != 0;
	}

	CMultiManager* Clone();
};
LINK_ENTITY_TO_CLASS(multi_manager, CMultiManager);

// Global Savedata for multi_manager
TYPEDESCRIPTION CMultiManager::m_SaveData[] =
	{
		DEFINE_FIELD(CMultiManager, m_cTargets, FIELD_INTEGER),
		DEFINE_FIELD(CMultiManager, m_index, FIELD_INTEGER),
		DEFINE_FIELD(CMultiManager, m_startTime, FIELD_TIME),
		DEFINE_ARRAY(CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS),
		DEFINE_ARRAY(CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS),
};

IMPLEMENT_SAVERESTORE(CMultiManager, CBaseToggle);

bool CMultiManager::KeyValue(KeyValueData* pkvd)
{
	// UNDONE: Maybe this should do something like this:
	//CBaseToggle::KeyValue( pkvd );
	// if ( !pkvd->fHandled )
	// ... etc.

	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		return true;
	}
	else // add this field to the target list
	{
		// this assumes that additional fields are targetnames and their values are delay values.
		if (m_cTargets < MAX_MULTI_TARGETS)
		{
			char tmp[128];

			UTIL_StripToken(pkvd->szKeyName, tmp);
			m_iTargetName[m_cTargets] = ALLOC_STRING(tmp);
			m_flTargetDelay[m_cTargets] = atof(pkvd->szValue);
			m_cTargets++;
			return true;
		}
	}

	return false;
}


void CMultiManager::Spawn()
{
	pev->solid = SOLID_NOT;
	SetUse(&CMultiManager::ManagerUse);
	SetThink(&CMultiManager::ManagerThink);

	// Sort targets
	// Quick and dirty bubble sort
	bool swapped = true;

	while (swapped)
	{
		swapped = false;
		for (int i = 1; i < m_cTargets; i++)
		{
			if (m_flTargetDelay[i] < m_flTargetDelay[i - 1])
			{
				// Swap out of order elements
				string_t name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i - 1];
				m_flTargetDelay[i] = m_flTargetDelay[i - 1];
				m_iTargetName[i - 1] = name;
				m_flTargetDelay[i - 1] = delay;
				swapped = true;
			}
		}
	}
}


bool CMultiManager::HasTarget(string_t targetname)
{
	for (int i = 0; i < m_cTargets; i++)
		if (FStrEq(STRING(targetname), STRING(m_iTargetName[i])))
			return true;

	return false;
}


// Designers were using this to fire targets that may or may not exist --
// so I changed it to use the standard target fire code, made it a little simpler.
void CMultiManager::ManagerThink()
{
	float time;

	time = gpGlobals->time - m_startTime;
	while (m_index < m_cTargets && m_flTargetDelay[m_index] <= time)
	{
		FireTargets(STRING(m_iTargetName[m_index]), m_hActivator, this, USE_TOGGLE, 0);
		m_index++;
	}

	if (m_index >= m_cTargets) // have we fired all targets?
	{
		SetThink(nullptr);
		if (IsClone())
		{
			UTIL_Remove(this);
			return;
		}
		SetUse(&CMultiManager::ManagerUse); // allow manager re-use
	}
	else
		pev->nextthink = m_startTime + m_flTargetDelay[m_index];
}

CMultiManager* CMultiManager::Clone()
{
	CMultiManager* pMulti = GetClassPtr((CMultiManager*)nullptr);

	edict_t* pEdict = pMulti->pev->pContainingEntity;
	memcpy(pMulti->pev, pev, sizeof(*pev));
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	memcpy(pMulti->m_iTargetName, m_iTargetName, sizeof(m_iTargetName));
	memcpy(pMulti->m_flTargetDelay, m_flTargetDelay, sizeof(m_flTargetDelay));

	return pMulti;
}


// The USE function builds the time table and starts the entity thinking.
void CMultiManager::ManagerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// In multiplayer games, clone the MM and execute in the clone (like a thread)
	// to allow multiple players to trigger the same multimanager
	if (ShouldClone())
	{
		CMultiManager* pClone = Clone();
		pClone->ManagerUse(pActivator, pCaller, useType, value);
		return;
	}

	m_hActivator = pActivator;
	m_index = 0;
	m_startTime = gpGlobals->time;

	SetUse(nullptr); // disable use until all targets have fired

	SetThink(&CMultiManager::ManagerThink);
	pev->nextthink = gpGlobals->time;
}

#if _DEBUG
void CMultiManager::ManagerReport()
{
	int cIndex;

	for (cIndex = 0; cIndex < m_cTargets; cIndex++)
	{
		Logger->debug("{} {}", STRING(m_iTargetName[cIndex]), m_flTargetDelay[cIndex]);
	}
}
#endif

//***********************************************************


//
// Render parameters trigger
//
// This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
// to its targets when triggered.
//


// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX (1 << 0)
#define SF_RENDER_MASKAMT (1 << 1)
#define SF_RENDER_MASKMODE (1 << 2)
#define SF_RENDER_MASKCOLOR (1 << 3)

class CRenderFxManager : public CBaseEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
};

LINK_ENTITY_TO_CLASS(env_render, CRenderFxManager);


void CRenderFxManager::Spawn()
{
	pev->solid = SOLID_NOT;
}

void CRenderFxManager::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!FStringNull(pev->target))
	{
		edict_t* pentTarget = nullptr;
		while (true)
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->target));
			if (FNullEnt(pentTarget))
				break;

			entvars_t* pevTarget = VARS(pentTarget);
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKFX))
				pevTarget->renderfx = pev->renderfx;
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKAMT))
				pevTarget->renderamt = pev->renderamt;
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKMODE))
				pevTarget->rendermode = pev->rendermode;
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKCOLOR))
				pevTarget->rendercolor = pev->rendercolor;
		}
	}
}



class CBaseTrigger : public CBaseToggle
{
public:
	void EXPORT TeleportTouch(CBaseEntity* pOther);
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT MultiTouch(CBaseEntity* pOther);
	void EXPORT HurtTouch(CBaseEntity* pOther);
	void ActivateMultiTrigger(CBaseEntity* pActivator);
	void EXPORT MultiWaitOver();
	void EXPORT CounterUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void InitTrigger();

	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS(trigger, CBaseTrigger);

/*
================
InitTrigger
================
*/
void CBaseTrigger::InitTrigger()
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (pev->angles != g_vecZero)
		SetMovedir(pev);
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SetModel(STRING(pev->model)); // set size and link into world
	if (CVAR_GET_FLOAT("showtriggers") == 0)
		SetBits(pev->effects, EF_NODRAW);

	if (!FStringNull(pev->message))
	{
		pev->message = ALLOC_ESCAPED_STRING(STRING(pev->message));
	}
}


//
// Cache user-entity-field values until spawn is called.
//

bool CBaseTrigger::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "count"))
	{
		m_cTriggersLeft = (int)atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		m_bitsDamageInflict = atoi(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

class CTriggerHurt : public CBaseTrigger
{
public:
	void Spawn() override;
	void EXPORT RadiationThink();
};

LINK_ENTITY_TO_CLASS(trigger_hurt, CTriggerHurt);

//
// trigger_monsterjump
//
class CTriggerMonsterJump : public CBaseTrigger
{
public:
	void Spawn() override;
	void Touch(CBaseEntity* pOther) override;
	void Think() override;
};

LINK_ENTITY_TO_CLASS(trigger_monsterjump, CTriggerMonsterJump);


void CTriggerMonsterJump::Spawn()
{
	SetMovedir(pev);

	InitTrigger();

	pev->nextthink = 0;
	pev->speed = 200;
	m_flHeight = 150;

	if (!FStringNull(pev->targetname))
	{ // if targetted, spawn turned off
		pev->solid = SOLID_NOT;
		UTIL_SetOrigin(pev, pev->origin); // Unlink from trigger list
		SetUse(&CTriggerMonsterJump::ToggleUse);
	}
}


void CTriggerMonsterJump::Think()
{
	pev->solid = SOLID_NOT;			  // kill the trigger for now !!!UNDONE
	UTIL_SetOrigin(pev, pev->origin); // Unlink from trigger list
	SetThink(nullptr);
}

void CTriggerMonsterJump::Touch(CBaseEntity* pOther)
{
	entvars_t* pevOther = pOther->pev;

	if (!FBitSet(pevOther->flags, FL_MONSTER))
	{ // touched by a non-monster.
		return;
	}

	pevOther->origin.z += 1;

	if (FBitSet(pevOther->flags, FL_ONGROUND))
	{ // clear the onground so physics don't bitch
		pevOther->flags &= ~FL_ONGROUND;
	}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;
	pev->nextthink = gpGlobals->time;
}

//=====================================

//
// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
//
//int gfToggleState = 0; // used to determine when all radiation trigger hurts have called 'RadiationThink'

void CTriggerHurt::Spawn()
{
	InitTrigger();
	SetTouch(&CTriggerHurt::HurtTouch);

	if (!FStringNull(pev->targetname))
	{
		SetUse(&CTriggerHurt::ToggleUse);
	}
	else
	{
		SetUse(nullptr);
	}

	if ((m_bitsDamageInflict & DMG_RADIATION) != 0)
	{
		SetThink(&CTriggerHurt::RadiationThink);
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.0, 0.5);
	}

	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_START_OFF)) // if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin); // Link into the list
}

// trigger hurt that causes radiation will do a radius
// check and set the player's geiger counter level
// according to distance from center of trigger

void CTriggerHurt::RadiationThink()
{

	edict_t* pentPlayer;
	CBasePlayer* pPlayer = nullptr;
	float flRange;
	entvars_t* pevTarget;
	Vector vecSpot1;
	Vector vecSpot2;
	Vector vecRange;
	Vector origin;
	Vector view_ofs;

	// check to see if a player is in pvs
	// if not, continue

	// set origin to center of trigger so that this check works
	origin = pev->origin;
	view_ofs = pev->view_ofs;

	pev->origin = (pev->absmin + pev->absmax) * 0.5;
	pev->view_ofs = pev->view_ofs * 0.0;

	pentPlayer = FIND_CLIENT_IN_PVS(edict());

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	// reset origin

	if (!FNullEnt(pentPlayer))
	{

		pPlayer = GetClassPtr((CBasePlayer*)VARS(pentPlayer));

		pevTarget = VARS(pentPlayer);

		// get range to player;

		vecSpot1 = (pev->absmin + pev->absmax) * 0.5;
		vecSpot2 = (pevTarget->absmin + pevTarget->absmax) * 0.5;

		vecRange = vecSpot1 - vecSpot2;
		flRange = vecRange.Length();

		// if player's current geiger counter range is larger
		// than range to this trigger hurt, reset player's
		// geiger counter range

		if (pPlayer->m_flgeigerRange >= flRange)
			pPlayer->m_flgeigerRange = flRange;
	}

	pev->nextthink = gpGlobals->time + 0.25;
}

//
// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
//
void CBaseTrigger::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pev->solid == SOLID_NOT)
	{ // if the trigger is off, turn it on
		pev->solid = SOLID_TRIGGER;

		// Force retouch
		gpGlobals->force_retouch++;
	}
	else
	{ // turn the trigger off
		pev->solid = SOLID_NOT;
	}
	UTIL_SetOrigin(pev, pev->origin);
}

// When touched, a hurt trigger does DMG points of damage each half-second
void CBaseTrigger::HurtTouch(CBaseEntity* pOther)
{
	float fldmg;

	if (0 == pOther->pev->takedamage)
		return;

	if ((pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH) != 0 && !pOther->IsPlayer())
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if ((pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS) != 0 && pOther->IsPlayer())
		return;

	static_assert(MAX_PLAYERS <= 32, "Rework the player mask logic to support more than 32 players");

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if (g_pGameRules->IsMultiplayer())
	{
		if (pev->dmgtime > gpGlobals->time)
		{
			if (gpGlobals->time != pev->pain_finished)
			{ // too early to hurt again, and not same frame with a different entity
				if (pOther->IsPlayer())
				{
					int playerMask = 1 << (pOther->entindex() - 1);

					// If I've already touched this player (this time), then bail out
					if ((pev->impulse & playerMask) != 0)
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if (pOther->IsPlayer())
			{
				int playerMask = 1 << (pOther->entindex() - 1);

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
	}
	else // Original code -- single player
	{
		if (pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished)
		{ // too early to hurt again, and not same frame with a different entity
			return;
		}
	}



	// If this is time_based damage (poison, radiation), override the pev->dmg with a
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger

	fldmg = pev->dmg * 0.5; // 0.5 seconds worth of damage, pev->dmg is damage/second


	// JAY: Cut this because it wasn't fully realized.  Damage is simpler now.
#if 0
	switch (m_bitsDamageInflict)
	{
	default: break;
	case DMG_POISON:		fldmg = POISON_DAMAGE / 4; break;
	case DMG_NERVEGAS:		fldmg = NERVEGAS_DAMAGE / 4; break;
	case DMG_RADIATION:		fldmg = RADIATION_DAMAGE / 4; break;
	case DMG_PARALYZE:		fldmg = PARALYZE_DAMAGE / 4; break; // UNDONE: cut this? should slow movement to 50%
	case DMG_ACID:			fldmg = ACID_DAMAGE / 4; break;
	case DMG_SLOWBURN:		fldmg = SLOWBURN_DAMAGE / 4; break;
	case DMG_SLOWFREEZE:	fldmg = SLOWFREEZE_DAMAGE / 4; break;
	}
#endif

	if (fldmg < 0)
		pOther->TakeHealth(-fldmg, m_bitsDamageInflict);
	else
		pOther->TakeDamage(pev, pev, fldmg, m_bitsDamageInflict);

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5; // half second delay until this trigger can hurt toucher again



	if (!FStringNull(pev->target))
	{
		// trigger has a target it wants to fire.
		if ((pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE) != 0)
		{
			// if the toucher isn't a client, don't fire the target!
			if (!pOther->IsPlayer())
			{
				return;
			}
		}

		SUB_UseTargets(pOther, USE_TOGGLE, 0);
		if ((pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE) != 0)
			pev->target = string_t::Null;
	}
}


/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)      secret
2)      beep beep
3)      large switch
4)
NEW
if a trigger has a NETNAME, that NETNAME will become the TARGET of the triggered object.
*/
class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS(trigger_multiple, CTriggerMultiple);


void CTriggerMultiple::Spawn()
{
	if (m_flWait == 0)
		m_flWait = 0.2;

	InitTrigger();

	ASSERTSZ(pev->health == 0, "trigger_multiple with health");
	//	UTIL_SetOrigin(pev, pev->origin);
	//	SetModel(STRING(pev->model));
	//	if (pev->health > 0)
	//		{
	//		if (FBitSet(pev->spawnflags, SPAWNFLAG_NOTOUCH))
	//			Logger->error("trigger_multiple spawn: health and notouch don't make sense");
	//		pev->max_health = pev->health;
	//UNDONE: where to get pfnDie from?
	//		pev->pfnDie = multi_killed;
	//		pev->takedamage = DAMAGE_YES;
	//		pev->solid = SOLID_BBOX;
	//		UTIL_SetOrigin(pev, pev->origin);  // make sure it links into the world
	//		}
	//	else
	{
		SetTouch(&CTriggerMultiple::MultiTouch);
	}
}


/*QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself.  You must set the key "target" to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.  Use "360" for an angle of 0.
sounds
1)      secret
2)      beep beep
3)      large switch
4)
*/
class CTriggerOnce : public CTriggerMultiple
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS(trigger_once, CTriggerOnce);
void CTriggerOnce::Spawn()
{
	m_flWait = -1;

	CTriggerMultiple::Spawn();
}



void CBaseTrigger::MultiTouch(CBaseEntity* pOther)
{
	entvars_t* pevToucher;

	pevToucher = pOther->pev;

	// Only touch clients, monsters, or pushables (depending on flags)
	if (((pevToucher->flags & FL_CLIENT) != 0 && (pev->spawnflags & SF_TRIGGER_NOCLIENTS) == 0) ||
		((pevToucher->flags & FL_MONSTER) != 0 && (pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS) != 0) ||
		(pev->spawnflags & SF_TRIGGER_PUSHABLES) != 0 && FClassnameIs(pevToucher, "func_pushable"))
	{

#if 0
		// if the trigger has an angles field, check player's facing direction
		if (pev->movedir != g_vecZero)
		{
			UTIL_MakeVectors(pevToucher->angles);
			if (DotProduct(gpGlobals->v_forward, pev->movedir) < 0)
				return;         // not facing the right way
		}
#endif

		ActivateMultiTrigger(pOther);
	}
}


//
// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
//
void CBaseTrigger::ActivateMultiTrigger(CBaseEntity* pActivator)
{
	if (pev->nextthink > gpGlobals->time)
		return; // still waiting for reset time

	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator))
		return;

	if (!FStringNull(pev->noise))
		EMIT_SOUND(ENT(pev), CHAN_VOICE, STRING(pev->noise), 1, ATTN_NORM);

	// don't trigger again until reset
	// pev->takedamage = DAMAGE_NO;

	m_hActivator = pActivator;
	SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);

	if (!FStringNull(pev->message) && pActivator->IsPlayer())
	{
		UTIL_ShowMessage(STRING(pev->message), pActivator);
		//		CLIENT_PRINTF( ENT( pActivator->pev ), print_center, STRING(pev->message) );
	}

	if (m_flWait > 0)
	{
		SetThink(&CBaseTrigger::MultiWaitOver);
		pev->nextthink = gpGlobals->time + m_flWait;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch(nullptr);
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(&CBaseTrigger::SUB_Remove);
	}
}


// the wait time has passed, so set back up for another activation
void CBaseTrigger::MultiWaitOver()
{
	//	if (pev->max_health)
	//		{
	//		pev->health		= pev->max_health;
	//		pev->takedamage	= DAMAGE_YES;
	//		pev->solid		= SOLID_BBOX;
	//		}
	SetThink(nullptr);
}


// ========================= COUNTING TRIGGER =====================================

//
// GLOBALS ASSUMED SET:  g_eoActivator
//
void CBaseTrigger::CounterUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_cTriggersLeft--;
	m_hActivator = pActivator;

	if (m_cTriggersLeft < 0)
		return;

	bool fTellActivator =
		(m_hActivator != nullptr) &&
		m_hActivator->IsPlayer() &&
		!FBitSet(pev->spawnflags, SPAWNFLAG_NOMESSAGE);
	if (m_cTriggersLeft != 0)
	{
		if (fTellActivator)
		{
			// UNDONE: I don't think we want these Quakesque messages
			switch (m_cTriggersLeft)
			{
			case 1:
				Logger->debug("Only 1 more to go...");
				break;
			case 2:
				Logger->debug("Only 2 more to go...");
				break;
			case 3:
				Logger->debug("Only 3 more to go...");
				break;
			default:
				Logger->debug("There are more to go...");
				break;
			}
		}
		return;
	}

	// !!!UNDONE: I don't think we want these Quakesque messages
	if (fTellActivator)
		Logger->debug("Sequence completed!");

	ActivateMultiTrigger(m_hActivator);
}


/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.
If nomessage is not set, it will print "1 more.. " etc when triggered and
"sequence complete" when finished.  After the counter has been triggered "cTriggersLeft"
times (default 2), it will fire all of it's targets and remove itself.
*/
class CTriggerCounter : public CBaseTrigger
{
public:
	void Spawn() override;
};
LINK_ENTITY_TO_CLASS(trigger_counter, CTriggerCounter);

void CTriggerCounter::Spawn()
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if (m_cTriggersLeft == 0)
		m_cTriggersLeft = 2;
	SetUse(&CTriggerCounter::CounterUse);
}

// ====================== TRIGGER_CHANGELEVEL ================================

class CTriggerVolume : public CPointEntity // Derive from point entity so this doesn't move across levels
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS(trigger_transition, CTriggerVolume);

// Define space that travels across a level transition
void CTriggerVolume::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetModel(STRING(pev->model)); // set size and link into world
	pev->model = string_t::Null;
	pev->modelindex = 0;
}


// Fires a target after level transition and then dies
class CFireAndDie : public CBaseDelay
{
public:
	void Spawn() override;
	void Precache() override;
	void Think() override;
	int ObjectCaps() override { return CBaseDelay::ObjectCaps() | FCAP_FORCE_TRANSITION; } // Always go across transitions
};
LINK_ENTITY_TO_CLASS(fireanddie, CFireAndDie);

void CFireAndDie::Spawn()
{
	pev->classname = MAKE_STRING("fireanddie");
	// Don't call Precache() - it should be called on restore
}


void CFireAndDie::Precache()
{
	// This gets called on restore
	pev->nextthink = gpGlobals->time + m_flDelay;
}


void CFireAndDie::Think()
{
	SUB_UseTargets(this, USE_TOGGLE, 0);
	UTIL_Remove(this);
}


#define SF_CHANGELEVEL_USEONLY 0x0002
class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT UseChangeLevel(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT TouchChangeLevel(CBaseEntity* pOther);
	void ChangeLevelNow(CBaseEntity* pActivator);

	static edict_t* FindLandmark(const char* pLandmarkName);
	static int ChangeList(LEVELLIST* pLevelList, int maxList);
	static bool AddTransitionToList(LEVELLIST* pLevelList, int listCount, const char* pMapName, const char* pLandmarkName, edict_t* pentLandmark);
	static bool InTransitionVolume(CBaseEntity* pEntity, char* pVolumeName);

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	char m_szMapName[cchMapNameMost];	   // trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost]; // trigger_changelevel only:  landmark on next map
	string_t m_changeTarget;
	float m_changeTargetDelay;
};
LINK_ENTITY_TO_CLASS(trigger_changelevel, CChangeLevel);

// Global Savedata for changelevel trigger
TYPEDESCRIPTION CChangeLevel::m_SaveData[] =
	{
		DEFINE_ARRAY(CChangeLevel, m_szMapName, FIELD_CHARACTER, cchMapNameMost),
		DEFINE_ARRAY(CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost),
		DEFINE_FIELD(CChangeLevel, m_changeTarget, FIELD_STRING),
		DEFINE_FIELD(CChangeLevel, m_changeTargetDelay, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CChangeLevel, CBaseTrigger);

//
// Cache user-entity-field values until spawn is called.
//

bool CChangeLevel::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "map"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			Logger->error("Map name '{}' too long (32 chars)", pkvd->szValue);
		strcpy(m_szMapName, pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "landmark"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			Logger->error("Landmark name '{}' too long (32 chars)", pkvd->szValue);
		strcpy(m_szLandmarkName, pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "changetarget"))
	{
		m_changeTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "changedelay"))
	{
		m_changeTargetDelay = atof(pkvd->szValue);
		return true;
	}

	return CBaseTrigger::KeyValue(pkvd);
}


/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/

void CChangeLevel::Spawn()
{
	if (FStrEq(m_szMapName, ""))
		Logger->debug("a trigger_changelevel doesn't have a map");

	if (FStrEq(m_szLandmarkName, ""))
		Logger->debug("trigger_changelevel to {} doesn't have a landmark", m_szMapName);

	if (0 == stricmp(m_szMapName, STRING(gpGlobals->mapname)))
	{
		Logger->error("trigger_changelevel points to the current map ({}), which does not work", STRING(gpGlobals->mapname));
	}

	if (!FStringNull(pev->targetname))
	{
		SetUse(&CChangeLevel::UseChangeLevel);
	}
	InitTrigger();
	if ((pev->spawnflags & SF_CHANGELEVEL_USEONLY) == 0)
		SetTouch(&CChangeLevel::TouchChangeLevel);
	//	Logger->debug("TRANSITION: {} ({})", m_szMapName, m_szLandmarkName);
}

FILE_GLOBAL char st_szNextMap[cchMapNameMost];
FILE_GLOBAL char st_szNextSpot[cchMapNameMost];

edict_t* CChangeLevel::FindLandmark(const char* pLandmarkName)
{
	edict_t* pentLandmark;

	pentLandmark = FIND_ENTITY_BY_STRING(nullptr, "targetname", pLandmarkName);
	while (!FNullEnt(pentLandmark))
	{
		// Found the landmark
		if (FClassnameIs(pentLandmark, "info_landmark"))
			return pentLandmark;
		else
			pentLandmark = FIND_ENTITY_BY_STRING(pentLandmark, "targetname", pLandmarkName);
	}
	Logger->error("Can't find landmark {}", pLandmarkName);
	return nullptr;
}


//=========================================================
// CChangeLevel :: Use - allows level transitions to be
// triggered by buttons, etc.
//
//=========================================================
void CChangeLevel::UseChangeLevel(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	ChangeLevelNow(pActivator);
}

void CChangeLevel::ChangeLevelNow(CBaseEntity* pActivator)
{
	edict_t* pentLandmark;
	LEVELLIST levels[16];

	ASSERT(!FStrEq(m_szMapName, ""));

	// Don't work in deathmatch
	if (g_pGameRules->IsDeathmatch())
		return;

	// Some people are firing these multiple times in a frame, disable
	if (gpGlobals->time == pev->dmgtime)
		return;

	pev->dmgtime = gpGlobals->time;


	CBaseEntity* pPlayer = UTIL_GetLocalPlayer();
	if (!InTransitionVolume(pPlayer, m_szLandmarkName))
	{
		Logger->debug("Player isn't in the transition volume {}, aborting", m_szLandmarkName);
		return;
	}

	// Create an entity to fire the changetarget
	if (!FStringNull(m_changeTarget))
	{
		CFireAndDie* pFireAndDie = GetClassPtr((CFireAndDie*)nullptr);
		if (pFireAndDie)
		{
			// Set target and delay
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			pFireAndDie->pev->origin = pPlayer->pev->origin;
			// Call spawn
			DispatchSpawn(pFireAndDie->edict());
		}
	}
	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy(st_szNextMap, m_szMapName);

	m_hActivator = pActivator;
	SUB_UseTargets(pActivator, USE_TOGGLE, 0);
	st_szNextSpot[0] = 0; // Init landmark to nullptr

	// look for a landmark entity
	pentLandmark = FindLandmark(m_szLandmarkName);
	if (!FNullEnt(pentLandmark))
	{
		strcpy(st_szNextSpot, m_szLandmarkName);
		gpGlobals->vecLandmarkOffset = VARS(pentLandmark)->origin;
	}
	//Logger->debug("Level touches {} levels", ChangeList(levels, std::size(levels)));
	Logger->debug("CHANGE LEVEL: {} {}", st_szNextMap, st_szNextSpot);
	CHANGE_LEVEL(st_szNextMap, st_szNextSpot);
}

//
// GLOBALS ASSUMED SET:  st_szNextMap
//
void CChangeLevel::TouchChangeLevel(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
		return;

	ChangeLevelNow(pOther);
}


// Add a transition to the list, but ignore duplicates
// (a designer may have placed multiple trigger_changelevels with the same landmark)
bool CChangeLevel::AddTransitionToList(LEVELLIST* pLevelList, int listCount, const char* pMapName, const char* pLandmarkName, edict_t* pentLandmark)
{
	int i;

	if (!pLevelList || !pMapName || !pLandmarkName || !pentLandmark)
		return false;

	for (i = 0; i < listCount; i++)
	{
		if (pLevelList[i].pentLandmark == pentLandmark && strcmp(pLevelList[i].mapName, pMapName) == 0)
			return false;
	}
	strcpy(pLevelList[listCount].mapName, pMapName);
	strcpy(pLevelList[listCount].landmarkName, pLandmarkName);
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS(pentLandmark)->origin;

	return true;
}

int BuildChangeList(LEVELLIST* pLevelList, int maxList)
{
	return CChangeLevel::ChangeList(pLevelList, maxList);
}


bool CChangeLevel::InTransitionVolume(CBaseEntity* pEntity, char* pVolumeName)
{
	edict_t* pentVolume;


	if ((pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION) != 0)
		return true;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if (pEntity->pev->movetype == MOVETYPE_FOLLOW)
	{
		if (pEntity->pev->aiment != nullptr)
			pEntity = CBaseEntity::Instance(pEntity->pev->aiment);
	}

	bool inVolume = true; // Unless we find a trigger_transition, everything is in the volume

	pentVolume = FIND_ENTITY_BY_TARGETNAME(nullptr, pVolumeName);
	while (!FNullEnt(pentVolume))
	{
		CBaseEntity* pVolume = CBaseEntity::Instance(pentVolume);

		if (pVolume && FClassnameIs(pVolume->pev, "trigger_transition"))
		{
			if (pVolume->Intersects(pEntity)) // It touches one, it's in the volume
				return true;
			else
				inVolume = false; // Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
		pentVolume = FIND_ENTITY_BY_TARGETNAME(pentVolume, pVolumeName);
	}

	return inVolume;
}


// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should
// be moved across.
int CChangeLevel::ChangeList(LEVELLIST* pLevelList, int maxList)
{
	edict_t *pentChangelevel, *pentLandmark;
	int i, count;

	count = 0;

	// Find all of the possible level changes on this BSP
	pentChangelevel = FIND_ENTITY_BY_STRING(nullptr, "classname", "trigger_changelevel");
	if (FNullEnt(pentChangelevel))
		return 0;
	while (!FNullEnt(pentChangelevel))
	{
		CChangeLevel* pTrigger;

		pTrigger = GetClassPtr((CChangeLevel*)VARS(pentChangelevel));
		if (pTrigger)
		{
			// Find the corresponding landmark
			pentLandmark = FindLandmark(pTrigger->m_szLandmarkName);
			if (pentLandmark)
			{
				// Build a list of unique transitions
				if (AddTransitionToList(pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark))
				{
					count++;
					if (count >= maxList) // FULL!!
						break;
				}
			}
		}
		pentChangelevel = FIND_ENTITY_BY_STRING(pentChangelevel, "classname", "trigger_changelevel");
	}

	//Token table is null at this point, so don't use CSaveRestoreBuffer::IsValidSaveRestoreData here.
	if (auto pSaveData = reinterpret_cast<SAVERESTOREDATA*>(gpGlobals->pSaveData);
		nullptr != pSaveData && pSaveData->pTable)
	{
		CSave saveHelper(*pSaveData);

		for (i = 0; i < count; i++)
		{
			int j, entityCount = 0;
			CBaseEntity* pEntList[MAX_ENTITY];
			int entityFlags[MAX_ENTITY];

			// Follow the linked list of entities in the PVS of the transition landmark
			edict_t* pent = UTIL_EntitiesInPVS(pLevelList[i].pentLandmark);

			// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
			while (!FNullEnt(pent))
			{
				CBaseEntity* pEntity = CBaseEntity::Instance(pent);
				if (pEntity)
				{
					// Logger->debug("Trying {}", STRING(pEntity->pev->classname));
					int caps = pEntity->ObjectCaps();
					if ((caps & FCAP_DONT_SAVE) == 0)
					{
						int flags = 0;

						// If this entity can be moved or is global, mark it
						if ((caps & FCAP_ACROSS_TRANSITION) != 0)
							flags |= FENTTABLE_MOVEABLE;
						if (!FStringNull(pEntity->pev->globalname) && !pEntity->IsDormant())
							flags |= FENTTABLE_GLOBAL;
						if (0 != flags)
						{
							pEntList[entityCount] = pEntity;
							entityFlags[entityCount] = flags;
							entityCount++;
							if (entityCount > MAX_ENTITY)
								Logger->error("Too many entities across a transition!");
						}
						//else
						//	Logger->debug("Failed {}", STRING(pEntity->pev->classname));
					}
					//else
					//	Logger->debug("DON'T SAVE {}", STRING(pEntity->pev->classname));
				}
				pent = pent->v.chain;
			}

			for (j = 0; j < entityCount; j++)
			{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if (0 != entityFlags[j] && InTransitionVolume(pEntList[j], pLevelList[i].landmarkName))
				{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex(pEntList[j]);
					// Flag it with the level number
					saveHelper.EntityFlagsSet(index, entityFlags[j] | (1 << i));
				}
				//else
				//	Logger->debug("Screened out {}", STRING(pEntList[j]->pev->classname));
			}
		}
	}

	return count;
}

// ============================== LADDER =======================================

class CLadder : public CBaseTrigger
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Precache() override;
};
LINK_ENTITY_TO_CLASS(func_ladder, CLadder);


bool CLadder::KeyValue(KeyValueData* pkvd)
{
	return CBaseTrigger::KeyValue(pkvd);
}


//=========================================================
// func_ladder - makes an area vertically negotiable
//=========================================================
void CLadder::Precache()
{
	// Do all of this in here because we need to 'convert' old saved games
	pev->solid = SOLID_NOT;
	pev->skin = CONTENTS_LADDER;
	if (CVAR_GET_FLOAT("showtriggers") == 0)
	{
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
	}
	pev->effects &= ~EF_NODRAW;
}


void CLadder::Spawn()
{
	Precache();

	SetModel(STRING(pev->model)); // set size and link into world
	pev->movetype = MOVETYPE_PUSH;
}


// ========================== A TRIGGER THAT PUSHES YOU ===============================

class CTriggerPush : public CBaseTrigger
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Touch(CBaseEntity* pOther) override;
};
LINK_ENTITY_TO_CLASS(trigger_push, CTriggerPush);


bool CTriggerPush::KeyValue(KeyValueData* pkvd)
{
	return CBaseTrigger::KeyValue(pkvd);
}


/*QUAKED trigger_push (.5 .5 .5) ? TRIG_PUSH_ONCE
Pushes the player
*/

void CTriggerPush::Spawn()
{
	if (pev->angles == g_vecZero)
		pev->angles.y = 360;
	InitTrigger();

	if (pev->speed == 0)
		pev->speed = 100;

	if (FBitSet(pev->spawnflags, SF_TRIGGER_PUSH_START_OFF)) // if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	SetUse(&CTriggerPush::ToggleUse);

	UTIL_SetOrigin(pev, pev->origin); // Link into the list
}


void CTriggerPush::Touch(CBaseEntity* pOther)
{
	entvars_t* pevToucher = pOther->pev;

	// UNDONE: Is there a better way than health to detect things that have physics? (clients/monsters)
	switch (pevToucher->movetype)
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
	case MOVETYPE_FOLLOW:
		return;
	}

	if (pevToucher->solid != SOLID_NOT && pevToucher->solid != SOLID_BSP)
	{
		// Instant trigger, just transfer velocity and remove
		if (FBitSet(pev->spawnflags, SF_TRIG_PUSH_ONCE))
		{
			pevToucher->velocity = pevToucher->velocity + (pev->speed * pev->movedir);
			if (pevToucher->velocity.z > 0)
				pevToucher->flags &= ~FL_ONGROUND;
			UTIL_Remove(this);
		}
		else
		{ // Push field, transfer to base velocity
			Vector vecPush = (pev->speed * pev->movedir);
			if ((pevToucher->flags & FL_BASEVELOCITY) != 0)
				vecPush = vecPush + pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;

			pevToucher->flags |= FL_BASEVELOCITY;
			// Logger->debug("Vel {}, base {}", pevToucher->velocity.z, pevToucher->basevelocity.z);
		}
	}
}


//======================================
// teleport trigger
//
//

void CBaseTrigger::TeleportTouch(CBaseEntity* pOther)
{
	entvars_t* pevToucher = pOther->pev;
	edict_t* pentTarget = nullptr;

	// Only teleport monsters or clients
	if (!FBitSet(pevToucher->flags, FL_CLIENT | FL_MONSTER))
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	if ((pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS) == 0)
	{ // no monsters allowed!
		if (FBitSet(pevToucher->flags, FL_MONSTER))
		{
			return;
		}
	}

	if ((pev->spawnflags & SF_TRIGGER_NOCLIENTS) != 0)
	{ // no clients allowed
		if (pOther->IsPlayer())
		{
			return;
		}
	}

	pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->target));
	if (FNullEnt(pentTarget))
		return;

	Vector tmp = VARS(pentTarget)->origin;

	if (pOther->IsPlayer())
	{
		tmp.z -= pOther->pev->mins.z; // make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin(pevToucher, tmp);

	pevToucher->angles = pentTarget->v.angles;

	if (pOther->IsPlayer())
	{
		pevToucher->v_angle = pentTarget->v.angles;
	}

	pevToucher->fixangle = 1;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
}


class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn() override;
};
LINK_ENTITY_TO_CLASS(trigger_teleport, CTriggerTeleport);

void CTriggerTeleport::Spawn()
{
	InitTrigger();

	SetTouch(&CTriggerTeleport::TeleportTouch);
}


LINK_ENTITY_TO_CLASS(info_teleport_destination, CPointEntity);



class CTriggerSave : public CBaseTrigger
{
public:
	void Spawn() override;
	void EXPORT SaveTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(trigger_autosave, CTriggerSave);

void CTriggerSave::Spawn()
{
	if (g_pGameRules->IsDeathmatch())
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	InitTrigger();
	SetTouch(&CTriggerSave::SaveTouch);
}

void CTriggerSave::SaveTouch(CBaseEntity* pOther)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	// Only save on clients
	if (!pOther->IsPlayer())
		return;

	SetTouch(nullptr);
	UTIL_Remove(this);
	SERVER_COMMAND("autosave\n");
}

#define SF_ENDSECTION_USEONLY 0x0001

class CTriggerEndSection : public CBaseTrigger
{
public:
	void Spawn() override;
	void EXPORT EndSectionTouch(CBaseEntity* pOther);
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT EndSectionUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
};
LINK_ENTITY_TO_CLASS(trigger_endsection, CTriggerEndSection);


void CTriggerEndSection::EndSectionUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Only save on clients
	if (pActivator && !pActivator->IsNetClient())
		return;

	SetUse(nullptr);

	if (!FStringNull(pev->message))
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	UTIL_Remove(this);
}

void CTriggerEndSection::Spawn()
{
	if (g_pGameRules->IsDeathmatch())
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	InitTrigger();

	SetUse(&CTriggerEndSection::EndSectionUse);
	// If it is a "use only" trigger, then don't set the touch function.
	if ((pev->spawnflags & SF_ENDSECTION_USEONLY) == 0)
		SetTouch(&CTriggerEndSection::EndSectionTouch);
}

void CTriggerEndSection::EndSectionTouch(CBaseEntity* pOther)
{
	// Only save on clients
	if (!pOther->IsNetClient())
		return;

	SetTouch(nullptr);

	if (!FStringNull(pev->message))
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	UTIL_Remove(this);
}

bool CTriggerEndSection::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "section"))
	{
		//		m_iszSectionName = ALLOC_STRING( pkvd->szValue );
		// Store this in message so we don't have to write save/restore for this ent
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseTrigger::KeyValue(pkvd);
}


class CTriggerGravity : public CBaseTrigger
{
public:
	void Spawn() override;
	void EXPORT GravityTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(trigger_gravity, CTriggerGravity);

void CTriggerGravity::Spawn()
{
	InitTrigger();
	SetTouch(&CTriggerGravity::GravityTouch);
}

void CTriggerGravity::GravityTouch(CBaseEntity* pOther)
{
	// Only save on clients
	if (!pOther->IsPlayer())
		return;

	pOther->pev->gravity = pev->gravity;
}







// this is a really bad idea.
class CTriggerChangeTarget : public CBaseDelay
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

private:
	string_t m_iszNewTarget;
};
LINK_ENTITY_TO_CLASS(trigger_changetarget, CTriggerChangeTarget);

TYPEDESCRIPTION CTriggerChangeTarget::m_SaveData[] =
	{
		DEFINE_FIELD(CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeTarget, CBaseDelay);

bool CTriggerChangeTarget::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszNewTarget"))
	{
		m_iszNewTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseDelay::KeyValue(pkvd);
}

void CTriggerChangeTarget::Spawn()
{
}


void CTriggerChangeTarget::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pTarget = UTIL_FindEntityByString(nullptr, "targetname", STRING(pev->target));

	if (pTarget)
	{
		pTarget->pev->target = m_iszNewTarget;
		CBaseMonster* pMonster = pTarget->MyMonsterPointer();
		if (pMonster)
		{
			pMonster->m_pGoalEnt = nullptr;
		}
	}
}




#define SF_CAMERA_PLAYER_POSITION 1
#define SF_CAMERA_PLAYER_TARGET 2
#define SF_CAMERA_PLAYER_TAKECONTROL 4

class CTriggerCamera : public CBaseDelay
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT FollowTarget();
	void Move();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	CBaseEntity* m_pentPath;
	string_t m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	bool m_state;
};
LINK_ENTITY_TO_CLASS(trigger_camera, CTriggerCamera);

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION CTriggerCamera::m_SaveData[] =
	{
		DEFINE_FIELD(CTriggerCamera, m_hPlayer, FIELD_EHANDLE),
		DEFINE_FIELD(CTriggerCamera, m_hTarget, FIELD_EHANDLE),
		DEFINE_FIELD(CTriggerCamera, m_pentPath, FIELD_CLASSPTR),
		DEFINE_FIELD(CTriggerCamera, m_sPath, FIELD_STRING),
		DEFINE_FIELD(CTriggerCamera, m_flWait, FIELD_FLOAT),
		DEFINE_FIELD(CTriggerCamera, m_flReturnTime, FIELD_TIME),
		DEFINE_FIELD(CTriggerCamera, m_flStopTime, FIELD_TIME),
		DEFINE_FIELD(CTriggerCamera, m_moveDistance, FIELD_FLOAT),
		DEFINE_FIELD(CTriggerCamera, m_targetSpeed, FIELD_FLOAT),
		DEFINE_FIELD(CTriggerCamera, m_initialSpeed, FIELD_FLOAT),
		DEFINE_FIELD(CTriggerCamera, m_acceleration, FIELD_FLOAT),
		DEFINE_FIELD(CTriggerCamera, m_deceleration, FIELD_FLOAT),
		DEFINE_FIELD(CTriggerCamera, m_state, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CTriggerCamera, CBaseDelay);

void CTriggerCamera::Spawn()
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT; // Remove model & collisions
	pev->renderamt = 0;		// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;

	m_initialSpeed = pev->speed;
	if (m_acceleration == 0)
		m_acceleration = 500;
	if (m_deceleration == 0)
		m_deceleration = 500;
}


bool CTriggerCamera::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "moveto"))
	{
		m_sPath = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "acceleration"))
	{
		m_acceleration = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "deceleration"))
	{
		m_deceleration = atof(pkvd->szValue);
		return true;
	}

	return CBaseDelay::KeyValue(pkvd);
}



void CTriggerCamera::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_state))
		return;

	// Toggle state
	m_state = !m_state;
	if (!m_state)
	{
		m_flReturnTime = gpGlobals->time;
		return;
	}
	if (!pActivator || !pActivator->IsPlayer())
	{
		pActivator = UTIL_GetLocalPlayer();

		if (!pActivator)
		{
			return;
		}
	}

	auto player = static_cast<CBasePlayer*>(pActivator);

	m_hPlayer = pActivator;

	m_flReturnTime = gpGlobals->time + m_flWait;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TARGET))
	{
		m_hTarget = m_hPlayer;
	}
	else
	{
		m_hTarget = GetNextTarget();
	}

	// Nothing to look at!
	if (m_hTarget == nullptr)
	{
		return;
	}


	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL))
	{
		player->EnableControl(false);
	}

	if (!FStringNull(m_sPath))
	{
		m_pentPath = Instance(FIND_ENTITY_BY_TARGETNAME(nullptr, STRING(m_sPath)));
	}
	else
	{
		m_pentPath = nullptr;
	}

	m_flStopTime = gpGlobals->time;
	if (m_pentPath)
	{
		if (m_pentPath->pev->speed != 0)
			m_targetSpeed = m_pentPath->pev->speed;

		m_flStopTime += m_pentPath->GetDelay();
	}

	// copy over player information
	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_POSITION))
	{
		UTIL_SetOrigin(pev, pActivator->pev->origin + pActivator->pev->view_ofs);
		pev->angles.x = -pActivator->pev->angles.x;
		pev->angles.y = pActivator->pev->angles.y;
		pev->angles.z = 0;
		pev->velocity = pActivator->pev->velocity;
	}
	else
	{
		pev->velocity = Vector(0, 0, 0);
	}

	SET_VIEW(pActivator->edict(), edict());

	player->m_hViewEntity = this;

	SetModel(STRING(pActivator->pev->model));

	// follow the player down
	SetThink(&CTriggerCamera::FollowTarget);
	pev->nextthink = gpGlobals->time;

	m_moveDistance = 0;
	Move();
}


void CTriggerCamera::FollowTarget()
{
	if (m_hPlayer == nullptr)
		return;

	if (m_hTarget == nullptr || m_flReturnTime < gpGlobals->time)
	{
		auto player = static_cast<CBasePlayer*>(static_cast<CBaseEntity*>(m_hPlayer));

		if (player->IsAlive())
		{
			SET_VIEW(player->edict(), player->edict());
			player->EnableControl(true);
		}

		player->m_hViewEntity = nullptr;
		player->m_bResetViewEntity = false;

		SUB_UseTargets(this, USE_TOGGLE, 0);
		pev->avelocity = Vector(0, 0, 0);
		m_state = false;
		return;
	}

	Vector vecGoal = UTIL_VecToAngles(m_hTarget->pev->origin - pev->origin);
	vecGoal.x = -vecGoal.x;

	if (pev->angles.y > 360)
		pev->angles.y -= 360;

	if (pev->angles.y < 0)
		pev->angles.y += 360;

	float dx = vecGoal.x - pev->angles.x;
	float dy = vecGoal.y - pev->angles.y;

	if (dx < -180)
		dx += 360;
	if (dx > 180)
		dx = dx - 360;

	if (dy < -180)
		dy += 360;
	if (dy > 180)
		dy = dy - 360;

	pev->avelocity.x = dx * 40 * 0.01;
	pev->avelocity.y = dy * 40 * 0.01;


	if (!(FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL)))
	{
		pev->velocity = pev->velocity * 0.8;
		if (pev->velocity.Length() < 10.0)
			pev->velocity = g_vecZero;
	}

	pev->nextthink = gpGlobals->time;

	Move();
}

void CTriggerCamera::Move()
{
	// Not moving on a path, return
	if (!m_pentPath)
		return;

	// Subtract movement from the previous frame
	m_moveDistance -= pev->speed * gpGlobals->frametime;

	// Have we moved enough to reach the target?
	if (m_moveDistance <= 0)
	{
		// Fire the passtarget if there is one
		if (!FStringNull(m_pentPath->pev->message))
		{
			FireTargets(STRING(m_pentPath->pev->message), this, this, USE_TOGGLE, 0);
			if (FBitSet(m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE))
				m_pentPath->pev->message = string_t::Null;
		}
		// Time to go to the next target
		m_pentPath = m_pentPath->GetNextTarget();

		// Set up next corner
		if (!m_pentPath)
		{
			pev->velocity = g_vecZero;
		}
		else
		{
			if (m_pentPath->pev->speed != 0)
				m_targetSpeed = m_pentPath->pev->speed;

			Vector delta = m_pentPath->pev->origin - pev->origin;
			m_moveDistance = delta.Length();
			pev->movedir = delta.Normalize();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();
		}
	}

	if (m_flStopTime > gpGlobals->time)
		pev->speed = UTIL_Approach(0, pev->speed, m_deceleration * gpGlobals->frametime);
	else
		pev->speed = UTIL_Approach(m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime);

	float fraction = 2 * gpGlobals->frametime;
	pev->velocity = ((pev->movedir * pev->speed) * fraction) + (pev->velocity * (1 - fraction));
}

class CTriggerPlayerFreeze : public CBaseDelay
{
public:
	static TYPEDESCRIPTION m_SaveData[];

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	void Spawn() override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	void EXPORT PlayerFreezeDelay();

public:
	bool m_bUnFrozen;
};

TYPEDESCRIPTION CTriggerPlayerFreeze::m_SaveData[] =
	{
		DEFINE_FIELD(CTriggerPlayerFreeze, m_bUnFrozen, FIELD_BOOLEAN),
};

LINK_ENTITY_TO_CLASS(trigger_playerfreeze, CTriggerPlayerFreeze);

bool CTriggerPlayerFreeze::Save(CSave& save)
{
	if (!CBaseDelay::Save(save))
		return false;

	return save.WriteFields("CTriggerPlayerFreeze", this, m_SaveData, std::size(m_SaveData));
}

bool CTriggerPlayerFreeze::Restore(CRestore& restore)
{
	if (!CBaseDelay::Restore(restore))
		return false;

	if (!restore.ReadFields("CTriggerPlayerFreeze", this, m_SaveData, std::size(m_SaveData)))
		return false;

	if (!m_bUnFrozen)
	{
		SetThink(&CTriggerPlayerFreeze::PlayerFreezeDelay);
		pev->nextthink = gpGlobals->time + 0.5;
	}

	return true;
}

void CTriggerPlayerFreeze::Spawn()
{
	if (g_pGameRules->IsDeathmatch())
		REMOVE_ENTITY(edict());
	else
		m_bUnFrozen = true;
}

void CTriggerPlayerFreeze::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_bUnFrozen = !m_bUnFrozen;

	//TODO: not made for multiplayer
	auto pPlayer = UTIL_GetLocalPlayer();

	if (!pPlayer)
	{
		return;
	}

	pPlayer->EnableControl(m_bUnFrozen);
}

void CTriggerPlayerFreeze::PlayerFreezeDelay()
{
	auto player = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(1));

	if (player)
		player->EnableControl(m_bUnFrozen);

	SetThink(nullptr);
}

/**
*	@brief Kills anything that touches it without gibbing it
*/
class CTriggerKillNoGib : public CBaseTrigger
{
public:
	void Spawn() override;

	void KillTouch(CBaseEntity* pOther);
};

LINK_ENTITY_TO_CLASS(trigger_kill_nogib, CTriggerKillNoGib);

void CTriggerKillNoGib::Spawn()
{
	InitTrigger();

	SetTouch(&CTriggerKillNoGib::KillTouch);
	SetUse(nullptr);

	//TODO: this needs to be removed in order to function
	pev->solid = SOLID_NOT;
}

void CTriggerKillNoGib::KillTouch(CBaseEntity* pOther)
{
	if (pOther->pev->takedamage != DAMAGE_NO)
		pOther->TakeDamage(pev, pOther->pev, 500000, DMG_NEVERGIB);
}

class CTriggerXenReturn : public CBaseTrigger
{
public:
	using BaseClass = CBaseTrigger;

	void Spawn() override;

	void EXPORT ReturnTouch(CBaseEntity* pOther);
};

LINK_ENTITY_TO_CLASS(trigger_xen_return, CTriggerXenReturn);

LINK_ENTITY_TO_CLASS(info_displacer_earth_target, CPointEntity);
LINK_ENTITY_TO_CLASS(info_displacer_xen_target, CPointEntity);

void CTriggerXenReturn::Spawn()
{
	InitTrigger();

	SetTouch(&CTriggerXenReturn::ReturnTouch);
	SetUse(nullptr);
}

void CTriggerXenReturn::ReturnTouch(CBaseEntity* pOther)
{
	if (!pOther->IsPlayer())
		return;

	auto pPlayer = static_cast<CBasePlayer*>(pOther);

	float flDist = 8192;

	CBaseEntity* pTarget = nullptr;

	//Find the earth target nearest to the player's original location.
	for (auto pDestination : UTIL_FindEntitiesByClassname("info_displacer_earth_target"))
	{
		const float flThisDist = (pPlayer->m_DisplacerReturn - pDestination->pev->origin).Length();

		if (flDist > flThisDist)
		{
			pTarget = pDestination;

			flDist = flThisDist;
		}
	}

	if (pTarget && !FNullEnt(pTarget->pev))
	{
		pPlayer->pev->flags &= ~FL_SKIPLOCALHOST;

		auto vecDest = pTarget->pev->origin;

		vecDest.z -= pPlayer->pev->mins.z;
		vecDest.z += 1;

		UTIL_SetOrigin(pPlayer->pev, vecDest);

		pPlayer->pev->angles = pTarget->pev->angles;
		pPlayer->pev->v_angle = pTarget->pev->angles;
		pPlayer->pev->fixangle = 1;

		pPlayer->pev->basevelocity = g_vecZero;
		pPlayer->pev->velocity = g_vecZero;

		pPlayer->pev->gravity = 1.0;

		pPlayer->m_SndRoomtype = pPlayer->m_DisplacerSndRoomtype;

		EMIT_SOUND(pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_self.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
	}
}

const auto SF_GENEWORM_HIT_TARGET_ONCE = 1 << 0;
const auto SF_GENEWORM_HIT_START_OFF = 1 << 1;
const auto SF_GENEWORM_HIT_NO_CLIENTS = 1 << 3;
const auto SF_GENEWORM_HIT_FIRE_CLIENT_ONLY = 1 << 4;
const auto SF_GENEWORM_HIT_TOUCH_CLIENT_ONLY = 1 << 5;

class COFTriggerGeneWormHit : public CBaseTrigger
{
public:
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void Precache() override;
	void Spawn() override;

	void EXPORT GeneWormHitTouch(CBaseEntity* pOther);

	static const char* pAttackSounds[];

	float m_flLastDamageTime;
};

TYPEDESCRIPTION COFTriggerGeneWormHit::m_SaveData[] =
	{
		DEFINE_FIELD(COFTriggerGeneWormHit, m_flLastDamageTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(COFTriggerGeneWormHit, CBaseTrigger);

const char* COFTriggerGeneWormHit::pAttackSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav"};

LINK_ENTITY_TO_CLASS(trigger_geneworm_hit, COFTriggerGeneWormHit);

void COFTriggerGeneWormHit::Precache()
{
	PRECACHE_SOUND_ARRAY(pAttackSounds);
}

void COFTriggerGeneWormHit::Spawn()
{
	Precache();

	InitTrigger();

	SetTouch(&COFTriggerGeneWormHit::GeneWormHitTouch);

	if (!FStringNull(pev->targetname))
	{
		SetUse(&COFTriggerGeneWormHit::ToggleUse);
	}
	else
	{
		SetUse(nullptr);
	}

	if ((pev->spawnflags & SF_GENEWORM_HIT_START_OFF) != 0)
	{
		pev->solid = SOLID_NOT;
	}

	UTIL_SetOrigin(pev, pev->origin);

	pev->dmg = GetSkillFloat("geneworm_dmg_hit"sv);
	m_flLastDamageTime = gpGlobals->time;
}

void COFTriggerGeneWormHit::GeneWormHitTouch(CBaseEntity* pOther)
{
	if (gpGlobals->time - m_flLastDamageTime >= 2 && pOther->pev->takedamage != DAMAGE_NO)
	{
		if ((pev->spawnflags & SF_GENEWORM_HIT_TOUCH_CLIENT_ONLY) != 0)
		{
			if (!pOther->IsPlayer())
				return;
		}

		if ((pev->spawnflags & SF_GENEWORM_HIT_NO_CLIENTS) == 0 || !pOther->IsPlayer())
		{
			if (!g_pGameRules->IsMultiplayer())
			{
				if (pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished)
					return;
			}
			else if (pev->dmgtime <= gpGlobals->time)
			{
				pev->impulse = 0;
				if (pOther->IsPlayer())
					pev->impulse |= 1 << (pOther->entindex() - 1);
			}
			else if (gpGlobals->time != pev->pain_finished)
			{
				if (!pOther->IsPlayer())
					return;

				const auto playerBit = 1 << (pOther->entindex() - 1);

				if ((pev->impulse & playerBit) != 0)
					return;

				pev->impulse |= playerBit;
			}

			pOther->TakeDamage(pev, pev, pev->dmg, DMG_CRUSH);

			EMIT_SOUND_DYN(pOther->edict(), CHAN_BODY, pAttackSounds[RANDOM_LONG(0, std::size(pAttackSounds) - 1)], VOL_NORM, 0.1, 0, RANDOM_LONG(-5, 5) + 100);

			pev->pain_finished = gpGlobals->time;
			m_flLastDamageTime = gpGlobals->time;

			if (!FStringNull(pev->target) &&
				((pev->spawnflags & SF_GENEWORM_HIT_FIRE_CLIENT_ONLY) == 0 || pOther->IsPlayer()))
			{
				SUB_UseTargets(pOther, USE_TOGGLE, 0);

				if ((pev->spawnflags & SF_GENEWORM_HIT_TARGET_ONCE) != 0)
					pev->target = string_t::Null;
			}
		}
	}
}

class CTriggerCTFGeneric : public CBaseTrigger
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	void Spawn() override;

	void Touch(CBaseEntity* pOther) override;

	bool KeyValue(KeyValueData* pkvd) override;

	USE_TYPE triggerType;
	CTFTeam team_no;
	float trigger_delay;
	float m_flTriggerDelayTime;
	int score;
	int team_score;
};

LINK_ENTITY_TO_CLASS(trigger_ctfgeneric, CTriggerCTFGeneric);

void CTriggerCTFGeneric::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	Touch(nullptr);
}

void CTriggerCTFGeneric::Spawn()
{
	InitTrigger();

	m_flTriggerDelayTime = 0;
}

void CTriggerCTFGeneric::Touch(CBaseEntity* pOther)
{
	if (m_flTriggerDelayTime <= gpGlobals->time)
	{
		CBasePlayer* pOtherPlayer = nullptr;

		if (pOther)
		{
			if (0 != score || !pOther->IsPlayer())
			{
				return;
			}

			pOtherPlayer = static_cast<CBasePlayer*>(pOther);

			if (team_no != CTFTeam::None && team_no != pOtherPlayer->m_iTeamNum)
			{
				return;
			}
		}

		SUB_UseTargets(this, triggerType, 0);

		//TODO: constrain team_no input to valid values
		if (0 != team_score)
			teamscores[static_cast<int>(team_no) - 1] += team_score;

		if (pOtherPlayer && score != 0)
		{
			pOtherPlayer->m_iCTFScore += score;
			pOtherPlayer->m_iOffense += score;
			g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgCTFScore, nullptr, nullptr);
			g_engfuncs.pfnWriteByte(pOtherPlayer->entindex());
			g_engfuncs.pfnWriteByte(pOtherPlayer->m_iCTFScore);
			g_engfuncs.pfnMessageEnd();

			pOtherPlayer->SendScoreInfoAll();

			UTIL_LogPrintf(
				"\"%s<%i><%u><%s>\" triggered \"%s\"\n",
				STRING(pOtherPlayer->pev->targetname),
				g_engfuncs.pfnGetPlayerUserId(pOtherPlayer->edict()),
				g_engfuncs.pfnGetPlayerWONId(pOtherPlayer->edict()),
				GetTeamName(pOtherPlayer->edict()),
				STRING(pev->targetname));
		}

		if (0 != team_score)
		{
			//TOOD: not sure why this check is here since pev must be valid if the entity exists
			if (!pOther && 0 == score && pev)
			{
				UTIL_LogPrintf("World triggered \"%s\"\n", STRING(pev->targetname));
			}

			DisplayTeamFlags(nullptr);
		}

		m_flTriggerDelayTime = gpGlobals->time + trigger_delay;
	}
}

bool CTriggerCTFGeneric::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("team_no", pkvd->szKeyName))
	{
		team_no = static_cast<CTFTeam>(atoi(pkvd->szValue));
		return true;
	}
	else if (FStrEq("trigger_delay", pkvd->szKeyName))
	{
		trigger_delay = atof(pkvd->szValue);

		if (trigger_delay == 0)
			trigger_delay = 5;

		return true;
	}
	else if (FStrEq("score", pkvd->szKeyName))
	{
		score = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq("team_score", pkvd->szKeyName))
	{
		team_score = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq("triggerstate", pkvd->szKeyName))
	{
		switch (atoi(pkvd->szValue))
		{
		case 1:
			triggerType = USE_ON;
			break;

		case 2:
			triggerType = USE_TOGGLE;
			break;

		default:
			triggerType = USE_OFF;
			break;
		}

		return true;
	}

	return false;
}
