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

#include "Platform.h"

//
// Misc utility code
//
#include "activity.h"
#include "enginecallback.h"

#include "sound/sentence_utils.h"
#include "utils/shared_utils.h"

// TODO: need a cleaner way to split server and client dependencies to avoid pulling them in with the precompiled header.
#ifndef CLIENT_DLL
#include "sound/SentencesSystem.h"
#endif

inline globalvars_t* gpGlobals = nullptr;

inline edict_t* FIND_ENTITY_BY_CLASSNAME(edict_t* entStart, const char* pszName)
{
	return FIND_ENTITY_BY_STRING(entStart, "classname", pszName);
}

inline edict_t* FIND_ENTITY_BY_TARGETNAME(edict_t* entStart, const char* pszName)
{
	return FIND_ENTITY_BY_STRING(entStart, "targetname", pszName);
}

// for doing a reverse lookup. Say you have a door, and want to find its button.
inline edict_t* FIND_ENTITY_BY_TARGET(edict_t* entStart, const char* pszName)
{
	return FIND_ENTITY_BY_STRING(entStart, "target", pszName);
}

// Keeps clutter down a bit, when using a float as a bit-vector
#define SetBits(flBitVector, bits) ((flBitVector) = (int)(flBitVector) | (bits))
#define ClearBits(flBitVector, bits) ((flBitVector) = (int)(flBitVector) & ~(bits))
#define FBitSet(flBitVector, bit) (((int)(flBitVector) & (bit)) != 0)

// Makes these more explicit, and easier to find
#define FILE_GLOBAL static
#define DLL_GLOBAL

// Until we figure out why "const" gives the compiler problems, we'll just have to use
// this bogus "empty" define to mark things as constant.
#define CONSTANT

// More explicit than "int"
typedef int EOFFSET;

// In case this ever changes
#define M_PI 3.14159265358979323846

// This is the glue that hooks .MAP entity class names to our CPP classes
// The _declspec forces them to be exported by name so we can do a lookup with GetProcAddress()
// The function is used to intialize / allocate the object for the entity
#define LINK_ENTITY_TO_CLASS(mapClassName, DLLClassName)    \
	extern "C" DLLEXPORT void mapClassName(entvars_t* pev); \
	void mapClassName(entvars_t* pev) { GetClassPtr((DLLClassName*)pev); }


//
// Conversion among the three types of "entity", including identity-conversions.
//
#ifdef DEBUG
edict_t* DBG_EntOfVars(const entvars_t* pev);
inline edict_t* ENT(const entvars_t* pev) { return DBG_EntOfVars(pev); }
#else
inline edict_t* ENT(const entvars_t* pev)
{
	return pev->pContainingEntity;
}
#endif
inline edict_t* ENT(edict_t* pent)
{
	return pent;
}
inline edict_t* ENT(EOFFSET eoffset) { return (*g_engfuncs.pfnPEntityOfEntOffset)(eoffset); }
inline EOFFSET OFFSET(EOFFSET eoffset) { return eoffset; }
inline EOFFSET OFFSET(const edict_t* pent)
{
#if _DEBUG
	if (!pent)
		ALERT(at_error, "Bad ent in OFFSET()\n");
#endif
	return (*g_engfuncs.pfnEntOffsetOfPEntity)(pent);
}
inline EOFFSET OFFSET(entvars_t* pev)
{
#if _DEBUG
	if (!pev)
		ALERT(at_error, "Bad pev in OFFSET()\n");
#endif
	return OFFSET(ENT(pev));
}
inline entvars_t* VARS(entvars_t* pev) { return pev; }

inline entvars_t* VARS(edict_t* pent)
{
	if (!pent)
		return nullptr;

	return &pent->v;
}

inline entvars_t* VARS(EOFFSET eoffset) { return VARS(ENT(eoffset)); }
inline int ENTINDEX(edict_t* pEdict) { return (*g_engfuncs.pfnIndexOfEdict)(pEdict); }
inline edict_t* INDEXENT(int iEdictNum) { return (*g_engfuncs.pfnPEntityOfEntIndex)(iEdictNum); }
inline void MESSAGE_BEGIN(int msg_dest, int msg_type, const float* pOrigin, entvars_t* ent)
{
	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ENT(ent));
}

// Testing the three types of "entity" for nullity
#define eoNullEntity 0
inline bool FNullEnt(EOFFSET eoffset)
{
	return eoffset == 0;
}
inline bool FNullEnt(const edict_t* pent) { return pent == nullptr || FNullEnt(OFFSET(pent)); }
inline bool FNullEnt(entvars_t* pev) { return pev == nullptr || FNullEnt(OFFSET(pev)); }

// Testing strings for nullity
#define iStringNull 0
inline bool FStringNull(int iString)
{
	return iString == iStringNull;
}

#define cchMapNameMost 32

// Dot products for view cone checking
#define VIEW_FIELD_FULL (float)-1.0		   // +-180 degrees
#define VIEW_FIELD_WIDE (float)-0.7		   // +-135 degrees 0.1 // +-85 degrees, used for full FOV checks
#define VIEW_FIELD_NARROW (float)0.7	   // +-45 degrees, more narrow check used to set up ranged attacks
#define VIEW_FIELD_ULTRA_NARROW (float)0.9 // +-25 degrees, more narrow check used to set up ranged attacks

// All monsters need this data
#define DONT_BLEED -1
#define BLOOD_COLOR_RED (byte)247
#define BLOOD_COLOR_YELLOW (byte)195
#define BLOOD_COLOR_GREEN BLOOD_COLOR_YELLOW

typedef enum
{

	MONSTERSTATE_NONE = 0,
	MONSTERSTATE_IDLE,
	MONSTERSTATE_COMBAT,
	MONSTERSTATE_ALERT,
	MONSTERSTATE_HUNT,
	MONSTERSTATE_PRONE,
	MONSTERSTATE_SCRIPT,
	MONSTERSTATE_PLAYDEAD,
	MONSTERSTATE_DEAD,

	MONSTERSTATE_COUNT //Must be last, not a valid state

} MONSTERSTATE;



// Things that toggle (buttons/triggers/doors) need this
typedef enum
{
	TS_AT_TOP,
	TS_AT_BOTTOM,
	TS_GOING_UP,
	TS_GOING_DOWN
} TOGGLE_STATE;

// Misc useful
inline bool FStrEq(const char* sz1, const char* sz2)
{
	return (strcmp(sz1, sz2) == 0);
}
inline bool FClassnameIs(edict_t* pent, const char* szClassname)
{
	return FStrEq(STRING(VARS(pent)->classname), szClassname);
}
inline bool FClassnameIs(entvars_t* pev, const char* szClassname)
{
	return FStrEq(STRING(pev->classname), szClassname);
}

class CBaseEntity;

// Misc. Prototypes
void UTIL_SetSize(entvars_t* pev, const Vector& vecMin, const Vector& vecMax);
float UTIL_VecToYaw(const Vector& vec);
Vector UTIL_VecToAngles(const Vector& vec);
float UTIL_AngleMod(float a);
float UTIL_AngleDiff(float destAngle, float srcAngle);

CBaseEntity* UTIL_FindEntityInSphere(CBaseEntity* pStartEntity, const Vector& vecCenter, float flRadius);
CBaseEntity* UTIL_FindEntityByString(CBaseEntity* pStartEntity, const char* szKeyword, const char* szValue);
CBaseEntity* UTIL_FindEntityByClassname(CBaseEntity* pStartEntity, const char* szName);
CBaseEntity* UTIL_FindEntityByTargetname(CBaseEntity* pStartEntity, const char* szName);
CBaseEntity* UTIL_FindEntityGeneric(const char* szName, Vector& vecSrc, float flRadius);

// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns nullptr
// Index is 1 based
CBaseEntity* UTIL_PlayerByIndex(int playerIndex);

#define UTIL_EntitiesInPVS(pent) (*g_engfuncs.pfnEntitiesInPVS)(pent)
void UTIL_MakeVectors(const Vector& vecAngles);

// Pass in an array of pointers and an array size, it fills the array and returns the number inserted
int UTIL_MonstersInSphere(CBaseEntity** pList, int listMax, const Vector& center, float radius);
int UTIL_EntitiesInBox(CBaseEntity** pList, int listMax, const Vector& mins, const Vector& maxs, int flagMask);

inline void UTIL_MakeVectorsPrivate(const Vector& vecAngles, float* p_vForward, float* p_vRight, float* p_vUp)
{
	g_engfuncs.pfnAngleVectors(vecAngles, p_vForward, p_vRight, p_vUp);
}

void UTIL_MakeAimVectors(const Vector& vecAngles); // like MakeVectors, but assumes pitch isn't inverted
void UTIL_MakeInvVectors(const Vector& vec, globalvars_t* pgv);

void UTIL_SetOrigin(entvars_t* pev, const Vector& vecOrigin);
void UTIL_ParticleEffect(const Vector& vecOrigin, const Vector& vecDirection, unsigned int ulColor, unsigned int ulCount);
void UTIL_ScreenShake(const Vector& center, float amplitude, float frequency, float duration, float radius);
void UTIL_ScreenShakeAll(const Vector& center, float amplitude, float frequency, float duration);
void UTIL_ShowMessage(const char* pString, CBaseEntity* pPlayer);
void UTIL_ShowMessageAll(const char* pString);
void UTIL_ScreenFadeAll(const Vector& color, float fadeTime, float holdTime, int alpha, int flags);
void UTIL_ScreenFade(CBaseEntity* pEntity, const Vector& color, float fadeTime, float fadeHold, int alpha, int flags);

typedef enum
{
	ignore_monsters = 1,
	dont_ignore_monsters = 0,
	missile = 2
} IGNORE_MONSTERS;
typedef enum
{
	ignore_glass = 1,
	dont_ignore_glass = 0
} IGNORE_GLASS;
void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, edict_t* pentIgnore, TraceResult* ptr);
void UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t* pentIgnore, TraceResult* ptr);
enum
{
	point_hull = 0,
	human_hull = 1,
	large_hull = 2,
	head_hull = 3
};
void UTIL_TraceHull(const Vector& vecStart, const Vector& vecEnd, IGNORE_MONSTERS igmon, int hullNumber, edict_t* pentIgnore, TraceResult* ptr);
TraceResult UTIL_GetGlobalTrace();
void UTIL_TraceModel(const Vector& vecStart, const Vector& vecEnd, int hullNumber, edict_t* pentModel, TraceResult* ptr);
Vector UTIL_GetAimVector(edict_t* pent, float flSpeed);
int UTIL_PointContents(const Vector& vec);

bool UTIL_IsMasterTriggered(string_t sMaster, CBaseEntity* pActivator);
void UTIL_BloodStream(const Vector& origin, const Vector& direction, int color, int amount);
void UTIL_BloodDrips(const Vector& origin, const Vector& direction, int color, int amount);
Vector UTIL_RandomBloodVector();
bool UTIL_ShouldShowBlood(int bloodColor);
void UTIL_BloodDecalTrace(TraceResult* pTrace, int bloodColor);
void UTIL_DecalTrace(TraceResult* pTrace, int decalNumber);
void UTIL_PlayerDecalTrace(TraceResult* pTrace, int playernum, int decalNumber, bool bIsCustom);
void UTIL_GunshotDecalTrace(TraceResult* pTrace, int decalNumber);
void UTIL_Sparks(const Vector& position);
void UTIL_Ricochet(const Vector& position, float scale);
void UTIL_StringToIntArray(int* pVector, int count, const char* pString);
Vector UTIL_ClampVectorToBox(const Vector& input, const Vector& clampSize);
float UTIL_Approach(float target, float value, float speed);
float UTIL_ApproachAngle(float target, float value, float speed);
float UTIL_AngleDistance(float next, float cur);

char* UTIL_VarArgs(const char* format, ...);
void UTIL_Remove(CBaseEntity* pEntity);
bool UTIL_IsValidEntity(edict_t* pent);
bool UTIL_TeamsMatch(const char* pTeamName1, const char* pTeamName2);

// Use for ease-in, ease-out style interpolation (accel/decel)
float UTIL_SplineFraction(float value, float scale);

// Search for water transition along a vertical line
float UTIL_WaterLevel(const Vector& position, float minz, float maxz);
void UTIL_Bubbles(Vector mins, Vector maxs, int count);
void UTIL_BubbleTrail(Vector from, Vector to, int count);

// allows precacheing of other entities
void UTIL_PrecacheOther(const char* szClassname);

// prints a message to each client
void UTIL_ClientPrintAll(int msg_dest, const char* msg_name, const char* param1 = nullptr, const char* param2 = nullptr, const char* param3 = nullptr, const char* param4 = nullptr);
inline void UTIL_CenterPrintAll(const char* msg_name, const char* param1 = nullptr, const char* param2 = nullptr, const char* param3 = nullptr, const char* param4 = nullptr)
{
	UTIL_ClientPrintAll(HUD_PRINTCENTER, msg_name, param1, param2, param3, param4);
}

class CBasePlayerItem;
class CBasePlayer;

// prints messages through the HUD
void ClientPrint(entvars_t* client, int msg_dest, const char* msg_name, const char* param1 = nullptr, const char* param2 = nullptr, const char* param3 = nullptr, const char* param4 = nullptr);

// prints a message to the HUD say (chat)
void UTIL_SayText(const char* pText, CBaseEntity* pEntity);
void UTIL_SayTextAll(const char* pText, CBaseEntity* pEntity);


typedef struct hudtextparms_s
{
	float x;
	float y;
	int effect;
	byte r1, g1, b1, a1;
	byte r2, g2, b2, a2;
	float fadeinTime;
	float fadeoutTime;
	float holdTime;
	float fxTime;
	int channel;
} hudtextparms_t;

// prints as transparent 'title' to the HUD
void UTIL_HudMessageAll(const hudtextparms_t& textparms, const char* pMessage);
void UTIL_HudMessage(CBaseEntity* pEntity, const hudtextparms_t& textparms, const char* pMessage);

// Writes message to console with timestamp and FragLog header.
void UTIL_LogPrintf(const char* fmt, ...);

// Sorta like FInViewCone, but for nonmonsters.
float UTIL_DotPoints(const Vector& vecSrc, const Vector& vecCheck, const Vector& vecDir);

void UTIL_StripToken(const char* pKey, char* pDest); // for redundant keynames

// Misc functions
void SetMovedir(entvars_t* pev);
Vector VecBModelOrigin(entvars_t* pevBModel);
int BuildChangeList(LEVELLIST* pLevelList, int maxList);

//
// How did I ever live without ASSERT?
//
#ifdef DEBUG
void DBG_AssertFunction(bool fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage);
#define ASSERT(f) DBG_AssertFunction(f, #f, __FILE__, __LINE__, nullptr)
#define ASSERTSZ(f, sz) DBG_AssertFunction(f, #f, __FILE__, __LINE__, sz)
#else // !DEBUG
#define ASSERT(f)
#define ASSERTSZ(f, sz)
#endif // !DEBUG

//
// Constants that were used only by QC (maybe not used at all now)
//
// Un-comment only as needed
//
#define LANGUAGE_GERMAN 1

inline DLL_GLOBAL int g_Language;

#define AMBIENT_SOUND_STATIC 0 // medium radius attenuation
#define AMBIENT_SOUND_EVERYWHERE 1
#define AMBIENT_SOUND_SMALLRADIUS 2
#define AMBIENT_SOUND_MEDIUMRADIUS 4
#define AMBIENT_SOUND_LARGERADIUS 8
#define AMBIENT_SOUND_START_SILENT 16
#define AMBIENT_SOUND_NOT_LOOPING 32

#define SPEAKER_START_SILENT 1 // wait for trigger 'on' to start announcements

constexpr int SND_VOLUME = 1 << 0;				// Volume is not 255
constexpr int SND_ATTENUATION = 1 << 1;			// Attenuation is not 1
constexpr int SND_LARGE_INDEX = 1 << 2;			// Sound or sentence index is larger than 8 bits
constexpr int SND_PITCH = 1 << 3;				// Pitch is not 100
constexpr int SND_SENTENCE = 1 << 4;			// This is a sentence
constexpr int SND_STOP = 1 << 5;				// duplicated in protocol.h stop sound
constexpr int SND_CHANGE_VOL = 1 << 6;			// duplicated in protocol.h change sound vol
constexpr int SND_CHANGE_PITCH = 1 << 7;		// duplicated in protocol.h change sound pitch
constexpr int SND_SPAWNING = 1 << 8;			// duplicated in protocol.h we're spawing, used in some cases for ambients
constexpr int SND_PLAY_WHEN_PAUSED = 1 << 9;	// For client side use only: start playing sound even when paused.

#define LFO_SQUARE 1
#define LFO_TRIANGLE 2
#define LFO_RANDOM 3

// func_rotating
#define SF_BRUSH_ROTATE_Y_AXIS 0
#define SF_BRUSH_ROTATE_INSTANT 1
#define SF_BRUSH_ROTATE_BACKWARDS 2
#define SF_BRUSH_ROTATE_Z_AXIS 4
#define SF_BRUSH_ROTATE_X_AXIS 8
#define SF_PENDULUM_AUTO_RETURN 16
#define SF_PENDULUM_PASSABLE 32


#define SF_BRUSH_ROTATE_SMALLRADIUS 128
#define SF_BRUSH_ROTATE_MEDIUMRADIUS 256
#define SF_BRUSH_ROTATE_LARGERADIUS 512

#define SVC_TEMPENTITY 23
#define SVC_INTERMISSION 30
#define SVC_CDTRACK 32
#define SVC_WEAPONANIM 35
#define SVC_ROOMTYPE 37
#define SVC_DIRECTOR 51



// triggers
#define SF_TRIGGER_ALLOWMONSTERS 1 // monsters allowed to fire this trigger
#define SF_TRIGGER_NOCLIENTS 2	   // players not allowed to fire this trigger
#define SF_TRIGGER_PUSHABLES 4	   // only pushables can fire this trigger

// func breakable
#define SF_BREAK_TRIGGER_ONLY 1 // may only be broken by trigger
#define SF_BREAK_TOUCH 2		// can be 'crashed through' by running player (plate glass)
#define SF_BREAK_PRESSURE 4		// can be broken by a player standing on it
#define SF_BREAK_CROWBAR 256	// instant break if hit with crowbar

// func_pushable (it's also func_breakable, so don't collide with those flags)
#define SF_PUSH_BREAKABLE 128

#define SF_LIGHT_START_OFF 1

#define SPAWNFLAG_NOMESSAGE 1
#define SPAWNFLAG_NOTOUCH 1

#define SF_TRIG_PUSH_ONCE 1


// Sound Utilities

void TEXTURETYPE_Init();
char TEXTURETYPE_Find(char* name);
float TEXTURETYPE_PlaySound(TraceResult* ptr, Vector vecSrc, Vector vecEnd, int iBulletType);

// NOTE: use EMIT_SOUND_DYN to set the pitch of a sound. Pitch of 100
// is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
// down to 1 is a lower pitch.   150 to 70 is the realistic range.
// EMIT_SOUND_DYN with pitch != 100 should be used sparingly, as it's not quite as
// fast as EMIT_SOUND (the pitchshift mixer is not native coded).

void EMIT_SOUND_DYN(edict_t* entity, int channel, const char* sample, float volume, float attenuation,
	int flags, int pitch);

void UTIL_EmitAmbientSound(edict_t* entity, const Vector& vecOrigin, const char* samp, float vol, float attenuation, int fFlags, int pitch);

inline void EMIT_SOUND(edict_t* entity, int channel, const char* sample, float volume, float attenuation)
{
	EMIT_SOUND_DYN(entity, channel, sample, volume, attenuation, 0, PITCH_NORM);
}

inline void STOP_SOUND(edict_t* entity, int channel, const char* sample)
{
	EMIT_SOUND_DYN(entity, channel, sample, 0, 0, SND_STOP, PITCH_NORM);
}

/**
*	@brief Just like @see EMIT_SOUND_DYN, but will skip the current host player if they have cl_lw turned on.
*	@details entity must be the current host entity for this to work, and must be called only inside a player's PostThink method.
*/
void EMIT_SOUND_PREDICTED(edict_t* entity, int channel, const char* sample, float volume, float attenuation,
	int flags, int pitch);

/**
*	@brief play a specific sentence over the HEV suit speaker - just pass player entity, and !sentencename
*/
void EMIT_SOUND_SUIT(edict_t* entity, const char* sample);

/**
*	@brief play a sentence, randomly selected from the passed in group id, over the HEV suit speaker
*/
void EMIT_GROUPID_SUIT(edict_t* entity, int isentenceg);

/**
*	@brief play a sentence, randomly selected from the passed in groupname
*/
void EMIT_GROUPNAME_SUIT(edict_t* entity, const char* groupname);

#define PRECACHE_SOUND_ARRAY(a)                        \
	{                                                  \
		for (std::size_t i = 0; i < std::size(a); i++) \
			PrecacheSound(a[i]);                      \
	}

#define EMIT_SOUND_ARRAY_DYN(chan, array) \
	EMIT_SOUND_DYN(ENT(pev), chan, array[RANDOM_LONG(0, std::size(array) - 1)], 1.0, ATTN_NORM, 0, RANDOM_LONG(95, 105));

#define RANDOM_SOUND_ARRAY(array) (array)[RANDOM_LONG(0, std::size((array)) - 1)]

#define PLAYBACK_EVENT(flags, who, index) PLAYBACK_EVENT_FULL(flags, who, index, 0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);
#define PLAYBACK_EVENT_DELAY(flags, who, index, delay) PLAYBACK_EVENT_FULL(flags, who, index, delay, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

#define GROUP_OP_AND 0
#define GROUP_OP_NAND 1

inline int g_groupmask = 0;
inline int g_groupop = 0;

class UTIL_GroupTrace
{
public:
	UTIL_GroupTrace(int groupmask, int op);
	~UTIL_GroupTrace();

private:
	int m_oldgroupmask, m_oldgroupop;
};

void UTIL_SetGroupTrace(int groupmask, int op);
void UTIL_UnsetGroupTrace();

float UTIL_WeaponTimeBase();

CBaseEntity* UTIL_FindEntityForward(CBaseEntity* pMe);

bool UTIL_IsMultiplayer();

bool UTIL_IsCTF();

inline void WRITE_COORD_VECTOR(const Vector& vec)
{
	WRITE_COORD(vec.x);
	WRITE_COORD(vec.y);
	WRITE_COORD(vec.z);
}

struct MinuteSecondTime
{
	const int Minutes;
	const int Seconds;
};

/**
*	@brief Converts seconds to minutes and seconds
*/
inline MinuteSecondTime SecondsToTime(const int seconds)
{
	const auto minutes = seconds / 60;
	return {minutes, seconds - (minutes * 60)};
}

template <typename T>
struct FindByClassnameFunctor
{
	static T* Find(T* pStartEntity, const char* pszClassname)
	{
		return static_cast<T*>(UTIL_FindEntityByClassname(pStartEntity, pszClassname));
	}
};

template <typename T>
struct FindByTargetnameFunctor
{
	static T* Find(T* pStartEntity, const char* pszName)
	{
		return static_cast<T*>(UTIL_FindEntityByTargetname(pStartEntity, pszName));
	}
};

template <typename T>
struct FindNextEntityFunctor
{
	static T* Find(T* pStartEntity)
	{
		//Start with first player, ignore world
		auto index = pStartEntity ? pStartEntity->entindex() + 1 : 1;

		auto entities = g_engfuncs.pfnPEntityOfEntIndexAllEntities(0);

		//Find the first entity that has a valid baseentity
		for (; index < gpGlobals->maxEntities; ++index)
		{
			auto entity = static_cast<CBaseEntity*>(GET_PRIVATE(&entities[index]));

			if (entity)
			{
				return static_cast<T*>(entity);
			}
		}

		return nullptr;
	}
};

template <typename T, typename FINDER>
class CEntityIterator
{
public:
	CEntityIterator()
		: m_pszName(""), m_pEntity(nullptr)
	{
	}

	CEntityIterator(const CEntityIterator&) = default;

	CEntityIterator(const char* const pszName, T* pEntity)
		: m_pszName(pszName), m_pEntity(pEntity)
	{
	}

	CEntityIterator& operator=(const CEntityIterator&) = default;

	const T* operator*() const { return m_pEntity; }

	T* operator*() { return m_pEntity; }

	T* operator->() { return m_pEntity; }

	void operator++()
	{
		m_pEntity = static_cast<T*>(FINDER::Find(m_pEntity, m_pszName));
	}

	void operator++(int)
	{
		++*this;
	}

	bool operator==(const CEntityIterator& other) const
	{
		return m_pEntity == other.m_pEntity;
	}

	bool operator!=(const CEntityIterator& other) const
	{
		return !(*this == other);
	}

private:
	const char* const m_pszName;
	T* m_pEntity;
};

/**
*	@brief Entity enumerator optimized for iteration from start
*/
template <typename T, typename FINDER>
class CEntityEnumerator
{
public:
	using Functor = FINDER;
	using iterator = CEntityIterator<T, Functor>;

public:
	CEntityEnumerator(const char* pszClassName)
		: m_pszName(pszClassName)
	{
	}

	iterator begin()
	{
		return {m_pszName, static_cast<T*>(Functor::Find(nullptr, m_pszName))};
	}

	iterator end()
	{
		return {m_pszName, nullptr};
	}

private:
	const char* const m_pszName;
};

/**
*	@brief Entity enumerator for iteration from a given start entity
*/
template <typename T, typename FINDER>
class CEntityEnumeratorWithStart
{
public:
	using Functor = FINDER;
	using iterator = CEntityIterator<T, Functor>;

public:
	CEntityEnumeratorWithStart(const char* pszClassName, T* pStartEntity)
		: m_pszName(pszClassName), m_pStartEntity(pStartEntity)
	{
	}

	iterator begin()
	{
		return {m_pszName, static_cast<T*>(Functor::Find(m_pStartEntity, m_pszName))};
	}

	iterator end()
	{
		return {m_pszName, nullptr};
	}

private:
	const char* const m_pszName;
	T* m_pStartEntity = nullptr;
};

template <typename T = CBaseEntity>
inline CEntityEnumerator<T, FindByClassnameFunctor<T>> UTIL_FindEntitiesByClassname(const char* pszClassName)
{
	return {pszClassName};
}

template <typename T = CBaseEntity>
inline CEntityEnumeratorWithStart<T, FindByClassnameFunctor<T>> UTIL_FindEntitiesByClassname(const char* pszClassName, T* pStartEntity)
{
	return {pszClassName, pStartEntity};
}

template <typename T = CBaseEntity>
inline CEntityEnumerator<T, FindByTargetnameFunctor<T>> UTIL_FindEntitiesByTargetname(const char* pszName)
{
	return {pszName};
}

template <typename T = CBaseEntity>
inline CEntityEnumeratorWithStart<T, FindByTargetnameFunctor<T>> UTIL_FindEntitiesByTargetname(const char* pszName, T* pStartEntity)
{
	return {pszName, pStartEntity};
}

template <typename T, typename FINDER>
class CNextEntityIterator
{
public:
	constexpr CNextEntityIterator() = default;

	constexpr CNextEntityIterator(T* pStartEntity)
		: m_pEntity(pStartEntity)
	{
	}

	constexpr CNextEntityIterator(const CNextEntityIterator&) = default;
	constexpr CNextEntityIterator& operator=(const CNextEntityIterator&) = default;

	constexpr const T* operator*() const { return m_pEntity; }

	constexpr T* operator*() { return m_pEntity; }

	constexpr T* operator->() { return m_pEntity; }

	void operator++()
	{
		m_pEntity = static_cast<T*>(FINDER::Find(m_pEntity));
	}

	void operator++(int)
	{
		++*this;
	}

	constexpr bool operator==(const CNextEntityIterator& other) const
	{
		return m_pEntity == other.m_pEntity;
	}

	constexpr bool operator!=(const CNextEntityIterator& other) const
	{
		return !(*this == other);
	}

private:
	T* m_pEntity = nullptr;
};

/**
*	@brief Entity enumerator optimized for iteration from start
*/
template <typename T, typename FINDER>
class CNextEntityEnumerator
{
public:
	using Functor = FINDER;
	using iterator = CNextEntityIterator<T, Functor>;

public:
	CNextEntityEnumerator() = default;

	iterator begin()
	{
		return {static_cast<T*>(Functor::Find(nullptr))};
	}

	iterator end()
	{
		return {nullptr};
	}
};

template <typename T = CBaseEntity>
inline CNextEntityEnumerator<T, FindNextEntityFunctor<T>> UTIL_FindEntities()
{
	return {};
}

/**
*	@brief Helper type to run a function when the helper is destroyed.
*	Useful for running cleanup on scope exit and function return.
*/
template <typename Func>
struct CallOnDestroy
{
	const Func Function;

	explicit CallOnDestroy(Func&& function)
		: Function(function)
	{
	}

	~CallOnDestroy()
	{
		Function();
	}
};
