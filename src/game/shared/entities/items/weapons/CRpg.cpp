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
#include "CRpg.h"
#include "UserMessages.h"

#ifndef CLIENT_DLL
#include "items/weapons/CLaserSpot.h"
#endif

LINK_ENTITY_TO_CLASS(weapon_rpg, CRpg);

#ifndef CLIENT_DLL
TYPEDESCRIPTION CRpg::m_SaveData[] =
	{
		DEFINE_FIELD(CRpg, m_fSpotActive, FIELD_BOOLEAN),
		DEFINE_FIELD(CRpg, m_cActiveRockets, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CRpg, CBasePlayerWeapon);

TYPEDESCRIPTION CRpgRocket::m_SaveData[] =
	{
		DEFINE_FIELD(CRpgRocket, m_flIgniteTime, FIELD_TIME),
		DEFINE_FIELD(CRpgRocket, m_pLauncher, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CRpgRocket, CGrenade);

LINK_ENTITY_TO_CLASS(rpg_rocket, CRpgRocket);

CRpgRocket::~CRpgRocket()
{
	if (m_pLauncher)
	{
		// my launcher is still around, tell it I'm dead.
		static_cast<CRpg*>(static_cast<CBaseEntity*>(m_pLauncher))->m_cActiveRockets--;
	}
}

CRpgRocket* CRpgRocket::CreateRpgRocket(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, CRpg* pLauncher)
{
	CRpgRocket* pRocket = g_EntityDictionary->Create<CRpgRocket>("rpg_rocket");

	pRocket->SetOrigin(vecOrigin);
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch(&CRpgRocket::RocketTouch);
	pRocket->m_pLauncher = pLauncher; // remember what RPG fired me.
	pLauncher->m_cActiveRockets++;	  // register this missile as active for the launcher
	pRocket->pev->owner = pOwner->edict();

	return pRocket;
}

void CRpgRocket::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SetModel("models/rpgrocket.mdl");
	SetSize(Vector(0, 0, 0), Vector(0, 0, 0));
	SetOrigin(pev->origin);

	SetThink(&CRpgRocket::IgniteThink);
	SetTouch(&CRpgRocket::ExplodeTouch);

	pev->angles.x -= 30;
	UTIL_MakeVectors(pev->angles);
	pev->angles.x = -(pev->angles.x + 30);

	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.4;

	pev->dmg = GetSkillFloat("plr_rpg"sv);
}

void CRpgRocket::RocketTouch(CBaseEntity* pOther)
{
	StopSound(CHAN_VOICE, "weapons/rocket1.wav");
	ExplodeTouch(pOther);
}

void CRpgRocket::Precache()
{
	PrecacheModel("models/rpgrocket.mdl");
	m_iTrail = PrecacheModel("sprites/smoke.spr");
	PrecacheSound("weapons/rocket1.wav");
}

void CRpgRocket::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EmitSound(CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5);

	// rocket trail
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex()); // entity
	WRITE_SHORT(m_iTrail);	 // model
	WRITE_BYTE(40);			 // life
	WRITE_BYTE(5);			 // width
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(255);		 // r, g, b
	WRITE_BYTE(255);		 // brightness

	MESSAGE_END(); // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink(&CRpgRocket::FollowThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CRpgRocket::FollowThink()
{
	CBaseEntity* pOther = nullptr;
	Vector vecTarget;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;

	UTIL_MakeAimVectors(pev->angles);

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;

	// Examine all entities within a reasonable radius
	while ((pOther = UTIL_FindEntityByClassname(pOther, "laser_spot")) != nullptr)
	{
		UTIL_TraceLine(pev->origin, pOther->pev->origin, dont_ignore_monsters, ENT(pev), &tr);
		// WeaponsLogger->debug("{}", tr.flFraction);
		if (tr.flFraction >= 0.90)
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length();
			vecDir = vecDir.Normalize();
			flDot = DotProduct(gpGlobals->v_forward, vecDir);
			if ((flDot > 0) && (flDist * (1 - flDot) < flMax))
			{
				flMax = flDist * (1 - flDot);
				vecTarget = vecDir;
			}
		}
	}

	pev->angles = UTIL_VecToAngles(vecTarget);

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if (gpGlobals->time - m_flIgniteTime < 1.0)
	{
		pev->velocity = pev->velocity * 0.2 + vecTarget * (flSpeed * 0.8 + 400);
		if (pev->waterlevel == WaterLevel::Head)
		{
			// go slow underwater
			if (pev->velocity.Length() > 300)
			{
				pev->velocity = pev->velocity.Normalize() * 300;
			}
			UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 4);
		}
		else
		{
			if (pev->velocity.Length() > 2000)
			{
				pev->velocity = pev->velocity.Normalize() * 2000;
			}
		}
	}
	else
	{
		if ((pev->effects & EF_LIGHT) != 0)
		{
			pev->effects = 0;
			StopSound(CHAN_VOICE, "weapons/rocket1.wav");
		}
		pev->velocity = pev->velocity * 0.2 + vecTarget * flSpeed * 0.798;
		if (pev->waterlevel == WaterLevel::Dry && pev->velocity.Length() < 1500)
		{
			Detonate();
		}
	}
	// WeaponsLogger->debug("{:.0f}", flSpeed);

	pev->nextthink = gpGlobals->time + 0.1;
}
#endif

void CRpg::OnCreate()
{
	CBasePlayerWeapon::OnCreate();
	m_iId = WEAPON_RPG;

	m_fSpotActive = true;

	if (UTIL_IsMultiplayer())
	{
		// more default ammo in multiplay.
		m_iDefaultAmmo = RPG_DEFAULT_GIVE * 2;
	}
	else
	{
		m_iDefaultAmmo = RPG_DEFAULT_GIVE;
	}

	m_WorldModel = pev->model = MAKE_STRING("models/w_rpg.mdl");
}

void CRpg::Reload()
{
	if (m_iClip == 1)
	{
		// don't bother with any of this if don't need to reload.
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	// because the RPG waits to autoreload when no missiles are active while  the LTD is on, the
	// weapons code is constantly calling into this function, but is often denied because
	// a) missiles are in flight, but the LTD is on
	// or
	// b) player is totally out of ammo and has nothing to switch to, and should be allowed to
	//    shine the designator around
	//
	// Set the next attack time into the future so that WeaponIdle will get called more often
	// than reload, allowing the RPG LTD to be updated

	m_flNextPrimaryAttack = GetNextAttackDelay(0.5);

	if (0 != m_cActiveRockets && m_fSpotActive)
	{
		// no reloading when there are active missiles tracking the designator.
		// ward off future autoreload attempts by setting next attack time into the future for a bit.
		return;
	}

#ifndef CLIENT_DLL
	if (m_pSpot && m_fSpotActive)
	{
		m_pSpot->Suspend(2.1);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.1;
	}
#endif

	if (m_iClip == 0)
	{
		const bool iResult = DefaultReload(RPG_MAX_CLIP, RPG_RELOAD, 2);

		if (iResult)
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
}

void CRpg::Precache()
{
	CBasePlayerWeapon::Precache();
	PrecacheModel(STRING(m_WorldModel));
	PrecacheModel("models/v_rpg.mdl");
	PrecacheModel("models/p_rpg.mdl");

	PrecacheSound("items/9mmclip1.wav");

	UTIL_PrecacheOther("laser_spot");
	UTIL_PrecacheOther("rpg_rocket");

	PrecacheSound("weapons/rocketfire1.wav");
	PrecacheSound("weapons/glauncher.wav"); // alternative fire sound

	m_usRpg = PRECACHE_EVENT(1, "events/rpg.sc");
}

bool CRpg::GetWeaponInfo(WeaponInfo& info)
{
	info.Name = STRING(pev->classname);
	info.AttackModeInfo[0].AmmoType = "rockets";
	info.AttackModeInfo[0].MagazineSize = RPG_MAX_CLIP;
	info.Slot = 3;
	info.Position = 0;
	info.Id = WEAPON_RPG;
	info.Weight = RPG_WEIGHT;

	return true;
}

void CRpg::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "rockets") >= 0)
	{
		pPlayer->EmitSound(CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

bool CRpg::Deploy()
{
	if (m_iClip == 0)
	{
		return DefaultDeploy("models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW_UL, "rpg");
	}

	return DefaultDeploy("models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW1, "rpg");
}

bool CRpg::CanHolster()
{
	if (m_fSpotActive && 0 != m_cActiveRockets)
	{
		// can't put away while guiding a missile.
		return false;
	}

	return true;
}

void CRpg::Holster()
{
	m_fInReload = false; // cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim(RPG_HOLSTER1);

#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed(nullptr, GIB_NEVER);
		m_pSpot = nullptr;
	}
#endif
}

void CRpg::PrimaryAttack()
{
	if (0 != m_iClip)
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

#ifndef CLIENT_DLL
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -8;

		CRpgRocket* pRocket = CRpgRocket::CreateRpgRocket(vecSrc, m_pPlayer->pev->v_angle, m_pPlayer, this);

		UTIL_MakeVectors(m_pPlayer->pev->v_angle); // RpgRocket::Create stomps on globals, so remake.
		pRocket->pev->velocity = pRocket->pev->velocity + gpGlobals->v_forward * DotProduct(m_pPlayer->pev->velocity, gpGlobals->v_forward);
#endif

		// firing RPG no longer turns on the designator. ALT fire is a toggle switch for the LTD.
		// Ken signed up for this as a global change (sjb)

		int flags;
#if defined(CLIENT_WEAPONS)
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif

		PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usRpg);

		m_iClip--;

		m_flNextPrimaryAttack = GetNextAttackDelay(1.5);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
	}
	else
	{
		PlayEmptySound();
	}
	UpdateSpot();
}

void CRpg::SecondaryAttack()
{
	m_fSpotActive = !m_fSpotActive;

#ifndef CLIENT_DLL
	if (!m_fSpotActive && m_pSpot)
	{
		m_pSpot->Killed(nullptr, GIB_NORMAL);
		m_pSpot = nullptr;
	}
#endif

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
}

void CRpg::WeaponIdle()
{
	UpdateSpot();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (0 != m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
		if (flRand <= 0.75 || m_fSpotActive)
		{
			if (m_iClip == 0)
				iAnim = RPG_IDLE_UL;
			else
				iAnim = RPG_IDLE;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 15.0;
		}
		else
		{
			if (m_iClip == 0)
				iAnim = RPG_FIDGET_UL;
			else
				iAnim = RPG_FIDGET;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.1;
		}

		ResetEmptySound();
		SendWeaponAnim(iAnim);
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	}
}

void CRpg::UpdateSpot()
{

#ifndef CLIENT_DLL
	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CLaserSpot::CreateSpot();
		}

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

		m_pSpot->SetOrigin(tr.vecEndPos);
	}
#endif
}

void CRpg::GetWeaponData(weapon_data_t& data)
{
	data.fuser2 = static_cast<float>(m_fSpotActive);
	data.fuser3 = m_cActiveRockets;
}

void CRpg::SetWeaponData(const weapon_data_t& data)
{
	m_fSpotActive = data.fuser2 != 0;
	m_cActiveRockets = data.fuser3;
}

class CRpgAmmo : public CBasePlayerAmmo
{
public:
	void OnCreate() override
	{
		CBasePlayerAmmo::OnCreate();

		// hand out more ammo per rocket in multiplayer.
		m_AmmoAmount = AMMO_RPGCLIP_GIVE;
		if (UTIL_IsMultiplayer())
		{
			m_AmmoAmount *= 2;
		}

		m_AmmoName = MAKE_STRING("rockets");
		pev->model = MAKE_STRING("models/w_rpgammo.mdl");
	}
};

LINK_ENTITY_TO_CLASS(ammo_rpgclip, CRpgAmmo);
