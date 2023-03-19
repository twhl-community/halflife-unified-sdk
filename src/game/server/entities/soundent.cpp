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
#include "soundent.h"

LINK_ENTITY_TO_CLASS(soundent, CSoundEnt);

void CSound::Clear()
{
	m_vecOrigin = g_vecZero;
	m_iType = 0;
	m_iVolume = 0;
	m_flExpireTime = 0;
	m_iNext = SOUNDLIST_EMPTY;
	m_iNextAudible = 0;
}

void CSound::Reset()
{
	m_vecOrigin = g_vecZero;
	m_iType = 0;
	m_iVolume = 0;
	m_iNext = SOUNDLIST_EMPTY;
}

bool CSound::FIsSound()
{
	if ((m_iType & (bits_SOUND_COMBAT | bits_SOUND_WORLD | bits_SOUND_PLAYER | bits_SOUND_DANGER)) != 0)
	{
		return true;
	}

	return false;
}

bool CSound::FIsScent()
{
	if ((m_iType & (bits_SOUND_CARCASS | bits_SOUND_MEAT | bits_SOUND_GARBAGE)) != 0)
	{
		return true;
	}

	return false;
}

void CSoundEnt::Spawn()
{
	pev->solid = SOLID_NOT;
	Initialize();

	pev->nextthink = gpGlobals->time + 1;
}

void CSoundEnt::Think()
{
	int iSound;
	int iPreviousSound;

	pev->nextthink = gpGlobals->time + 0.3; // how often to check the sound list.

	iPreviousSound = SOUNDLIST_EMPTY;
	iSound = m_iActiveSound;

	while (iSound != SOUNDLIST_EMPTY)
	{
		if (m_SoundPool[iSound].m_flExpireTime <= gpGlobals->time && m_SoundPool[iSound].m_flExpireTime != SOUND_NEVER_EXPIRE)
		{
			int iNext = m_SoundPool[iSound].m_iNext;

			// move this sound back into the free list
			FreeSound(iSound, iPreviousSound);

			iSound = iNext;
		}
		else
		{
			iPreviousSound = iSound;
			iSound = m_SoundPool[iSound].m_iNext;
		}
	}

	if (m_fShowReport)
	{
		Logger->trace("Soundlist: {} / {} ({})\n",
			ISoundsInList(SOUNDLISTTYPE_ACTIVE), ISoundsInList(SOUNDLISTTYPE_FREE), ISoundsInList(SOUNDLISTTYPE_ACTIVE) - m_cLastActiveSounds);
		m_cLastActiveSounds = ISoundsInList(SOUNDLISTTYPE_ACTIVE);
	}
}

void CSoundEnt::Precache()
{
}

void CSoundEnt::FreeSound(int iSound, int iPrevious)
{
	if (!pSoundEnt)
	{
		// no sound ent!
		return;
	}

	if (iPrevious != SOUNDLIST_EMPTY)
	{
		// iSound is not the head of the active list, so
		// must fix the index for the Previous sound
		//		pSoundEnt->m_SoundPool[ iPrevious ].m_iNext = m_SoundPool[ iSound ].m_iNext;
		pSoundEnt->m_SoundPool[iPrevious].m_iNext = pSoundEnt->m_SoundPool[iSound].m_iNext;
	}
	else
	{
		// the sound we're freeing IS the head of the active list.
		pSoundEnt->m_iActiveSound = pSoundEnt->m_SoundPool[iSound].m_iNext;
	}

	// make iSound the head of the Free list.
	pSoundEnt->m_SoundPool[iSound].m_iNext = pSoundEnt->m_iFreeSound;
	pSoundEnt->m_iFreeSound = iSound;
}

int CSoundEnt::IAllocSound()
{
	int iNewSound;

	if (m_iFreeSound == SOUNDLIST_EMPTY)
	{
		// no free sound!
		Logger->debug("Free Sound List is full!");
		return SOUNDLIST_EMPTY;
	}

	// there is at least one sound available, so move it to the
	// Active sound list, and return its SoundPool index.

	iNewSound = m_iFreeSound; // copy the index of the next free sound

	m_iFreeSound = m_SoundPool[m_iFreeSound].m_iNext; // move the index down into the free list.

	m_SoundPool[iNewSound].m_iNext = m_iActiveSound; // point the new sound at the top of the active list.

	m_iActiveSound = iNewSound; // now make the new sound the top of the active list. You're done.

	return iNewSound;
}

void CSoundEnt::InsertSound(int iType, const Vector& vecOrigin, int iVolume, float flDuration)
{
	int iThisSound;

	if (!pSoundEnt)
	{
		// no sound ent!
		return;
	}

	iThisSound = pSoundEnt->IAllocSound();

	if (iThisSound == SOUNDLIST_EMPTY)
	{
		Logger->debug("Could not AllocSound() for InsertSound() (DLL)");
		return;
	}

	pSoundEnt->m_SoundPool[iThisSound].m_vecOrigin = vecOrigin;
	pSoundEnt->m_SoundPool[iThisSound].m_iType = iType;
	pSoundEnt->m_SoundPool[iThisSound].m_iVolume = iVolume;
	pSoundEnt->m_SoundPool[iThisSound].m_flExpireTime = gpGlobals->time + flDuration;
}

void CSoundEnt::Initialize()
{
	int i;
	int iSound;

	m_cLastActiveSounds;
	m_iFreeSound = 0;
	m_iActiveSound = SOUNDLIST_EMPTY;

	for (i = 0; i < MAX_WORLD_SOUNDS; i++)
	{ // clear all sounds, and link them into the free sound list.
		m_SoundPool[i].Clear();
		m_SoundPool[i].m_iNext = i + 1;
	}

	m_SoundPool[i - 1].m_iNext = SOUNDLIST_EMPTY; // terminate the list here.


	// now reserve enough sounds for each client
	for (i = 0; i < gpGlobals->maxClients; i++)
	{
		iSound = pSoundEnt->IAllocSound();

		if (iSound == SOUNDLIST_EMPTY)
		{
			Logger->debug("Could not AllocSound() for Client Reserve! (DLL)");
			return;
		}

		pSoundEnt->m_SoundPool[iSound].m_flExpireTime = SOUND_NEVER_EXPIRE;
	}

	if (CVAR_GET_FLOAT("displaysoundlist") == 1)
	{
		m_fShowReport = true;
	}
	else
	{
		m_fShowReport = false;
	}
}

int CSoundEnt::ISoundsInList(int iListType)
{
	int iThisSound = [=, this]()
	{
		switch (iListType)
		{
		case SOUNDLISTTYPE_FREE: return m_iFreeSound;
		case SOUNDLISTTYPE_ACTIVE: return m_iActiveSound;
		default:
			Logger->debug("Unknown Sound List Type!");
			return SOUNDLIST_EMPTY;
		}
	}();

	int i = 0;

	while (iThisSound != SOUNDLIST_EMPTY)
	{
		i++;

		iThisSound = m_SoundPool[iThisSound].m_iNext;
	}

	return i;
}

int CSoundEnt::ActiveList()
{
	if (!pSoundEnt)
	{
		return SOUNDLIST_EMPTY;
	}

	return pSoundEnt->m_iActiveSound;
}

int CSoundEnt::FreeList()
{
	if (!pSoundEnt)
	{
		return SOUNDLIST_EMPTY;
	}

	return pSoundEnt->m_iFreeSound;
}

CSound* CSoundEnt::SoundPointerForIndex(int iIndex)
{
	if (!pSoundEnt)
	{
		return nullptr;
	}

	if (iIndex > (MAX_WORLD_SOUNDS - 1))
	{
		Logger->debug("SoundPointerForIndex() - Index too large!");
		return nullptr;
	}

	if (iIndex < 0)
	{
		Logger->debug("SoundPointerForIndex() - Index < 0!");
		return nullptr;
	}

	return &pSoundEnt->m_SoundPool[iIndex];
}

int CSoundEnt::ClientSoundIndex(edict_t* pClient)
{
	int iReturn = ENTINDEX(pClient) - 1;

#ifdef _DEBUG
	if (iReturn < 0 || iReturn > gpGlobals->maxClients)
	{
		Logger->debug("** ClientSoundIndex returning a bogus value! **");
	}
#endif // _DEBUG

	return iReturn;
}
