/***
 *
 *	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 *   This source code contains proprietary and confidential information of
 *   Valve LLC and its suppliers.  Access to this code is restricted to
 *   persons who have executed a written SDK license with Valve.  Any access,
 *   use or distribution of this code by or to any unlicensed person is illegal.
 *
 ****/

#pragma once

#define ZOMBIE_AE_ATTACK_RIGHT 0x01
#define ZOMBIE_AE_ATTACK_LEFT 0x02
#define ZOMBIE_AE_ATTACK_BOTH 0x03

#define ZOMBIE_FLINCH_DELAY 2 // at most one flinch every n secs

class CZombie : public CBaseMonster
{
public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	int IgnoreConditions() override;

	float m_flNextFlinch = 0;

	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void AttackSound();

	static constexpr const char* pAttackSounds[] =
		{
			"zombie/zo_attack1.wav",
			"zombie/zo_attack2.wav",
		};

	static constexpr const char* pIdleSounds[] =
		{
			"zombie/zo_idle1.wav",
			"zombie/zo_idle2.wav",
			"zombie/zo_idle3.wav",
			"zombie/zo_idle4.wav",
		};

	static constexpr const char* pAlertSounds[] =
		{
			"zombie/zo_alert10.wav",
			"zombie/zo_alert20.wav",
			"zombie/zo_alert30.wav",
		};

	static constexpr const char* pPainSounds[] =
		{
			"zombie/zo_pain1.wav",
			"zombie/zo_pain2.wav",
		};

	static constexpr const char* pAttackHitSounds[] =
		{
			"zombie/claw_strike1.wav",
			"zombie/claw_strike2.wav",
			"zombie/claw_strike3.wav",
		};

	static constexpr const char* pAttackMissSounds[] =
		{
			"zombie/claw_miss1.wav",
			"zombie/claw_miss2.wav",
		};

	// No range attacks
	bool CheckRangeAttack1(float flDot, float flDist) override { return false; }
	bool CheckRangeAttack2(float flDot, float flDist) override { return false; }
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

protected:
	virtual float GetOneSlashDamage();
	virtual float GetBothSlashDamage();

	void ZombieSlashAttack(float damage, const Vector& punchAngle, const Vector& velocity, bool playAttackSound = true);

	// Take 30% damage from bullets by default
	virtual float GetBulletDamageFraction() const { return 0.3f; }
};
