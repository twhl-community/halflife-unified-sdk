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
*	spawn, think, and touch functions for trains, etc
*/

#include "cbase.h"
#include "client.h"
#include "trains.h"

class CFuncPlat;

static void PlatSpawnInsideTrigger(CFuncPlat* platform);

#define SF_PLAT_TOGGLE 0x0001

class CBasePlatTrain : public CBaseToggle
{
public:
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	bool KeyValue(KeyValueData* pkvd) override;
	void Precache() override;

	// This is done to fix spawn flag collisions between this class and a derived class
	virtual bool IsTogglePlat() { return (pev->spawnflags & SF_PLAT_TOGGLE) != 0; }

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_MoveSound; // sound a plat makes while moving
	string_t m_StopSound; // sound a plat makes when it stops
	float m_volume;		  // Sound volume
};

TYPEDESCRIPTION CBasePlatTrain::m_SaveData[] =
	{
		DEFINE_FIELD(CBasePlatTrain, m_MoveSound, FIELD_SOUNDNAME),
		DEFINE_FIELD(CBasePlatTrain, m_StopSound, FIELD_SOUNDNAME),
		DEFINE_FIELD(CBasePlatTrain, m_volume, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CBasePlatTrain, CBaseToggle);

bool CBasePlatTrain::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "lip"))
	{
		m_flLip = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_flHeight = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "rotation"))
	{
		m_vecFinalAngle.x = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_MoveSound = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
		m_StopSound = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_volume = atof(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

void CBasePlatTrain::Precache()
{
	// set the plat's "in-motion" sound
	if (FStrEq("", STRING(m_MoveSound)))
	{
		m_MoveSound = ALLOC_STRING("common/null.wav");
	}

	PrecacheSound(STRING(m_MoveSound));

	// set the plat's 'reached destination' stop sound
	if (FStrEq("", STRING(m_StopSound)))
	{
		m_StopSound = ALLOC_STRING("common/null.wav");
	}

	PrecacheSound(STRING(m_StopSound));
}

/**
*	@details Plats are always drawn in the extended position, so they will light correctly.
*
*	If the plat is the target of another trigger or button, it will start out disabled in
*	the extended position until it is trigger, when it will lower and become a normal plat.
*	
*	If the "height" key is set, that will determine the amount the plat moves, instead of
*	being implicitly determined by the model's height.
*/
class CFuncPlat : public CBasePlatTrain
{
public:
	void Spawn() override;
	void Precache() override;
	void Setup();

	void Blocked(CBaseEntity* pOther) override;

	/**
	*	@brief Start bringing platform down.
	*/
	void EXPORT PlatUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void EXPORT CallGoDown() { GoDown(); }
	void EXPORT CallHitTop() { HitTop(); }
	void EXPORT CallHitBottom() { HitBottom(); }

	/**
	*	@brief Platform is at bottom, now starts moving up
	*/
	virtual void GoUp();

	/**
	*	@brief Platform is at top, now starts moving down.
	*/
	virtual void GoDown();

	/**
	*	@brief Platform has hit top. Pauses, then starts back down again.
	*/
	virtual void HitTop();

	/**
	*	@brief Platform has hit bottom. Stops and waits forever.
	*/
	virtual void HitBottom();
};

LINK_ENTITY_TO_CLASS(func_plat, CFuncPlat);

// UNDONE: Need to save this!!! It needs class & linkage
class CPlatTrigger : public CBaseEntity
{
public:
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

	/**
	*	@brief Create a trigger entity for a platform.
	*/
	void SpawnInsideTrigger(CFuncPlat* pPlatform);

	/**
	*	@brief When the platform's trigger field is touched, the platform ???
	*/
	void Touch(CBaseEntity* pOther) override;
	EntityHandle<CFuncPlat> m_hPlatform;
};

LINK_ENTITY_TO_CLASS(func_plat_trigger, CPlatTrigger);

void CFuncPlat::Setup()
{
	if (m_flTLength == 0)
		m_flTLength = 80;
	if (m_flTWidth == 0)
		m_flTWidth = 10;

	pev->angles = g_vecZero;

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SetOrigin(pev->origin); // set size and link into world
	SetSize(pev->mins, pev->maxs);
	SetModel(STRING(pev->model));

	// vecPosition1 is the top position, vecPosition2 is the bottom
	m_vecPosition1 = pev->origin;
	m_vecPosition2 = pev->origin;
	if (m_flHeight != 0)
		m_vecPosition2.z = pev->origin.z - m_flHeight;
	else
		m_vecPosition2.z = pev->origin.z - pev->size.z + 8;
	if (pev->speed == 0)
		pev->speed = 150;

	if (m_volume == 0)
		m_volume = 0.85;
}

void CFuncPlat::Precache()
{
	CBasePlatTrain::Precache();
	// PrecacheSound("plats/platmove1.wav");
	// PrecacheSound("plats/platstop1.wav");
	if (!IsTogglePlat())
		PlatSpawnInsideTrigger(this); // the "start moving" trigger
}

void CFuncPlat::Spawn()
{
	Setup();

	Precache();

	// If this platform is the target of some button, it starts at the TOP position,
	// and is brought down by that button.  Otherwise, it starts at BOTTOM.
	if (!FStringNull(pev->targetname))
	{
		SetOrigin(m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		SetUse(&CFuncPlat::PlatUse);
	}
	else
	{
		SetOrigin(m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
	}
}

static void PlatSpawnInsideTrigger(CFuncPlat* platform)
{
	g_EntityDictionary->Create<CPlatTrigger>("func_plat_trigger")->SpawnInsideTrigger(platform);
}

void CPlatTrigger::SpawnInsideTrigger(CFuncPlat* pPlatform)
{
	m_hPlatform = pPlatform;
	// Create trigger entity, "point" it at the owning platform, give it a touch method
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->origin = pPlatform->pev->origin;

	// Establish the trigger field's size
	Vector vecTMin = pPlatform->pev->mins + Vector(25, 25, 0);
	Vector vecTMax = pPlatform->pev->maxs + Vector(25, 25, 8);
	vecTMin.z = vecTMax.z - (pPlatform->m_vecPosition1.z - pPlatform->m_vecPosition2.z + 8);
	if (pPlatform->pev->size.x <= 50)
	{
		vecTMin.x = (pPlatform->pev->mins.x + pPlatform->pev->maxs.x) / 2;
		vecTMax.x = vecTMin.x + 1;
	}
	if (pPlatform->pev->size.y <= 50)
	{
		vecTMin.y = (pPlatform->pev->mins.y + pPlatform->pev->maxs.y) / 2;
		vecTMax.y = vecTMin.y + 1;
	}
	SetSize(vecTMin, vecTMax);
}

void CPlatTrigger::Touch(CBaseEntity* pOther)
{
	// Platform was removed, remove trigger
	if (!m_hPlatform)
	{
		UTIL_Remove(this);
		return;
	}

	// Ignore touches by non-players
	if (!pOther->IsPlayer())
		return;

	// Ignore touches by corpses
	if (!pOther->IsAlive())
		return;

	CFuncPlat* platform = m_hPlatform;

	// Make linked platform go up/down.
	if (platform->m_toggle_state == TS_AT_BOTTOM)
		platform->GoUp();
	else if (platform->m_toggle_state == TS_AT_TOP)
		platform->pev->nextthink = platform->pev->ltime + 1; // delay going down
}

void CFuncPlat::PlatUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (IsTogglePlat())
	{
		// Top is off, bottom is on
		bool on = (m_toggle_state == TS_AT_BOTTOM) ? true : false;

		if (!ShouldToggle(useType, on))
			return;

		if (m_toggle_state == TS_AT_TOP)
			GoDown();
		else if (m_toggle_state == TS_AT_BOTTOM)
			GoUp();
	}
	else
	{
		SetUse(nullptr);

		if (m_toggle_state == TS_AT_TOP)
			GoDown();
	}
}

void CFuncPlat::GoDown()
{
	EmitSound(CHAN_STATIC, STRING(m_MoveSound), m_volume, ATTN_NORM);

	ASSERT(m_toggle_state == TS_AT_TOP || m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone(&CFuncPlat::CallHitBottom);
	LinearMove(m_vecPosition2, pev->speed);
}

void CFuncPlat::HitBottom()
{
	StopSound(CHAN_STATIC, STRING(m_MoveSound));
	EmitSound(CHAN_WEAPON, STRING(m_StopSound), m_volume, ATTN_NORM);

	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;
}

void CFuncPlat::GoUp()
{
	EmitSound(CHAN_STATIC, STRING(m_MoveSound), m_volume, ATTN_NORM);

	ASSERT(m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_GOING_UP;
	SetMoveDone(&CFuncPlat::CallHitTop);
	LinearMove(m_vecPosition1, pev->speed);
}

void CFuncPlat::HitTop()
{
	StopSound(CHAN_STATIC, STRING(m_MoveSound));
	EmitSound(CHAN_WEAPON, STRING(m_StopSound), m_volume, ATTN_NORM);

	ASSERT(m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_AT_TOP;

	if (!IsTogglePlat())
	{
		// After a delay, the platform will automatically start going down again.
		SetThink(&CFuncPlat::CallGoDown);
		pev->nextthink = pev->ltime + 3;
	}
}

void CFuncPlat::Blocked(CBaseEntity* pOther)
{
	Logger->trace("{} Blocked by {}", STRING(pev->classname), STRING(pOther->pev->classname));
	// Hurt the blocker a little
	pOther->TakeDamage(this, this, 1, DMG_CRUSH);

	StopSound(CHAN_STATIC, STRING(m_MoveSound));

	// Send the platform back where it came from
	ASSERT(m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN);
	if (m_toggle_state == TS_GOING_UP)
		GoDown();
	else if (m_toggle_state == TS_GOING_DOWN)
		GoUp();
}

class CFuncPlatRot : public CFuncPlat
{
public:
	void Spawn() override;
	void SetupRotation();

	void GoUp() override;
	void GoDown() override;
	void HitTop() override;
	void HitBottom() override;

	void RotMove(Vector& destAngle, float time);
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	Vector m_end, m_start;
};

LINK_ENTITY_TO_CLASS(func_platrot, CFuncPlatRot);

TYPEDESCRIPTION CFuncPlatRot::m_SaveData[] =
	{
		DEFINE_FIELD(CFuncPlatRot, m_end, FIELD_VECTOR),
		DEFINE_FIELD(CFuncPlatRot, m_start, FIELD_VECTOR),
};

IMPLEMENT_SAVERESTORE(CFuncPlatRot, CFuncPlat);

void CFuncPlatRot::SetupRotation()
{
	if (m_vecFinalAngle.x != 0) // This plat rotates too!
	{
		CBaseToggle::AxisDir(this);
		m_start = pev->angles;
		m_end = pev->angles + pev->movedir * m_vecFinalAngle.x;
	}
	else
	{
		m_start = g_vecZero;
		m_end = g_vecZero;
	}
	if (!FStringNull(pev->targetname)) // Start at top
	{
		pev->angles = m_end;
	}
}

void CFuncPlatRot::Spawn()
{
	CFuncPlat::Spawn();
	SetupRotation();
}

void CFuncPlatRot::GoDown()
{
	CFuncPlat::GoDown();
	RotMove(m_start, pev->nextthink - pev->ltime);
}

void CFuncPlatRot::HitBottom()
{
	CFuncPlat::HitBottom();
	pev->avelocity = g_vecZero;
	pev->angles = m_start;
}

void CFuncPlatRot::GoUp()
{
	CFuncPlat::GoUp();
	RotMove(m_end, pev->nextthink - pev->ltime);
}

void CFuncPlatRot::HitTop()
{
	CFuncPlat::HitTop();
	pev->avelocity = g_vecZero;
	pev->angles = m_end;
}

void CFuncPlatRot::RotMove(Vector& destAngle, float time)
{
	// set destdelta to the vector needed to move
	Vector vecDestDelta = destAngle - pev->angles;

	// Travel time is so short, we're practically there already;  so make it so.
	if (time >= 0.1)
		pev->avelocity = vecDestDelta / time;
	else
	{
		pev->avelocity = vecDestDelta;
		pev->nextthink = pev->ltime + 1;
	}
}

/**
*	@brief Trains are moving platforms that players can ride.
*	The targets origin specifies the min point of the train at each corner.
*	The train spawns at the first target it is pointing at.
*	If the train is the target of a button or trigger, it will not begin moving until activated.
*/
class CFuncTrain : public CBasePlatTrain
{
public:
	void Spawn() override;
	void Activate() override;
	void OverrideReset() override;

	void Blocked(CBaseEntity* pOther) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;


	void EXPORT Wait();

	/**
	*	@brief path corner needs to change to next target
	*/
	void EXPORT Next();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	entvars_t* m_pevCurrentTarget;
	bool m_activated;
};

LINK_ENTITY_TO_CLASS(func_train, CFuncTrain);

TYPEDESCRIPTION CFuncTrain::m_SaveData[] =
	{
		DEFINE_FIELD(CFuncTrain, m_pevCurrentTarget, FIELD_EVARS),
		DEFINE_FIELD(CFuncTrain, m_activated, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CFuncTrain, CBasePlatTrain);

void CFuncTrain::Blocked(CBaseEntity* pOther)

{
	if (gpGlobals->time < m_flActivateFinished)
		return;

	m_flActivateFinished = gpGlobals->time + 0.5;

	pOther->TakeDamage(this, this, pev->dmg, DMG_CRUSH);
}

void CFuncTrain::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if ((pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER) != 0)
	{
		// Move toward my target
		pev->spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
		Next();
	}
	else
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// Pop back to last target if it's available
		if (pev->enemy)
			pev->target = pev->enemy->v.targetname;
		pev->nextthink = 0;
		pev->velocity = g_vecZero;
		EmitSound(CHAN_VOICE, STRING(m_StopSound), m_volume, ATTN_NORM);
	}
}

void CFuncTrain::Wait()
{
	// Fire the pass target if there is one
	if (!FStringNull(m_pevCurrentTarget->message))
	{
		FireTargets(STRING(m_pevCurrentTarget->message), this, this, USE_TOGGLE, 0);
		if (FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_FIREONCE))
			m_pevCurrentTarget->message = string_t::Null;
	}

	// need pointer to LAST target.
	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_TRAIN_WAIT_RETRIGGER) || (pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER) != 0)
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// clear the sound channel.
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		EmitSound(CHAN_VOICE, STRING(m_StopSound), m_volume, ATTN_NORM);
		pev->nextthink = 0;
		return;
	}

	// Logger->debug("{}", m_flWait);

	if (m_flWait != 0)
	{ // -1 wait will wait forever!
		pev->nextthink = pev->ltime + m_flWait;
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		EmitSound(CHAN_VOICE, STRING(m_StopSound), m_volume, ATTN_NORM);
		SetThink(&CFuncTrain::Next);
	}
	else
	{
		Next(); // do it RIGHT now!
	}
}

void CFuncTrain::Next()
{
	CBaseEntity* pTarg;


	// now find our next target
	pTarg = GetNextTarget();

	if (!pTarg)
	{
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		// Play stop sound
		EmitSound(CHAN_VOICE, STRING(m_StopSound), m_volume, ATTN_NORM);
		return;
	}

	// Save last target in case we need to find it again
	pev->message = pev->target;

	pev->target = pTarg->pev->target;
	m_flWait = pTarg->GetDelay();

	if (m_pevCurrentTarget && m_pevCurrentTarget->speed != 0)
	{ // don't copy speed from target if it is 0 (uninitialized)
		pev->speed = m_pevCurrentTarget->speed;
		Logger->trace("Train {} speed to {:4.2f}", STRING(pev->targetname), pev->speed);
	}
	m_pevCurrentTarget = pTarg->pev; // keep track of this since path corners change our target for us.

	pev->enemy = pTarg->edict(); // hack

	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_TELEPORT))
	{
		// Path corner has indicated a teleport to the next corner.
		SetBits(pev->effects, EF_NOINTERP);
		SetOrigin(pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5);
		Wait(); // Get on with doing the next path corner.
	}
	else
	{
		// Normal linear move.

		// CHANGED this from CHAN_VOICE to CHAN_STATIC around OEM beta time because trains should
		// use CHAN_STATIC for their movement sounds to prevent sound field problems.
		// this is not a hack or temporary fix, this is how things should be. (sjb).
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		EmitSound(CHAN_STATIC, STRING(m_MoveSound), m_volume, ATTN_NORM);
		ClearBits(pev->effects, EF_NOINTERP);
		SetMoveDone(&CFuncTrain::Wait);
		LinearMove(pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5, pev->speed);
	}
}

void CFuncTrain::Activate()
{
	// Not yet active, so teleport to first target
	if (!m_activated)
	{
		m_activated = true;
		auto target = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));

		if (!target)
		{
			target = World;
		}

		pev->target = target->pev->target;
		m_pevCurrentTarget = target->pev; // keep track of this since path corners change our target for us.

		SetOrigin(target->pev->origin - (pev->mins + pev->maxs) * 0.5);

		if (FStringNull(pev->targetname))
		{ // not triggered, so start immediately
			pev->nextthink = pev->ltime + 0.1;
			SetThink(&CFuncTrain::Next);
		}
		else
			pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
	}
}

void CFuncTrain::Spawn()
{
	Precache();
	if (pev->speed == 0)
		pev->speed = 100;

	if (FStringNull(pev->target))
		Logger->debug("FuncTrain with no target");

	if (pev->dmg == 0)
		pev->dmg = 2;

	pev->movetype = MOVETYPE_PUSH;

	if (FBitSet(pev->spawnflags, SF_TRACKTRAIN_PASSABLE))
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	SetModel(STRING(pev->model));
	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);

	m_activated = false;

	if (m_volume == 0)
		m_volume = 0.85;
}

void CFuncTrain::OverrideReset()
{
	CBaseEntity* pTarg;

	// Are we moving?
	if (pev->velocity != g_vecZero && pev->nextthink != 0)
	{
		pev->target = pev->message;
		// now find our next target
		pTarg = GetNextTarget();
		if (!pTarg)
		{
			pev->nextthink = 0;
			pev->velocity = g_vecZero;
		}
		else // Keep moving for 0.1 secs, then find path_corner again and restart
		{
			SetThink(&CFuncTrain::Next);
			pev->nextthink = pev->ltime + 0.1;
		}
	}
}

/**
 *	@brief Sprite that can follow a path like a train
 */
class CSpriteTrain : public CBasePlatTrain
{
public:
	void Spawn() override;
	void Precache() override;
	void Activate() override;
	void OverrideReset() override;

	void Blocked(CBaseEntity* pOther) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;


	void EXPORT Wait();

	/**
	*	@brief path corner needs to change to next target
	*/
	void EXPORT Next();

	void Think() override;

	void Animate(float frames);

	void LinearMove(const Vector& vecDest, float flSpeed);

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	entvars_t* m_pevCurrentTarget;
	bool m_activated;

	float m_maxFrame;
	float m_lastTime;
	bool m_waiting;
	bool m_nexting;
	float m_nextTime;
	float m_waitTime;
	bool m_stopSprite;
};

LINK_ENTITY_TO_CLASS(env_spritetrain, CSpriteTrain);

TYPEDESCRIPTION CSpriteTrain::m_SaveData[] =
	{
		DEFINE_FIELD(CSpriteTrain, m_pevCurrentTarget, FIELD_EVARS),
		DEFINE_FIELD(CSpriteTrain, m_activated, FIELD_BOOLEAN),
		DEFINE_FIELD(CSpriteTrain, m_maxFrame, FIELD_FLOAT),
		DEFINE_FIELD(CSpriteTrain, m_lastTime, FIELD_TIME),
		DEFINE_FIELD(CSpriteTrain, m_waiting, FIELD_BOOLEAN),
		DEFINE_FIELD(CSpriteTrain, m_nexting, FIELD_BOOLEAN),
		DEFINE_FIELD(CSpriteTrain, m_nextTime, FIELD_FLOAT),
		DEFINE_FIELD(CSpriteTrain, m_waitTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CSpriteTrain, CBasePlatTrain);

void CSpriteTrain::Animate(float frames)
{
	if (m_maxFrame > 1)
	{
		if (pev->framerate == 0)
		{
			pev->framerate = 10;
		}

		pev->frame = fmod(pev->frame + frames, m_maxFrame);
	}
}

void CSpriteTrain::LinearMove(const Vector& vecDest, float flSpeed)
{
	m_vecFinalDest = vecDest;

	if (vecDest == pev->origin)
	{
		Wait();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - pev->origin;

	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to LinearMoveDone when dest is reached
	m_waiting = true;
	m_stopSprite = true;
	m_waitTime = pev->ltime + flTravelTime;

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->velocity = vecDestDelta / flTravelTime;
}

void CSpriteTrain::Blocked(CBaseEntity* pOther)

{
	if (pev->ltime < m_flActivateFinished)
		return;

	m_flActivateFinished = pev->ltime + 0.5;

	pOther->TakeDamage(this, this, pev->dmg, DMG_CRUSH);
}

void CSpriteTrain::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if ((pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER) != 0)
	{
		// Move toward my target
		pev->spawnflags &= ~SF_TRAIN_WAIT_RETRIGGER;
		Next();
	}
	else
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// Pop back to last target if it's available
		if (pev->enemy)
			pev->target = pev->enemy->v.targetname;

		pev->velocity = g_vecZero;
		EmitSound(CHAN_VOICE, STRING(m_StopSound), m_volume, ATTN_NORM);

		m_nexting = true;
		m_nextTime = pev->ltime + m_flWait;
	}
}

void CSpriteTrain::Wait()
{
	// Fire the pass target if there is one
	if (!FStringNull(m_pevCurrentTarget->message))
	{
		FireTargets(STRING(m_pevCurrentTarget->message), this, this, USE_TOGGLE, 0);
		if (FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_FIREONCE))
			m_pevCurrentTarget->message = string_t::Null;
	}

	// need pointer to LAST target.
	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_TRAIN_WAIT_RETRIGGER) || (pev->spawnflags & SF_TRAIN_WAIT_RETRIGGER) != 0)
	{
		pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
		// clear the sound channel.
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		EmitSound(CHAN_VOICE, STRING(m_StopSound), m_volume, ATTN_NORM);
		return;
	}

	// Logger->debug("{}", m_flWait);

	if (m_flWait != 0)
	{ // -1 wait will wait forever!
		pev->nextthink = pev->ltime + m_flWait;
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		EmitSound(CHAN_VOICE, STRING(m_StopSound), m_volume, ATTN_NORM);

		m_nexting = true;
		m_nextTime = pev->ltime + m_flWait;
	}
	else
	{
		Next(); // do it RIGHT now!
	}
}

void CSpriteTrain::Next()
{
	CBaseEntity* pTarg;


	// now find our next target
	pTarg = GetNextTarget();

	if (!pTarg)
	{
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		// Play stop sound
		EmitSound(CHAN_VOICE, STRING(m_StopSound), m_volume, ATTN_NORM);
		return;
	}

	// Save last target in case we need to find it again
	pev->message = pev->target;

	pev->target = pTarg->pev->target;
	m_flWait = pTarg->GetDelay();

	if (m_pevCurrentTarget && m_pevCurrentTarget->speed != 0)
	{ // don't copy speed from target if it is 0 (uninitialized)
		pev->speed = m_pevCurrentTarget->speed;
		Logger->trace("Train {} speed to {:4.2f}", STRING(pev->targetname), pev->speed);
	}
	m_pevCurrentTarget = pTarg->pev; // keep track of this since path corners change our target for us.

	pev->enemy = pTarg->edict(); // hack

	if (FBitSet(m_pevCurrentTarget->spawnflags, SF_CORNER_TELEPORT))
	{
		// Path corner has indicated a teleport to the next corner.
		SetBits(pev->effects, EF_NOINTERP);
		SetOrigin(pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5);
		Wait(); // Get on with doing the next path corner.
	}
	else
	{
		// Normal linear move.

		// CHANGED this from CHAN_VOICE to CHAN_STATIC around OEM beta time because trains should
		// use CHAN_STATIC for their movement sounds to prevent sound field problems.
		// this is not a hack or temporary fix, this is how things should be. (sjb).
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		EmitSound(CHAN_STATIC, STRING(m_MoveSound), m_volume, ATTN_NORM);
		ClearBits(pev->effects, EF_NOINTERP);

		LinearMove(pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5, pev->speed);
	}
}

void CSpriteTrain::Activate()
{
	// Not yet active, so teleport to first target
	if (!m_activated)
	{
		m_activated = true;
		auto target = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));

		if (!target)
		{
			target = World;
		}

		pev->target = target->pev->target;
		m_pevCurrentTarget = target->pev; // keep track of this since path corners change our target for us.

		SetOrigin(target->pev->origin - (pev->mins + pev->maxs) * 0.5);

		if (FStringNull(pev->targetname))
		{ // not triggered, so start immediately
			m_nexting = true;
			pev->nextthink = pev->ltime + 0.1;
			Logger->debug("time={}, nexttime={}", pev->ltime, m_nextTime);
		}
		else
			pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
	}
}

void CSpriteTrain::Spawn()
{
	Precache();
	if (pev->speed == 0)
		pev->speed = 100;

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	if (FStringNull(pev->target))
		Logger->debug("FuncTrain with no target");

	if (pev->dmg == 0)
		pev->dmg = 2;

	pev->movetype = MOVETYPE_PUSH;

	pev->solid = SOLID_NOT;

	SetModel(STRING(pev->model));
	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);

	m_activated = false;

	if (m_volume == 0)
		m_volume = 0.85;

	m_maxFrame = MODEL_FRAMES(pev->modelindex) - 1;
	m_lastTime = pev->ltime;
	pev->nextthink = pev->ltime + 0.1;
	m_waiting = false;
	m_nexting = false;
	m_waitTime = pev->ltime;
	m_nextTime = pev->ltime;
	m_stopSprite = false;
}

void CSpriteTrain::Precache()
{
	PrecacheModel(STRING(pev->model));

	CBasePlatTrain::Precache();
}

void CSpriteTrain::OverrideReset()
{
	CBaseEntity* pTarg;

	// Are we moving?
	if (pev->velocity != g_vecZero && pev->nextthink != 0)
	{
		pev->target = pev->message;
		// now find our next target
		pTarg = GetNextTarget();
		if (!pTarg)
		{
			pev->nextthink = 0.1;
			pev->velocity = g_vecZero;
		}
		else // Keep moving for 0.1 secs, then find path_corner again and restart
		{
			m_nextTime = pev->ltime + 0.1;
			m_nexting = true;
		}
	}
}

void CSpriteTrain::Think()
{
	Animate(pev->framerate * (pev->ltime - m_lastTime));

	if (m_flWait != -1)
	{
		if (m_waiting && pev->ltime >= m_waitTime)
		{
			if (m_stopSprite)
			{
				pev->velocity = g_vecZero;
				m_stopSprite = false;
			}

			m_waiting = false;

			Wait();
		}
	}

	if (m_flWait != -1)
	{
		if (m_nexting && pev->ltime >= m_nextTime)
		{
			if (m_stopSprite)
			{
				pev->velocity = g_vecZero;
				m_stopSprite = false;
			}

			m_nexting = false;

			Next();
		}
	}

	pev->nextthink = pev->ltime + 0.1;
	m_lastTime = pev->ltime;
}

TYPEDESCRIPTION CFuncTrackTrain::m_SaveData[] =
	{
		DEFINE_FIELD(CFuncTrackTrain, m_ppath, FIELD_CLASSPTR),
		DEFINE_FIELD(CFuncTrackTrain, m_length, FIELD_FLOAT),
		DEFINE_FIELD(CFuncTrackTrain, m_height, FIELD_FLOAT),
		DEFINE_FIELD(CFuncTrackTrain, m_speed, FIELD_FLOAT),
		DEFINE_FIELD(CFuncTrackTrain, m_dir, FIELD_FLOAT),
		DEFINE_FIELD(CFuncTrackTrain, m_startSpeed, FIELD_FLOAT),
		DEFINE_FIELD(CFuncTrackTrain, m_controlMins, FIELD_VECTOR),
		DEFINE_FIELD(CFuncTrackTrain, m_controlMaxs, FIELD_VECTOR),
		DEFINE_FIELD(CFuncTrackTrain, m_sounds, FIELD_SOUNDNAME),
		DEFINE_FIELD(CFuncTrackTrain, m_flVolume, FIELD_FLOAT),
		DEFINE_FIELD(CFuncTrackTrain, m_flBank, FIELD_FLOAT),
		DEFINE_FIELD(CFuncTrackTrain, m_oldSpeed, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CFuncTrackTrain, CBaseEntity);
LINK_ENTITY_TO_CLASS(func_tracktrain, CFuncTrackTrain);

bool CFuncTrackTrain::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wheels"))
	{
		m_length = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_height = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "startspeed"))
	{
		m_startSpeed = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_flVolume = (float)(atoi(pkvd->szValue));
		m_flVolume *= 0.1;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "bank"))
	{
		m_flBank = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CFuncTrackTrain::NextThink(float thinkTime, bool alwaysThink)
{
	if (alwaysThink)
		pev->flags |= FL_ALWAYSTHINK;
	else
		pev->flags &= ~FL_ALWAYSTHINK;

	pev->nextthink = thinkTime;
}

void CFuncTrackTrain::Blocked(CBaseEntity* pOther)
{
	// Blocker is on-ground on the train
	if (FBitSet(pOther->pev->flags, FL_ONGROUND) && pOther->GetGroundEntity() == this)
	{
		float deltaSpeed = fabs(pev->speed);
		if (deltaSpeed > 50)
			deltaSpeed = 50;
		if (0 == pOther->pev->velocity.z)
			pOther->pev->velocity.z += deltaSpeed;
		return;
	}
	else
		pOther->pev->velocity = (pOther->pev->origin - pev->origin).Normalize() * pev->dmg;

	Logger->trace("TRAIN({}): Blocked by {} (dmg:{:.2f})", STRING(pev->targetname), STRING(pOther->pev->classname), pev->dmg);
	if (pev->dmg <= 0)
		return;
	// we can't hurt this thing, so we're not concerned with it
	pOther->TakeDamage(this, this, pev->dmg, DMG_CRUSH);
}

void CFuncTrackTrain::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (useType != USE_SET)
	{
		if (!ShouldToggle(useType, (pev->speed != 0)))
			return;

		if (pev->speed == 0)
		{
			pev->speed = m_speed * m_dir;

			Next();
		}
		else
		{
			pev->speed = 0;
			pev->velocity = g_vecZero;
			pev->avelocity = g_vecZero;
			StopTrainSound();
			SetThink(nullptr);
		}
	}
	else
	{
		float delta = value;

		delta = ((int)(pev->speed * 4) / (int)m_speed) * 0.25 + 0.25 * delta;
		if (delta > 1)
			delta = 1;
		else if (delta < -1)
			delta = -1;
		if ((pev->spawnflags & SF_TRACKTRAIN_FORWARDONLY) != 0)
		{
			if (delta < 0)
				delta = 0;
		}
		pev->speed = m_speed * delta;
		Next();
		Logger->debug("TRAIN({}), speed to {:.2f}", STRING(pev->targetname), pev->speed);
	}
}

// TODO: move to mathlib
static float Fix(float angle)
{
	while (angle < 0)
		angle += 360;
	while (angle > 360)
		angle -= 360;

	return angle;
}

static void FixupAngles(Vector& v)
{
	v.x = Fix(v.x);
	v.y = Fix(v.y);
	v.z = Fix(v.z);
}

#define TRAIN_STARTPITCH 60
#define TRAIN_MAXPITCH 200
#define TRAIN_MAXSPEED 1000 // approx max speed for sound pitch calculation

void CFuncTrackTrain::StopTrainSound()
{
	// if sound playing, stop it
	if (m_soundPlaying && !FStringNull(m_sounds))
	{
		StopSound(CHAN_STATIC, STRING(m_sounds));
		EmitSound(CHAN_ITEM, "plats/ttrain_brake1.wav", m_flVolume, ATTN_NORM);
	}

	m_soundPlaying = false;
}

void CFuncTrackTrain::UpdateTrainSound()
{
	if (FStringNull(m_sounds))
		return;

	const float flpitch = TRAIN_STARTPITCH + (fabs(pev->speed) * (TRAIN_MAXPITCH - TRAIN_STARTPITCH) / TRAIN_MAXSPEED);

	// If a player has joined we need to restart the train sound so they can hear it.
	if (!m_soundPlaying || m_LastPlayerJoinTimeCheck < g_LastPlayerJoinTime)
	{
		if (!m_soundPlaying)
		{
			// play startup sound for train
			EmitSound(CHAN_ITEM, "plats/ttrain_start1.wav", m_flVolume, ATTN_NORM);
		}

		EmitSoundDyn(CHAN_STATIC, STRING(m_sounds), m_flVolume, ATTN_NORM, 0, (int)flpitch);
		m_soundPlaying = true;
	}
	else if (flpitch != m_CachedPitch)
	{
		// update pitch
		EmitSoundDyn(CHAN_STATIC, STRING(m_sounds), m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, (int)flpitch);
	}

	m_CachedPitch = flpitch;
	m_LastPlayerJoinTimeCheck = g_LastPlayerJoinTime;
}

void CFuncTrackTrain::Next()
{
	float time = 0.5;

	if (0 == pev->speed)
	{
		Logger->trace("TRAIN({}): Speed is 0", STRING(pev->targetname));
		StopTrainSound();
		return;
	}

	//	if ( !m_ppath )
	//		m_ppath = CPathTrack::Instance(UTIL_FindEntityByTargetname(nullptr, STRING(pev->target)));
	if (!m_ppath)
	{
		Logger->trace("TRAIN({}): Lost path", STRING(pev->targetname));
		StopTrainSound();
		return;
	}

	UpdateTrainSound();

	Vector nextPos = pev->origin;

	nextPos.z -= m_height;
	CPathTrack* pnext = m_ppath->LookAhead(&nextPos, pev->speed * 0.1, true);
	nextPos.z += m_height;

	pev->velocity = (nextPos - pev->origin) * 10;
	Vector nextFront = pev->origin;

	nextFront.z -= m_height;
	if (m_length > 0)
		m_ppath->LookAhead(&nextFront, m_length, false);
	else
		m_ppath->LookAhead(&nextFront, 100, false);
	nextFront.z += m_height;

	Vector delta = nextFront - pev->origin;
	Vector angles = UTIL_VecToAngles(delta);
	// The train actually points west
	angles.y += 180;

	// !!!  All of this crap has to be done to make the angles not wrap around, revisit this.
	FixupAngles(angles);
	FixupAngles(pev->angles);

	if (!pnext || (delta.x == 0 && delta.y == 0))
		angles = pev->angles;

	float vy, vx;
	if ((pev->spawnflags & SF_TRACKTRAIN_NOPITCH) == 0)
		vx = UTIL_AngleDistance(angles.x, pev->angles.x);
	else
		vx = 0;
	vy = UTIL_AngleDistance(angles.y, pev->angles.y);

	pev->avelocity.y = vy * 10;
	pev->avelocity.x = vx * 10;

	if (m_flBank != 0)
	{
		if (pev->avelocity.y < -5)
			pev->avelocity.z = UTIL_AngleDistance(UTIL_ApproachAngle(-m_flBank, pev->angles.z, m_flBank * 2), pev->angles.z);
		else if (pev->avelocity.y > 5)
			pev->avelocity.z = UTIL_AngleDistance(UTIL_ApproachAngle(m_flBank, pev->angles.z, m_flBank * 2), pev->angles.z);
		else
			pev->avelocity.z = UTIL_AngleDistance(UTIL_ApproachAngle(0, pev->angles.z, m_flBank * 4), pev->angles.z) * 4;
	}

	if (pnext)
	{
		if (pnext != m_ppath)
		{
			CPathTrack* pFire;
			if (pev->speed >= 0)
				pFire = pnext;
			else
				pFire = m_ppath;

			m_ppath = pnext;
			// Fire the pass target if there is one
			if (!FStringNull(pFire->pev->message))
			{
				FireTargets(STRING(pFire->pev->message), this, this, USE_TOGGLE, 0);
				if (FBitSet(pFire->pev->spawnflags, SF_PATH_FIREONCE))
					pFire->pev->message = string_t::Null;
			}

			if ((pFire->pev->spawnflags & SF_PATH_DISABLE_TRAIN) != 0)
				pev->spawnflags |= SF_TRACKTRAIN_NOCONTROL;

			// Don't override speed if under user control
			if ((pev->spawnflags & SF_TRACKTRAIN_NOCONTROL) != 0)
			{
				if (pFire->pev->speed != 0)
				{ // don't copy speed from target if it is 0 (uninitialized)
					pev->speed = pFire->pev->speed;
					Logger->trace("TrackTrain {} speed to {:4.2f}", STRING(pev->targetname), pev->speed);
				}
			}
		}
		SetThink(&CFuncTrackTrain::Next);
		NextThink(pev->ltime + time, true);
	}
	else // end of path, stop
	{
		StopTrainSound();
		pev->velocity = (nextPos - pev->origin);
		pev->avelocity = g_vecZero;
		float distance = pev->velocity.Length();
		m_oldSpeed = pev->speed;


		pev->speed = 0;

		// Move to the dead end

		// Are we there yet?
		if (distance > 0)
		{
			// no, how long to get there?
			time = distance / m_oldSpeed;
			pev->velocity = pev->velocity * (m_oldSpeed / distance);
			SetThink(&CFuncTrackTrain::DeadEnd);
			NextThink(pev->ltime + time, false);
		}
		else
		{
			DeadEnd();
		}
	}
}

void CFuncTrackTrain::DeadEnd()
{
	// Fire the dead-end target if there is one
	CPathTrack *pTrack, *pNext;

	pTrack = m_ppath;

	// Find the dead end path node
	// HACKHACK -- This is bugly, but the train can actually stop moving at a different node depending on it's speed
	// so we have to traverse the list to it's end.
	if (pTrack)
	{
		if (m_oldSpeed < 0)
		{
			do
			{
				pNext = pTrack->ValidPath(pTrack->GetPrevious(), true);
				if (pNext)
					pTrack = pNext;
			} while (nullptr != pNext);
		}
		else
		{
			do
			{
				pNext = pTrack->ValidPath(pTrack->GetNext(), true);
				if (pNext)
					pTrack = pNext;
			} while (nullptr != pNext);
		}
	}

	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;



	if (pTrack)
	{
		Logger->trace("TRAIN({}): Dead end at ", STRING(pev->targetname), STRING(pTrack->pev->targetname));
		if (!FStringNull(pTrack->pev->netname))
			FireTargets(STRING(pTrack->pev->netname), this, this, USE_TOGGLE, 0);
	}
	else
		Logger->trace("TRAIN({}): Dead end", STRING(pev->targetname));
}

void CFuncTrackTrain::SetControls(CBaseEntity* controls)
{
	Vector offset = controls->pev->origin - pev->oldorigin;

	m_controlMins = controls->pev->mins + offset;
	m_controlMaxs = controls->pev->maxs + offset;
}

bool CFuncTrackTrain::OnControls(CBaseEntity* controller)
{
	Vector offset = controller->pev->origin - pev->origin;

	if ((pev->spawnflags & SF_TRACKTRAIN_NOCONTROL) != 0)
		return false;

	// Transform offset into local coordinates
	UTIL_MakeVectors(pev->angles);
	Vector local;
	local.x = DotProduct(offset, gpGlobals->v_forward);
	local.y = -DotProduct(offset, gpGlobals->v_right);
	local.z = DotProduct(offset, gpGlobals->v_up);

	if (local.x >= m_controlMins.x && local.y >= m_controlMins.y && local.z >= m_controlMins.z &&
		local.x <= m_controlMaxs.x && local.y <= m_controlMaxs.y && local.z <= m_controlMaxs.z)
		return true;

	return false;
}

void CFuncTrackTrain::Find()
{
	m_ppath = CPathTrack::Instance(UTIL_FindEntityByTargetname(nullptr, STRING(pev->target)));
	if (!m_ppath)
		return;

	if (!m_ppath->ClassnameIs("path_track"))
	{
		Logger->error("func_tracktrain must be on a path of path_track");
		m_ppath = nullptr;
		return;
	}

	Vector nextPos = m_ppath->pev->origin;
	nextPos.z += m_height;

	Vector look = nextPos;
	look.z -= m_height;
	m_ppath->LookAhead(&look, m_length, false);
	look.z += m_height;

	pev->angles = UTIL_VecToAngles(look - nextPos);
	// The train actually points west
	pev->angles.y += 180;

	if ((pev->spawnflags & SF_TRACKTRAIN_NOPITCH) != 0)
		pev->angles.x = 0;
	SetOrigin(nextPos);
	NextThink(pev->ltime + 0.1, false);
	SetThink(&CFuncTrackTrain::Next);
	pev->speed = m_startSpeed;

	UpdateTrainSound();
}

void CFuncTrackTrain::NearestPath()
{
	CBaseEntity* pTrack = nullptr;
	CBaseEntity* pNearest = nullptr;
	float dist, closest;

	closest = 1024;

	while ((pTrack = UTIL_FindEntityInSphere(pTrack, pev->origin, 1024)) != nullptr)
	{
		// filter out non-tracks
		if ((pTrack->pev->flags & (FL_CLIENT | FL_MONSTER)) == 0 && pTrack->ClassnameIs("path_track"))
		{
			dist = (pev->origin - pTrack->pev->origin).Length();
			if (dist < closest)
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if (!pNearest)
	{
		Logger->debug("Can't find a nearby track !!!");
		SetThink(nullptr);
		return;
	}

	Logger->trace("TRAIN: {}, Nearest track is {}", STRING(pev->targetname), STRING(pNearest->pev->targetname));
	// If I'm closer to the next path_track on this path, then it's my real path
	pTrack = ((CPathTrack*)pNearest)->GetNext();
	if (pTrack)
	{
		if ((pev->origin - pTrack->pev->origin).Length() < (pev->origin - pNearest->pev->origin).Length())
			pNearest = pTrack;
	}

	m_ppath = (CPathTrack*)pNearest;

	if (pev->speed != 0)
	{
		NextThink(pev->ltime + 0.1, false);
		SetThink(&CFuncTrackTrain::Next);
	}
}

void CFuncTrackTrain::OverrideReset()
{
	NextThink(pev->ltime + 0.1, false);
	SetThink(&CFuncTrackTrain::NearestPath);
}

CFuncTrackTrain* CFuncTrackTrain::Instance(CBaseEntity* pent)
{
	if (pent && pent->ClassnameIs("func_tracktrain"))
		return static_cast<CFuncTrackTrain*>(pent);
	return nullptr;
}

void CFuncTrackTrain::Spawn()
{
	if (pev->speed == 0)
		m_speed = 100;
	else
		m_speed = pev->speed;

	pev->speed = 0;
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->impulse = m_speed;

	m_dir = 1;

	if (FStringNull(pev->target))
		Logger->debug("FuncTrain with no target");

	if ((pev->spawnflags & SF_TRACKTRAIN_PASSABLE) != 0)
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SetModel(STRING(pev->model));

	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);

	// Cache off placed origin for train controls
	pev->oldorigin = pev->origin;

	m_controlMins = pev->mins;
	m_controlMaxs = pev->maxs;
	m_controlMaxs.z += 72;
	// start trains on the next frame, to make sure their targets have had
	// a chance to spawn/activate
	NextThink(pev->ltime + 0.1, false);
	SetThink(&CFuncTrackTrain::Find);
	Precache();
}

void CFuncTrackTrain::Precache()
{
	if (m_flVolume == 0.0)
		m_flVolume = 1.0;

	if (FStrEq("", STRING(m_sounds)))
	{
		m_sounds = string_t::Null;
	}
	else
	{
		PrecacheSound(STRING(m_sounds));
	}

	PrecacheSound("plats/ttrain_brake1.wav");
	PrecacheSound("plats/ttrain_start1.wav");
}

/**
*	@brief This class defines the volume of space that the player must stand in to control the train
*/
class CFuncTrainControls : public CBaseEntity
{
public:
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Spawn() override;
	void EXPORT Find();
};

LINK_ENTITY_TO_CLASS(func_traincontrols, CFuncTrainControls);

void CFuncTrainControls::Find()
{
	CBaseEntity* target = nullptr;

	do
	{
		target = UTIL_FindEntityByTargetname(target, STRING(pev->target));
	} while (!FNullEnt(target) && !target->ClassnameIs("func_tracktrain"));

	if (FNullEnt(target))
	{
		Logger->debug("No train {}", STRING(pev->target));
		return;
	}

	CFuncTrackTrain* ptrain = CFuncTrackTrain::Instance(target);
	ptrain->SetControls(this);
	UTIL_Remove(this);
}

void CFuncTrainControls::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetModel(STRING(pev->model));

	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);

	SetThink(&CFuncTrainControls::Find);
	pev->nextthink = gpGlobals->time;
}

#define SF_TRACK_ACTIVATETRAIN 0x00000001
#define SF_TRACK_RELINK 0x00000002
#define SF_TRACK_ROTMOVE 0x00000004
#define SF_TRACK_STARTBOTTOM 0x00000008
#define SF_TRACK_DONT_MOVE 0x00000010

enum TRAIN_CODE
{
	TRAIN_SAFE,
	TRAIN_BLOCKING,
	TRAIN_FOLLOWING
};

/**
*	@brief Track changer / Train elevator
*	@details This entity is a rotating/moving platform that will carry a train to a new track.
*	It must be larger in X-Y planar area than the train,
*	since it must contain the train within these dimensions in order to operate when the train is near it.
*/
class CFuncTrackChange : public CFuncPlatRot
{
public:
	void Spawn() override;
	void Precache() override;

	//	void	Blocked() override;
	void EXPORT GoUp() override;
	void EXPORT GoDown() override;

	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT Find();
	TRAIN_CODE EvaluateTrain(CPathTrack* pcurrent);
	void UpdateTrain(Vector& dest);
	void HitBottom() override;
	void HitTop() override;
	void Touch(CBaseEntity* pOther) override;
	virtual void UpdateAutoTargets(int toggleState);
	bool IsTogglePlat() override { return true; }

	void DisableUse() { m_use = false; }
	void EnableUse() { m_use = true; }
	bool UseEnabled() { return m_use; }

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void OverrideReset() override;


	CPathTrack* m_trackTop;
	CPathTrack* m_trackBottom;

	CFuncTrackTrain* m_train;

	string_t m_trackTopName;
	string_t m_trackBottomName;
	string_t m_trainName;
	TRAIN_CODE m_code;
	int m_targetState;
	bool m_use;
};

LINK_ENTITY_TO_CLASS(func_trackchange, CFuncTrackChange);

TYPEDESCRIPTION CFuncTrackChange::m_SaveData[] =
	{
		DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trackTop, FIELD_CLASSPTR),
		DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trackBottom, FIELD_CLASSPTR),
		DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_train, FIELD_CLASSPTR),
		DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trackTopName, FIELD_STRING),
		DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trackBottomName, FIELD_STRING),
		DEFINE_GLOBAL_FIELD(CFuncTrackChange, m_trainName, FIELD_STRING),
		DEFINE_FIELD(CFuncTrackChange, m_code, FIELD_INTEGER),
		DEFINE_FIELD(CFuncTrackChange, m_targetState, FIELD_INTEGER),
		DEFINE_FIELD(CFuncTrackChange, m_use, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CFuncTrackChange, CFuncPlatRot);

void CFuncTrackChange::Spawn()
{
	Setup();
	if (FBitSet(pev->spawnflags, SF_TRACK_DONT_MOVE))
		m_vecPosition2.z = pev->origin.z;

	SetupRotation();

	if (FBitSet(pev->spawnflags, SF_TRACK_STARTBOTTOM))
	{
		SetOrigin(m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
		pev->angles = m_start;
		m_targetState = TS_AT_TOP;
	}
	else
	{
		SetOrigin(m_vecPosition1);
		m_toggle_state = TS_AT_TOP;
		pev->angles = m_end;
		m_targetState = TS_AT_BOTTOM;
	}

	EnableUse();
	pev->nextthink = pev->ltime + 2.0;
	SetThink(&CFuncTrackChange::Find);
	Precache();
}

void CFuncTrackChange::Precache()
{
	// Can't trigger sound
	PrecacheSound("buttons/button11.wav");

	CFuncPlatRot::Precache();
}

// UNDONE: Filter touches before re-evaluating the train.
void CFuncTrackChange::Touch(CBaseEntity* pOther)
{
#if 0
	TRAIN_CODE code;
#endif
}

bool CFuncTrackChange::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "train"))
	{
		m_trainName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "toptrack"))
	{
		m_trackTopName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "bottomtrack"))
	{
		m_trackBottomName = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CFuncPlatRot::KeyValue(pkvd); // Pass up to base class
}

void CFuncTrackChange::OverrideReset()
{
	pev->nextthink = pev->ltime + 1.0;
	SetThink(&CFuncTrackChange::Find);
}

void CFuncTrackChange::Find()
{
	// Find track entities
	auto target = UTIL_FindEntityByTargetname(nullptr, STRING(m_trackTopName));
	if (!FNullEnt(target))
	{
		m_trackTop = CPathTrack::Instance(target);
		target = UTIL_FindEntityByTargetname(nullptr, STRING(m_trackBottomName));
		if (!FNullEnt(target))
		{
			m_trackBottom = CPathTrack::Instance(target);
			target = UTIL_FindEntityByTargetname(nullptr, STRING(m_trainName));
			if (!FNullEnt(target))
			{
				m_train = CFuncTrackTrain::Instance(UTIL_FindEntityByTargetname(nullptr, STRING(m_trainName)));

				if (!m_train)
				{
					Logger->error("Can't find train for track change! {}", STRING(m_trainName));
					return;
				}
				Vector center = (pev->absmin + pev->absmax) * 0.5;
				m_trackBottom = m_trackBottom->Nearest(center);
				m_trackTop = m_trackTop->Nearest(center);
				UpdateAutoTargets(m_toggle_state);
				SetThink(nullptr);
				return;
			}
			else
			{
				Logger->error("Can't find train for track change! {}", STRING(m_trainName));
				// TODO: probably unnecessary
				target = UTIL_FindEntityByTargetname(nullptr, STRING(m_trainName));
			}
		}
		else
			Logger->error("Can't find bottom track for track change! {}", STRING(m_trackBottomName));
	}
	else
		Logger->error("Can't find top track for track change! {}", STRING(m_trackTopName));
}

TRAIN_CODE CFuncTrackChange::EvaluateTrain(CPathTrack* pcurrent)
{
	// Go ahead and work, we don't have anything to switch, so just be an elevator
	if (!pcurrent || !m_train)
		return TRAIN_SAFE;

	if (m_train->m_ppath == pcurrent || (pcurrent->m_pprevious && m_train->m_ppath == pcurrent->m_pprevious) ||
		(pcurrent->m_pnext && m_train->m_ppath == pcurrent->m_pnext))
	{
		if (m_train->pev->speed != 0)
			return TRAIN_BLOCKING;

		Vector dist = pev->origin - m_train->pev->origin;
		float length = dist.Length2D();
		if (length < m_train->m_length) // Empirically determined close distance
			return TRAIN_FOLLOWING;
		else if (length > (150 + m_train->m_length))
			return TRAIN_SAFE;

		return TRAIN_BLOCKING;
	}

	return TRAIN_SAFE;
}

void CFuncTrackChange::UpdateTrain(Vector& dest)
{
	float time = (pev->nextthink - pev->ltime);

	m_train->pev->velocity = pev->velocity;
	m_train->pev->avelocity = pev->avelocity;
	m_train->NextThink(m_train->pev->ltime + time, false);

	// Attempt at getting the train to rotate properly around the origin of the trackchange
	if (time <= 0)
		return;

	Vector offset = m_train->pev->origin - pev->origin;
	Vector delta = dest - pev->angles;
	// Transform offset into local coordinates
	UTIL_MakeInvVectors(delta, gpGlobals);
	Vector local;
	local.x = DotProduct(offset, gpGlobals->v_forward);
	local.y = DotProduct(offset, gpGlobals->v_right);
	local.z = DotProduct(offset, gpGlobals->v_up);

	local = local - offset;
	m_train->pev->velocity = pev->velocity + (local * (1.0 / time));
}

void CFuncTrackChange::GoDown()
{
	if (m_code == TRAIN_BLOCKING)
		return;

	// HitBottom may get called during CFuncPlat::GoDown(), so set up for that
	// before you call GoDown()

	UpdateAutoTargets(TS_GOING_DOWN);
	// If ROTMOVE, move & rotate
	if (FBitSet(pev->spawnflags, SF_TRACK_DONT_MOVE))
	{
		SetMoveDone(&CFuncTrackChange::CallHitBottom);
		m_toggle_state = TS_GOING_DOWN;
		AngularMove(m_start, pev->speed);
	}
	else
	{
		CFuncPlat::GoDown();
		SetMoveDone(&CFuncTrackChange::CallHitBottom);
		RotMove(m_start, pev->nextthink - pev->ltime);
	}
	// Otherwise, rotate first, move second

	// If the train is moving with the platform, update it
	if (m_code == TRAIN_FOLLOWING)
	{
		UpdateTrain(m_start);
		m_train->m_ppath = nullptr;
	}
}

void CFuncTrackChange::GoUp()
{
	if (m_code == TRAIN_BLOCKING)
		return;

	// HitTop may get called during CFuncPlat::GoUp(), so set up for that
	// before you call GoUp();

	UpdateAutoTargets(TS_GOING_UP);
	if (FBitSet(pev->spawnflags, SF_TRACK_DONT_MOVE))
	{
		m_toggle_state = TS_GOING_UP;
		SetMoveDone(&CFuncTrackChange::CallHitTop);
		AngularMove(m_end, pev->speed);
	}
	else
	{
		// If ROTMOVE, move & rotate
		CFuncPlat::GoUp();
		SetMoveDone(&CFuncTrackChange::CallHitTop);
		RotMove(m_end, pev->nextthink - pev->ltime);
	}

	// Otherwise, move first, rotate second

	// If the train is moving with the platform, update it
	if (m_code == TRAIN_FOLLOWING)
	{
		UpdateTrain(m_end);
		m_train->m_ppath = nullptr;
	}
}

void CFuncTrackChange::UpdateAutoTargets(int toggleState)
{
	if (!m_trackTop || !m_trackBottom)
		return;

	if (toggleState == TS_AT_TOP)
		ClearBits(m_trackTop->pev->spawnflags, SF_PATH_DISABLED);
	else
		SetBits(m_trackTop->pev->spawnflags, SF_PATH_DISABLED);

	if (toggleState == TS_AT_BOTTOM)
		ClearBits(m_trackBottom->pev->spawnflags, SF_PATH_DISABLED);
	else
		SetBits(m_trackBottom->pev->spawnflags, SF_PATH_DISABLED);
}

void CFuncTrackChange::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (m_toggle_state != TS_AT_TOP && m_toggle_state != TS_AT_BOTTOM)
		return;

	// If train is in "safe" area, but not on the elevator, play alarm sound
	if (m_toggle_state == TS_AT_TOP)
		m_code = EvaluateTrain(m_trackTop);
	else if (m_toggle_state == TS_AT_BOTTOM)
		m_code = EvaluateTrain(m_trackBottom);
	else
		m_code = TRAIN_BLOCKING;
	if (m_code == TRAIN_BLOCKING)
	{
		// Play alarm and return
		EmitSound(CHAN_VOICE, "buttons/button11.wav", 1, ATTN_NORM);
		return;
	}

	// Otherwise, it's safe to move
	// If at top, go down
	// at bottom, go up

	DisableUse();
	if (m_toggle_state == TS_AT_TOP)
		GoDown();
	else
		GoUp();
}

void CFuncTrackChange::HitBottom()
{
	CFuncPlatRot::HitBottom();
	if (m_code == TRAIN_FOLLOWING)
	{
		//		UpdateTrain();
		m_train->SetTrack(m_trackBottom);
	}
	SetThink(nullptr);
	pev->nextthink = -1;

	UpdateAutoTargets(m_toggle_state);

	EnableUse();
}

void CFuncTrackChange::HitTop()
{
	CFuncPlatRot::HitTop();
	if (m_code == TRAIN_FOLLOWING)
	{
		//		UpdateTrain();
		m_train->SetTrack(m_trackTop);
	}

	// Don't let the plat go back down
	SetThink(nullptr);
	pev->nextthink = -1;
	UpdateAutoTargets(m_toggle_state);
	EnableUse();
}

class CFuncTrackAuto : public CFuncTrackChange
{
public:
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void UpdateAutoTargets(int toggleState) override;
};

LINK_ENTITY_TO_CLASS(func_trackautochange, CFuncTrackAuto);

void CFuncTrackAuto::UpdateAutoTargets(int toggleState)
{
	CPathTrack *pTarget, *pNextTarget;

	if (!m_trackTop || !m_trackBottom)
		return;

	if (m_targetState == TS_AT_TOP)
	{
		pTarget = m_trackTop->GetNext();
		pNextTarget = m_trackBottom->GetNext();
	}
	else
	{
		pTarget = m_trackBottom->GetNext();
		pNextTarget = m_trackTop->GetNext();
	}
	if (pTarget)
	{
		ClearBits(pTarget->pev->spawnflags, SF_PATH_DISABLED);
		if (m_code == TRAIN_FOLLOWING && m_train && m_train->pev->speed == 0)
			m_train->Use(this, this, USE_ON, 0);
	}

	if (pNextTarget)
		SetBits(pNextTarget->pev->spawnflags, SF_PATH_DISABLED);
}

void CFuncTrackAuto::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CPathTrack* pTarget;

	if (!UseEnabled())
		return;

	if (m_toggle_state == TS_AT_TOP)
		pTarget = m_trackTop;
	else if (m_toggle_state == TS_AT_BOTTOM)
		pTarget = m_trackBottom;
	else
		pTarget = nullptr;

	if (pActivator->ClassnameIs("func_tracktrain"))
	{
		m_code = EvaluateTrain(pTarget);
		// Safe to fire?
		if (m_code == TRAIN_FOLLOWING && m_toggle_state != m_targetState)
		{
			DisableUse();
			if (m_toggle_state == TS_AT_TOP)
				GoDown();
			else
				GoUp();
		}
	}
	else
	{
		if (pTarget)
			pTarget = pTarget->GetNext();
		if (pTarget && m_train->m_ppath != pTarget && ShouldToggle(useType, TS_AT_TOP != m_targetState))
		{
			if (m_targetState == TS_AT_TOP)
				m_targetState = TS_AT_BOTTOM;
			else
				m_targetState = TS_AT_TOP;
		}

		UpdateAutoTargets(m_targetState);
	}
}

#define FGUNTARGET_START_ON 0x0001

/**
*	@details pev->speed is the travel speed
*	pev->health is current health
*	pev->max_health is the amount to reset to each time it starts
*/
class CGunTarget : public CBaseMonster
{
public:
	void Spawn() override;
	void Activate() override;
	void EXPORT Next();
	void EXPORT Start();
	void EXPORT Wait();
	void Stop() override;

	int BloodColor() override { return DONT_BLEED; }
	int Classify() override { return CLASS_MACHINE; }
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	Vector BodyTarget(const Vector& posSrc) override { return pev->origin; }

	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

private:
	bool m_on;
};

LINK_ENTITY_TO_CLASS(func_guntarget, CGunTarget);

TYPEDESCRIPTION CGunTarget::m_SaveData[] =
	{
		DEFINE_FIELD(CGunTarget, m_on, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CGunTarget, CBaseMonster);

void CGunTarget::Spawn()
{
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SetOrigin(pev->origin);
	SetModel(STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 100;

	// Don't take damage until "on"
	pev->takedamage = DAMAGE_NO;
	pev->flags |= FL_MONSTER;

	m_on = false;
	pev->max_health = pev->health;

	if ((pev->spawnflags & FGUNTARGET_START_ON) != 0)
	{
		SetThink(&CGunTarget::Start);
		pev->nextthink = pev->ltime + 0.3;
	}
}

void CGunTarget::Activate()
{
	CBaseEntity* pTarg;

	// now find our next target
	pTarg = GetNextTarget();
	if (pTarg)
	{
		m_hTargetEnt = pTarg;
		SetOrigin(pTarg->pev->origin - (pev->mins + pev->maxs) * 0.5);
	}
}

void CGunTarget::Start()
{
	Use(this, this, USE_ON, 0);
}

void CGunTarget::Next()
{
	SetThink(nullptr);

	m_hTargetEnt = GetNextTarget();
	CBaseEntity* pTarget = m_hTargetEnt;

	if (!pTarget)
	{
		Stop();
		return;
	}
	SetMoveDone(&CGunTarget::Wait);
	LinearMove(pTarget->pev->origin - (pev->mins + pev->maxs) * 0.5, pev->speed);
}

void CGunTarget::Wait()
{
	CBaseEntity* pTarget = m_hTargetEnt;

	if (!pTarget)
	{
		Stop();
		return;
	}

	// Fire the pass target if there is one
	if (!FStringNull(pTarget->pev->message))
	{
		FireTargets(STRING(pTarget->pev->message), this, this, USE_TOGGLE, 0);
		if (FBitSet(pTarget->pev->spawnflags, SF_CORNER_FIREONCE))
			pTarget->pev->message = string_t::Null;
	}

	m_flWait = pTarget->GetDelay();

	pev->target = pTarget->pev->target;
	SetThink(&CGunTarget::Next);
	if (m_flWait != 0)
	{ // -1 wait will wait forever!
		pev->nextthink = pev->ltime + m_flWait;
	}
	else
	{
		Next(); // do it RIGHT now!
	}
}

void CGunTarget::Stop()
{
	pev->velocity = g_vecZero;
	pev->nextthink = 0;
	pev->takedamage = DAMAGE_NO;
}

bool CGunTarget::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (pev->health > 0)
	{
		pev->health -= flDamage;
		if (pev->health <= 0)
		{
			pev->health = 0;
			Stop();
			if (!FStringNull(pev->message))
				FireTargets(STRING(pev->message), this, this, USE_TOGGLE, 0);
		}
	}
	return false;
}

void CGunTarget::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_on))
		return;

	if (m_on)
	{
		Stop();
	}
	else
	{
		pev->takedamage = DAMAGE_AIM;
		m_hTargetEnt = GetNextTarget();
		if (m_hTargetEnt == nullptr)
			return;
		pev->health = pev->max_health;
		Next();
	}
}
