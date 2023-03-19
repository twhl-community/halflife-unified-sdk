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

#pragma once

#include "CBaseToggle.h"

/**
*	@brief sounds that doors and buttons make when locked/unlocked
*/
struct locksound_t
{
	string_t sLockedSound;		//!< sound a door makes when it's locked
	string_t sLockedSentence;	//!< sentence group played when door is locked
	string_t sUnlockedSound;	//!< sound a door makes when it's unlocked
	string_t sUnlockedSentence; //!< sentence group played when door is unlocked

	int iLockedSentence;   //!< which sentence in sentence group to play next
	int iUnlockedSentence; //!< which sentence in sentence group to play next

	float flwaitSound;	  //!< time delay between playing consecutive 'locked/unlocked' sounds
	float flwaitSentence; //!< time delay between playing consecutive sentences
	byte bEOFLocked;	  //!< true if hit end of list of locked sentences
	byte bEOFUnlocked;	  //!< true if hit end of list of unlocked sentences
};

/**
*	@brief play door or button locked or unlocked sounds.
*	@details pass in pointer to valid locksound struct.
*	if flocked is true, play 'door is locked' sound, otherwise play 'door is unlocked' sound
*	NOTE: this routine is shared by doors and buttons
*/
void PlayLockSounds(CBaseEntity* entity, locksound_t* pls, bool flocked, bool fbutton);

/**
*	@brief Generic Button
*	@details When a button is touched, it moves some distance in the direction of its angle,
*	triggers all of its targets, waits some time, then returns to its original position where it can be triggered again.
*/
class CBaseButton : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;

	/**
	*	@brief Starts the button moving "in/up".
	*/
	void ButtonActivate();

	/**
	*	@brief Touching a button simply "activates" it.
	*/
	void EXPORT ButtonTouch(CBaseEntity* pOther);

	/**
	*	@brief Makes flagged buttons spark when turned off
	*/
	void EXPORT ButtonSpark();

	/**
	*	@brief Button has reached the "in/up" position.  Activate its "targets", and pause before "popping out".
	*/
	void EXPORT TriggerAndWait();

	/**
	*	@brief Starts the button moving "out/down".
	*/
	void EXPORT ButtonReturn();

	/**
	*	@brief Button has returned to start state. Quiesce it.
	*/
	void EXPORT ButtonBackHome();

	void EXPORT ButtonUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	enum BUTTON_CODE
	{
		BUTTON_NOTHING,
		BUTTON_ACTIVATE,
		BUTTON_RETURN
	};
	BUTTON_CODE ButtonResponseToTouch();

	static TYPEDESCRIPTION m_SaveData[];
	// Buttons that don't take damage can be IMPULSE used
	int ObjectCaps() override { return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | (pev->takedamage ? 0 : FCAP_IMPULSE_USE); }

	bool m_fStayPushed; // button stays pushed in until touched again?
	bool m_fRotating;	// a rotating button?  default is a sliding button.

	string_t m_strChangeTarget; // if this field is not null, this is an index into the engine string array.
								// when this button is touched, it's target entity's TARGET field will be set
								// to the button's ChangeTarget. This allows you to make a func_train switch paths, etc.

	locksound_t m_ls; // door lock sounds

	string_t m_LockedSound; // ordinals from entity selection
	string_t m_LockedSentence;
	string_t m_UnlockedSound;
	string_t m_UnlockedSentence;
	string_t m_sounds;
};
