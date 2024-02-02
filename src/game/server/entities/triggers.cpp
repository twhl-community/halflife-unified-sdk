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

/**
 *	@file
 *	spawn and use functions for editor-placed triggers
 */

#include "cbase.h"
#include "CBaseTrigger.h"
#include "trains.h" // trigger_camera has train functionality
#include "CHalfLifeCTFplay.h"
#include "ctf/ctf_goals.h"
#include "UserMessages.h"

#define SF_TRIGGER_PUSH_START_OFF 2		   // spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_TARGETONCE 1	   // Only fire hurt target once
#define SF_TRIGGER_HURT_START_OFF 2		   // spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_NO_CLIENTS 8	   // spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE 16  // trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32 // only clients may touch this trigger.

/**
 *	@brief Modifies an entity's friction
 */
class CFrictionModifier : public CBaseEntity
{
	DECLARE_CLASS(CFrictionModifier, CBaseEntity);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;

	/**
	 *	@brief Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
	 */
	void ChangeFriction(CBaseEntity* pOther);

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	float m_frictionFraction; // Sorry, couldn't resist this name :)
};

LINK_ENTITY_TO_CLASS(func_friction, CFrictionModifier);

BEGIN_DATAMAP(CFrictionModifier)
DEFINE_FIELD(m_frictionFraction, FIELD_FLOAT),
	DEFINE_FUNCTION(ChangeFriction),
	END_DATAMAP();

void CFrictionModifier::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	SetModel(STRING(pev->model)); // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch(&CFrictionModifier::ChangeFriction);
}

void CFrictionModifier::ChangeFriction(CBaseEntity* pOther)
{
	if (pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE)
		pOther->pev->friction = m_frictionFraction;
}

bool CFrictionModifier::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "modifier"))
	{
		m_frictionFraction = atof(pkvd->szValue) / 100.0;
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

#define SF_AUTO_FIREONCE 0x0001

/**
 *	@brief This trigger will fire when the level spawns (or respawns if not fire once)
 *	It will check a global state before firing. It supports delay and killtargets
 */
class CAutoTrigger : public CBaseDelay
{
	DECLARE_CLASS(CAutoTrigger, CBaseDelay);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Precache() override;
	void Think() override;

	int ObjectCaps() override { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	string_t m_globalstate;
	USE_TYPE triggerType;
};

LINK_ENTITY_TO_CLASS(trigger_auto, CAutoTrigger);

BEGIN_DATAMAP(CAutoTrigger)
DEFINE_FIELD(m_globalstate, FIELD_STRING),
	DEFINE_FIELD(triggerType, FIELD_INTEGER),
	END_DATAMAP();

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
	DECLARE_CLASS(CTriggerRelay, CBaseDelay);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	USE_TYPE triggerType;
};

LINK_ENTITY_TO_CLASS(trigger_relay, CTriggerRelay);

BEGIN_DATAMAP(CTriggerRelay)
DEFINE_FIELD(triggerType, FIELD_INTEGER),
	END_DATAMAP();

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
	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator, m_UseLocked))
	{
		return;
	}

	SUB_UseTargets(this, triggerType, 0);
	if ((pev->spawnflags & SF_RELAY_FIREONCE) != 0)
		UTIL_Remove(this);
}

// Note since this is 0 clones will also clone themselves.
#define SF_MULTIMAN_CLONE 0x80000000
#define SF_MULTIMAN_THREAD 0x00000001

constexpr int MAX_MULTI_TARGETS = 64; // maximum number of targets a single multi_manager entity may be assigned.

/**
 *	@brief when fired, will fire up to MAX_MULTI_TARGETS targets at specified times.
 *	@details FLAG: THREAD (create clones when triggered)
 *	FLAG: CLONE (this is a clone for a threaded execution)
 */
class CMultiManager : public CBaseToggle
{
	DECLARE_CLASS(CMultiManager, CBaseToggle);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void ManagerThink();
	void ManagerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

#if _DEBUG
	void ManagerReport();
#endif

	bool HasTarget(string_t targetname) override;

	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	int m_cTargets;							   // the total number of targets in this manager's fire list.
	int m_index;							   // Current target
	float m_startTime;						   // Time we started firing
	string_t m_iTargetName[MAX_MULTI_TARGETS]; // list if indexes into global string array
	float m_flTargetDelay[MAX_MULTI_TARGETS];  // delay (in seconds) from time of manager fire to target fire
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

BEGIN_DATAMAP(CMultiManager)
DEFINE_FIELD(m_cTargets, FIELD_INTEGER),
	DEFINE_FIELD(m_index, FIELD_INTEGER),
	DEFINE_FIELD(m_startTime, FIELD_TIME),
	DEFINE_ARRAY(m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS),
	DEFINE_ARRAY(m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS),
	DEFINE_FUNCTION(ManagerThink),
	DEFINE_FUNCTION(ManagerUse),

#if _DEBUG
	DEFINE_FUNCTION(ManagerReport),
#endif
	END_DATAMAP();

bool CMultiManager::KeyValue(KeyValueData* pkvd)
{
	// UNDONE: Maybe this should do something like this:
	// CBaseToggle::KeyValue( pkvd );
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
		else
		{
			Logger->warn("{}:{}: Too many targets! (> {})", GetClassname(), entindex(), MAX_MULTI_TARGETS);
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
	CMultiManager* pMulti = g_EntityDictionary->Create<CMultiManager>("multi_manager");

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

	IOLogger->trace("{}:{}:{} started", GetClassname(), entindex(), GetTargetname());
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

// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX (1 << 0)
#define SF_RENDER_MASKAMT (1 << 1)
#define SF_RENDER_MASKMODE (1 << 2)
#define SF_RENDER_MASKCOLOR (1 << 3)

/**
 *	@brief This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
 *	to its targets when triggered.
 */
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
		for (auto target : UTIL_FindEntitiesByTargetname(STRING(pev->target)))
		{
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKFX))
				target->pev->renderfx = pev->renderfx;
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKAMT))
				target->pev->renderamt = pev->renderamt;
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKMODE))
				target->pev->rendermode = pev->rendermode;
			if (!FBitSet(pev->spawnflags, SF_RENDER_MASKCOLOR))
				target->pev->rendercolor = pev->rendercolor;
		}
	}
}

/**
 *	@brief hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
 */
class CTriggerHurt : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerHurt, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;

	/**
	 *	@brief When touched, a hurt trigger does DMG points of damage each half-second
	 */
	void HurtTouch(CBaseEntity* pOther);

	/**
	 *	@brief trigger hurt that causes radiation will do a radius check and set the player's geiger counter level
	 *	according to distance from center of trigger
	 */
	void RadiationThink();

private:
	int m_bitsDamageInflict; // DMG_ damage type that the trigger does
};

BEGIN_DATAMAP(CTriggerHurt)
DEFINE_FIELD(m_bitsDamageInflict, FIELD_INTEGER),
	DEFINE_FUNCTION(HurtTouch),
	DEFINE_FUNCTION(RadiationThink),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(trigger_hurt, CTriggerHurt);

bool CTriggerHurt::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		m_bitsDamageInflict = atoi(pkvd->szValue);
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

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

	SetOrigin(pev->origin); // Link into the list
}

void CTriggerHurt::HurtTouch(CBaseEntity* pOther)
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
		pOther->GiveHealth(-fldmg, m_bitsDamageInflict);
	else
		pOther->TakeDamage(this, this, fldmg, m_bitsDamageInflict);

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

void CTriggerHurt::RadiationThink()
{
	float flRange;
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

	CBasePlayer* player = UTIL_FindClientInPVS(this);

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	// reset origin

	if (player)
	{
		// get range to player;

		vecSpot1 = (pev->absmin + pev->absmax) * 0.5;
		vecSpot2 = (player->pev->absmin + player->pev->absmax) * 0.5;

		vecRange = vecSpot1 - vecSpot2;
		flRange = vecRange.Length();

		// if player's current geiger counter range is larger
		// than range to this trigger hurt, reset player's
		// geiger counter range

		if (player->m_flgeigerRange >= flRange)
			player->m_flgeigerRange = flRange;
	}

	pev->nextthink = gpGlobals->time + 0.25;
}

class CTriggerMonsterJump : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerMonsterJump, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Touch(CBaseEntity* pOther) override;
	void Think() override;

private:
	float m_flHeight;
};

BEGIN_DATAMAP(CTriggerMonsterJump)
DEFINE_FIELD(m_flHeight, FIELD_FLOAT),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(trigger_monsterjump, CTriggerMonsterJump);

void CTriggerMonsterJump::Spawn()
{
	SetMovedir(this);

	InitTrigger();

	pev->nextthink = 0;
	pev->speed = 200;
	m_flHeight = 150;

	if (!FStringNull(pev->targetname))
	{ // if targetted, spawn turned off
		pev->solid = SOLID_NOT;
		SetOrigin(pev->origin); // Unlink from trigger list
		SetUse(&CTriggerMonsterJump::ToggleUse);
	}
}

void CTriggerMonsterJump::Think()
{
	pev->solid = SOLID_NOT; // kill the trigger for now !!!UNDONE
	SetOrigin(pev->origin); // Unlink from trigger list
	SetThink(nullptr);
}

void CTriggerMonsterJump::Touch(CBaseEntity* pOther)
{
	if (!FBitSet(pOther->pev->flags, FL_MONSTER))
	{ // touched by a non-monster.
		return;
	}

	pOther->pev->origin.z += 1;

	if (FBitSet(pOther->pev->flags, FL_ONGROUND))
	{ // clear the onground so physics don't bitch
		pOther->pev->flags &= ~FL_ONGROUND;
	}

	// toss the monster!
	pOther->pev->velocity = pev->movedir * pev->speed;
	pOther->pev->velocity.z += m_flHeight;
	pev->nextthink = gpGlobals->time;
}

/**
 *	@brief Variable sized repeatable trigger. Must be targeted at one or more entities.
 */
class CTriggerMultiple : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerMultiple, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	void Spawn() override;

	void MultiTouch(CBaseEntity* pOther);
};

BEGIN_DATAMAP(CTriggerMultiple)
DEFINE_FUNCTION(MultiTouch),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(trigger_multiple, CTriggerMultiple);

void CTriggerMultiple::Spawn()
{
	if (m_flWait == 0)
		m_flWait = 0.2;

	InitTrigger();

	ASSERTSZ(pev->health == 0, "trigger_multiple with health");
	SetTouch(&CTriggerMultiple::MultiTouch);
}

void CTriggerMultiple::MultiTouch(CBaseEntity* pOther)
{
	// Only touch clients, monsters, or pushables (depending on flags)
	if (((pOther->pev->flags & FL_CLIENT) != 0 && (pev->spawnflags & SF_TRIGGER_NOCLIENTS) == 0) ||
		((pOther->pev->flags & FL_MONSTER) != 0 && (pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS) != 0) ||
		(pev->spawnflags & SF_TRIGGER_PUSHABLES) != 0 && pOther->ClassnameIs("func_pushable"))
	{

#if 0
		// if the trigger has an angles field, check player's facing direction
		if (pev->movedir != g_vecZero)
		{
			UTIL_MakeVectors(pOther->pev->angles);
			if (DotProduct(gpGlobals->v_forward, pev->movedir) < 0)
				return;         // not facing the right way
		}
#endif

		ActivateMultiTrigger(pOther);
	}
}

/**
 *	@brief Variable sized trigger. Triggers once, then removes itself.
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

/**
 *	@brief Acts as an intermediary for an action that takes multiple inputs.
 */
class CTriggerCounter : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerCounter, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;

	void CounterUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

private:
	int m_cTriggersLeft; // trigger_counter only, # of activations remaining
};

BEGIN_DATAMAP(CTriggerCounter)
DEFINE_FIELD(m_cTriggersLeft, FIELD_INTEGER),
	DEFINE_FUNCTION(CounterUse),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(trigger_counter, CTriggerCounter);

bool CTriggerCounter::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "count"))
	{
		m_cTriggersLeft = atoi(pkvd->szValue);
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CTriggerCounter::Spawn()
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if (m_cTriggersLeft == 0)
		m_cTriggersLeft = 2;
	SetUse(&CTriggerCounter::CounterUse);
}

void CTriggerCounter::CounterUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
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

/**
 *	@brief makes an area vertically negotiable
 */
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

/**
 *	@brief Pushes the player
 */
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

	SetOrigin(pev->origin); // Link into the list
}

void CTriggerPush::Touch(CBaseEntity* pOther)
{
	// UNDONE: Is there a better way than health to detect things that have physics? (clients/monsters)
	switch (pOther->pev->movetype)
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
	case MOVETYPE_FOLLOW:
		return;
	}

	if (pOther->pev->solid != SOLID_NOT && pOther->pev->solid != SOLID_BSP)
	{
		// Instant trigger, just transfer velocity and remove
		if (FBitSet(pev->spawnflags, SF_TRIG_PUSH_ONCE))
		{
			pOther->pev->velocity = pOther->pev->velocity + (pev->speed * pev->movedir);
			if (pOther->pev->velocity.z > 0)
				pOther->pev->flags &= ~FL_ONGROUND;
			UTIL_Remove(this);
		}
		else
		{ // Push field, transfer to base velocity
			Vector vecPush = (pev->speed * pev->movedir);
			if ((pOther->pev->flags & FL_BASEVELOCITY) != 0)
				vecPush = vecPush + pOther->pev->basevelocity;

			pOther->pev->basevelocity = vecPush;

			pOther->pev->flags |= FL_BASEVELOCITY;
			// Logger->debug("Vel {}, base {}", pOther->pev->velocity.z, pOther->pev->basevelocity.z);
		}
	}
}

class CTriggerGravity : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerGravity, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void GravityTouch(CBaseEntity* pOther);
};

BEGIN_DATAMAP(CTriggerGravity)
DEFINE_FUNCTION(GravityTouch),
	END_DATAMAP();

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
	DECLARE_CLASS(CTriggerChangeTarget, CBaseDelay);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

private:
	string_t m_iszNewTarget;
};

LINK_ENTITY_TO_CLASS(trigger_changetarget, CTriggerChangeTarget);

BEGIN_DATAMAP(CTriggerChangeTarget)
DEFINE_FIELD(m_iszNewTarget, FIELD_STRING),
	END_DATAMAP();

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
	CBaseEntity* pTarget = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));

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
	DECLARE_CLASS(CTriggerCamera, CBaseDelay);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void FollowTarget();
	void Move();

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	EntityHandle<CBasePlayer> m_hPlayer;
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

BEGIN_DATAMAP(CTriggerCamera)
DEFINE_FIELD(m_hPlayer, FIELD_EHANDLE),
	DEFINE_FIELD(m_hTarget, FIELD_EHANDLE),
	DEFINE_FIELD(m_pentPath, FIELD_CLASSPTR),
	DEFINE_FIELD(m_sPath, FIELD_STRING),
	DEFINE_FIELD(m_flWait, FIELD_FLOAT),
	DEFINE_FIELD(m_flReturnTime, FIELD_TIME),
	DEFINE_FIELD(m_flStopTime, FIELD_TIME),
	DEFINE_FIELD(m_moveDistance, FIELD_FLOAT),
	DEFINE_FIELD(m_targetSpeed, FIELD_FLOAT),
	DEFINE_FIELD(m_initialSpeed, FIELD_FLOAT),
	DEFINE_FIELD(m_acceleration, FIELD_FLOAT),
	DEFINE_FIELD(m_deceleration, FIELD_FLOAT),
	DEFINE_FIELD(m_state, FIELD_BOOLEAN),
	DEFINE_FUNCTION(FollowTarget),
	END_DATAMAP();

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
		if (!g_pGameRules->IsMultiplayer())
		{
			pActivator = UTIL_GetLocalPlayer();
		}

		if (!pActivator)
		{
			return;
		}
	}

	auto player = ToBasePlayer(pActivator);

	m_hPlayer = player;

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
		m_pentPath = UTIL_FindEntityByTargetname(nullptr, STRING(m_sPath));

		// TODO: this was probably unintential.
		if (!m_pentPath)
		{
			m_pentPath = World;
		}
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
		SetOrigin(pActivator->pev->origin + pActivator->pev->view_ofs);
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
		CBasePlayer* player = m_hPlayer;

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
	DECLARE_CLASS(CTriggerPlayerFreeze, CBaseDelay);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

public:
	bool m_AllPlayers = false;
	bool m_bUnFrozen = true;
};

BEGIN_DATAMAP(CTriggerPlayerFreeze)
DEFINE_FIELD(m_AllPlayers, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bUnFrozen, FIELD_BOOLEAN),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(trigger_playerfreeze, CTriggerPlayerFreeze);

bool CTriggerPlayerFreeze::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "all_players"))
	{
		m_AllPlayers = atoi(pkvd->szValue) != 0;
		return true;
	}

	return BaseClass::KeyValue(pkvd);
}

void CTriggerPlayerFreeze::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_bUnFrozen = !m_bUnFrozen;

	const auto executor = [this](CBasePlayer* player)
	{
		player->EnableControl(m_bUnFrozen);
	};

	if (m_AllPlayers)
	{
		for (auto player : UTIL_FindPlayers())
		{
			executor(player);
		}
	}
	else
	{
		CBasePlayer* player = ToBasePlayer(pActivator);

		if (!player && !g_pGameRules->IsMultiplayer())
		{
			player = UTIL_GetLocalPlayer();
		}

		if (player)
		{
			executor(player);
		}
	}
}

/**
 *	@brief Kills anything that touches it without gibbing it
 */
class CTriggerKillNoGib : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerKillNoGib, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	void Spawn() override;

	void KillTouch(CBaseEntity* pOther);
};

BEGIN_DATAMAP(CTriggerKillNoGib)
DEFINE_FUNCTION(KillTouch),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(trigger_kill_nogib, CTriggerKillNoGib);

void CTriggerKillNoGib::Spawn()
{
	InitTrigger();

	SetTouch(&CTriggerKillNoGib::KillTouch);
	SetUse(nullptr);

	// TODO: this needs to be removed in order to function
	pev->solid = SOLID_NOT;
}

void CTriggerKillNoGib::KillTouch(CBaseEntity* pOther)
{
	if (pOther->pev->takedamage != DAMAGE_NO)
		pOther->TakeDamage(this, pOther, 500000, DMG_NEVERGIB);
}

class CTriggerXenReturn : public CBaseTrigger
{
	DECLARE_CLASS(CTriggerXenReturn, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	void Spawn() override;

	void ReturnTouch(CBaseEntity* pOther);
};

BEGIN_DATAMAP(CTriggerXenReturn)
DEFINE_FUNCTION(ReturnTouch),
	END_DATAMAP();

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
	auto player = ToBasePlayer(pOther);

	if (!player)
		return;

	float flDist = 8192;

	CBaseEntity* pTarget = nullptr;

	// Find the earth target nearest to the player's original location.
	for (auto pDestination : UTIL_FindEntitiesByClassname("info_displacer_earth_target"))
	{
		const float flThisDist = (player->m_DisplacerReturn - pDestination->pev->origin).Length();

		if (flDist > flThisDist)
		{
			pTarget = pDestination;

			flDist = flThisDist;
		}
	}

	if (!FNullEnt(pTarget))
	{
		player->pev->flags &= ~FL_SKIPLOCALHOST;

		auto vecDest = pTarget->pev->origin;

		vecDest.z -= player->pev->mins.z;
		vecDest.z += 1;

		player->SetOrigin(vecDest);

		player->pev->angles = pTarget->pev->angles;
		player->pev->v_angle = pTarget->pev->angles;
		player->pev->fixangle = FIXANGLE_ABSOLUTE;

		player->pev->basevelocity = g_vecZero;
		player->pev->velocity = g_vecZero;

		player->pev->gravity = 1.0;

		player->m_SndRoomtype = player->m_DisplacerSndRoomtype;

		player->EmitSound(CHAN_WEAPON, "weapons/displacer_self.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
	}
}

const auto SF_GENEWORM_HIT_TARGET_ONCE = 1 << 0;
const auto SF_GENEWORM_HIT_START_OFF = 1 << 1;
const auto SF_GENEWORM_HIT_NO_CLIENTS = 1 << 3;
const auto SF_GENEWORM_HIT_FIRE_CLIENT_ONLY = 1 << 4;
const auto SF_GENEWORM_HIT_TOUCH_CLIENT_ONLY = 1 << 5;

class COFTriggerGeneWormHit : public CBaseTrigger
{
	DECLARE_CLASS(COFTriggerGeneWormHit, CBaseTrigger);
	DECLARE_DATAMAP();

public:
	void Precache() override;
	void Spawn() override;

	void GeneWormHitTouch(CBaseEntity* pOther);

	static const char* pAttackSounds[];

	float m_flLastDamageTime;
};

BEGIN_DATAMAP(COFTriggerGeneWormHit)
DEFINE_FIELD(m_flLastDamageTime, FIELD_TIME),
	DEFINE_FUNCTION(GeneWormHitTouch),
	END_DATAMAP();

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

	SetOrigin(pev->origin);

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

			pOther->TakeDamage(this, this, pev->dmg, DMG_CRUSH);

			pOther->EmitSoundDyn(CHAN_BODY, pAttackSounds[RANDOM_LONG(0, std::size(pAttackSounds) - 1)], VOL_NORM, 0.1, 0, RANDOM_LONG(-5, 5) + 100);

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
	if (m_flTriggerDelayTime > gpGlobals->time)
	{
		return;
	}

	CBasePlayer* pOtherPlayer = nullptr;

	if (pOther)
	{
		pOtherPlayer = ToBasePlayer(pOther);

		if (0 != score || !pOtherPlayer)
		{
			return;
		}

		if (team_no != CTFTeam::None && team_no != pOtherPlayer->m_iTeamNum)
		{
			return;
		}
	}

	SUB_UseTargets(this, triggerType, 0);

	// TODO: constrain team_no input to valid values
	if (0 != team_score)
		teamscores[static_cast<int>(team_no) - 1] += team_score;

	if (pOtherPlayer && score != 0)
	{
		pOtherPlayer->m_iCTFScore += score;
		pOtherPlayer->m_iOffense += score;
		MESSAGE_BEGIN(MSG_ALL, gmsgCTFScore);
		g_engfuncs.pfnWriteByte(pOtherPlayer->entindex());
		g_engfuncs.pfnWriteByte(pOtherPlayer->m_iCTFScore);
		g_engfuncs.pfnMessageEnd();

		pOtherPlayer->SendScoreInfoAll();

		CGameRules::Logger->trace("{} triggered \"{}\"", PlayerLogInfo{*pOtherPlayer}, STRING(pev->targetname));
	}

	if (0 != team_score)
	{
		// TOOD: not sure why this check is here since pev must be valid if the entity exists
		if (!pOther && 0 == score && pev)
		{
			CGameRules::Logger->trace("World triggered \"{}\"", STRING(pev->targetname));
		}

		DisplayTeamFlags(nullptr);
	}

	m_flTriggerDelayTime = gpGlobals->time + trigger_delay;
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
