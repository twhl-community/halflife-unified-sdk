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
#include "doors.h"

/**
 *	@brief if two doors touch, they are assumed to be connected and operate as a unit.
 */
class CBaseDoor : public CBaseToggle
{
	DECLARE_CLASS(CBaseDoor, CBaseToggle);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void Blocked(CBaseEntity* pOther) override;


	int ObjectCaps() override
	{
		if ((pev->spawnflags & SF_ITEM_USE_ONLY) != 0)
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;
		else
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	};

	// used to selectivly override defaults
	/**
	 *	@brief Doors not tied to anything (e.g. button, another door) can be touched, to make them activate.
	 */
	void DoorTouch(CBaseEntity* pOther);

	// local functions
	/**
	 *	@brief Causes the door to "do its thing", i.e. start moving, and cascade activation.
	 */
	bool DoorActivate();

	/**
	 *	@brief Starts the door going to its "up" position (simply ToggleData->vecPosition2).
	 */
	void DoorGoUp();

	/**
	 *	@brief Starts the door going to its "down" position (simply ToggleData->vecPosition1).
	 */
	void DoorGoDown();

	/**
	 *	@brief The door has reached the "up" position. Either go back down, or wait for another activation.
	 */
	void DoorHitTop();

	/**
	 *	@brief The door has reached the "down" position. Back to quiescence.
	 */
	void DoorHitBottom();

	byte m_bHealthValue; // some doors are medi-kit doors, they give players health

	string_t m_MoveSound; // sound a door makes while moving
	string_t m_StopSound; // sound a door makes when it stops

	locksound_t m_ls; // door lock sounds

	string_t m_LockedSound;
	string_t m_LockedSentence;
	string_t m_UnlockedSound;
	string_t m_UnlockedSentence;
};

BEGIN_DATAMAP(CBaseDoor)
DEFINE_FIELD(m_bHealthValue, FIELD_CHARACTER),
	DEFINE_FIELD(m_MoveSound, FIELD_SOUNDNAME),
	DEFINE_FIELD(m_StopSound, FIELD_SOUNDNAME),

	DEFINE_FIELD(m_LockedSound, FIELD_SOUNDNAME),
	DEFINE_FIELD(m_LockedSentence, FIELD_STRING),
	DEFINE_FIELD(m_UnlockedSound, FIELD_SOUNDNAME),
	DEFINE_FIELD(m_UnlockedSentence, FIELD_STRING),
	DEFINE_FUNCTION(DoorTouch),
	DEFINE_FUNCTION(DoorGoUp),
	DEFINE_FUNCTION(DoorGoDown),
	DEFINE_FUNCTION(DoorHitTop),
	DEFINE_FUNCTION(DoorHitBottom),
	END_DATAMAP();

#define DOOR_SENTENCEWAIT 6
#define DOOR_SOUNDWAIT 3
#define BUTTON_SOUNDWAIT 0.5

void PlayLockSounds(CBaseEntity* entity, locksound_t* pls, bool flocked, bool fbutton)
{
	// LOCKED SOUND

	// CONSIDER: consolidate the locksound_t struct (all entries are duplicates for lock/unlock)
	// CONSIDER: and condense this code.
	float flsoundwait;

	if (fbutton)
		flsoundwait = BUTTON_SOUNDWAIT;
	else
		flsoundwait = DOOR_SOUNDWAIT;

	if (flocked)
	{
		bool fplaysound = (!FStringNull(pls->sLockedSound) && gpGlobals->time > pls->flwaitSound);
		bool fplaysentence = (!FStringNull(pls->sLockedSentence) && 0 == pls->bEOFLocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		if (fplaysound && fplaysentence)
			fvol = 0.25;
		else
			fvol = 1.0;

		// if there is a locked sound, and we've debounced, play sound
		if (fplaysound)
		{
			// play 'door locked' sound
			entity->EmitSound(CHAN_ITEM, STRING(pls->sLockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// if there is a sentence, we've not played all in list, and we've debounced, play sound
		if (fplaysentence)
		{
			// play next 'door locked' sentence in group
			int iprev = pls->iLockedSentence;

			pls->iLockedSentence = sentences::g_Sentences.PlaySequentialSz(entity, STRING(pls->sLockedSentence),
				0.85, ATTN_NORM, 0, 100, pls->iLockedSentence, false);
			pls->iUnlockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFLocked = static_cast<int>(iprev == pls->iLockedSentence);

			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
	else
	{
		// UNLOCKED SOUND

		bool fplaysound = (!FStringNull(pls->sUnlockedSound) && gpGlobals->time > pls->flwaitSound);
		bool fplaysentence = (!FStringNull(pls->sUnlockedSentence) && 0 == pls->bEOFUnlocked && gpGlobals->time > pls->flwaitSentence);
		float fvol;

		// if playing both sentence and sound, lower sound volume so we hear sentence
		if (fplaysound && fplaysentence)
			fvol = 0.25;
		else
			fvol = 1.0;

		// play 'door unlocked' sound if set
		if (fplaysound)
		{
			entity->EmitSound(CHAN_ITEM, STRING(pls->sUnlockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		// play next 'door unlocked' sentence in group
		if (fplaysentence)
		{
			int iprev = pls->iUnlockedSentence;

			pls->iUnlockedSentence = sentences::g_Sentences.PlaySequentialSz(entity, STRING(pls->sUnlockedSentence),
				0.85, ATTN_NORM, 0, 100, pls->iUnlockedSentence, false);
			pls->iLockedSentence = 0;

			// make sure we don't keep calling last sentence in list
			pls->bEOFUnlocked = static_cast<int>(iprev == pls->iUnlockedSentence);
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
}

bool CBaseDoor::KeyValue(KeyValueData* pkvd)
{

	if (FStrEq(pkvd->szKeyName, "skin")) // skin is used for content type
	{
		pev->skin = atof(pkvd->szValue);
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
	else if (FStrEq(pkvd->szKeyName, "healthvalue"))
	{
		m_bHealthValue = atof(pkvd->szValue);
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
	else if (FStrEq(pkvd->szKeyName, "WaveHeight"))
	{
		pev->scale = atof(pkvd->szValue) * (1.0 / 8.0);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(func_door, CBaseDoor);

/**
 *	@brief func_water - same as a door.
 */
LINK_ENTITY_TO_CLASS(func_water, CBaseDoor);

void CBaseDoor::Spawn()
{
	Precache();
	SetMovedir(this);

	if (pev->skin == 0)
	{ // normal door
		if (FBitSet(pev->spawnflags, SF_DOOR_PASSABLE))
			pev->solid = SOLID_NOT;
		else
			pev->solid = SOLID_BSP;
	}
	else
	{ // special contents
		pev->solid = SOLID_NOT;
		SetBits(pev->spawnflags, SF_DOOR_SILENT); // water is silent for now
	}

	pev->movetype = MOVETYPE_PUSH;
	SetOrigin(pev->origin);
	SetModel(STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 100;

	m_vecPosition1 = pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + (pev->movedir * (fabs(pev->movedir.x * (pev->size.x - 2)) + fabs(pev->movedir.y * (pev->size.y - 2)) + fabs(pev->movedir.z * (pev->size.z - 2)) - m_flLip));
	ASSERTSZ(m_vecPosition1 != m_vecPosition2, "door start/end positions are equal");
	if (FBitSet(pev->spawnflags, SF_DOOR_START_OPEN))
	{ // swap pos1 and pos2, put door at pos2
		SetOrigin(m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}

	m_toggle_state = TS_AT_BOTTOM;

	// if the door is flagged for USE button activation only, use nullptr touch function
	if (FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
	{
		SetTouch(nullptr);
	}
	else // touchable button
		SetTouch(&CBaseDoor::DoorTouch);
}

void CBaseDoor::Precache()
{
	// set the door's "in-motion" sound
	if (FStrEq("", STRING(m_MoveSound)))
	{
		m_MoveSound = ALLOC_STRING("common/null.wav");
	}

	PrecacheSound(STRING(m_MoveSound));

	// set the door's 'reached destination' stop sound
	if (FStrEq("", STRING(m_StopSound)))
	{
		m_StopSound = ALLOC_STRING("common/null.wav");
	}

	PrecacheSound(STRING(m_StopSound));

	// get door button sounds, for doors which are directly 'touched' to open

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

void CBaseDoor::DoorTouch(CBaseEntity* pOther)
{
	// Ignore touches by anything but players
	if (!pOther->IsPlayer())
		return;

	// If door has master, and it's not ready to trigger,
	// play 'locked' sound

	if (!FStringNull(m_sMaster) && !UTIL_IsMasterTriggered(m_sMaster, pOther, m_UseLocked))
		PlayLockSounds(this, &m_ls, true, false);

	// If door is somebody's target, then touching does nothing.
	// You have to activate the owner (e.g. button).

	if (!FStringNull(pev->targetname))
	{
		// play locked sound
		PlayLockSounds(this, &m_ls, true, false);
		return;
	}

	m_hActivator = pOther; // remember who activated the door

	if (DoorActivate())
		SetTouch(nullptr); // Temporarily disable the touch function, until movement is finished.
}

void CBaseDoor::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_hActivator = pActivator;
	// if not ready to be used, ignore "use" command.
	if (m_toggle_state == TS_AT_BOTTOM || FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
		DoorActivate();
}

bool CBaseDoor::DoorActivate()
{
	if (!UTIL_IsMasterTriggered(m_sMaster, m_hActivator, m_UseLocked))
		return false;

	if (FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
	{ // door should close
		DoorGoDown();
	}
	else
	{ // door should open

		if (m_hActivator != nullptr && m_hActivator->IsPlayer())
		{ // give health if player opened the door (medikit)
			m_hActivator->GiveHealth(m_bHealthValue, DMG_GENERIC);
		}

		// play door unlock sounds
		PlayLockSounds(this, &m_ls, false, false);

		DoorGoUp();
	}

	return true;
}

void CBaseDoor::DoorGoUp()
{
	// It could be going-down, if blocked.
	ASSERT(m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN);

	// emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if (!FBitSet(pev->spawnflags, SF_DOOR_SILENT))
	{
		if (m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN)
			EmitSound(CHAN_STATIC, STRING(m_MoveSound), 1, ATTN_NORM);
	}

	m_toggle_state = TS_GOING_UP;

	SetMoveDone(&CBaseDoor::DoorHitTop);
	if (ClassnameIs("func_door_rotating")) // !!! BUGBUG Triggered doors don't work with this yet
	{
		float sign = 1.0;

		if (m_hActivator != nullptr)
		{
			CBaseEntity* activator = m_hActivator;

			if (!FBitSet(pev->spawnflags, SF_DOOR_ONEWAY) && 0 != pev->movedir.y) // Y axis rotation, move away from the player
			{
				Vector vec = activator->pev->origin - pev->origin;
				Vector angles = activator->pev->angles;
				angles.x = 0;
				angles.z = 0;
				UTIL_MakeVectors(angles);
				//			Vector vnext = (pevToucher->origin + (pevToucher->velocity * 10)) - pev->origin;
				UTIL_MakeVectors(activator->pev->angles);
				Vector vnext = (activator->pev->origin + (gpGlobals->v_forward * 10)) - pev->origin;
				if ((vec.x * vnext.y - vec.y * vnext.x) < 0)
					sign = -1.0;
			}
		}
		AngularMove(m_vecAngle2 * sign, pev->speed);
	}
	else
		LinearMove(m_vecPosition2, pev->speed);
}

void CBaseDoor::DoorHitTop()
{
	if (!FBitSet(pev->spawnflags, SF_DOOR_SILENT))
	{
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		EmitSound(CHAN_STATIC, STRING(m_StopSound), 1, ATTN_NORM);
	}

	ASSERT(m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_AT_TOP;

	// toggle-doors don't come down automatically, they wait for refire.
	if (FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN))
	{
		// Re-instate touch method, movement is complete
		if (!FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
			SetTouch(&CBaseDoor::DoorTouch);
	}
	else
	{
		// In flWait seconds, DoorGoDown will fire, unless wait is -1, then door stays open
		pev->nextthink = pev->ltime + m_flWait;
		SetThink(&CBaseDoor::DoorGoDown);

		if (m_flWait == -1)
		{
			pev->nextthink = -1;
		}
	}

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if (!FStringNull(pev->netname) && (pev->spawnflags & SF_DOOR_START_OPEN) != 0)
		FireTargets(STRING(pev->netname), m_hActivator, this, USE_TOGGLE, 0);

	SUB_UseTargets(m_hActivator, USE_TOGGLE, 0); // this isn't finished
}

void CBaseDoor::DoorGoDown()
{
	if (!FBitSet(pev->spawnflags, SF_DOOR_SILENT))
	{
		if (m_toggle_state != TS_GOING_UP && m_toggle_state != TS_GOING_DOWN)
			EmitSound(CHAN_STATIC, STRING(m_MoveSound), 1, ATTN_NORM);
	}

#ifdef DOOR_ASSERT
	ASSERT(m_toggle_state == TS_AT_TOP);
#endif // DOOR_ASSERT
	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone(&CBaseDoor::DoorHitBottom);
	if (ClassnameIs("func_door_rotating")) // rotating door
		AngularMove(m_vecAngle1, pev->speed);
	else
		LinearMove(m_vecPosition1, pev->speed);
}

void CBaseDoor::DoorHitBottom()
{
	if (!FBitSet(pev->spawnflags, SF_DOOR_SILENT))
	{
		StopSound(CHAN_STATIC, STRING(m_MoveSound));
		EmitSound(CHAN_STATIC, STRING(m_StopSound), 1, ATTN_NORM);
	}

	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;

	// Re-instate touch method, cycle is complete
	if (FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
	{ // use only door
		SetTouch(nullptr);
	}
	else // touchable door
		SetTouch(&CBaseDoor::DoorTouch);

	SUB_UseTargets(m_hActivator, USE_TOGGLE, 0); // this isn't finished

	// Fire the close target (if startopen is set, then "top" is closed) - netname is the close target
	if (!FStringNull(pev->netname) && (pev->spawnflags & SF_DOOR_START_OPEN) == 0)
		FireTargets(STRING(pev->netname), m_hActivator, this, USE_TOGGLE, 0);
}

void CBaseDoor::Blocked(CBaseEntity* pOther)
{
	// Hurt the blocker a little.
	if (0 != pev->dmg)
		pOther->TakeDamage(this, this, pev->dmg, DMG_CRUSH);

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast

	if (m_flWait >= 0)
	{
		if (m_toggle_state == TS_GOING_DOWN)
		{
			DoorGoUp();
		}
		else
		{
			DoorGoDown();
		}
	}

	// Block all door pieces with the same targetname here.
	if (!FStringNull(pev->targetname))
	{
		CBaseDoor* pDoor = nullptr;

		for (auto target : UTIL_FindEntitiesByTargetname(STRING(pev->targetname)))
		{
			if (target != this)
			{
				if (target->ClassnameIs("func_door") || target->ClassnameIs("func_door_rotating"))
				{
					pDoor = static_cast<CBaseDoor*>(target);

					if (pDoor->m_flWait >= 0)
					{
						if (pDoor->pev->velocity == pev->velocity && pDoor->pev->avelocity == pev->velocity)
						{
							// this is the most hacked, evil, bastardized thing I've ever seen. kjb
							if (pDoor->ClassnameIs("func_door"))
							{ // set origin to realign normal doors
								pDoor->pev->origin = pev->origin;
								pDoor->pev->velocity = g_vecZero; // stop!
							}
							else
							{ // set angles to realign rotating doors
								pDoor->pev->angles = pev->angles;
								pDoor->pev->avelocity = g_vecZero;
							}
						}

						if (!FBitSet(pev->spawnflags, SF_DOOR_SILENT))
							StopSound(CHAN_STATIC, STRING(m_MoveSound));

						if (pDoor->m_toggle_state == TS_GOING_DOWN)
							pDoor->DoorGoUp();
						else
							pDoor->DoorGoDown();
					}
				}
			}
		}
	}
}

class CRotDoor : public CBaseDoor
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS(func_door_rotating, CRotDoor);

void CRotDoor::Spawn()
{
	Precache();
	// set the axis of rotation
	CBaseToggle::AxisDir(this);

	// check for clockwise rotation
	if (FBitSet(pev->spawnflags, SF_DOOR_ROTATE_BACKWARDS))
		pev->movedir = pev->movedir * -1;

	// m_flWait			= 2; who the hell did this? (sjb)
	m_vecAngle1 = pev->angles;
	m_vecAngle2 = pev->angles + pev->movedir * m_flMoveDistance;

	ASSERTSZ(m_vecAngle1 != m_vecAngle2, "rotating door start/end positions are equal");

	if (FBitSet(pev->spawnflags, SF_DOOR_PASSABLE))
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	pev->movetype = MOVETYPE_PUSH;
	SetOrigin(pev->origin);
	SetModel(STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 100;

	// DOOR_START_OPEN is to allow an entity to be lighted in the closed position
	// but spawn in the open position
	if (FBitSet(pev->spawnflags, SF_DOOR_START_OPEN))
	{ // swap pos1 and pos2, put door at pos2, invert movement direction
		pev->angles = m_vecAngle2;
		Vector vecSav = m_vecAngle1;
		m_vecAngle2 = m_vecAngle1;
		m_vecAngle1 = vecSav;
		pev->movedir = pev->movedir * -1;
	}

	m_toggle_state = TS_AT_BOTTOM;

	if (FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
	{
		SetTouch(nullptr);
	}
	else // touchable button
		SetTouch(&CRotDoor::DoorTouch);
}

class CMomentaryDoor : public CBaseToggle
{
	DECLARE_CLASS(CMomentaryDoor, CBaseToggle);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Precache() override;

	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps() override { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	/**
	 *	@brief The door has reached needed position.
	 */
	void DoorMoveDone();
	void StopMoveSound();

	string_t m_MoveSound; // sound a door makes while moving
	string_t m_StopSound;
};

LINK_ENTITY_TO_CLASS(momentary_door, CMomentaryDoor);

BEGIN_DATAMAP(CMomentaryDoor)
DEFINE_FIELD(m_MoveSound, FIELD_SOUNDNAME),
	DEFINE_FIELD(m_StopSound, FIELD_SOUNDNAME),
	DEFINE_FUNCTION(DoorMoveDone),
	DEFINE_FUNCTION(StopMoveSound),
	END_DATAMAP();

void CMomentaryDoor::Spawn()
{
	SetMovedir(this);

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SetOrigin(pev->origin);
	SetModel(STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 100;
	if (pev->dmg == 0)
		pev->dmg = 2;

	m_vecPosition1 = pev->origin;
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + (pev->movedir * (fabs(pev->movedir.x * (pev->size.x - 2)) + fabs(pev->movedir.y * (pev->size.y - 2)) + fabs(pev->movedir.z * (pev->size.z - 2)) - m_flLip));
	ASSERTSZ(m_vecPosition1 != m_vecPosition2, "door start/end positions are equal");

	if (FBitSet(pev->spawnflags, SF_DOOR_START_OPEN))
	{ // swap pos1 and pos2, put door at pos2
		SetOrigin(m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}
	SetTouch(nullptr);

	Precache();
}

void CMomentaryDoor::Precache()
{
	// set the door's "in-motion" sound
	if (FStringNull(m_MoveSound))
	{
		m_MoveSound = ALLOC_STRING("common/null.wav");
	}

	PrecacheSound(STRING(m_MoveSound));

	m_StopSound = ALLOC_STRING("common/null.wav");

	PrecacheSound(STRING(m_StopSound));
}

bool CMomentaryDoor::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_MoveSound = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
		//		m_bStopSnd = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "healthvalue"))
	{
		//		m_bHealthValue = atof(pkvd->szValue);
		return true;
	}

	return CBaseToggle::KeyValue(pkvd);
}

void CMomentaryDoor::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (useType != USE_SET) // Momentary buttons will pass down a float in here
		return;

	if (value > 1.0)
		value = 1.0;
	if (value < 0.0)
		value = 0.0;

	Vector move = m_vecPosition1 + (value * (m_vecPosition2 - m_vecPosition1));

	Vector delta = move - pev->origin;
	// float speed = delta.Length() * 10;
	float speed = delta.Length() / 0.1; // move there in 0.1 sec
	if (speed == 0)
		return;

	// This entity only thinks when it moves, so if it's thinking, it's in the process of moving
	// play the sound when it starts moving (not yet thinking)
	if (pev->nextthink < pev->ltime || pev->nextthink == 0)
		EmitSound(CHAN_STATIC, STRING(m_MoveSound), 1, ATTN_NORM);
	// If we already moving to designated point, return
	else if (move == m_vecFinalDest)
		return;

	SetMoveDone(&CMomentaryDoor::DoorMoveDone);
	LinearMove(move, speed);
}

void CMomentaryDoor::DoorMoveDone()
{
	// Stop sounds at the next think, rather than here as another
	// Use call might immediately follow the end of this move
	// This think function will be replaced by LinearMove if that happens.
	SetThink(&CMomentaryDoor::StopMoveSound);
	pev->nextthink = pev->ltime + 0.1f;
}

void CMomentaryDoor::StopMoveSound()
{
	StopSound(CHAN_STATIC, STRING(m_MoveSound));
	EmitSound(CHAN_STATIC, STRING(m_StopSound), 1, ATTN_NORM);
	SetThink(nullptr);
}
