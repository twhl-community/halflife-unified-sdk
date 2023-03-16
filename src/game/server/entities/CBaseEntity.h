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

#include <memory>

#include <spdlog/logger.h>

#include "Platform.h"
#include "extdll.h"
#include "util.h"
#include "skill.h"

class CBaseEntity;
class CBaseMonster;
class CBasePlayerWeapon;
class CCineMonster;
class COFSquadTalkMonster;
class CSound;
class CSquadMonster;
class CTalkMonster;
struct ReplacementMap;

#define MAX_PATH_SIZE 10 // max number of nodes available for a path.

// These are caps bits to indicate what an object's capabilities (currently used for save/restore and level transitions)
#define FCAP_CUSTOMSAVE 0x00000001
#define FCAP_ACROSS_TRANSITION 0x00000002 // should transfer between transitions
#define FCAP_MUST_SPAWN 0x00000004		  // Spawn after restore
#define FCAP_DONT_SAVE 0x80000000		  // Don't save this
#define FCAP_IMPULSE_USE 0x00000008		  // can be used by the player
#define FCAP_CONTINUOUS_USE 0x00000010	  // can be used by the player
#define FCAP_ONOFF_USE 0x00000020		  // can be used by the player
#define FCAP_DIRECTIONAL_USE 0x00000040	  // Player sends +/- 1 when using (currently only tracktrains)
#define FCAP_MASTER 0x00000080			  // Can be used to "master" other entities (like multisource)

// UNDONE: This will ignore transition volumes (trigger_transition), but not the PVS!!!
#define FCAP_FORCE_TRANSITION 0x00000080 // ALWAYS goes across transitions

#define SF_NORESPAWN (1 << 30) // !!!set this bit on guns and stuff that should never respawn.

#define EXPORT DLLEXPORT

enum USE_TYPE
{
	USE_OFF = 0,
	USE_ON = 1,
	USE_SET = 2,
	USE_TOGGLE = 3
};

template <typename T>
using TBASEPTR = void (T::*)();

template <typename T>
using TENTITYFUNCPTR = void (T::*)(CBaseEntity* pOther);

template <typename T>
using TUSEPTR = void (T::*)(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

using BASEPTR = TBASEPTR<CBaseEntity>;
using ENTITYFUNCPTR = TENTITYFUNCPTR<CBaseEntity>;
using USEPTR = TUSEPTR<CBaseEntity>;

// For CLASSIFY
#define CLASS_NONE 0
#define CLASS_MACHINE 1
#define CLASS_PLAYER 2
#define CLASS_HUMAN_PASSIVE 3
#define CLASS_HUMAN_MILITARY 4
#define CLASS_ALIEN_MILITARY 5
#define CLASS_ALIEN_PASSIVE 6
#define CLASS_ALIEN_MONSTER 7
#define CLASS_ALIEN_PREY 8
#define CLASS_ALIEN_PREDATOR 9
#define CLASS_INSECT 10
#define CLASS_PLAYER_ALLY 11
#define CLASS_PLAYER_BIOWEAPON 12		 // hornets and snarks.launched by players
#define CLASS_ALIEN_BIOWEAPON 13		 // hornets and snarks.launched by the alien menace
#define CLASS_HUMAN_MILITARY_FRIENDLY 14 // Opposing Force friendlies
#define CLASS_ALIEN_RACE_X 15
#define CLASS_CTFITEM 30
#define CLASS_BARNACLE 99 // special because no one pays attention to it, and it eats a wide cross-section of creatures.

// Defines the range of valid class values for use in IRelationship.
constexpr int CLASS_FIRST = CLASS_MACHINE;
constexpr int CLASS_LAST = CLASS_ALIEN_RACE_X;

// people gib if their health is <= this at the time of death
#define GIB_HEALTH_VALUE -30

#define bits_CAP_DUCK (1 << 0)		 // crouch
#define bits_CAP_JUMP (1 << 1)		 // jump/leap
#define bits_CAP_STRAFE (1 << 2)	 // strafe ( walk/run sideways)
#define bits_CAP_SQUAD (1 << 3)		 // can form squads
#define bits_CAP_SWIM (1 << 4)		 // proficiently navigate in water
#define bits_CAP_CLIMB (1 << 5)		 // climb ladders/ropes
#define bits_CAP_USE (1 << 6)		 // open doors/push buttons/pull levers
#define bits_CAP_HEAR (1 << 7)		 // can hear forced sounds
#define bits_CAP_AUTO_DOORS (1 << 8) // can trigger auto doors
#define bits_CAP_OPEN_DOORS (1 << 9) // can open manual doors
#define bits_CAP_TURN_HEAD (1 << 10) // can turn head, always bone controller 0

#define bits_CAP_RANGE_ATTACK1 (1 << 11) // can do a range attack 1
#define bits_CAP_RANGE_ATTACK2 (1 << 12) // can do a range attack 2
#define bits_CAP_MELEE_ATTACK1 (1 << 13) // can do a melee attack 1
#define bits_CAP_MELEE_ATTACK2 (1 << 14) // can do a melee attack 2

#define bits_CAP_FLY (1 << 15) // can fly, move all around

#define bits_CAP_DOORS_GROUP (bits_CAP_USE | bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS)

// when calling KILLED(), a value that governs gib behavior is expected to be
// one of these three values
#define GIB_NORMAL 0 // gib if entity was overkilled
#define GIB_NEVER 1	 // never gib, no matter how much death damage is done ( freezing, etc )
#define GIB_ALWAYS 2 // always gib ( Houndeye Shock, Barnacle Bite )

// TODO: 4 is used as a magic number in FireBullets(Player) above. Refactor.
#define TRACER_FREQ 4 // Tracers fire every 4 bullets

void FireTargets(const char* targetName, CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

//
// Base Entity.  All entity types derive from this
//
class CBaseEntity
{
public:
	static inline std::shared_ptr<spdlog::logger> Logger;
	static inline std::shared_ptr<spdlog::logger> IOLogger;

	static inline CBaseEntity* World = nullptr;

	// Constructor.  Set engine to use C/C++ callback functions
	// pointers to engine data
	entvars_t* pev; // Don't need to save/restore this pointer, the engine resets it

	// path corners
	CBaseEntity* m_pGoalEnt; // path corner we are heading towards
	CBaseEntity* m_pLink;	 // used for temporary link-list operations.

	/**
	 *	@brief Entity flags sent to the client in ::AddToFullPack
	 */
	byte m_EFlags = 0;

	virtual ~CBaseEntity() {}

	// Common helper functions

	const char* GetClassname() const { return STRING(pev->classname); }

	inline bool ClassnameIs(const char* szClassname) const
	{
		return FStrEq(STRING(pev->classname), szClassname);
	}

	const char* GetGlobalname() const { return STRING(pev->globalname); }

	const char* GetTargetname() const { return STRING(pev->targetname); }

	const char* GetTarget() const { return STRING(pev->target); }

	const char* GetModelName() const { return STRING(pev->model); }

	const char* GetNetname() const { return STRING(pev->netname); }

	const char* GetMessage() const { return STRING(pev->message); }

	int PrecacheModel(const char* s);
	void SetModel(const char* s);

	int PrecacheSound(const char* s);

	void SetSize(const Vector& min, const Vector& max);

	CBaseEntity* GetOwner()
	{
		return GET_PRIVATE<CBaseEntity>(pev->owner);
	}

	void SetOwner(CBaseEntity* owner)
	{
		pev->owner = owner ? owner->edict() : nullptr;
	}

	CBaseEntity* GetGroundEntity()
	{
		return GET_PRIVATE<CBaseEntity>(pev->groundentity);
	}

	void SetGroundEntity(CBaseEntity* entity)
	{
		pev->groundentity = entity ? entity->edict() : nullptr;
	}

	// initialization functions

	/**
	 *	@brief Called immediately after the constructor has finished to complete initialization.
	 *	@details Call the base class version at the start when overriding this function.
	 */
	virtual void OnCreate();

	/**
	 *	@brief Called immediately before the destructor is executed.
	 *	@details Call the base class version at the end when overriding this function.
	 */
	virtual void OnDestroy();

	virtual void Spawn() {}
	virtual void Precache() {}

	/**
	 *	@brief Handles keyvalues in CBaseEntity that must be handled,
	 *	even if an entity does not call the base class version of KeyValue.
	 */
	bool RequiredKeyValue(KeyValueData* pkvd);

	void LoadReplacementFiles();

	virtual bool KeyValue(KeyValueData* pkvd) { return false; }
	virtual bool Save(CSave& save);
	virtual bool Restore(CRestore& restore);
	virtual int ObjectCaps() { return FCAP_ACROSS_TRANSITION; }
	virtual void Activate() {}

	// Setup the object->object collision box (pev->mins / pev->maxs is the object->world collision box)
	virtual void SetObjectCollisionBox();

	// Classify - returns the type of group (i.e, "houndeye", or "human military" so that monsters with different classnames
	// still realize that they are teammates. (overridden for monsters that form groups)
	virtual int Classify() { return CLASS_NONE; }
	virtual void DeathNotice(CBaseEntity* child) {} // monster maker children use this to tell the monster maker that they have died.


	static TYPEDESCRIPTION m_SaveData[];

	virtual void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
	virtual bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType);
	virtual bool TakeHealth(float flHealth, int bitsDamageType);
	virtual void Killed(CBaseEntity* attacker, int iGib);
	virtual int BloodColor() { return DONT_BLEED; }
	virtual void TraceBleed(float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
	virtual bool IsTriggered(CBaseEntity* pActivator) { return true; }
	virtual CBaseMonster* MyMonsterPointer() { return nullptr; }
	virtual CTalkMonster* MyTalkMonsterPointer() { return nullptr; }
	virtual CSquadMonster* MySquadMonsterPointer() { return nullptr; }
	virtual COFSquadTalkMonster* MySquadTalkMonsterPointer() { return nullptr; }
	virtual int GetToggleState() { return TS_AT_TOP; }
	virtual float GetDelay() { return 0; }
	virtual bool IsMoving() { return pev->velocity != g_vecZero; }
	virtual void OverrideReset() {}
	virtual int DamageDecal(int bitsDamageType);
	// This is ONLY used by the node graph to test movement through a door
	virtual void SetToggleState(int state) {}
	virtual void StartSneaking() {}
	virtual void StopSneaking() {}
	virtual bool OnControls(CBaseEntity* controller) { return false; }
	virtual bool IsSneaking() { return false; }
	virtual bool IsAlive() { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	virtual bool IsBSPModel() { return pev->solid == SOLID_BSP || pev->movetype == MOVETYPE_PUSHSTEP; }
	virtual bool ReflectGauss() { return (IsBSPModel() && !pev->takedamage); }
	virtual bool HasTarget(string_t targetname) { return FStrEq(STRING(targetname), GetTarget()); }
	virtual bool IsInWorld();
	virtual bool IsPlayer() { return false; }
	virtual bool IsNetClient() { return false; }
	virtual const char* TeamID() { return ""; }


	//	virtual void	SetActivator( CBaseEntity *pActivator ) {}
	virtual CBaseEntity* GetNextTarget();

	// fundamental callbacks
	BASEPTR m_pfnThink;
	ENTITYFUNCPTR m_pfnTouch;
	USEPTR m_pfnUse;
	ENTITYFUNCPTR m_pfnBlocked;

	virtual void Think()
	{
		if (m_pfnThink)
			(this->*m_pfnThink)();
	}
	virtual void Touch(CBaseEntity* pOther)
	{
		if (m_pfnTouch)
			(this->*m_pfnTouch)(pOther);
	}
	virtual void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
	{
		if (m_pfnUse)
			(this->*m_pfnUse)(pActivator, pCaller, useType, value);
	}
	virtual void Blocked(CBaseEntity* pOther)
	{
		if (m_pfnBlocked)
			(this->*m_pfnBlocked)(pOther);
	}

	void* operator new(size_t stAllocateBlock)
	{
		// Allocate zero-initialized memory.
		auto memory = ::operator new(stAllocateBlock);
		std::memset(memory, 0, stAllocateBlock);
		return memory;
	}

	// Don't call delete on entities directly, tell the engine to delete it instead.
	void operator delete(void* pMem)
	{
		::operator delete(pMem);
	}

	void UpdateOnRemove();

	// common member functions
	void EXPORT SUB_Remove();
	void EXPORT SUB_DoNothing();
	void EXPORT SUB_StartFadeOut();
	void EXPORT SUB_FadeOut();
	void EXPORT SUB_CallUseToggle() { this->Use(this, this, USE_TOGGLE, 0); }
	bool ShouldToggle(USE_TYPE useType, bool currentState);
	void FireBullets(unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int iDamage = 0, CBaseEntity* attacker = nullptr);
	Vector FireBulletsPlayer(unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq = 4, int iDamage = 0, CBaseEntity* attacker = nullptr, int shared_rand = 0);

	virtual CBaseEntity* Respawn() { return nullptr; }

	void SUB_UseTargets(CBaseEntity* pActivator, USE_TYPE useType, float value);
	// Do the bounding boxes of these two intersect?
	bool Intersects(CBaseEntity* pOther);
	void MakeDormant();
	bool IsDormant();
	bool IsLockedByMaster() { return false; }

	static CBaseEntity* Instance(edict_t* pent)
	{
		if (!pent)
			return World;
		return (CBaseEntity*)GET_PRIVATE(pent);
	}

	static CBaseEntity* Instance(entvars_t* pev)
	{
		if (!pev)
			return World;

		return Instance(ENT(pev));
	}

	static CBaseEntity* Instance(CBaseEntity* ent)
	{
		if (!ent)
			return World;

		return ent;
	}

	template <typename T>
	static T* Instance(edict_t* pent)
	{
		if (!pent)
			pent = INDEXENT(0);
		CBaseEntity* pEnt = (CBaseEntity*)GET_PRIVATE(pent);
		return static_cast<T*>(pEnt);
	}

	template <typename T>
	static T* Instance(entvars_t* pev) { return Instance<T>(ENT(pev)); }

	// Ugly technique to override base member functions
	// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a
	// member function of a base class.  static_cast is a sleezy way around that problem.

private:
	// Ugly code to lookup all functions to make sure they are exported when set.
	void FunctionCheck(const void* pFunction, const char* name)
	{
		if (pFunction && !NAME_FOR_FUNCTION((uint32)pFunction))
			CBaseEntity::Logger->error("No EXPORT: {}:{} ({:#08X})", GetClassname(), name, (uint32)pFunction);
	}

	template <typename T, typename Dest, typename Source>
	Dest FunctionSet(Dest& pointer, Source func, const char* name)
	{
#ifdef _DEBUG
		if (nullptr == dynamic_cast<T*>(this))
		{
			// If this happens, it means you tried to set a function from a class that this entity doesn't inherit from.
			CBaseEntity::Logger->error("Trying to set function that is not a member of this entity's class hierarchy: {}:{}",
				GetClassname(), name);
		}
#endif

		pointer = static_cast<Dest>(func);

#ifdef _DEBUG
		// Pointer to member functions store the function address as a pointer at the start of the variable,
		// so this extracts that and turns it into an address we can use.
		// This only works if the function is non-virtual.
		// Note that this is incredibly ugly and should be changed when possible to not rely on implementation details.
		const void* address = reinterpret_cast<const void*>(*reinterpret_cast<const int*>(&pointer));

		FunctionCheck(address, name);
#endif

		return pointer;
	}

public:
	template <typename T>
	BASEPTR ThinkSet(TBASEPTR<T> func, const char* name)
	{
		return FunctionSet<T>(m_pfnThink, func, name);
	}

	template <typename T>
	ENTITYFUNCPTR TouchSet(TENTITYFUNCPTR<T> func, const char* name)
	{
		return FunctionSet<T>(m_pfnTouch, func, name);
	}

	template <typename T>
	USEPTR UseSet(TUSEPTR<T> func, const char* name)
	{
		return FunctionSet<T>(m_pfnUse, func, name);
	}

	template <typename T>
	ENTITYFUNCPTR BlockedSet(TENTITYFUNCPTR<T> func, const char* name)
	{
		return FunctionSet<T>(m_pfnBlocked, func, name);
	}

	// Overloads to catch explicit setting to null.
	BASEPTR ThinkSet(std::nullptr_t, const char* name)
	{
		m_pfnThink = nullptr;
		return m_pfnThink;
	}

	ENTITYFUNCPTR TouchSet(std::nullptr_t, const char* name)
	{
		m_pfnTouch = nullptr;
		return m_pfnTouch;
	}

	USEPTR UseSet(std::nullptr_t, const char* name)
	{
		m_pfnUse = nullptr;
		return m_pfnUse;
	}

	ENTITYFUNCPTR BlockedSet(std::nullptr_t, const char* name)
	{
		m_pfnBlocked = nullptr;
		return m_pfnBlocked;
	}

	// virtual functions used by a few classes

	// used by monsters that are created by the MonsterMaker
	virtual void UpdateOwner() {}


	//
	static CBaseEntity* Create(const char* szName, const Vector& vecOrigin, const Vector& vecAngles, edict_t* pentOwner = nullptr);

	virtual bool FBecomeProne() { return false; }
	edict_t* edict() { return ENT(pev); }
	int entindex() { return ENTINDEX(edict()); }

	virtual Vector Center() { return (pev->absmax + pev->absmin) * 0.5; } // center point of entity
	virtual Vector EyePosition() { return pev->origin + pev->view_ofs; }  // position of eyes
	virtual Vector EarPosition() { return pev->origin + pev->view_ofs; }  // position of ears
	virtual Vector BodyTarget(const Vector& posSrc) { return Center(); }  // position to shoot at

	virtual int Illumination() { return GETENTITYILLUM(ENT(pev)); }

	virtual bool FVisible(CBaseEntity* pEntity);
	virtual bool FVisible(const Vector& vecOrigin);

	static float GetSkillFloat(std::string_view name)
	{
		return g_Skill.GetValue(name);
	}

	// Sound playback.
	void EmitSound(int channel, const char* sample, float volume, float attenuation);
	void EmitSoundDyn(int channel, const char* sample, float volume, float attenuation, int flags, int pitch);
	void EmitAmbientSound(const Vector& vecOrigin, const char* samp, float vol, float attenuation, int fFlags, int pitch);
	void StopSound(int channel, const char* sample);

	string_t m_ModelReplacementFileName;
	string_t m_SoundReplacementFileName;
	string_t m_SentenceReplacementFileName;

	const ReplacementMap* m_ModelReplacement{};
	const ReplacementMap* m_SoundReplacement{};
	const ReplacementMap* m_SentenceReplacement{};

	Vector m_CustomHullMin{vec3_origin};
	Vector m_CustomHullMax{vec3_origin};
	bool m_HasCustomHullMin{false};
	bool m_HasCustomHullMax{false};

	// We use this variables to store each ammo count.
	int ammo_9mm;
	int ammo_357;
	int ammo_bolts;
	int ammo_buckshot;
	int ammo_rockets;
	int ammo_uranium;
	int ammo_hornets;
	int ammo_argrens;
	int ammo_spores;
	int ammo_762;
	// Special stuff for grenades and satchels.
	float m_flStartThrow;
	float m_flReleaseThrow;
	int m_chargeReady;
	int m_fInAttack;

	int m_fireState;
};

inline bool FNullEnt(CBaseEntity* ent) { return (ent == nullptr) || FNullEnt(ent->edict()); }

#define SetThink(a) ThinkSet(a, #a)
#define SetTouch(a) TouchSet(a, #a)
#define SetUse(a) UseSet(a, #a)
#define SetBlocked(a) BlockedSet(a, #a)
