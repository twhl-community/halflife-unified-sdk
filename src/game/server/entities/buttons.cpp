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
 *	button-related code
 */

#include "cbase.h"
#include "doors.h"

#define SF_BUTTON_DONTMOVE 1
#define SF_ROTBUTTON_NOTSOLID 1
#define SF_BUTTON_TOGGLE 32		  // button stays pushed until reactivated
#define SF_BUTTON_SPARK_IF_OFF 64 // button sparks in OFF state
#define SF_BUTTON_TOUCH_ONLY 256  // button only fires as a result of USE key.

#define SF_GLOBAL_SET 1 // Set global state to initial state on spawn

class CEnvGlobal : public CPointEntity
{
	DECLARE_CLASS(CEnvGlobal, CPointEntity);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	string_t m_globalstate;
	int m_triggermode;
	int m_initialstate;
};

BEGIN_DATAMAP(CEnvGlobal)
DEFINE_FIELD(m_globalstate, FIELD_STRING),
	DEFINE_FIELD(m_triggermode, FIELD_INTEGER),
	DEFINE_FIELD(m_initialstate, FIELD_INTEGER),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(env_global, CEnvGlobal);

bool CEnvGlobal::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "globalstate")) // State name
	{
		m_globalstate = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "triggermode"))
	{
		m_triggermode = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "initialstate"))
	{
		m_initialstate = atoi(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

void CEnvGlobal::Spawn()
{
	if (FStringNull(m_globalstate))
	{
		REMOVE_ENTITY(edict());
		return;
	}
	if (FBitSet(pev->spawnflags, SF_GLOBAL_SET))
	{
		if (!gGlobalState.EntityInTable(m_globalstate))
			gGlobalState.EntityAdd(m_globalstate, gpGlobals->mapname, (GLOBALESTATE)m_initialstate);
	}
}

void CEnvGlobal::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	GLOBALESTATE oldState = gGlobalState.EntityGetState(m_globalstate);
	GLOBALESTATE newState;

	switch (m_triggermode)
	{
	case 0:
		newState = GLOBAL_OFF;
		break;

	case 1:
		newState = GLOBAL_ON;
		break;

	case 2:
		newState = GLOBAL_DEAD;
		break;

	default:
	case 3:
		if (oldState == GLOBAL_ON)
			newState = GLOBAL_OFF;
		else if (oldState == GLOBAL_OFF)
			newState = GLOBAL_ON;
		else
			newState = oldState;
	}

	if (gGlobalState.EntityInTable(m_globalstate))
		gGlobalState.EntitySetState(m_globalstate, newState);
	else
		gGlobalState.EntityAdd(m_globalstate, gpGlobals->mapname, newState);
}

BEGIN_DATAMAP(CMultiSource)
//!!!BUGBUG FIX
DEFINE_ARRAY(m_rgEntities, FIELD_EHANDLE, MS_MAX_TARGETS),
	DEFINE_ARRAY(m_rgTriggered, FIELD_INTEGER, MS_MAX_TARGETS),
	DEFINE_FIELD(m_iTotal, FIELD_INTEGER),
	DEFINE_FIELD(m_globalstate, FIELD_STRING),
	DEFINE_FUNCTION(Register),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(multisource, CMultiSource);

bool CMultiSource::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CPointEntity::KeyValue(pkvd);
}

#define SF_MULTI_INIT 1

void CMultiSource::Spawn()
{
	// set up think for later registration

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->spawnflags |= SF_MULTI_INIT; // Until it's initialized
	SetThink(&CMultiSource::Register);
}

void CMultiSource::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	int i = 0;

	// Find the entity in our list
	while (i < m_iTotal)
		if (m_rgEntities[i++] == pCaller)
			break;

	// if we didn't find it, report error and leave
	if (i > m_iTotal)
	{
		IOLogger->debug("MultiSrc:Used by non member {}.", STRING(pCaller->pev->classname));
		return;
	}

	// CONSIDER: a Use input to the multisource always toggles.  Could check useType for ON/OFF/TOGGLE

	m_rgTriggered[i - 1] ^= 1;

	//
	if (IsTriggered(pActivator))
	{
		IOLogger->trace("Multisource {} enabled ({} inputs)", STRING(pev->targetname), m_iTotal);
		USE_TYPE targetUseType = USE_TOGGLE;
		if (!FStringNull(m_globalstate))
			targetUseType = USE_ON;
		SUB_UseTargets(nullptr, targetUseType, 0);
	}
}

bool CMultiSource::IsTriggered(CBaseEntity*)
{
	// Is everything triggered?
	int i = 0;

	// Still initializing?
	if ((pev->spawnflags & SF_MULTI_INIT) != 0)
		return false;

	while (i < m_iTotal)
	{
		if (m_rgTriggered[i] == 0)
			break;
		i++;
	}

	if (i == m_iTotal)
	{
		if (FStringNull(m_globalstate) || gGlobalState.EntityGetState(m_globalstate) == GLOBAL_ON)
			return true;
	}

	return false;
}

void CMultiSource::Register()
{
	m_iTotal = 0;
	std::fill(std::begin(m_rgEntities), std::end(m_rgEntities), EHANDLE{});

	SetThink(nullptr);

	// search for all entities which target this multisource (pev->targetname)
	for (auto target : UTIL_FindEntitiesByTarget(STRING(pev->targetname)))
	{
		if (m_iTotal >= MS_MAX_TARGETS)
		{
			break;
		}

		m_rgEntities[m_iTotal++] = target;
	}

	for (auto target : UTIL_FindEntitiesByClassname("multi_manager"))
	{
		if (m_iTotal >= MS_MAX_TARGETS)
		{
			break;
		}

		if (target->HasTarget(pev->targetname))
		{
			m_rgEntities[m_iTotal++] = target;
		}
	}

	pev->spawnflags &= ~SF_MULTI_INIT;
}

BEGIN_DATAMAP(CBaseButton)
DEFINE_FIELD(m_fStayPushed, FIELD_BOOLEAN),
	DEFINE_FIELD(m_fRotating, FIELD_BOOLEAN),

	DEFINE_FIELD(m_sounds, FIELD_SOUNDNAME),
	DEFINE_FIELD(m_LockedSound, FIELD_SOUNDNAME),
	DEFINE_FIELD(m_LockedSentence, FIELD_STRING),
	DEFINE_FIELD(m_UnlockedSound, FIELD_SOUNDNAME),
	DEFINE_FIELD(m_UnlockedSentence, FIELD_STRING),
	DEFINE_FIELD(m_strChangeTarget, FIELD_STRING),
	//	DEFINE_FIELD(m_ls, FIELD_???),   // This is restored in Precache()
	DEFINE_FUNCTION(ButtonTouch),
	DEFINE_FUNCTION(ButtonSpark),
	DEFINE_FUNCTION(TriggerAndWait),
	DEFINE_FUNCTION(ButtonReturn),
	DEFINE_FUNCTION(ButtonBackHome),
	DEFINE_FUNCTION(ButtonUse),
	END_DATAMAP();

void CBaseButton::Precache()
{
	if (FBitSet(pev->spawnflags, SF_BUTTON_SPARK_IF_OFF)) // this button should spark in OFF state
	{
		PrecacheSound("buttons/spark1.wav");
		PrecacheSound("buttons/spark2.wav");
		PrecacheSound("buttons/spark3.wav");
		PrecacheSound("buttons/spark4.wav");
		PrecacheSound("buttons/spark5.wav");
		PrecacheSound("buttons/spark6.wav");
	}

	// get door button sounds, for doors which require buttons to open

	if (!FStringNull(m_LockedSound))
	{
		PrecacheSound(STRING(m_LockedSound));
		m_ls.sLockedSound = m_LockedSound;
	}

	if (!FStringNull(m_UnlockedSound))
	{
		PrecacheSound(STRING(m_UnlockedSound));
		m_ls.sUnlockedSound = m_UnlockedSound;
	}

	// get sentence group names, for doors which are directly 'touched' to open
	m_ls.sLockedSentence = m_LockedSentence;
	m_ls.sUnlockedSentence = m_UnlockedSentence;
}

bool CBaseButton::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "changetarget"))
	{
		m_strChangeTarget = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_sound"))
	{
		m_LockedSound = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_sentence"))
	{
		m_LockedSentence = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "unlocked_sound"))
	{
		m_UnlockedSound = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "unlocked_sentence"))
	{
		m_UnlockedSentence = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

bool CBaseButton::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	BUTTON_CODE code = ButtonResponseToTouch();

	if (code == BUTTON_NOTHING)
		return false;
	// Temporarily disable the touch function, until movement is finished.
	SetTouch(nullptr);

	m_hActivator = CBaseEntity::Instance(attacker);
	if (m_hActivator == nullptr)
		return false;

	if (code == BUTTON_RETURN)
	{
		EmitSound(CHAN_VOICE, STRING(m_sounds), 1, ATTN_NORM);

		// Toggle buttons fire when they get back to their "home" position
		if ((pev->spawnflags & SF_BUTTON_TOGGLE) == 0)
			SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);
		ButtonReturn();
	}
	else // code == BUTTON_ACTIVATE
		ButtonActivate();

	return false;
}

LINK_ENTITY_TO_CLASS(func_button, CBaseButton);

void CBaseButton::Spawn()
{
	//----------------------------------------------------
	// determine sounds for buttons
	// a sound of 0 should not make a sound
	//----------------------------------------------------
	if (FStrEq("", STRING(m_sounds)))
	{
		m_sounds = ALLOC_STRING("common/null.wav");
	}

	PrecacheSound(STRING(m_sounds));

	Precache();

	if (FBitSet(pev->spawnflags, SF_BUTTON_SPARK_IF_OFF)) // this button should spark in OFF state
	{
		SetThink(&CBaseButton::ButtonSpark);
		pev->nextthink = gpGlobals->time + 0.5; // no hurry, make sure everything else spawns
	}

	SetMovedir(this);

	pev->movetype = MOVETYPE_PUSH;
	pev->solid = SOLID_BSP;
	SetModel(STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 40;

	if (pev->health > 0)
	{
		pev->takedamage = DAMAGE_YES;
	}

	if (m_flWait == 0)
		m_flWait = 1;
	if (m_flLip == 0)
		m_flLip = 4;

	m_toggle_state = TS_AT_BOTTOM;
	m_vecPosition1 = pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + (pev->movedir * (fabs(pev->movedir.x * (pev->size.x - 2)) + fabs(pev->movedir.y * (pev->size.y - 2)) + fabs(pev->movedir.z * (pev->size.z - 2)) - m_flLip));


	// Is this a non-moving button?
	if (((m_vecPosition2 - m_vecPosition1).Length() < 1) || (pev->spawnflags & SF_BUTTON_DONTMOVE) != 0)
		m_vecPosition2 = m_vecPosition1;

	m_fStayPushed = (m_flWait == -1 ? true : false);
	m_fRotating = false;

	// if the button is flagged for USE button activation only, take away it's touch function and add a use function

	if (FBitSet(pev->spawnflags, SF_BUTTON_TOUCH_ONLY)) // touchable button
	{
		SetTouch(&CBaseButton::ButtonTouch);
	}
	else
	{
		SetTouch(nullptr);
		SetUse(&CBaseButton::ButtonUse);
	}
}

void DoSpark(CBaseEntity* entity, const Vector& location)
{
	Vector tmp = location + entity->pev->size * 0.5;
	UTIL_Sparks(tmp);

	float flVolume = RANDOM_FLOAT(0.25, 0.75) * 0.4; // random volume range
	switch ((int)(RANDOM_FLOAT(0, 1) * 6))
	{
	case 0:
		entity->EmitSound(CHAN_VOICE, "buttons/spark1.wav", flVolume, ATTN_NORM);
		break;
	case 1:
		entity->EmitSound(CHAN_VOICE, "buttons/spark2.wav", flVolume, ATTN_NORM);
		break;
	case 2:
		entity->EmitSound(CHAN_VOICE, "buttons/spark3.wav", flVolume, ATTN_NORM);
		break;
	case 3:
		entity->EmitSound(CHAN_VOICE, "buttons/spark4.wav", flVolume, ATTN_NORM);
		break;
	case 4:
		entity->EmitSound(CHAN_VOICE, "buttons/spark5.wav", flVolume, ATTN_NORM);
		break;
	case 5:
		entity->EmitSound(CHAN_VOICE, "buttons/spark6.wav", flVolume, ATTN_NORM);
		break;
	}
}

void CBaseButton::ButtonSpark()
{
	SetThink(&CBaseButton::ButtonSpark);
	pev->nextthink = pev->ltime + (0.1 + RANDOM_FLOAT(0, 1.5)); // spark again at random interval

	DoSpark(this, pev->absmin);
}

void CBaseButton::ButtonUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Ignore touches if button is moving, or pushed-in and waiting to auto-come-out.
	// UNDONE: Should this use ButtonResponseToTouch() too?
	if (m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN)
		return;

	m_hActivator = pActivator;
	if (m_toggle_state == TS_AT_TOP)
	{
		if (!m_fStayPushed && FBitSet(pev->spawnflags, SF_BUTTON_TOGGLE))
		{
			EmitSound(CHAN_VOICE, STRING(m_sounds), 1, ATTN_NORM);
			ButtonReturn();
		}
	}
	else
		ButtonActivate();
}

CBaseButton::BUTTON_CODE CBaseButton::ButtonResponseToTouch()
{
	// Ignore touches if button is moving, or pushed-in and waiting to auto-come-out.
	if (m_toggle_state == TS_GOING_UP ||
		m_toggle_state == TS_GOING_DOWN ||
		(m_toggle_state == TS_AT_TOP && !m_fStayPushed && !FBitSet(pev->spawnflags, SF_BUTTON_TOGGLE)))
		return BUTTON_NOTHING;

	if (m_toggle_state == TS_AT_TOP)
	{
		if ((FBitSet(pev->spawnflags, SF_BUTTON_TOGGLE)) && !m_fStayPushed)
		{
			return BUTTON_RETURN;
		}
	}
	else
		return BUTTON_ACTIVATE;

	return BUTTON_NOTHING;
}

void CBaseButton::ButtonTouch(CBaseEntity* pOther)
{
	// Ignore touches by anything but players
	if (!pOther->IsPlayer())
		return;

	m_hActivator = pOther;

	BUTTON_CODE code = ButtonResponseToTouch();

	if (code == BUTTON_NOTHING)
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther, m_UseLocked))
	{
		// play button locked sound
		PlayLockSounds(this, &m_ls, true, true);
		return;
	}

	// Temporarily disable the touch function, until movement is finished.
	SetTouch(nullptr);

	if (code == BUTTON_RETURN)
	{
		EmitSound(CHAN_VOICE, STRING(m_sounds), 1, ATTN_NORM);
		SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);
		ButtonReturn();
	}
	else // code == BUTTON_ACTIVATE
		ButtonActivate();
}

void CBaseButton::ButtonActivate()
{
	EmitSound(CHAN_VOICE, STRING(m_sounds), 1, ATTN_NORM);

	if (!UTIL_IsMasterTriggered(m_sMaster, m_hActivator, m_UseLocked))
	{
		// button is locked, play locked sound
		PlayLockSounds(this, &m_ls, true, true);
		return;
	}
	else
	{
		// button is unlocked, play unlocked sound
		PlayLockSounds(this, &m_ls, false, true);
	}

	ASSERT(m_toggle_state == TS_AT_BOTTOM);
	m_toggle_state = TS_GOING_UP;

	SetMoveDone(&CBaseButton::TriggerAndWait);
	if (!m_fRotating)
		LinearMove(m_vecPosition2, pev->speed);
	else
		AngularMove(m_vecAngle2, pev->speed);
}

void CBaseButton::TriggerAndWait()
{
	ASSERT(m_toggle_state == TS_GOING_UP);

	if (!UTIL_IsMasterTriggered(m_sMaster, m_hActivator, m_UseLocked))
		return;

	m_toggle_state = TS_AT_TOP;

	// If button automatically comes back out, start it moving out.
	// Else re-instate touch method
	if (m_fStayPushed || FBitSet(pev->spawnflags, SF_BUTTON_TOGGLE))
	{
		if (!FBitSet(pev->spawnflags, SF_BUTTON_TOUCH_ONLY)) // this button only works if USED, not touched!
		{
			// ALL buttons are now use only
			SetTouch(nullptr);
		}
		else
			SetTouch(&CBaseButton::ButtonTouch);
	}
	else
	{
		pev->nextthink = pev->ltime + m_flWait;
		SetThink(&CBaseButton::ButtonReturn);
	}

	pev->frame = 1; // use alternate textures


	SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);
}

void CBaseButton::ButtonReturn()
{
	ASSERT(m_toggle_state == TS_AT_TOP);
	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone(&CBaseButton::ButtonBackHome);
	if (!m_fRotating)
		LinearMove(m_vecPosition1, pev->speed);
	else
		AngularMove(m_vecAngle1, pev->speed);

	pev->frame = 0; // use normal textures
}

void CBaseButton::ButtonBackHome()
{
	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;

	if (FBitSet(pev->spawnflags, SF_BUTTON_TOGGLE))
	{
		// EmitSound(CHAN_VOICE, STRING(m_sounds), 1, ATTN_NORM);

		SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);
	}


	if (!FStringNull(pev->target))
	{
		for (auto target : UTIL_FindEntitiesByTargetname(STRING(pev->target)))
		{
			if (!target->ClassnameIs("multisource"))
				continue;

			target->Use(m_hActivator, this, USE_TOGGLE, 0);
		}
	}

	// Re-instate touch method, movement cycle is complete.
	if (!FBitSet(pev->spawnflags, SF_BUTTON_TOUCH_ONLY)) // this button only works if USED, not touched!
	{
		// All buttons are now use only
		SetTouch(nullptr);
	}
	else
		SetTouch(&CBaseButton::ButtonTouch);

	// reset think for a sparking button
	// func_rot_button's X Axis spawnflag overlaps with this one so don't use it here.
	if (!ClassnameIs("func_rot_button") && FBitSet(pev->spawnflags, SF_BUTTON_SPARK_IF_OFF))
	{
		SetThink(&CBaseButton::ButtonSpark);
		pev->nextthink = pev->ltime + 0.5; // no hurry.
	}
}

/**
 *	@brief Rotating button (aka "lever")
 */
class CRotButton : public CBaseButton
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS(func_rot_button, CRotButton);

void CRotButton::Spawn()
{
	//----------------------------------------------------
	// determine sounds for buttons
	// a sound of 0 should not make a sound
	//----------------------------------------------------
	if (FStrEq("", STRING(m_sounds)))
	{
		m_sounds = ALLOC_STRING("common/null.wav");
	}

	PrecacheSound(STRING(m_sounds));

	// set the axis of rotation
	CBaseToggle::AxisDir(this);

	// check for clockwise rotation
	if (FBitSet(pev->spawnflags, SF_DOOR_ROTATE_BACKWARDS))
		pev->movedir = pev->movedir * -1;

	pev->movetype = MOVETYPE_PUSH;

	if ((pev->spawnflags & SF_ROTBUTTON_NOTSOLID) != 0)
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	SetModel(STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 40;

	if (m_flWait == 0)
		m_flWait = 1;

	if (pev->health > 0)
	{
		pev->takedamage = DAMAGE_YES;
	}

	m_toggle_state = TS_AT_BOTTOM;
	m_vecAngle1 = pev->angles;
	m_vecAngle2 = pev->angles + pev->movedir * m_flMoveDistance;
	ASSERTSZ(m_vecAngle1 != m_vecAngle2, "rotating button start/end positions are equal");

	m_fStayPushed = (m_flWait == -1 ? true : false);
	m_fRotating = true;

	// if the button is flagged for USE button activation only, take away it's touch function and add a use function
	if (!FBitSet(pev->spawnflags, SF_BUTTON_TOUCH_ONLY))
	{
		SetTouch(nullptr);
		SetUse(&CRotButton::ButtonUse);
	}
	else // touchable button
		SetTouch(&CRotButton::ButtonTouch);

	// SetTouch( ButtonTouch );
}


/**
 *	@brief Make this button behave like a door (HACKHACK)
 *	This will disable use and make the button solid
 *	rotating buttons were made SOLID_NOT by default since their were some collision problems with them...
 */
#define SF_MOMENTARY_DOOR 0x0001

class CMomentaryRotButton : public CBaseToggle
{
	DECLARE_CLASS(CMomentaryRotButton, CBaseToggle);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	int ObjectCaps() override
	{
		int flags = CBaseToggle::ObjectCaps() & (~FCAP_ACROSS_TRANSITION);
		if ((pev->spawnflags & SF_MOMENTARY_DOOR) != 0)
			return flags;
		return flags | FCAP_CONTINUOUS_USE;
	}
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void Off();
	void Return();
	void UpdateSelf(float value);
	void UpdateSelfReturn(float value);
	void UpdateAllButtons(float value, bool start);

	void PlaySound();
	void UpdateTarget(float value);

	bool m_lastUsed;
	int m_direction;
	float m_returnSpeed;
	Vector m_start;
	Vector m_end;
	string_t m_sounds;
};

BEGIN_DATAMAP(CMomentaryRotButton)
DEFINE_FIELD(m_lastUsed, FIELD_BOOLEAN),
	DEFINE_FIELD(m_direction, FIELD_INTEGER),
	DEFINE_FIELD(m_returnSpeed, FIELD_FLOAT),
	DEFINE_FIELD(m_start, FIELD_VECTOR),
	DEFINE_FIELD(m_end, FIELD_VECTOR),
	DEFINE_FIELD(m_sounds, FIELD_SOUNDNAME),
	DEFINE_FUNCTION(Off),
	DEFINE_FUNCTION(Return),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(momentary_rot_button, CMomentaryRotButton);

void CMomentaryRotButton::Spawn()
{
	CBaseToggle::AxisDir(this);

	if (pev->speed == 0)
		pev->speed = 100;

	if (m_flMoveDistance < 0)
	{
		m_start = pev->angles + pev->movedir * m_flMoveDistance;
		m_end = pev->angles;
		m_direction = 1; // This will toggle to -1 on the first use()
		m_flMoveDistance = -m_flMoveDistance;
	}
	else
	{
		m_start = pev->angles;
		m_end = pev->angles + pev->movedir * m_flMoveDistance;
		m_direction = -1; // This will toggle to +1 on the first use()
	}

	if ((pev->spawnflags & SF_MOMENTARY_DOOR) != 0)
		pev->solid = SOLID_BSP;
	else
		pev->solid = SOLID_NOT;

	pev->movetype = MOVETYPE_PUSH;
	SetOrigin(pev->origin);
	SetModel(STRING(pev->model));

	if (FStrEq("", STRING(m_sounds)))
	{
		m_sounds = ALLOC_STRING("common/null.wav");
	}

	PrecacheSound(STRING(m_sounds));
	m_lastUsed = false;
}

bool CMomentaryRotButton::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "returnspeed"))
	{
		m_returnSpeed = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

void CMomentaryRotButton::PlaySound()
{
	EmitSound(CHAN_VOICE, STRING(m_sounds), 1, ATTN_NORM);
}

// BUGBUG: This design causes a latentcy.  When the button is retriggered, the first impulse
// will send the target in the wrong direction because the parameter is calculated based on the
// current, not future position.
void CMomentaryRotButton::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	pev->ideal_yaw = CBaseToggle::AxisDelta(pev->spawnflags, pev->angles, m_start) / m_flMoveDistance;

	UpdateAllButtons(pev->ideal_yaw, true);

	// Calculate destination angle and use it to predict value, this prevents sending target in wrong direction on retriggering
	Vector dest = pev->angles + pev->avelocity * (pev->nextthink - pev->ltime);
	float value1 = CBaseToggle::AxisDelta(pev->spawnflags, dest, m_start) / m_flMoveDistance;
	UpdateTarget(value1);
}

void CMomentaryRotButton::UpdateAllButtons(float value, bool start)
{
	// Update all rot buttons attached to the same target
	for (auto target : UTIL_FindEntitiesByTarget(STRING(pev->target)))
	{
		if (target->ClassnameIs("momentary_rot_button"))
		{
			auto pEntity = static_cast<CMomentaryRotButton*>(target);
			if (start)
				pEntity->UpdateSelf(value);
			else
				pEntity->UpdateSelfReturn(value);
		}
	}
}

void CMomentaryRotButton::UpdateSelf(float value)
{
	bool fplaysound = false;

	if (!m_lastUsed)
	{
		fplaysound = true;
		m_direction = -m_direction;
	}
	m_lastUsed = true;

	pev->nextthink = pev->ltime + 0.1;
	if (m_direction > 0 && value >= 1.0)
	{
		pev->avelocity = g_vecZero;
		pev->angles = m_end;
		return;
	}
	else if (m_direction < 0 && value <= 0)
	{
		pev->avelocity = g_vecZero;
		pev->angles = m_start;
		return;
	}

	if (fplaysound)
		PlaySound();

	// HACKHACK -- If we're going slow, we'll get multiple player packets per frame, bump nexthink on each one to avoid stalling
	if (pev->nextthink < pev->ltime)
		pev->nextthink = pev->ltime + 0.1;
	else
		pev->nextthink += 0.1;

	pev->avelocity = (m_direction * pev->speed) * pev->movedir;
	SetThink(&CMomentaryRotButton::Off);
}

void CMomentaryRotButton::UpdateTarget(float value)
{
	if (!FStringNull(pev->target))
	{
		for (auto target : UTIL_FindEntitiesByTargetname(STRING(pev->target)))
		{
			target->Use(this, this, USE_SET, value);
		}
	}
}

void CMomentaryRotButton::Off()
{
	StopSound(CHAN_VOICE, STRING(m_sounds));

	pev->avelocity = g_vecZero;
	m_lastUsed = false;
	if (FBitSet(pev->spawnflags, SF_PENDULUM_AUTO_RETURN) && m_returnSpeed > 0)
	{
		SetThink(&CMomentaryRotButton::Return);
		pev->nextthink = pev->ltime + 0.1;
		m_direction = -1;
	}
	else
		SetThink(nullptr);
}

void CMomentaryRotButton::Return()
{
	float value = CBaseToggle::AxisDelta(pev->spawnflags, pev->angles, m_start) / m_flMoveDistance;

	UpdateAllButtons(value, false); // This will end up calling UpdateSelfReturn() n times, but it still works right
	if (value > 0)
		UpdateTarget(value);
}

void CMomentaryRotButton::UpdateSelfReturn(float value)
{
	if (value <= 0)
	{
		pev->avelocity = g_vecZero;
		pev->angles = m_start;
		pev->nextthink = -1;
		SetThink(nullptr);
	}
	else
	{
		pev->avelocity = -m_returnSpeed * pev->movedir;
		pev->nextthink = pev->ltime + 0.1;
	}
}

class CEnvSpark : public CPointEntity
{
	DECLARE_CLASS(CEnvSpark, CPointEntity);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Precache() override;
	void SparkThink();
	void SparkStart(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void SparkStop(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	bool KeyValue(KeyValueData* pkvd) override;

	float m_flDelay;
};

BEGIN_DATAMAP(CEnvSpark)
DEFINE_FIELD(m_flDelay, FIELD_FLOAT),
	DEFINE_FUNCTION(SparkThink),
	DEFINE_FUNCTION(SparkStart),
	DEFINE_FUNCTION(SparkStop),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(env_spark, CEnvSpark);
LINK_ENTITY_TO_CLASS(env_debris, CEnvSpark);

void CEnvSpark::Spawn()
{
	SetThink(nullptr);
	SetUse(nullptr);

	if (FBitSet(pev->spawnflags, 32)) // Use for on/off
	{
		if (FBitSet(pev->spawnflags, 64)) // Start on
		{
			SetThink(&CEnvSpark::SparkThink); // start sparking
			SetUse(&CEnvSpark::SparkStop);	  // set up +USE to stop sparking
		}
		else
			SetUse(&CEnvSpark::SparkStart);
	}
	else
		SetThink(&CEnvSpark::SparkThink);

	pev->nextthink = gpGlobals->time + (0.1 + RANDOM_FLOAT(0, 1.5));

	if (m_flDelay <= 0)
		m_flDelay = 1.5;

	Precache();
}

void CEnvSpark::Precache()
{
	PrecacheSound("buttons/spark1.wav");
	PrecacheSound("buttons/spark2.wav");
	PrecacheSound("buttons/spark3.wav");
	PrecacheSound("buttons/spark4.wav");
	PrecacheSound("buttons/spark5.wav");
	PrecacheSound("buttons/spark6.wav");
}

bool CEnvSpark::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "MaxDelay"))
	{
		m_flDelay = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CEnvSpark::SparkThink()
{
	pev->nextthink = gpGlobals->time + 0.1 + RANDOM_FLOAT(0, m_flDelay);
	DoSpark(this, pev->origin);
}

void CEnvSpark::SparkStart(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetUse(&CEnvSpark::SparkStop);
	SetThink(&CEnvSpark::SparkThink);
	pev->nextthink = gpGlobals->time + (0.1 + RANDOM_FLOAT(0, m_flDelay));
}

void CEnvSpark::SparkStop(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetUse(&CEnvSpark::SparkStart);
	SetThink(nullptr);
}

#define SF_BTARGET_USE 0x0001
#define SF_BTARGET_ON 0x0002

class CButtonTarget : public CBaseEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	int ObjectCaps() override;
};

LINK_ENTITY_TO_CLASS(button_target, CButtonTarget);

void CButtonTarget::Spawn()
{
	pev->movetype = MOVETYPE_PUSH;
	pev->solid = SOLID_BSP;
	SetModel(STRING(pev->model));
	pev->takedamage = DAMAGE_YES;

	if (FBitSet(pev->spawnflags, SF_BTARGET_ON))
		pev->frame = 1;
}

void CButtonTarget::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, 0 != pev->frame))
		return;
	pev->frame = 1 - pev->frame;
	if (0 != pev->frame)
		SUB_UseTargets(pActivator, USE_ON, 0);
	else
		SUB_UseTargets(pActivator, USE_OFF, 0);
}

int CButtonTarget::ObjectCaps()
{
	int caps = CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;

	if (FBitSet(pev->spawnflags, SF_BTARGET_USE))
		return caps | FCAP_IMPULSE_USE;
	else
		return caps;
}

bool CButtonTarget::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	Use(Instance(attacker), this, USE_TOGGLE, 0);

	return true;
}
