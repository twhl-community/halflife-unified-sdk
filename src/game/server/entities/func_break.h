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

enum Explosions
{
	expRandom,
	expDirected
};
enum Materials
{
	matGlass = 0,
	matWood,
	matMetal,
	matFlesh,
	matCinderBlock,
	matCeilingTile,
	matComputer,
	matUnbreakableGlass,
	matRocks,
	matNone,
	matLastMaterial
};

#define NUM_SHARDS 6 // this many shards spawned when breakable objects break;

/**
*	@brief bmodel that breaks into pieces after taking damage
*/
class CBreakable : public CBaseDelay
{
	DECLARE_CLASS(CBreakable, CBaseDelay);
	DECLARE_DATAMAP();

public:
	// basic functions
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT BreakTouch(CBaseEntity* pOther);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	/**
	*	@brief play shard sound when func_breakable takes damage.
	*	the more damage, the louder the shard sound.
	*/
	void DamageSound();

	/**
	*	@brief Special takedamage for func_breakable.
	*	Allows us to make exceptions that are breakable-specific
	*/
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;
	// To spark when hit
	void TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	bool IsBreakable();

	int DamageDecal(int bitsDamageType) override;

	void EXPORT Die();
	int ObjectCaps() override { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	inline bool Explodable() { return ExplosionMagnitude() > 0; }
	inline int ExplosionMagnitude() { return pev->impulse; }
	inline void ExplosionSetMagnitude(int magnitude) { pev->impulse = magnitude; }

	static void MaterialSoundPrecache(CBaseEntity* self, Materials precacheMaterial);
	static void MaterialSoundRandom(CBaseEntity* self, Materials soundMaterial, float volume);
	static const char** MaterialSoundList(Materials precacheMaterial, int& soundCount);

	static const char* pSoundsWood[];
	static const char* pSoundsFlesh[];
	static const char* pSoundsGlass[];
	static const char* pSoundsMetal[];
	static const char* pSoundsConcrete[];
	static const char* pSpawnObjects[];

	Materials m_Material;
	Explosions m_Explosion;
	int m_idShard;
	float m_angle;
	string_t m_iszGibModel;
	string_t m_iszSpawnObject;
};
