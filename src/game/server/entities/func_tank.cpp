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
#include "explode.h"
#include "UserMessages.h"

#define SF_TANK_ACTIVE 0x0001
#define SF_TANK_PLAYER 0x0002
#define SF_TANK_HUMANS 0x0004
#define SF_TANK_ALIENS 0x0008
#define SF_TANK_LINEOFSIGHT 0x0010
#define SF_TANK_CANCONTROL 0x0020
#define SF_TANK_SOUNDON 0x8000

enum TANKBULLET
{
	TANK_BULLET_NONE = 0,
	TANK_BULLET_9MM = 1,
	TANK_BULLET_MP5 = 2,
	TANK_BULLET_12MM = 3,
};

//			Custom damage
//			env_laser (duration is 0.5 rate of fire)
//			rockets
//			explosion?

class CFuncTank : public CBaseEntity
{
	DECLARE_CLASS(CFuncTank, CBaseEntity);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void Think() override;
	void PostRestore() override;
	void TrackTarget();

	/**
	 *	@brief Fire targets and spawn sprites
	 */
	virtual void Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker);
	virtual Vector UpdateTargetPosition(CBaseEntity* pTarget)
	{
		return pTarget->BodyTarget(pev->origin);
	}

	void StartRotSound();
	void StopRotSound();

	// Bmodels don't go across transitions
	int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	inline bool IsActive() { return (pev->spawnflags & SF_TANK_ACTIVE) != 0; }
	inline void TankActivate()
	{
		pev->spawnflags |= SF_TANK_ACTIVE;
		pev->nextthink = pev->ltime + 0.1;
		m_fireLast = 0;
	}
	inline void TankDeactivate()
	{
		pev->spawnflags &= ~SF_TANK_ACTIVE;
		m_fireLast = 0;
		StopRotSound();
	}
	inline bool CanFire() { return (gpGlobals->time - m_lastSightTime) <= m_persist; }
	bool InRange(float range);

	// Acquire a target.
	CBaseEntity* FindTarget(CBaseEntity* pvsPlayer);

	void TankTrace(const Vector& vecStart, const Vector& vecForward, const Vector& vecSpread, TraceResult& tr);

	Vector BarrelPosition()
	{
		Vector forward, right, up;
		AngleVectors(pev->angles, forward, right, up);
		return pev->origin + (forward * m_barrelPos.x) + (right * m_barrelPos.y) + (up * m_barrelPos.z);
	}

	/**
	 *	@brief If barrel is offset, add in additional rotation
	 */
	void AdjustAnglesForBarrel(Vector& angles, float distance);

	bool OnControls(CBaseEntity* controller) override;
	bool StartControl(CBasePlayer* pController);
	void StopControl();

	/**
	 *	@brief Called each frame by the player's ItemPostFrame
	 */
	void ControllerPostFrame();

	void UpdateOnRemove() override;

private:
	void CreateTargetLaser();

protected:
	CBasePlayer* m_pController;
	float m_flNextAttack;
	Vector m_vecControllerUsePos;

	float m_yawCenter;	  // "Center" yaw
	float m_yawRate;	  // Max turn rate to track targets
	float m_yawRange;	  // Range of turning motion (one-sided: 30 is +/- 30 degress from center)
						  // Zero is full rotation
	float m_yawTolerance; // Tolerance angle

	float m_pitchCenter;	// "Center" pitch
	float m_pitchRate;		// Max turn rate on pitch
	float m_pitchRange;		// Range of pitch motion as above
	float m_pitchTolerance; // Tolerance angle

	float m_fireLast;	   // Last time I fired
	float m_fireRate;	   // How many rounds/second
	float m_lastSightTime; // Last time I saw target
	float m_persist;	   // Persistence of firing (how long do I shoot when I can't see)
	float m_minRange;	   // Minimum range to aim/track
	float m_maxRange;	   // Max range to aim/track

	Vector m_barrelPos;	 // Length of the freakin barrel
	float m_spriteScale; // Scale of any sprites we shoot
	string_t m_iszSpriteSmoke;
	string_t m_iszSpriteFlash;
	TANKBULLET m_bulletType; // Bullet type
	int m_iBulletDamage;	 // 0 means use Bullet type's default damage

	Vector m_sightOrigin; // Last sight of target
	int m_spread;		  // firing spread

	EHANDLE m_hEnemy;

	// 0 - player only
	// 1 - all targets allied to player
	int m_iEnemyType;

	bool m_EnableTargetLaser = false;
	bool m_RecreateTargetLaser = false; // Don't save; used to restore the client side laser
	string_t m_TargetLaserSpriteName = MAKE_STRING(g_pModelNameLaser);
	int m_TargetLaserModelIndex = 0;
	int m_TargetLaserWidth = 1;
	Vector m_TargetLaserColor{255, 0, 0};
};

BEGIN_DATAMAP(CFuncTank)
DEFINE_FIELD(m_yawCenter, FIELD_FLOAT),
	DEFINE_FIELD(m_yawRate, FIELD_FLOAT),
	DEFINE_FIELD(m_yawRange, FIELD_FLOAT),
	DEFINE_FIELD(m_yawTolerance, FIELD_FLOAT),
	DEFINE_FIELD(m_pitchCenter, FIELD_FLOAT),
	DEFINE_FIELD(m_pitchRate, FIELD_FLOAT),
	DEFINE_FIELD(m_pitchRange, FIELD_FLOAT),
	DEFINE_FIELD(m_pitchTolerance, FIELD_FLOAT),
	DEFINE_FIELD(m_fireLast, FIELD_TIME),
	DEFINE_FIELD(m_fireRate, FIELD_FLOAT),
	DEFINE_FIELD(m_lastSightTime, FIELD_TIME),
	DEFINE_FIELD(m_persist, FIELD_FLOAT),
	DEFINE_FIELD(m_minRange, FIELD_FLOAT),
	DEFINE_FIELD(m_maxRange, FIELD_FLOAT),
	DEFINE_FIELD(m_barrelPos, FIELD_VECTOR),
	DEFINE_FIELD(m_spriteScale, FIELD_FLOAT),
	DEFINE_FIELD(m_iszSpriteSmoke, FIELD_STRING),
	DEFINE_FIELD(m_iszSpriteFlash, FIELD_STRING),
	DEFINE_FIELD(m_bulletType, FIELD_INTEGER),
	DEFINE_FIELD(m_sightOrigin, FIELD_VECTOR),
	DEFINE_FIELD(m_spread, FIELD_INTEGER),
	DEFINE_FIELD(m_pController, FIELD_CLASSPTR),
	DEFINE_FIELD(m_vecControllerUsePos, FIELD_VECTOR),
	DEFINE_FIELD(m_flNextAttack, FIELD_TIME),
	DEFINE_FIELD(m_iBulletDamage, FIELD_INTEGER),
	DEFINE_FIELD(m_hEnemy, FIELD_EHANDLE),
	DEFINE_FIELD(m_iEnemyType, FIELD_INTEGER),
	DEFINE_FIELD(m_EnableTargetLaser, FIELD_BOOLEAN),
	DEFINE_FIELD(m_TargetLaserSpriteName, FIELD_STRING),
	DEFINE_FIELD(m_TargetLaserWidth, FIELD_INTEGER),
	DEFINE_FIELD(m_TargetLaserColor, FIELD_VECTOR),
	END_DATAMAP();

static Vector gTankSpread[] =
	{
		Vector(0, 0, 0),			 // perfect
		Vector(0.025, 0.025, 0.025), // small cone
		Vector(0.05, 0.05, 0.05),	 // medium cone
		Vector(0.1, 0.1, 0.1),		 // large cone
		Vector(0.25, 0.25, 0.25),	 // extra-large cone
};

#define MAX_FIRING_SPREADS std::size(gTankSpread)

void CFuncTank::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_PUSH; // so it doesn't get pushed by anything
	pev->solid = SOLID_BSP;
	SetModel(STRING(pev->model));

	m_yawCenter = pev->angles.y;
	m_pitchCenter = pev->angles.x;

	if (IsActive())
		pev->nextthink = pev->ltime + 1.0;

	m_sightOrigin = BarrelPosition(); // Point at the end of the barrel

	if (m_fireRate <= 0)
		m_fireRate = 1;
	// TODO: needs to be >=
	if (static_cast<std::size_t>(m_spread) > MAX_FIRING_SPREADS)
		m_spread = 0;

	pev->oldorigin = pev->origin;
}

void CFuncTank::Precache()
{
	if (!FStringNull(m_iszSpriteSmoke))
		PrecacheModel(STRING(m_iszSpriteSmoke));
	if (!FStringNull(m_iszSpriteFlash))
		PrecacheModel(STRING(m_iszSpriteFlash));

	if (!FStringNull(pev->noise))
		PrecacheSound(STRING(pev->noise));

	m_TargetLaserModelIndex = PrecacheModel(STRING(m_TargetLaserSpriteName));
}

bool CFuncTank::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "yawrate"))
	{
		m_yawRate = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "yawrange"))
	{
		m_yawRange = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "yawtolerance"))
	{
		m_yawTolerance = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchrange"))
	{
		m_pitchRange = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchrate"))
	{
		m_pitchRate = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchtolerance"))
	{
		m_pitchTolerance = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "firerate"))
	{
		m_fireRate = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "barrel"))
	{
		m_barrelPos.x = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "barrely"))
	{
		m_barrelPos.y = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "barrelz"))
	{
		m_barrelPos.z = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "spritescale"))
	{
		m_spriteScale = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "spritesmoke"))
	{
		m_iszSpriteSmoke = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "spriteflash"))
	{
		m_iszSpriteFlash = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "rotatesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "persistence"))
	{
		m_persist = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "bullet"))
	{
		m_bulletType = (TANKBULLET)atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "bullet_damage"))
	{
		m_iBulletDamage = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "firespread"))
	{
		m_spread = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "minRange"))
	{
		m_minRange = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "maxRange"))
	{
		m_maxRange = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "enemytype"))
	{
		m_iEnemyType = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "enable_target_laser"))
	{
		m_EnableTargetLaser = atoi(pkvd->szValue) != 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "target_laser_sprite"))
	{
		m_TargetLaserSpriteName = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "target_laser_width"))
	{
		m_TargetLaserWidth = std::clamp(atoi(pkvd->szValue), 0, 255);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "target_laser_color"))
	{
		UTIL_StringToVector(m_TargetLaserColor, pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

bool CFuncTank::OnControls(CBaseEntity* controller)
{
	if ((pev->spawnflags & SF_TANK_CANCONTROL) == 0)
		return false;

	if ((m_vecControllerUsePos - controller->pev->origin).Length() < 30)
		return true;

	return false;
}

bool CFuncTank::StartControl(CBasePlayer* pController)
{
	if (m_pController != nullptr)
		return false;

	// Team only or disabled?
	if (!FStringNull(m_sMaster))
	{
		if (!UTIL_IsMasterTriggered(m_sMaster, pController, m_UseLocked))
			return false;
	}

	CBaseEntity::Logger->debug("using TANK!");

	m_pController = pController;
	if (m_pController->m_pActiveWeapon)
	{
		m_pController->m_pActiveWeapon->Holster();
		m_pController->pev->weaponmodel = string_t::Null;
		m_pController->pev->viewmodel = string_t::Null;
	}

	m_pController->m_iHideHUD |= HIDEHUD_WEAPONS;
	m_vecControllerUsePos = m_pController->pev->origin;

	pev->nextthink = pev->ltime + 0.1;

	CreateTargetLaser();

	return true;
}

void CFuncTank::StopControl()
{
	if (!m_pController)
		return;

	m_pController->EquipWeapon();

	CBaseEntity::Logger->debug("stopped using TANK");

	m_pController->m_iHideHUD &= ~HIDEHUD_WEAPONS;

	MESSAGE_BEGIN(MSG_ONE, gmsgTgtLaser, nullptr, m_pController);
	WRITE_BYTE(0);
	MESSAGE_END();

	pev->nextthink = 0;
	m_pController = nullptr;

	if (IsActive())
		pev->nextthink = pev->ltime + 1.0;
}

void CFuncTank::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
	StopControl();
}

void CFuncTank::CreateTargetLaser()
{
	if (!m_EnableTargetLaser)
	{
		return;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgTgtLaser, nullptr, m_pController);
	WRITE_BYTE(1);
	WRITE_SHORT(entindex());
	WRITE_SHORT(m_TargetLaserModelIndex);
	WRITE_BYTE(m_TargetLaserWidth);
	WRITE_BYTE(m_TargetLaserColor.x);
	WRITE_BYTE(m_TargetLaserColor.y);
	WRITE_BYTE(m_TargetLaserColor.z);
	MESSAGE_END();
}

void CFuncTank::ControllerPostFrame()
{
	ASSERT(m_pController != nullptr);

	if (gpGlobals->time < m_flNextAttack)
		return;

	if ((m_pController->pev->button & IN_ATTACK) != 0)
	{
		Vector vecForward;
		AngleVectors(pev->angles, &vecForward, nullptr, nullptr);

		m_fireLast = gpGlobals->time - (1 / m_fireRate) - 0.01; // to make sure the gun doesn't fire too many bullets

		Fire(BarrelPosition(), vecForward, m_pController);

		// HACKHACK -- make some noise (that the AI can hear)
		if (m_pController && m_pController->IsPlayer())
			m_pController->m_iWeaponVolume = LOUD_GUN_VOLUME;

		m_flNextAttack = gpGlobals->time + (1 / m_fireRate);
	}
}

void CFuncTank::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if ((pev->spawnflags & SF_TANK_CANCONTROL) != 0)
	{ // player controlled turret

		auto player = ToBasePlayer(pActivator);

		if (!player)
			return;

		if (value == 2 && useType == USE_SET)
		{
			ControllerPostFrame();
		}
		else if (!m_pController && useType != USE_OFF)
		{
			player->m_pTank = this;
			StartControl(player);
		}
		else
		{
			StopControl();
		}
	}
	else
	{
		if (!ShouldToggle(useType, IsActive()))
			return;

		if (IsActive())
			TankDeactivate();
		else
			TankActivate();
	}
}

CBaseEntity* CFuncTank::FindTarget(CBaseEntity* pvsPlayer)
{
	auto pPlayerTarget = pvsPlayer;

	if (!pPlayerTarget)
		return pPlayerTarget;

	m_pLink = nullptr;
	auto flIdealDist = m_maxRange;
	CBaseEntity* pIdealTarget = nullptr;

	if (pPlayerTarget->IsAlive() && !FBitSet(pPlayerTarget->pev->flags, FL_NOTARGET))
	{
		const auto distance = (pPlayerTarget->pev->origin - pev->origin).Length2D();

		if (InRange(distance))
		{
			TraceResult tr;
			UTIL_TraceLine(BarrelPosition(), pPlayerTarget->pev->origin + pPlayerTarget->pev->view_ofs, dont_ignore_monsters, edict(), &tr);

			if (tr.pHit == pPlayerTarget->edict())
			{
				if (0 == m_iEnemyType)
					return pPlayerTarget;

				flIdealDist = distance;
				pIdealTarget = pPlayerTarget;
			}
		}
	}

	const float maxDist = m_maxRange > 0 ? m_maxRange : 2048;

	Vector size(maxDist, maxDist, maxDist);

	CBaseEntity* pList[100];
	const auto count = UTIL_EntitiesInBox(pList, std::size(pList), pev->origin - size, pev->origin + size, FL_MONSTER | FL_CLIENT);

	for (auto i = 0; i < count; ++i)
	{
		auto pEntity = pList[i];

		if (this == pEntity)
			continue;

		if ((pEntity->pev->spawnflags & SF_MONSTER_PRISONER) != 0)
			continue;

		if (pEntity->pev->health <= 0)
			continue;

		auto pMonster = pEntity->MyMonsterPointer();

		if (!pMonster)
			continue;

		if (pMonster->IRelationship(pPlayerTarget) != Relationship::Ally)
			continue;

		if ((pEntity->pev->flags & FL_NOTARGET) != 0)
			continue;

		if (!FVisible(pEntity))
			continue;

		if (pEntity->IsPlayer() && (pev->spawnflags & SF_TANK_ACTIVE) != 0)
		{
			if (pMonster->FInViewCone(this))
			{
				pev->spawnflags &= ~SF_TANK_ACTIVE;
			}
		}

		pEntity->m_pLink = m_pLink;
		m_pLink = pEntity;
	}

	for (auto pEntity = m_pLink; pEntity; pEntity = pEntity->m_pLink)
	{
		const auto distance = (pEntity->pev->origin - pev->origin).Length();

		if (InRange(distance) && (!pIdealTarget || flIdealDist > distance))
		{
			TraceResult tr;
			UTIL_TraceLine(BarrelPosition(), pEntity->pev->origin + pEntity->pev->view_ofs, dont_ignore_monsters, edict(), &tr);

			if ((pev->spawnflags & SF_TANK_LINEOFSIGHT) != 0)
			{
				if (tr.pHit == pEntity->edict())
				{
					flIdealDist = distance;
				}
				if (tr.pHit == pEntity->edict())
					pIdealTarget = pEntity;
			}
			else
			{
				flIdealDist = distance;
				pIdealTarget = pEntity;
			}
		}
	}

	return pIdealTarget;
}

bool CFuncTank::InRange(float range)
{
	if (range < m_minRange)
		return false;
	if (m_maxRange > 0 && range > m_maxRange)
		return false;

	return true;
}

void CFuncTank::Think()
{
	if (m_RecreateTargetLaser)
	{
		if (m_pController && m_pController->m_HasActivated)
		{
			m_RecreateTargetLaser = false;
			CreateTargetLaser();
		}
	}

	pev->avelocity = g_vecZero;
	TrackTarget();

	if (fabs(pev->avelocity.x) > 1 || fabs(pev->avelocity.y) > 1)
		StartRotSound();
	else
		StopRotSound();
}

void CFuncTank::PostRestore()
{
	BaseClass::PostRestore();

	if (m_EnableTargetLaser)
	{
		m_RecreateTargetLaser = true;
	}
}

void CFuncTank::TrackTarget()
{
	TraceResult tr;
	CBasePlayer* pPlayer = UTIL_FindClientInPVS(this);
	bool updateTime = false, lineOfSight;
	Vector angles, direction, targetPosition;
	CBaseEntity* pTarget = nullptr;

	const Vector barrelEnd = BarrelPosition();

	// Get a position to aim for
	if (m_pController)
	{
		// Tanks attempt to mirror the player's angles
		angles = m_pController->pev->v_angle;
		angles[0] = 0 - angles[0];
		pev->nextthink = pev->ltime + 0.05;
	}
	else
	{
		if (IsActive())
			pev->nextthink = pev->ltime + 0.1;
		else
			return;

		if (!pPlayer)
		{
			if (IsActive())
				pev->nextthink = pev->ltime + 2; // Wait 2 secs
			return;
		}

		// Keep tracking the same target unless the target is in cover and a better one is around.
		pTarget = m_hEnemy;

		// Forget targets that enable notarget.
		if (pTarget && FBitSet(pTarget->pev->flags, FL_NOTARGET))
		{
			pTarget = nullptr;
			m_hEnemy = nullptr;
		}

		if (pTarget)
		{
			targetPosition = pTarget->pev->origin + pTarget->pev->view_ofs;
			UTIL_TraceLine(barrelEnd, targetPosition, dont_ignore_monsters, edict(), &tr);

			if (tr.flFraction != 1.0 && tr.pHit != pTarget->edict())
			{
				pTarget = nullptr;
			}
		}

		if (!pTarget || !pTarget->IsAlive())
		{
			pTarget = FindTarget(pPlayer);

			// Keep shooting at our last target.
			if (!pTarget)
			{
				pTarget = m_hEnemy;
			}

			// Nothing to shoot at.
			if (!pTarget)
			{
				return;
			}

			m_hEnemy = pTarget;
		}

		// Calculate angle needed to aim at target
		targetPosition = pTarget->pev->origin + pTarget->pev->view_ofs;
		float range = (targetPosition - barrelEnd).Length();

		if (!InRange(range))
			return;

		UTIL_TraceLine(barrelEnd, targetPosition, dont_ignore_monsters, edict(), &tr);

		lineOfSight = false;
		// No line of sight, don't track
		if (tr.flFraction == 1.0 || tr.pHit == pTarget->edict())
		{
			lineOfSight = true;

			if (InRange(range) && pTarget->IsAlive())
			{
				updateTime = true;
				m_sightOrigin = UpdateTargetPosition(pTarget);
			}
		}

		// Track sight origin

		// !!! I'm not sure what i changed
		direction = m_sightOrigin - pev->origin;
		//		direction = m_sightOrigin - barrelEnd;
		angles = UTIL_VecToAngles(direction);

		// Calculate the additional rotation to point the end of the barrel at the target (not the gun's center)
		AdjustAnglesForBarrel(angles, direction.Length());
	}

	angles.x = -angles.x;

	// Force the angles to be relative to the center position
	angles.y = m_yawCenter + UTIL_AngleDistance(angles.y, m_yawCenter);
	angles.x = m_pitchCenter + UTIL_AngleDistance(angles.x, m_pitchCenter);

	// Limit against range in y
	if (angles.y > m_yawCenter + m_yawRange)
	{
		angles.y = m_yawCenter + m_yawRange;
		updateTime = false; // Don't update if you saw the player, but out of range
	}
	else if (angles.y < (m_yawCenter - m_yawRange))
	{
		angles.y = (m_yawCenter - m_yawRange);
		updateTime = false; // Don't update if you saw the player, but out of range
	}

	if (updateTime)
		m_lastSightTime = gpGlobals->time;

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance(angles.y, pev->angles.y);
	pev->avelocity.y = distY * 10;
	if (pev->avelocity.y > m_yawRate)
		pev->avelocity.y = m_yawRate;
	else if (pev->avelocity.y < -m_yawRate)
		pev->avelocity.y = -m_yawRate;

	// Limit against range in x
	if (angles.x > m_pitchCenter + m_pitchRange)
		angles.x = m_pitchCenter + m_pitchRange;
	else if (angles.x < m_pitchCenter - m_pitchRange)
		angles.x = m_pitchCenter - m_pitchRange;

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance(angles.x, pev->angles.x);
	pev->avelocity.x = distX * 10;

	if (pev->avelocity.x > m_pitchRate)
		pev->avelocity.x = m_pitchRate;
	else if (pev->avelocity.x < -m_pitchRate)
		pev->avelocity.x = -m_pitchRate;

	if (m_pController)
		return;

	if (CanFire() && ((fabs(distX) < m_pitchTolerance && fabs(distY) < m_yawTolerance) || (pev->spawnflags & SF_TANK_LINEOFSIGHT) != 0))
	{
		bool fire = false;
		Vector forward;
		AngleVectors(pev->angles, &forward, nullptr, nullptr);

		if ((pev->spawnflags & SF_TANK_LINEOFSIGHT) != 0)
		{
			float length = direction.Length();
			UTIL_TraceLine(barrelEnd, barrelEnd + forward * length, dont_ignore_monsters, edict(), &tr);
			if (tr.pHit == pTarget->edict())
				fire = true;
		}
		else
			fire = true;

		if (fire)
		{
			Fire(BarrelPosition(), forward, this);
		}
		else
			m_fireLast = 0;
	}
	else
		m_fireLast = 0;
}

void CFuncTank::AdjustAnglesForBarrel(Vector& angles, float distance)
{
	if (m_barrelPos.y != 0 || m_barrelPos.z != 0)
	{
		distance -= m_barrelPos.z;
		const float d2 = distance * distance;
		if (0 != m_barrelPos.y)
		{
			const float r2 = m_barrelPos.y * m_barrelPos.y;

			if (d2 > r2)
			{
				angles.y += (180.0 / PI) * atan2(m_barrelPos.y, sqrt(d2 - r2));
			}
		}
		if (0 != m_barrelPos.z)
		{
			const float r2 = m_barrelPos.z * m_barrelPos.z;

			if (d2 > r2)
			{
				angles.x += (180.0 / PI) * atan2(-m_barrelPos.z, sqrt(d2 - r2));
			}
		}
	}
}

void CFuncTank::Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker)
{
	if (m_fireLast != 0)
	{
		if (!FStringNull(m_iszSpriteSmoke))
		{
			CSprite* pSprite = CSprite::SpriteCreate(STRING(m_iszSpriteSmoke), barrelEnd, true);
			pSprite->AnimateAndDie(RANDOM_FLOAT(15.0, 20.0));
			pSprite->SetTransparency(kRenderTransAlpha, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, 255, kRenderFxNone);
			pSprite->pev->velocity.z = RANDOM_FLOAT(40, 80);
			pSprite->SetScale(m_spriteScale);
		}
		if (!FStringNull(m_iszSpriteFlash))
		{
			CSprite* pSprite = CSprite::SpriteCreate(STRING(m_iszSpriteFlash), barrelEnd, true);
			pSprite->AnimateAndDie(60);
			pSprite->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation);
			pSprite->SetScale(m_spriteScale);

			// Hack Hack, make it stick around for at least 100 ms.
			pSprite->pev->nextthink += 0.1;
		}
		SUB_UseTargets(this, USE_TOGGLE, 0);
	}
	m_fireLast = gpGlobals->time;
}

void CFuncTank::TankTrace(const Vector& vecStart, const Vector& vecForward, const Vector& vecSpread, TraceResult& tr)
{
	// get circular gaussian spread
	float x, y, z;
	do
	{
		x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
		z = x * x + y * y;
	} while (z > 1);
	Vector vecDir = vecForward +
					x * vecSpread.x * gpGlobals->v_right +
					y * vecSpread.y * gpGlobals->v_up;
	Vector vecEnd;

	vecEnd = vecStart + vecDir * 4096;
	UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, edict(), &tr);
}

void CFuncTank::StartRotSound()
{
	if (FStringNull(pev->noise) || (pev->spawnflags & SF_TANK_SOUNDON) != 0)
		return;
	pev->spawnflags |= SF_TANK_SOUNDON;
	EmitSound(CHAN_STATIC, STRING(pev->noise), 0.85, ATTN_NORM);
}

void CFuncTank::StopRotSound()
{
	if ((pev->spawnflags & SF_TANK_SOUNDON) != 0)
		StopSound(CHAN_STATIC, STRING(pev->noise));
	pev->spawnflags &= ~SF_TANK_SOUNDON;
}

class CFuncTankGun : public CFuncTank
{
public:
	void Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker) override;
};

LINK_ENTITY_TO_CLASS(func_tank, CFuncTankGun);

void CFuncTankGun::Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker)
{
	int i;

	if (m_fireLast != 0)
	{
		// FireBullets needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors(pev->angles);

		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		if (bulletCount > 0)
		{
			for (i = 0; i < bulletCount; i++)
			{
				switch (m_bulletType)
				{
				case TANK_BULLET_9MM:
					FireBullets(1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_9MM, 1, m_iBulletDamage, attacker);
					break;

				case TANK_BULLET_MP5:
					FireBullets(1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_MP5, 1, m_iBulletDamage, attacker);
					break;

				case TANK_BULLET_12MM:
					FireBullets(1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_12MM, 1, m_iBulletDamage, attacker);
					break;

				default:
				case TANK_BULLET_NONE:
					break;
				}
			}
			CFuncTank::Fire(barrelEnd, forward, attacker);
		}
	}
	else
		CFuncTank::Fire(barrelEnd, forward, attacker);
}

class CFuncTankLaser : public CFuncTank
{
	DECLARE_CLASS(CFuncTankLaser, CFuncTank);
	DECLARE_DATAMAP();

public:
	void Activate() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker) override;
	void Think() override;
	CLaser* GetLaser();

private:
	CLaser* m_pLaser;
	float m_laserTime;
};

LINK_ENTITY_TO_CLASS(func_tanklaser, CFuncTankLaser);

BEGIN_DATAMAP(CFuncTankLaser)
DEFINE_FIELD(m_pLaser, FIELD_CLASSPTR),
	DEFINE_FIELD(m_laserTime, FIELD_TIME),
	END_DATAMAP();

void CFuncTankLaser::Activate()
{
	if (!GetLaser())
	{
		UTIL_Remove(this);
		CBaseEntity::Logger->error("Laser tank with no env_laser!");
	}
	else
	{
		m_pLaser->TurnOff();
	}
}

bool CFuncTankLaser::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "laserentity"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CFuncTank::KeyValue(pkvd);
}

CLaser* CFuncTankLaser::GetLaser()
{
	if (m_pLaser)
		return m_pLaser;

	for (auto laser : UTIL_FindEntitiesByTargetname(STRING(pev->message)))
	{
		if (laser->ClassnameIs("env_laser"))
		{
			m_pLaser = static_cast<CLaser*>(laser);
			break;
		}
	}

	return m_pLaser;
}

void CFuncTankLaser::Think()
{
	if (m_pLaser && (gpGlobals->time > m_laserTime))
		m_pLaser->TurnOff();

	CFuncTank::Think();
}

void CFuncTankLaser::Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker)
{
	int i;
	TraceResult tr;

	if (m_fireLast != 0 && GetLaser())
	{
		// TankTrace needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors(pev->angles);

		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		if (0 != bulletCount)
		{
			for (i = 0; i < bulletCount; i++)
			{
				m_pLaser->pev->origin = barrelEnd;
				TankTrace(barrelEnd, forward, gTankSpread[m_spread], tr);

				m_laserTime = gpGlobals->time;
				m_pLaser->TurnOn();
				m_pLaser->pev->dmgtime = gpGlobals->time - 1.0;
				m_pLaser->FireAtPoint(tr);
				m_pLaser->pev->nextthink = 0;
			}
			CFuncTank::Fire(barrelEnd, forward, this);
		}
	}
	else
	{
		CFuncTank::Fire(barrelEnd, forward, this);
	}
}

class CFuncTankRocket : public CFuncTank
{
public:
	void Precache() override;
	void Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker) override;
};

LINK_ENTITY_TO_CLASS(func_tankrocket, CFuncTankRocket);

void CFuncTankRocket::Precache()
{
	UTIL_PrecacheOther("rpg_rocket");
	CFuncTank::Precache();
}

void CFuncTankRocket::Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker)
{
	int i;

	if (m_fireLast != 0)
	{
		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		if (bulletCount > 0)
		{
			for (i = 0; i < bulletCount; i++)
			{
				Create("rpg_rocket", barrelEnd, pev->angles, this);
			}
			CFuncTank::Fire(barrelEnd, forward, this);
		}
	}
	else
		CFuncTank::Fire(barrelEnd, forward, this);
}

class CFuncTankMortar : public CFuncTank
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker) override;
};

LINK_ENTITY_TO_CLASS(func_tankmortar, CFuncTankMortar);

bool CFuncTankMortar::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "iMagnitude"))
	{
		pev->impulse = atoi(pkvd->szValue);
		return true;
	}

	return CFuncTank::KeyValue(pkvd);
}

void CFuncTankMortar::Fire(const Vector& barrelEnd, const Vector& forward, CBaseEntity* attacker)
{
	if (m_fireLast != 0)
	{
		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		// Only create 1 explosion
		if (bulletCount > 0)
		{
			TraceResult tr;

			// TankTrace needs gpGlobals->v_up, etc.
			UTIL_MakeAimVectors(pev->angles);

			TankTrace(barrelEnd, forward, gTankSpread[m_spread], tr);

			ExplosionCreate(tr.vecEndPos, pev->angles, this, pev->impulse, true);

			CFuncTank::Fire(barrelEnd, forward, this);
		}
	}
	else
		CFuncTank::Fire(barrelEnd, forward, this);
}

class CFuncTankControls : public CBaseEntity
{
	DECLARE_CLASS(CFuncTankControls, CBaseEntity);
	DECLARE_DATAMAP();

public:
	int ObjectCaps() override;
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void Think() override;

	CFuncTank* m_pTank;
};

LINK_ENTITY_TO_CLASS(func_tankcontrols, CFuncTankControls);

BEGIN_DATAMAP(CFuncTankControls)
DEFINE_FIELD(m_pTank, FIELD_CLASSPTR),
	END_DATAMAP();

int CFuncTankControls::ObjectCaps()
{
	return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;
}

void CFuncTankControls::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{ // pass the Use command onto the controls
	if (m_pTank)
		m_pTank->Use(pActivator, pCaller, useType, value);

	ASSERT(m_pTank != nullptr); // if this fails,  most likely means save/restore hasn't worked properly
}

void CFuncTankControls::Think()
{
	for (auto target : UTIL_FindEntitiesByTargetname(STRING(pev->target)))
	{
		if (0 == strncmp(STRING(target->pev->classname), "func_tank", 9))
		{
			m_pTank = static_cast<CFuncTank*>(target);
			return;
		}
	}

	CBaseEntity::Logger->debug("No tank {}", STRING(pev->target));
}

void CFuncTankControls::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SetModel(STRING(pev->model));

	SetSize(pev->mins, pev->maxs);
	SetOrigin(pev->origin);

	pev->nextthink = gpGlobals->time + 0.3; // After all the func_tank's have spawned

	CBaseEntity::Spawn();
}
