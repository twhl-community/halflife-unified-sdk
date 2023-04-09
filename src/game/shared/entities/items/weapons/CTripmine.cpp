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
#include "CTripmine.h"

#define TRIPMINE_PRIMARY_VOLUME 450

#ifndef CLIENT_DLL

class CTripmineGrenade : public CGrenade
{
	DECLARE_CLASS(CTripmineGrenade, CGrenade);
	DECLARE_DATAMAP();

public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;

	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	void WarningThink();
	void PowerupThink();
	void BeamBreakThink();
	void DelayDeathThink();
	void Killed(CBaseEntity* attacker, int iGib) override;

	void MakeBeam();
	void KillBeam();

	float m_flPowerUp;
	Vector m_vecDir;
	Vector m_vecEnd;
	float m_flBeamLength;

	EHANDLE m_hOwner;
	CBeam* m_pBeam;
	Vector m_posOwner;
	Vector m_angleOwner;
	EHANDLE m_RealOwner; // tracelines don't hit PEV->OWNER, which means a player couldn't detonate his own trip mine, so we store the owner here.
};

LINK_ENTITY_TO_CLASS(monster_tripmine, CTripmineGrenade);

BEGIN_DATAMAP(CTripmineGrenade)
DEFINE_FIELD(m_flPowerUp, FIELD_TIME),
	DEFINE_FIELD(m_vecDir, FIELD_VECTOR),
	DEFINE_FIELD(m_vecEnd, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_flBeamLength, FIELD_FLOAT),
	DEFINE_FIELD(m_hOwner, FIELD_EHANDLE),
	// Don't save, recreate.
	// DEFINE_FIELD(m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(m_posOwner, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_angleOwner, FIELD_VECTOR),
	DEFINE_FIELD(m_RealOwner, FIELD_EHANDLE),
	DEFINE_FUNCTION(WarningThink),
	DEFINE_FUNCTION(PowerupThink),
	DEFINE_FUNCTION(BeamBreakThink),
	DEFINE_FUNCTION(DelayDeathThink),
	END_DATAMAP();

void CTripmineGrenade::OnCreate()
{
	CGrenade::OnCreate();

	pev->model = MAKE_STRING("models/v_tripmine.mdl");
}

void CTripmineGrenade::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SetModel(STRING(pev->model));
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_WORLD;
	ResetSequenceInfo();
	pev->framerate = 0;

	SetSize(Vector(-8, -8, -8), Vector(8, 8, 8));
	SetOrigin(pev->origin);

	// TODO: define constant
	if ((pev->spawnflags & 1) != 0)
	{
		// power up quickly
		m_flPowerUp = gpGlobals->time + 1.0;
	}
	else
	{
		// power up in 2.5 seconds
		m_flPowerUp = gpGlobals->time + 2.5;
	}

	SetThink(&CTripmineGrenade::PowerupThink);
	pev->nextthink = gpGlobals->time + 0.2;

	pev->takedamage = DAMAGE_YES;
	pev->dmg = GetSkillFloat("plr_tripmine"sv);
	pev->health = 1; // don't let die normally

	if (auto owner = GetOwner(); owner)
	{
		// play deploy sound
		EmitSound(CHAN_VOICE, "weapons/mine_deploy.wav", 1.0, ATTN_NORM);
		EmitSound(CHAN_BODY, "weapons/mine_charge.wav", 0.2, ATTN_NORM); // chargeup

		m_RealOwner = owner; // see CTripmineGrenade for why.
	}

	UTIL_MakeAimVectors(pev->angles);

	m_vecDir = gpGlobals->v_forward;
	m_vecEnd = pev->origin + m_vecDir * 2048;
}

void CTripmineGrenade::Precache()
{
	PrecacheModel(STRING(pev->model));
	PrecacheSound("weapons/mine_deploy.wav");
	PrecacheSound("weapons/mine_activate.wav");
	PrecacheSound("weapons/mine_charge.wav");
}

void CTripmineGrenade::WarningThink()
{
	// play warning sound
	// EmitSound(CHAN_VOICE, "buttons/Blip2.wav", 1.0, ATTN_NORM);

	// set to power up
	SetThink(&CTripmineGrenade::PowerupThink);
	pev->nextthink = gpGlobals->time + 1.0;
}

void CTripmineGrenade::PowerupThink()
{
	TraceResult tr;

	if (m_hOwner == nullptr)
	{
		// find an owner
		edict_t* oldowner = pev->owner;
		pev->owner = nullptr;
		UTIL_TraceLine(pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 32, dont_ignore_monsters, edict(), &tr);
		if (0 != tr.fStartSolid || (oldowner && tr.pHit == oldowner))
		{
			pev->owner = oldowner;
			m_flPowerUp += 0.1;
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}
		if (tr.flFraction < 1.0)
		{
			pev->owner = tr.pHit;
			m_hOwner = CBaseEntity::Instance(pev->owner);
			m_posOwner = m_hOwner->pev->origin;
			m_angleOwner = m_hOwner->pev->angles;
		}
		else
		{
			StopSound(CHAN_VOICE, "weapons/mine_deploy.wav");
			StopSound(CHAN_BODY, "weapons/mine_charge.wav");
			SetThink(&CTripmineGrenade::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1;
			AILogger->debug("WARNING:Tripmine at {:.0f} removed\n", pev->origin);
			KillBeam();
			return;
		}
	}
	else if (m_posOwner != m_hOwner->pev->origin || m_angleOwner != m_hOwner->pev->angles)
	{
		// disable
		StopSound(CHAN_VOICE, "weapons/mine_deploy.wav");
		StopSound(CHAN_BODY, "weapons/mine_charge.wav");
		CTripmine* pMine = static_cast<CTripmine*>(Create("weapon_tripmine", pev->origin + m_vecDir * 24, pev->angles));
		pMine->m_RespawnDelay = ITEM_NEVER_RESPAWN_DELAY;

		SetThink(&CTripmineGrenade::SUB_Remove);
		KillBeam();
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}
	// AILogger->debug("{} {:.0f}", pev->owner, m_pOwner->pev->origin);

	if (gpGlobals->time > m_flPowerUp)
	{
		// make solid
		pev->solid = SOLID_BBOX;
		SetOrigin(pev->origin);

		MakeBeam();

		// play enabled sound
		EmitSoundDyn(CHAN_VOICE, "weapons/mine_activate.wav", 0.5, ATTN_NORM, 1.0, 75);
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CTripmineGrenade::KillBeam()
{
	if (m_pBeam)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = nullptr;
	}
}

void CTripmineGrenade::MakeBeam()
{
	TraceResult tr;

	// AIlogger->debug("serverflags {}", gpGlobals->serverflags);

	UTIL_TraceLine(pev->origin, m_vecEnd, dont_ignore_monsters, edict(), &tr);

	m_flBeamLength = tr.flFraction;

	// set to follow laser spot
	SetThink(&CTripmineGrenade::BeamBreakThink);
	pev->nextthink = gpGlobals->time + 0.1;

	Vector vecTmpEnd = pev->origin + m_vecDir * 2048 * m_flBeamLength;

	m_pBeam = CBeam::BeamCreate(g_pModelNameLaser, 10);
	// Mark as temporary so the beam will be recreated on save game load and level transitions.
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	// PointEntInit causes clients to use the position of whatever the previous entity to use this edict had until the server updates them.
	// m_pBeam->PointEntInit(vecTmpEnd, entindex());
	m_pBeam->PointsInit(pev->origin, vecTmpEnd);
	m_pBeam->SetColor(0, 214, 198);
	m_pBeam->SetScrollRate(255);
	m_pBeam->SetBrightness(64);
}

void CTripmineGrenade::BeamBreakThink()
{
	bool bBlowup = false;

	TraceResult tr;

	// HACKHACK Set simple box using this really nice global!
	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	UTIL_TraceLine(pev->origin, m_vecEnd, dont_ignore_monsters, edict(), &tr);

	// AILogger->debug("{} : {}", tr.flFraction, m_flBeamLength);

	// respawn detect.
	if (!m_pBeam)
	{
		// Use the same trace parameters as the original trace above so the right entity is hit.
		TraceResult tr2;
		// Clear out old owner so it can be hit by traces.
		pev->owner = nullptr;
		UTIL_TraceLine(pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 32, dont_ignore_monsters, edict(), &tr2);
		MakeBeam();
		if (tr2.pHit)
		{
			// reset owner too
			pev->owner = tr2.pHit;
			m_hOwner = CBaseEntity::Instance(tr2.pHit);
		}
	}

	if (fabs(m_flBeamLength - tr.flFraction) > 0.001)
	{
		bBlowup = true;
	}
	else
	{
		if (m_hOwner == nullptr)
			bBlowup = true;
		else if (m_posOwner != m_hOwner->pev->origin)
			bBlowup = true;
		else if (m_angleOwner != m_hOwner->pev->angles)
			bBlowup = true;
	}

	if (bBlowup)
	{
		// a bit of a hack, but all CGrenade code passes pev->owner along to make sure the proper player gets credit for the kill
		// so we have to restore pev->owner from pRealOwner, because an entity's tracelines don't strike it's pev->owner which meant
		// that a player couldn't trigger his own tripmine. Now that the mine is exploding, it's safe the restore the owner so the
		// CGrenade code knows who the explosive really belongs to.
		SetOwner(m_RealOwner);
		pev->health = 0;
		Killed(GetOwner(), GIB_NORMAL);
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

bool CTripmineGrenade::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (gpGlobals->time < m_flPowerUp && flDamage < pev->health)
	{
		// disable
		// Create( "weapon_tripmine", pev->origin + m_vecDir * 24, pev->angles );
		SetThink(&CTripmineGrenade::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1;
		KillBeam();
		return false;
	}
	return CGrenade::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);
}

void CTripmineGrenade::Killed(CBaseEntity* attacker, int iGib)
{
	pev->takedamage = DAMAGE_NO;

	if (attacker && (attacker->pev->flags & FL_CLIENT) != 0)
	{
		// some client has destroyed this mine, he'll get credit for any kills
		SetOwner(attacker);
	}

	SetThink(&CTripmineGrenade::DelayDeathThink);
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.1, 0.3);

	EmitSound(CHAN_BODY, "common/null.wav", 0.5, ATTN_NORM); // shut off chargeup
}

void CTripmineGrenade::DelayDeathThink()
{
	KillBeam();
	TraceResult tr;
	UTIL_TraceLine(pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 64, dont_ignore_monsters, edict(), &tr);

	Explode(&tr, DMG_BLAST);
}
#endif

LINK_ENTITY_TO_CLASS(weapon_tripmine, CTripmine);

void CTripmine::OnCreate()
{
	CBasePlayerWeapon::OnCreate();
	m_iId = WEAPON_TRIPMINE;
	m_iDefaultAmmo = TRIPMINE_DEFAULT_GIVE;
	m_WorldModel = pev->model = MAKE_STRING("models/v_tripmine.mdl");
}

void CTripmine::Spawn()
{
	CBasePlayerWeapon::Spawn();

	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_GROUND;
	// ResetSequenceInfo( );
	pev->framerate = 0;

	// HACK: force the body to the first person view by default so it doesn't show up as a huge tripmine for a second.
#ifdef CLIENT_DLL
	pev->body = 0;
#endif

	if (!UTIL_IsMultiplayer())
	{
		SetSize(Vector(-16, -16, 0), Vector(16, 16, 28));
	}
}

void CTripmine::Precache()
{
	CBasePlayerWeapon::Precache();
	PrecacheModel(STRING(m_WorldModel));
	PrecacheModel("models/v_tripmine.mdl");
	PrecacheModel("models/p_tripmine.mdl");
	UTIL_PrecacheOther("monster_tripmine");

	m_usTripFire = PRECACHE_EVENT(1, "events/tripfire.sc");
}

bool CTripmine::GetWeaponInfo(WeaponInfo& info)
{
	info.Name = STRING(pev->classname);
	info.AttackModeInfo[0].AmmoType = "Trip Mine";
	info.AttackModeInfo[0].MagazineSize = WEAPON_NOCLIP;
	info.Slot = 4;
	info.Position = 2;
	info.Id = WEAPON_TRIPMINE;
	info.Weight = TRIPMINE_WEIGHT;
	info.Flags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return true;
}

bool CTripmine::Deploy()
{
	pev->body = 0;
	return DefaultDeploy("models/v_tripmine.mdl", "models/p_tripmine.mdl", TRIPMINE_DRAW, "trip");
}

void CTripmine::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (0 == m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		// out of mines
		m_pPlayer->ClearWeaponBit(m_iId);
		SetThink(&CTripmine::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
	}

	SendWeaponAnim(TRIPMINE_HOLSTER);
	m_pPlayer->EmitSound(CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CTripmine::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;

	UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 128, dont_ignore_monsters, m_pPlayer->edict(), &tr);

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usTripFire, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

	if (tr.flFraction < 1.0)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity && (pEntity->pev->flags & FL_CONVEYOR) == 0)
		{
			Vector angles = UTIL_VecToAngles(tr.vecPlaneNormal);

			Create("monster_tripmine", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer);

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

			if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			{
				// no more mines!
				RetireWeapon();
				return;
			}
		}
		else
		{
			// WeaponsLogger->debug("no deploy");
		}
	}
	else
	{
	}

	m_flNextPrimaryAttack = GetNextAttackDelay(0.3);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CTripmine::WeaponIdle()
{
	// If we're here then we're in a player's inventory, and need to use this body
	pev->body = 0;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		SendWeaponAnim(TRIPMINE_DRAW);
	}
	else
	{
		RetireWeapon();
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.25)
	{
		iAnim = TRIPMINE_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 30.0;
	}
	else if (flRand <= 0.75)
	{
		iAnim = TRIPMINE_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 30.0;
	}
	else
	{
		iAnim = TRIPMINE_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 100.0 / 30.0;
	}

	SendWeaponAnim(iAnim);
}
