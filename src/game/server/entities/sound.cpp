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

#include <fmt/format.h>

#include "cbase.h"
#include "talkmonster.h"
#include "sound/MaterialSystem.h"

/**
 *	@brief runtime pitch shift and volume fadein/out structure
 *	@details NOTE: IF YOU CHANGE THIS STRUCT YOU MUST CHANGE THE SAVE/RESTORE VERSION NUMBER
 *	SEE BELOW (in the typedescription for the class)
 */
struct dynpitchvol_t
{
	// NOTE: do not change the order of these parameters
	// NOTE: unless you also change order of rgdpvpreset array elements!
	int preset;

	int pitchrun;	// pitch shift % when sound is running 0 - 255
	int pitchstart; // pitch shift % when sound stops or starts 0 - 255
	int spinup;		// spinup time 0 - 100
	int spindown;	// spindown time 0 - 100

	int volrun;	  // volume change % when sound is running 0 - 10
	int volstart; // volume change % when sound stops or starts 0 - 10
	int fadein;	  // volume fade in time 0 - 100
	int fadeout;  // volume fade out time 0 - 100

	// Low Frequency Oscillator
	int lfotype; // 0) off 1) square 2) triangle 3) random
	int lforate; // 0 - 1000, how fast lfo osciallates

	int lfomodpitch; // 0-100 mod of current pitch. 0 is off.
	int lfomodvol;	 // 0-100 mod of current volume. 0 is off.

	int cspinup; // each trigger hit increments counter and spinup pitch


	int cspincount;

	int pitch;
	int spinupsav;
	int spindownsav;
	int pitchfrac;

	int vol;
	int fadeinsav;
	int fadeoutsav;
	int volfrac;

	int lfofrac;
	int lfomult;
};

#define CDPVPRESETMAX 27

/**
 *	@brief presets for runtime pitch and vol modulation of ambient sounds
 */
dynpitchvol_t rgdpvpreset[CDPVPRESETMAX] =
	{
		// pitch	pstart	spinup	spindwn	volrun	volstrt	fadein	fadeout	lfotype	lforate	modptch modvol	cspnup
		{1, 255, 75, 95, 95, 10, 1, 50, 95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{2, 255, 85, 70, 88, 10, 1, 20, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{3, 255, 100, 50, 75, 10, 1, 10, 75, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{4, 100, 100, 0, 0, 10, 1, 90, 90, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{5, 100, 100, 0, 0, 10, 1, 80, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{6, 100, 100, 0, 0, 10, 1, 50, 70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{7, 100, 100, 0, 0, 5, 1, 40, 50, 1, 50, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{8, 100, 100, 0, 0, 5, 1, 40, 50, 1, 150, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{9, 100, 100, 0, 0, 5, 1, 40, 50, 1, 750, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{10, 128, 100, 50, 75, 10, 1, 30, 40, 2, 8, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{11, 128, 100, 50, 75, 10, 1, 30, 40, 2, 25, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{12, 128, 100, 50, 75, 10, 1, 30, 40, 2, 70, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{13, 50, 50, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{14, 70, 70, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{15, 90, 90, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{16, 120, 120, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{17, 180, 180, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{18, 255, 255, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{19, 200, 75, 90, 90, 10, 1, 50, 90, 2, 100, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{20, 255, 75, 97, 90, 10, 1, 50, 90, 1, 40, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{21, 100, 100, 0, 0, 10, 1, 30, 50, 3, 15, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{22, 160, 160, 0, 0, 10, 1, 50, 50, 3, 500, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{23, 255, 75, 88, 0, 10, 1, 40, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{24, 200, 20, 95, 70, 10, 1, 70, 70, 3, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{25, 180, 100, 50, 60, 10, 1, 40, 60, 2, 90, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{26, 60, 60, 0, 0, 10, 1, 40, 70, 3, 80, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{27, 128, 90, 10, 10, 10, 1, 20, 40, 1, 5, 10, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

/**
 *	@brief general-purpose user-defined static sound
 */
class CAmbientGeneric : public CBaseEntity
{
	DECLARE_CLASS(CAmbientGeneric, CBaseEntity);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Precache() override;

	/**
	 *	@brief turns an ambient sound on or off.
	 *	If the ambient is a looping sound, mark sound as active (m_fActive) if it's playing, innactive if not.
	 *	If the sound is not a looping sound, never mark it as active.
	 */
	void ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	/**
	 *	@brief Think at 5hz if we are dynamically modifying pitch or volume of the playing sound.
	 *	This function will ramp pitch and/or volume up or down, modify pitch/volume with lfo if active.
	 */
	void RampThink();

	/**
	 *	@brief Init all ramp params in preparation to play a new sound
	 */
	void InitModulationParms();

	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	float m_flAttenuation; // attenuation value
	dynpitchvol_t m_dpv;

	bool m_fActive;	 // only true when the entity is playing a looping sound
	bool m_fLooping; // true when the sound played will loop
};

LINK_ENTITY_TO_CLASS(ambient_generic, CAmbientGeneric);

BEGIN_DATAMAP(CAmbientGeneric)
DEFINE_FIELD(m_flAttenuation, FIELD_FLOAT),
	DEFINE_FIELD(m_fActive, FIELD_BOOLEAN),
	DEFINE_FIELD(m_fLooping, FIELD_BOOLEAN),

	// HACKHACK - This is not really in the spirit of the save/restore design, but save this
	// out as a binary data block.  If the dynpitchvol_t is changed, old saved games will NOT
	// load these correctly, so bump the save/restore version if you change the size of the struct
	// The right way to do this is to split the input parms (read in keyvalue) into members and re-init this
	// struct in Precache(), but it's unlikely that the struct will change, so it's not worth the time right now.
	DEFINE_ARRAY(m_dpv, FIELD_CHARACTER, sizeof(dynpitchvol_t)),
	DEFINE_FUNCTION(ToggleUse),
	DEFINE_FUNCTION(RampThink),
	END_DATAMAP();

void CAmbientGeneric::Spawn()
{
	/*
			-1 : "Default"
			0  : "Everywhere"
			200 : "Small Radius"
			125 : "Medium Radius"
			80  : "Large Radius"
	*/

	if (FBitSet(pev->spawnflags, AMBIENT_SOUND_EVERYWHERE))
	{
		m_flAttenuation = ATTN_NONE;
	}
	else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_SMALLRADIUS))
	{
		m_flAttenuation = ATTN_IDLE;
	}
	else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_MEDIUMRADIUS))
	{
		m_flAttenuation = ATTN_STATIC;
	}
	else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_LARGERADIUS))
	{
		m_flAttenuation = ATTN_NORM;
	}
	else
	{ // if the designer didn't set a sound attenuation, default to one.
		m_flAttenuation = ATTN_STATIC;
	}

	const char* szSoundFile = STRING(pev->message);

	if (FStringNull(pev->message) || strlen(szSoundFile) < 1)
	{
		Logger->error("EMPTY AMBIENT AT: {}", pev->origin);
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(&CAmbientGeneric::SUB_Remove);
		return;
	}
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	// Set up think function for dynamic modification
	// of ambient sound's pitch or volume. Don't
	// start thinking yet.

	SetThink(&CAmbientGeneric::RampThink);
	pev->nextthink = 0;

	// allow on/off switching via 'use' function.

	SetUse(&CAmbientGeneric::ToggleUse);

	m_fActive = false;

	if (FBitSet(pev->spawnflags, AMBIENT_SOUND_NOT_LOOPING))
		m_fLooping = false;
	else
		m_fLooping = true;
	Precache();
}

void CAmbientGeneric::Precache()
{
	const char* szSoundFile = STRING(pev->message);

	if (!FStringNull(pev->message) && strlen(szSoundFile) > 1)
	{
		if (*szSoundFile != '!')
			PrecacheSound(szSoundFile);
	}
	// init all dynamic modulation parms
	InitModulationParms();

	if (!FBitSet(pev->spawnflags, AMBIENT_SOUND_START_SILENT))
	{
		// start the sound ASAP
		if (m_fLooping)
			m_fActive = true;
	}
	if (m_fActive)
	{
		EmitAmbientSound(pev->origin, szSoundFile,
			(m_dpv.vol * 0.01), m_flAttenuation, SND_SPAWNING, m_dpv.pitch);

		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CAmbientGeneric::RampThink()
{
	const char* szSoundFile = STRING(pev->message);
	int pitch = m_dpv.pitch;
	int vol = m_dpv.vol;
	int flags = 0;
	bool fChanged = false; // false if pitch and vol remain unchanged this round
	int prev;

	if (0 == m_dpv.spinup && 0 == m_dpv.spindown && 0 == m_dpv.fadein && 0 == m_dpv.fadeout && 0 == m_dpv.lfotype)
		return; // no ramps or lfo, stop thinking

	// ==============
	// pitch envelope
	// ==============
	if (0 != m_dpv.spinup || 0 != m_dpv.spindown)
	{
		prev = m_dpv.pitchfrac >> 8;

		if (m_dpv.spinup > 0)
			m_dpv.pitchfrac += m_dpv.spinup;
		else if (m_dpv.spindown > 0)
			m_dpv.pitchfrac -= m_dpv.spindown;

		pitch = m_dpv.pitchfrac >> 8;

		if (pitch > m_dpv.pitchrun)
		{
			pitch = m_dpv.pitchrun;
			m_dpv.spinup = 0; // done with ramp up
		}

		if (pitch < m_dpv.pitchstart)
		{
			pitch = m_dpv.pitchstart;
			m_dpv.spindown = 0; // done with ramp down

			// shut sound off
			EmitAmbientSound(pev->origin, szSoundFile,
				0, 0, SND_STOP, 0);

			// return without setting nextthink
			return;
		}

		if (pitch > 255)
			pitch = 255;
		if (pitch < 1)
			pitch = 1;

		m_dpv.pitch = pitch;

		fChanged |= (prev != pitch);
		flags |= SND_CHANGE_PITCH;
	}

	// ==================
	// amplitude envelope
	// ==================
	if (0 != m_dpv.fadein || 0 != m_dpv.fadeout)
	{
		prev = m_dpv.volfrac >> 8;

		if (m_dpv.fadein > 0)
			m_dpv.volfrac += m_dpv.fadein;
		else if (m_dpv.fadeout > 0)
			m_dpv.volfrac -= m_dpv.fadeout;

		vol = m_dpv.volfrac >> 8;

		if (vol > m_dpv.volrun)
		{
			vol = m_dpv.volrun;
			m_dpv.fadein = 0; // done with ramp up
		}

		if (vol < m_dpv.volstart)
		{
			vol = m_dpv.volstart;
			m_dpv.fadeout = 0; // done with ramp down

			// shut sound off
			EmitAmbientSound(pev->origin, szSoundFile,
				0, 0, SND_STOP, 0);

			// return without setting nextthink
			return;
		}

		if (vol > 100)
			vol = 100;
		if (vol < 1)
			vol = 1;

		m_dpv.vol = vol;

		fChanged |= (prev != vol);
		flags |= SND_CHANGE_VOL;
	}

	// ===================
	// pitch/amplitude LFO
	// ===================
	if (0 != m_dpv.lfotype)
	{
		int pos;

		if (m_dpv.lfofrac > 0x6fffffff)
			m_dpv.lfofrac = 0;

		// update lfo, lfofrac/255 makes a triangle wave 0-255
		m_dpv.lfofrac += m_dpv.lforate;
		pos = m_dpv.lfofrac >> 8;

		if (m_dpv.lfofrac < 0)
		{
			m_dpv.lfofrac = 0;
			m_dpv.lforate = abs(m_dpv.lforate);
			pos = 0;
		}
		else if (pos > 255)
		{
			pos = 255;
			m_dpv.lfofrac = (255 << 8);
			m_dpv.lforate = -abs(m_dpv.lforate);
		}

		switch (m_dpv.lfotype)
		{
		case LFO_SQUARE:
			if (pos < 128)
				m_dpv.lfomult = 255;
			else
				m_dpv.lfomult = 0;

			break;
		case LFO_RANDOM:
			if (pos == 255)
				m_dpv.lfomult = RANDOM_LONG(0, 255);
			break;
		case LFO_TRIANGLE:
		default:
			m_dpv.lfomult = pos;
			break;
		}

		if (0 != m_dpv.lfomodpitch)
		{
			prev = pitch;

			// pitch 0-255
			pitch += ((m_dpv.lfomult - 128) * m_dpv.lfomodpitch) / 100;

			if (pitch > 255)
				pitch = 255;
			if (pitch < 1)
				pitch = 1;


			fChanged |= (prev != pitch);
			flags |= SND_CHANGE_PITCH;
		}

		if (0 != m_dpv.lfomodvol)
		{
			// vol 0-100
			prev = vol;

			vol += ((m_dpv.lfomult - 128) * m_dpv.lfomodvol) / 100;

			if (vol > 100)
				vol = 100;
			if (vol < 0)
				vol = 0;

			fChanged |= (prev != vol);
			flags |= SND_CHANGE_VOL;
		}
	}

	// Send update to playing sound only if we actually changed
	// pitch or volume in this routine.

	if (0 != flags && fChanged)
	{
		if (pitch == PITCH_NORM)
			pitch = PITCH_NORM + 1; // don't send 'no pitch' !

		EmitAmbientSound(pev->origin, szSoundFile,
			(vol * 0.01), m_flAttenuation, flags, pitch);
	}

	// update ramps at 5hz
	pev->nextthink = gpGlobals->time + 0.2;
	return;
}

void CAmbientGeneric::InitModulationParms()
{
	int pitchinc;

	m_dpv.volrun = pev->health * 10; // 0 - 100
	if (m_dpv.volrun > 100)
		m_dpv.volrun = 100;
	if (m_dpv.volrun < 0)
		m_dpv.volrun = 0;

	// get presets
	if (m_dpv.preset != 0 && m_dpv.preset <= CDPVPRESETMAX)
	{
		// load preset values
		m_dpv = rgdpvpreset[m_dpv.preset - 1];

		// fixup preset values, just like
		// fixups in KeyValue routine.
		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;

		m_dpv.volstart *= 10;
		m_dpv.volrun *= 10;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;

		m_dpv.lforate *= 256;

		m_dpv.fadeinsav = m_dpv.fadein;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		m_dpv.spinupsav = m_dpv.spinup;
		m_dpv.spindownsav = m_dpv.spindown;
	}

	m_dpv.fadein = m_dpv.fadeinsav;
	m_dpv.fadeout = 0;

	if (0 != m_dpv.fadein)
		m_dpv.vol = m_dpv.volstart;
	else
		m_dpv.vol = m_dpv.volrun;

	m_dpv.spinup = m_dpv.spinupsav;
	m_dpv.spindown = 0;

	if (0 != m_dpv.spinup)
		m_dpv.pitch = m_dpv.pitchstart;
	else
		m_dpv.pitch = m_dpv.pitchrun;

	if (m_dpv.pitch == 0)
		m_dpv.pitch = PITCH_NORM;

	m_dpv.pitchfrac = m_dpv.pitch << 8;
	m_dpv.volfrac = m_dpv.vol << 8;

	m_dpv.lfofrac = 0;
	m_dpv.lforate = abs(m_dpv.lforate);

	m_dpv.cspincount = 1;

	if (0 != m_dpv.cspinup)
	{
		pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;

		m_dpv.pitchrun = m_dpv.pitchstart + pitchinc;
		if (m_dpv.pitchrun > 255)
			m_dpv.pitchrun = 255;
	}

	if ((0 != m_dpv.spinupsav || 0 != m_dpv.spindownsav || (0 != m_dpv.lfotype && 0 != m_dpv.lfomodpitch)) && (m_dpv.pitch == PITCH_NORM))
		m_dpv.pitch = PITCH_NORM + 1; // must never send 'no pitch' as first pitch
									  // if we intend to pitch shift later!
}

void CAmbientGeneric::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	const char* szSoundFile = STRING(pev->message);
	float fraction;

	if (useType != USE_TOGGLE)
	{
		if ((m_fActive && useType == USE_ON) || (!m_fActive && useType == USE_OFF))
			return;
	}
	// Directly change pitch if arg passed. Only works if sound is already playing.

	if (useType == USE_SET && m_fActive) // Momentary buttons will pass down a float in here
	{

		fraction = value;

		if (fraction > 1.0)
			fraction = 1.0;
		if (fraction < 0.0)
			fraction = 0.01;

		m_dpv.pitch = fraction * 255;

		EmitAmbientSound(pev->origin, szSoundFile,
			0, 0, SND_CHANGE_PITCH, m_dpv.pitch);

		return;
	}

	// Toggle

	// m_fActive is true only if a looping sound is playing.

	if (m_fActive)
	{ // turn sound off

		if (0 != m_dpv.cspinup)
		{
			// Don't actually shut off. Each toggle causes
			// incremental spinup to max pitch

			if (m_dpv.cspincount <= m_dpv.cspinup)
			{
				int pitchinc;

				// start a new spinup
				m_dpv.cspincount++;

				pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;

				m_dpv.spinup = m_dpv.spinupsav;
				m_dpv.spindown = 0;

				m_dpv.pitchrun = m_dpv.pitchstart + pitchinc * m_dpv.cspincount;
				if (m_dpv.pitchrun > 255)
					m_dpv.pitchrun = 255;

				pev->nextthink = gpGlobals->time + 0.1;
			}
		}
		else
		{
			m_fActive = false;

			// HACKHACK - this makes the code in Precache() work properly after a save/restore
			pev->spawnflags |= AMBIENT_SOUND_START_SILENT;

			if (0 != m_dpv.spindownsav || 0 != m_dpv.fadeoutsav)
			{
				// spin it down (or fade it) before shutoff if spindown is set
				m_dpv.spindown = m_dpv.spindownsav;
				m_dpv.spinup = 0;

				m_dpv.fadeout = m_dpv.fadeoutsav;
				m_dpv.fadein = 0;
				pev->nextthink = gpGlobals->time + 0.1;
			}
			else
				EmitAmbientSound(pev->origin, szSoundFile,
					0, 0, SND_STOP, 0);
		}
	}
	else
	{ // turn sound on

		// only toggle if this is a looping sound.  If not looping, each
		// trigger will cause the sound to play.  If the sound is still
		// playing from a previous trigger press, it will be shut off
		// and then restarted.

		if (m_fLooping)
			m_fActive = true;
		else
			// shut sound off now - may be interrupting a long non-looping sound
			EmitAmbientSound(pev->origin, szSoundFile,
				0, 0, SND_STOP, 0);

		// init all ramp params for startup

		InitModulationParms();

		EmitAmbientSound(pev->origin, szSoundFile,
			(m_dpv.vol * 0.01), m_flAttenuation, 0, m_dpv.pitch);

		pev->nextthink = gpGlobals->time + 0.1;
	}
}

bool CAmbientGeneric::KeyValue(KeyValueData* pkvd)
{
	// NOTE: changing any of the modifiers in this code
	// NOTE: also requires changing InitModulationParms code.

	// preset
	if (FStrEq(pkvd->szKeyName, "preset"))
	{
		m_dpv.preset = atoi(pkvd->szValue);
		return true;
	}

	// pitchrun
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		m_dpv.pitchrun = atoi(pkvd->szValue);

		if (m_dpv.pitchrun > 255)
			m_dpv.pitchrun = 255;
		if (m_dpv.pitchrun < 0)
			m_dpv.pitchrun = 0;

		return true;
	}

	// pitchstart
	else if (FStrEq(pkvd->szKeyName, "pitchstart"))
	{
		m_dpv.pitchstart = atoi(pkvd->szValue);

		if (m_dpv.pitchstart > 255)
			m_dpv.pitchstart = 255;
		if (m_dpv.pitchstart < 0)
			m_dpv.pitchstart = 0;

		return true;
	}

	// spinup
	else if (FStrEq(pkvd->szKeyName, "spinup"))
	{
		m_dpv.spinup = atoi(pkvd->szValue);

		if (m_dpv.spinup > 100)
			m_dpv.spinup = 100;
		if (m_dpv.spinup < 0)
			m_dpv.spinup = 0;

		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;
		m_dpv.spinupsav = m_dpv.spinup;
		return true;
	}

	// spindown
	else if (FStrEq(pkvd->szKeyName, "spindown"))
	{
		m_dpv.spindown = atoi(pkvd->szValue);

		if (m_dpv.spindown > 100)
			m_dpv.spindown = 100;
		if (m_dpv.spindown < 0)
			m_dpv.spindown = 0;

		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;
		m_dpv.spindownsav = m_dpv.spindown;
		return true;
	}

	// volstart
	else if (FStrEq(pkvd->szKeyName, "volstart"))
	{
		m_dpv.volstart = atoi(pkvd->szValue);

		if (m_dpv.volstart > 10)
			m_dpv.volstart = 10;
		if (m_dpv.volstart < 0)
			m_dpv.volstart = 0;

		m_dpv.volstart *= 10; // 0 - 100

		return true;
	}

	// fadein
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_dpv.fadein = atoi(pkvd->szValue);

		if (m_dpv.fadein > 100)
			m_dpv.fadein = 100;
		if (m_dpv.fadein < 0)
			m_dpv.fadein = 0;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;
		m_dpv.fadeinsav = m_dpv.fadein;
		return true;
	}

	// fadeout
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_dpv.fadeout = atoi(pkvd->szValue);

		if (m_dpv.fadeout > 100)
			m_dpv.fadeout = 100;
		if (m_dpv.fadeout < 0)
			m_dpv.fadeout = 0;

		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		return true;
	}

	// lfotype
	else if (FStrEq(pkvd->szKeyName, "lfotype"))
	{
		m_dpv.lfotype = atoi(pkvd->szValue);
		if (m_dpv.lfotype > 4)
			m_dpv.lfotype = LFO_TRIANGLE;
		return true;
	}

	// lforate
	else if (FStrEq(pkvd->szKeyName, "lforate"))
	{
		m_dpv.lforate = atoi(pkvd->szValue);

		if (m_dpv.lforate > 1000)
			m_dpv.lforate = 1000;
		if (m_dpv.lforate < 0)
			m_dpv.lforate = 0;

		m_dpv.lforate *= 256;

		return true;
	}
	// lfomodpitch
	else if (FStrEq(pkvd->szKeyName, "lfomodpitch"))
	{
		m_dpv.lfomodpitch = atoi(pkvd->szValue);
		if (m_dpv.lfomodpitch > 100)
			m_dpv.lfomodpitch = 100;
		if (m_dpv.lfomodpitch < 0)
			m_dpv.lfomodpitch = 0;


		return true;
	}

	// lfomodvol
	else if (FStrEq(pkvd->szKeyName, "lfomodvol"))
	{
		m_dpv.lfomodvol = atoi(pkvd->szValue);
		if (m_dpv.lfomodvol > 100)
			m_dpv.lfomodvol = 100;
		if (m_dpv.lfomodvol < 0)
			m_dpv.lfomodvol = 0;

		return true;
	}

	// cspinup
	else if (FStrEq(pkvd->szKeyName, "cspinup"))
	{
		m_dpv.cspinup = atoi(pkvd->szValue);
		if (m_dpv.cspinup > 100)
			m_dpv.cspinup = 100;
		if (m_dpv.cspinup < 0)
			m_dpv.cspinup = 0;

		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

// if you change this, also change CAmbientMusic::KeyValue!
enum class AmbientMusicCommand
{
	Play = 0,
	Loop,
	Fadeout,
	Stop
};

enum class AmbientMusicTargetSelector
{
	AllPlayers = 0,
	Activator,
	Radius
};

// Used internally only, level designers should not use this flag directly!
constexpr int SF_AMBIENTMUSIC_REMOVEONFIRE = 1 << 0;

/**
 *	@brief Plays music.
 */
class CAmbientMusic : public CBaseEntity
{
	DECLARE_CLASS(CAmbientMusic, CBaseEntity);
	DECLARE_DATAMAP();

public:
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void TriggerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void RadiusThink();

private:
	std::string GetCommand() const;

private:
	string_t m_FileName;
	AmbientMusicCommand m_Command = AmbientMusicCommand::Play;
	AmbientMusicTargetSelector m_TargetSelector = AmbientMusicTargetSelector::AllPlayers;
	float m_Radius = 0;
};

LINK_ENTITY_TO_CLASS(ambient_music, CAmbientMusic);

BEGIN_DATAMAP(CAmbientMusic)
DEFINE_FIELD(m_FileName, FIELD_STRING),
	DEFINE_FIELD(m_Command, FIELD_INTEGER),
	DEFINE_FIELD(m_TargetSelector, FIELD_INTEGER),
	DEFINE_FIELD(m_Radius, FIELD_FLOAT),
	DEFINE_FUNCTION(TriggerUse),
	DEFINE_FUNCTION(RadiusThink),
	END_DATAMAP();

bool CAmbientMusic::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "filename"))
	{
		m_FileName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "command"))
	{
		m_Command = static_cast<AmbientMusicCommand>(atoi(pkvd->szValue));

		if (std::clamp(m_Command, AmbientMusicCommand::Play, AmbientMusicCommand::Stop) != m_Command)
		{
			Logger->warn("Invalid ambient_music command {}, falling back to \"Stop\"", static_cast<int>(m_Command));
			m_Command = AmbientMusicCommand::Stop;
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "target_selector"))
	{
		m_TargetSelector = static_cast<AmbientMusicTargetSelector>(atoi(pkvd->szValue));

		if (std::clamp(m_TargetSelector, AmbientMusicTargetSelector::AllPlayers, AmbientMusicTargetSelector::Radius) != m_TargetSelector)
		{
			Logger->warn("Invalid ambient_music target selector {}, falling back to \"All Players\"", static_cast<int>(m_TargetSelector));
			m_TargetSelector = AmbientMusicTargetSelector::AllPlayers;
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_Radius = atof(pkvd->szValue);

		if (const float absolute = abs(m_Radius); absolute != m_Radius)
		{
			Logger->warn("Negative ambient_music radius {}, using absolute value {}", m_Radius, absolute);
			m_Radius = absolute;
		}

		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "remove_on_fire"))
	{
		if (atoi(pkvd->szValue) == 1)
		{
			pev->spawnflags |= SF_AMBIENTMUSIC_REMOVEONFIRE;
		}
		else
		{
			pev->spawnflags &= ~SF_AMBIENTMUSIC_REMOVEONFIRE;
		}
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CAmbientMusic::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if (m_TargetSelector == AmbientMusicTargetSelector::Radius)
	{
		if (m_Radius > 0)
		{
			SetThink(&CAmbientMusic::RadiusThink);
			pev->nextthink = gpGlobals->time + 1.0;
		}
	}
	else
	{
		SetUse(&CAmbientMusic::TriggerUse);
	}
}

void CAmbientMusic::TriggerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	const std::string command = GetCommand();

	auto executor = [&](CBaseEntity* player)
	{
		if (player)
		{
			CLIENT_COMMAND(player->edict(), "%s", command.c_str());
		}
	};

	switch (m_TargetSelector)
	{
	case AmbientMusicTargetSelector::AllPlayers:
	{
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			executor(UTIL_PlayerByIndex(i));
		}
		break;
	}

	case AmbientMusicTargetSelector::Activator:
	{
		if (pActivator && pActivator->IsPlayer())
		{
			executor(pActivator);
		}
		break;
	}

	case AmbientMusicTargetSelector::Radius:
	{
		Logger->error("Invalid target selector for ambient_music \"{}\"", GetTargetname());
		break;
	}
	}

	if ((pev->spawnflags & SF_AMBIENTMUSIC_REMOVEONFIRE) != 0)
	{
		UTIL_Remove(this);
	}
}

void CAmbientMusic::RadiusThink()
{
	const std::string command = GetCommand();

	const float radiusSquared = m_Radius * m_Radius;

	bool touchedAPlayer = false;

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto player = UTIL_PlayerByIndex(i);

		if (player != nullptr && (player->pev->origin - pev->origin).LengthSquared() <= radiusSquared)
		{
			CLIENT_COMMAND(player->edict(), "%s", command.c_str());
			touchedAPlayer = true;
		}
	}

	if ((pev->spawnflags & SF_AMBIENTMUSIC_REMOVEONFIRE) != 0 && touchedAPlayer)
	{
		UTIL_Remove(this);
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.5;
	}
}

std::string CAmbientMusic::GetCommand() const
{
	// Compute the command now in case mappers change the keyvalues after spawn.
	return [this]()
	{
		switch (m_Command)
		{
		case AmbientMusicCommand::Play: return fmt::format("music play \"{}\"\n", STRING(m_FileName));
		case AmbientMusicCommand::Loop: return fmt::format("music loop \"{}\"\n", STRING(m_FileName));
		case AmbientMusicCommand::Fadeout: return fmt::format("music fadeout\n");
		default:
		case AmbientMusicCommand::Stop: return fmt::format("music stop\n");
		}
	}();
}

/**
 *	@brief A sound entity that will set player roomtype when player moves in range and sight.
 *	@details A client that is visible and in range of a sound entity will have its room_type set by that sound entity.
 *	If two or more sound entities are contending for a client,
 *	then the nearest sound entity to the client will set the client's room_type.
 *	A client's room_type will remain set to its prior value until a new in-range,
 *	visible sound entity resets a new room_type.
 */
class CEnvSound : public CPointEntity
{
	DECLARE_CLASS(CEnvSound, CPointEntity);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;

	/**
	*	@brief A client that is visible and in range of a sound entity will have its room_type set by that sound entity.If two or more
// sound entities are contending for a client, then the nearest
// sound entity to the client will set the client's room_type.
// A client's room_type will remain set to its prior value until
// a new in-range, visible sound entity resets a new room_type.
	*/
	void Think() override;

	float m_flRadius;
	int m_Roomtype;
};

LINK_ENTITY_TO_CLASS(env_sound, CEnvSound);

BEGIN_DATAMAP(CEnvSound)
DEFINE_FIELD(m_flRadius, FIELD_FLOAT),
	DEFINE_FIELD(m_Roomtype, FIELD_INTEGER),
	END_DATAMAP();

bool CEnvSound::KeyValue(KeyValueData* pkvd)
{

	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_flRadius = atof(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "roomtype"))
	{
		m_Roomtype = atoi(pkvd->szValue);
		return true;
	}

	return false;
}

/**
 *	@brief returns true if the given sound entity (pSound) is in range and can see the given player entity (target)
 */
bool FEnvSoundInRange(CEnvSound* pSound, CBaseEntity* target, float& flRange)
{
	const Vector vecSpot1 = pSound->pev->origin + pSound->pev->view_ofs;
	const Vector vecSpot2 = target->pev->origin + target->pev->view_ofs;
	TraceResult tr;

	UTIL_TraceLine(vecSpot1, vecSpot2, ignore_monsters, pSound->edict(), &tr);

	// check if line of sight crosses water boundary, or is blocked

	if ((0 != tr.fInOpen && 0 != tr.fInWater) || tr.flFraction != 1)
		return false;

	// calc range from sound entity to player

	const Vector vecRange = tr.vecEndPos - vecSpot1;
	flRange = vecRange.Length();

	return pSound->m_flRadius >= flRange;
}

// CONSIDER: if player in water state, autoset roomtype to 14,15 or 16.

void CEnvSound::Think()
{
	const bool shouldThinkFast = [this]()
	{
		// get pointer to client if visible; FIND_CLIENT_IN_PVS will
		// cycle through visible clients on consecutive calls.
		edict_t* pentPlayer = FIND_CLIENT_IN_PVS(edict());

		if (FNullEnt(pentPlayer))
			return false; // no player in pvs of sound entity, slow it down

		// check to see if this is the sound entity that is currently affecting this player
		auto pPlayer = GET_PRIVATE<CBasePlayer>(pentPlayer);
		float flRange;

		if (pPlayer->m_SndLast && pPlayer->m_SndLast == this)
		{
			// this is the entity currently affecting player, check for validity
			if (pPlayer->m_SndRoomtype != 0 && pPlayer->m_flSndRange != 0)
			{
				// we're looking at a valid sound entity affecting
				// player, make sure it's still valid, update range
				if (FEnvSoundInRange(this, pPlayer, flRange))
				{
					pPlayer->m_flSndRange = flRange;
					return true;
				}
				else
				{
					// current sound entity affecting player is no longer valid,
					// flag this state by clearing sound handle and range.
					// NOTE: we do not actually change the player's room_type
					// NOTE: until we have a new valid room_type to change it to.

					pPlayer->m_SndLast = nullptr;
					pPlayer->m_flSndRange = 0;
				}
			}

			// entity is affecting player but is out of range,
			// wait passively for another entity to usurp it...
			return false;
		}

		// if we got this far, we're looking at an entity that is contending
		// for current player sound. the closest entity to player wins.
		if (FEnvSoundInRange(this, pPlayer, flRange))
		{
			if (flRange < pPlayer->m_flSndRange || pPlayer->m_flSndRange == 0)
			{
				// new entity is closer to player, so it wins.
				pPlayer->m_SndLast = this;
				pPlayer->m_SndRoomtype = m_Roomtype;
				pPlayer->m_flSndRange = flRange;

				// New room type is sent to player in CBasePlayer::UpdateClientData.

				// crank up nextthink rate for new active sound entity
			}
			// player is not closer to the contending sound entity.
			// this effectively cranks up the think rate of env_sound entities near the player.
		}

		// player is in pvs of sound entity, but either not visible or not in range. do nothing.

		return true;
	}();

	pev->nextthink = gpGlobals->time + (shouldThinkFast ? 0.25 : 0.75);
}

void CEnvSound::Spawn()
{
	// spread think times
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.0, 0.5);
}

// ===================== MATERIAL TYPE DETECTION, MAIN ROUTINES ========================

/**
 *	@brief play a strike sound based on the texture that was hit by the attack traceline.
 *	VecSrc/VecEnd are the original traceline endpoints used by the attacker
 *	@param iBulletType the type of bullet that hit the texture.
 *	@return volume of strike instrument (crowbar) to play
 */
float TEXTURETYPE_PlaySound(TraceResult* ptr, Vector vecSrc, Vector vecEnd, int iBulletType)
{
	// hit the world, try to play sound based on texture material type

	char chTextureType;
	float fvol;
	float fvolbar;
	const char* pTextureName;
	const char* rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;

	if (!g_pGameRules->PlayTextureSounds())
		return 0.0;

	CBaseEntity* pEntity = CBaseEntity::Instance(ptr->pHit);

	chTextureType = 0;

	if (pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	else
	{
		// hit world

		// find texture under strike, get material type

		// get texture from entity or world (world is ent(0))
		if (pEntity)
			pTextureName = TRACE_TEXTURE(ENT(pEntity->pev), vecSrc, vecEnd);
		else
			pTextureName = TRACE_TEXTURE(CBaseEntity::World->edict(), vecSrc, vecEnd);

		if (pTextureName)
		{
			pTextureName = g_MaterialSystem.StripTexturePrefix(pTextureName);

			// Logger->debug("texture hit: {}", pTextureName);

			// get texture type
			chTextureType = g_MaterialSystem.FindTextureType(pTextureName);
		}
	}

	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		fvol = 0.9;
		fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL:
		fvol = 0.9;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:
		fvol = 0.9;
		fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:
		fvol = 0.5;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE:
		fvol = 0.9;
		fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH:
		fvol = 0.9;
		fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD:
		fvol = 0.9;
		fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if (iBulletType == BULLET_PLAYER_CROWBAR)
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;
		fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	case CHAR_TEX_SNOW:
		fvol = 0.9;
		fvolbar = 0.1;
		rgsz[0] = "player/pl_snow1.wav";
		rgsz[1] = "player/pl_snow2.wav";
		rgsz[2] = "player/pl_snow3.wav";
		cnt = 3;
		break;
	}

	// did we hit a breakable?

	if (pEntity && pEntity->ClassnameIs("func_breakable"))
	{
		// drop volumes, the object will already play a damaged sound
		fvol /= 1.5;
		fvolbar /= 2.0;
	}
	else if (chTextureType == CHAR_TEX_COMPUTER)
	{
		// play random spark if computer

		if (ptr->flFraction != 1.0 && RANDOM_LONG(0, 1))
		{
			UTIL_Sparks(ptr->vecEndPos);

			float flVolume = RANDOM_FLOAT(0.7, 1.0); // random volume range
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				CBaseEntity::World->EmitAmbientSound(ptr->vecEndPos, "buttons/spark5.wav", flVolume, ATTN_NORM, 0, 100);
				break;
			case 1:
				CBaseEntity::World->EmitAmbientSound(ptr->vecEndPos, "buttons/spark6.wav", flVolume, ATTN_NORM, 0, 100);
				break;
				// case 0: EmitSound(CHAN_VOICE, "buttons/spark5.wav", flVolume, ATTN_NORM);	break;
				// case 1: EmitSound(CHAN_VOICE, "buttons/spark6.wav", flVolume, ATTN_NORM);	break;
			}
		}
	}

	// play material hit sound
	CBaseEntity::World->EmitAmbientSound(ptr->vecEndPos, rgsz[RANDOM_LONG(0, cnt - 1)], fvol, fattn, 0, 96 + RANDOM_LONG(0, 0xf));
	// m_pPlayer->EmitSoundDyn(CHAN_WEAPON, rgsz[RANDOM_LONG(0,cnt-1)], fvol, ATTN_NORM, 0, 96 + RANDOM_LONG(0,0xf));

	return fvolbar;
}

/**
 *	@brief Used for announcements per level, for door lock/unlock spoken voice.
 */
class CSpeaker : public CBaseEntity
{
	DECLARE_CLASS(CSpeaker, CBaseEntity);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Precache() override;

	/**
	 *	@brief if an announcement is pending, cancel it. If no announcement is pending, start one.
	 */
	void ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void SpeakerThink();

	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	int m_preset; // preset number
};

LINK_ENTITY_TO_CLASS(speaker, CSpeaker);

BEGIN_DATAMAP(CSpeaker)
DEFINE_FIELD(m_preset, FIELD_INTEGER),
	DEFINE_FUNCTION(ToggleUse),
	DEFINE_FUNCTION(SpeakerThink),
	END_DATAMAP();

void CSpeaker::Spawn()
{
	const char* szSoundFile = STRING(pev->message);

	if (0 == m_preset && (FStringNull(pev->message) || strlen(szSoundFile) < 1))
	{
		Logger->error("SPEAKER with no Level/Sentence! at: {}", pev->origin);
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(&CSpeaker::SUB_Remove);
		return;
	}
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;


	SetThink(&CSpeaker::SpeakerThink);
	pev->nextthink = 0.0;

	// allow on/off switching via 'use' function.

	SetUse(&CSpeaker::ToggleUse);

	Precache();
}

#define ANNOUNCE_MINUTES_MIN 0.25
#define ANNOUNCE_MINUTES_MAX 2.25

void CSpeaker::Precache()
{
	if (!FBitSet(pev->spawnflags, SPEAKER_START_SILENT))
		// set first announcement time for random n second
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(5.0, 15.0);
}

void CSpeaker::SpeakerThink()
{
	float flvolume = pev->health * 0.1;
	float flattenuation = 0.3;
	int flags = 0;
	int pitch = 100;


	// Wait for the talkmonster to finish first.
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
	{
		pev->nextthink = CTalkMonster::g_talkWaitTime + RANDOM_FLOAT(5, 10);
		return;
	}

	const char* const szSoundFile = [this]()
	{
		// go lookup preset text, assign szSoundFile
		switch (m_preset)
		{
		case 0: return STRING(pev->message);
		case 1: return "C1A0_";
		case 2: return "C1A1_";
		case 3: return "C1A2_";
		case 4: return "C1A3_";
		case 5: return "C1A4_";
		case 6: return "C2A1_";
		case 7: return "C2A2_";
		case 8: return "C2A3_";
		case 9: return "C2A4_";
		case 10: return "C2A5_";
		case 11: return "C3A1_";
		case 12: return "C3A2_";
		default: return "";
		}
	}();

	if (szSoundFile[0] == '!')
	{
		// play single sentence, one shot
		EmitAmbientSound(pev->origin, szSoundFile,
			flvolume, flattenuation, flags, pitch);

		// shut off and reset
		pev->nextthink = 0.0;
	}
	else
	{
		// make random announcement from sentence group

		if (sentences::g_Sentences.PlayRndSz(this, szSoundFile, flvolume, flattenuation, flags, pitch) < 0)
			Logger->debug("Level Design Error!: SPEAKER has bad sentence group name: {}", szSoundFile);

		// set next announcement time for random 5 to 10 minute delay
		pev->nextthink = gpGlobals->time +
						 RANDOM_FLOAT(ANNOUNCE_MINUTES_MIN * 60.0, ANNOUNCE_MINUTES_MAX * 60.0);

		CTalkMonster::g_talkWaitTime = gpGlobals->time + 5; // time delay until it's ok to speak: used so that two NPCs don't talk at once
	}

	return;
}

void CSpeaker::ToggleUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	bool fActive = (pev->nextthink > 0.0);

	// fActive is true only if an announcement is pending

	if (useType != USE_TOGGLE)
	{
		// ignore if we're just turning something on that's already on, or
		// turning something off that's already off.
		if ((fActive && useType == USE_ON) || (!fActive && useType == USE_OFF))
			return;
	}

	if (useType == USE_ON)
	{
		// turn on announcements
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	if (useType == USE_OFF)
	{
		// turn off announcements
		pev->nextthink = 0.0;
		return;
	}

	// Toggle announcements


	if (fActive)
	{
		// turn off announcements
		pev->nextthink = 0.0;
	}
	else
	{
		// turn on announcements
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

bool CSpeaker::KeyValue(KeyValueData* pkvd)
{

	// preset
	if (FStrEq(pkvd->szKeyName, "preset"))
	{
		m_preset = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}
