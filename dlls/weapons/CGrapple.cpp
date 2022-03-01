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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "customentity.h"
#include "UserMessages.h"

#include "CGrappleTip.h"

#ifndef CLIENT_DLL
#include "gamerules.h"
#else
#include "cl_dll.h"
#endif

#include "CGrapple.h"

#ifndef CLIENT_DLL
extern cvar_t oldgrapple;

TYPEDESCRIPTION CGrapple::m_SaveData[] =
	{
		DEFINE_FIELD(CGrapple, m_pBeam, FIELD_CLASSPTR),
		DEFINE_FIELD(CGrapple, m_flShootTime, FIELD_TIME),
		DEFINE_FIELD(CGrapple, m_FireState, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CGrapple, CGrapple::BaseClass);
#endif

LINK_ENTITY_TO_CLASS(weapon_grapple, CGrapple);

void CGrapple::Precache()
{
	PRECACHE_MODEL("models/v_bgrap.mdl");
	PRECACHE_MODEL("models/w_bgrap.mdl");
	PRECACHE_MODEL("models/p_bgrap.mdl");

	PRECACHE_SOUND("weapons/bgrapple_release.wav");
	PRECACHE_SOUND("weapons/bgrapple_impact.wav");
	PRECACHE_SOUND("weapons/bgrapple_fire.wav");
	PRECACHE_SOUND("weapons/bgrapple_cough.wav");
	PRECACHE_SOUND("weapons/bgrapple_pull.wav");
	PRECACHE_SOUND("weapons/bgrapple_wait.wav");
	PRECACHE_SOUND("weapons/alienweap_draw.wav");
	PRECACHE_SOUND("barnacle/bcl_chew1.wav");
	PRECACHE_SOUND("barnacle/bcl_chew2.wav");
	PRECACHE_SOUND("barnacle/bcl_chew3.wav");

	PRECACHE_MODEL("sprites/tongue.spr");

	UTIL_PrecacheOther("grapple_tip");
}

void CGrapple::Spawn()
{
	Precache();

	m_iId = WEAPON_GRAPPLE;

	SET_MODEL(edict(), "models/w_bgrap.mdl");

	m_iClip = WEAPON_NOCLIP;

	m_pTip = nullptr;

	m_bGrappling = false;

	FallInit();
}

bool CGrapple::Deploy()
{
	return DefaultDeploy("models/v_bgrap.mdl", "models/p_bgrap.mdl", BGRAPPLE_UP, "gauss");
}

void CGrapple::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim(BGRAPPLE_DOWN);

	if (m_FireState != FireState::OFF)
	{
		EndAttack();
	}
}

void CGrapple::WeaponIdle()
{
	ResetEmptySound();

	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		if (m_FireState != FireState::OFF)
			EndAttack();

		m_bMissed = false;

		const float flNextIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		int iAnim;

		if (flNextIdle <= 0.5)
		{
			iAnim = BGRAPPLE_LONGIDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10.0;
		}
		else if (flNextIdle > 0.95)
		{
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_cough.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

			iAnim = BGRAPPLE_COUGH;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.6;
		}
		else
		{
			iAnim = BGRAPPLE_BREATHE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.566;
		}

		SendWeaponAnim(iAnim);
	}
}

void CGrapple::PrimaryAttack()
{
	if (m_bMissed)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		return;
	}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	if (m_pTip)
	{
		if (m_pTip->IsStuck())
		{
			CBaseEntity* pTarget = m_pTip->GetGrappleTarget();

			if (!pTarget)
			{
				EndAttack();
				return;
			}

			if (m_pTip->GetGrappleType() > CGrappleTip::TargetClass::SMALL)
			{
				m_pPlayer->pev->movetype = MOVETYPE_FLY;
				//Tells the physics code that the player is not on a ladder - Solokiller
				m_pPlayer->pev->flags |= FL_IMMUNE_LAVA;
			}

			if (m_bMomentaryStuck)
			{
				SendWeaponAnim(BGRAPPLE_FIRETRAVEL);

				EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125);

				if (pTarget->IsPlayer())
				{
					EMIT_SOUND_DYN(pTarget->edict(), CHAN_STATIC, "weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125);
				}

				m_bMomentaryStuck = false;
			}

			switch (m_pTip->GetGrappleType())
			{
			case CGrappleTip::TargetClass::NOT_A_TARGET:
				break;

			case CGrappleTip::TargetClass::SMALL:
				//pTarget->BarnacleVictimGrabbed( this );
				UTIL_SetOrigin(m_pTip->pev, pTarget->Center());

				pTarget->pev->velocity = pTarget->pev->velocity + (m_pPlayer->pev->origin - pTarget->pev->origin);

				if (pTarget->pev->velocity.Length() > 450.0)
				{
					pTarget->pev->velocity = pTarget->pev->velocity.Normalize() * 450.0;
				}
				break;

			case CGrappleTip::TargetClass::MEDIUM:
			case CGrappleTip::TargetClass::LARGE:
			case CGrappleTip::TargetClass::FIXED:
				//pTarget->BarnacleVictimGrabbed( this );

				if (m_pTip->GetGrappleType() != CGrappleTip::TargetClass::FIXED)
					UTIL_SetOrigin(m_pTip->pev, pTarget->Center());

				m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + (m_pTip->pev->origin - m_pPlayer->pev->origin);

				if (m_pPlayer->pev->velocity.Length() > 450.0)
				{
					m_pPlayer->pev->velocity = m_pPlayer->pev->velocity.Normalize() * 450.0;

					Vector vecPitch = UTIL_VecToAngles(m_pPlayer->pev->velocity);

					if ((vecPitch.x > 55.0 && vecPitch.x < 205.0) || vecPitch.x < -55.0)
					{
						m_bGrappling = false;
						m_pPlayer->SetAnimation(PLAYER_IDLE);
					}
					else
					{
						m_bGrappling = true;
						m_pPlayer->SetAnimation(PLAYER_GRAPPLE);
					}
				}
				else
				{
					m_bGrappling = false;
					m_pPlayer->SetAnimation(PLAYER_IDLE);
				}

				break;
			}
		}

		if (m_pTip->HasMissed())
		{
			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_release.wav", 0.98, ATTN_NORM, 0, 125);

			EndAttack();
			return;
		}
	}

	if (m_FireState != FireState::OFF)
	{
		m_pPlayer->m_iWeaponVolume = 450;

		if (m_flShootTime != 0.0 && gpGlobals->time > m_flShootTime)
		{
			SendWeaponAnim(BGRAPPLE_FIREWAITING);

			m_pPlayer->pev->punchangle.x += 2.0;

			Fire(m_pPlayer->GetGunPosition(), gpGlobals->v_forward);

			EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_pull.wav", 0.98, ATTN_NORM, 0, 125);

			m_flShootTime = 0;
		}
	}
	else
	{
		m_bMomentaryStuck = true;

		SendWeaponAnim(BGRAPPLE_FIRE);

		m_pPlayer->m_iWeaponVolume = 450;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;

		if (UTIL_IsMultiplayer())
		{
			m_flShootTime = gpGlobals->time;
		}
		else
		{
			m_flShootTime = gpGlobals->time + 0.35;
		}

		EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/bgrapple_fire.wav", 0.98, ATTN_NORM, 0, 125);

		m_FireState = FireState::CHARGE;
	}

	if (!m_pTip)
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;
		return;
	}

	if (m_pTip->GetGrappleType() != CGrappleTip::TargetClass::FIXED && m_pTip->IsStuck())
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);

		Vector vecSrc = m_pPlayer->GetGunPosition();

		Vector vecEnd = vecSrc + gpGlobals->v_forward * 16.0;

		TraceResult tr;

		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr);

#ifndef CLIENT_DLL
		if (tr.flFraction >= 1.0)
		{
			UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &tr);

			if (tr.flFraction < 1.0)
			{
				//If we've hit a solid object see if we're hurting it
				if (!tr.pHit || FNullEnt(tr.pHit) || GET_PRIVATE<CBaseEntity>(tr.pHit)->IsBSPModel())
				{
					FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
				}
			}
		}
#endif

		if (tr.flFraction < 1.0)
		{
			auto pHit = Instance(tr.pHit);

			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

			if (pHit)
			{
				bool bValidTarget = false;

				if (pHit->IsPlayer())
				{
					m_pTip->SetGrappleTarget(pHit);

					bValidTarget = true;
				}
				else if (m_pTip->ClassifyTarget(pHit) != CGrappleTip::TargetClass::NOT_A_TARGET)
				{
					bValidTarget = true;
				}

				if (bValidTarget)
				{
					if (m_flDamageTime + 0.5 < gpGlobals->time)
					{
#ifndef CLIENT_DLL
						ClearMultiDamage();

						float flDamage = gSkillData.plrDmgGrapple;

						if (g_pGameRules->IsMultiplayer())
						{
							flDamage *= 2;
						}

						pHit->TraceAttack(pev, flDamage, gpGlobals->v_forward, &tr, DMG_ALWAYSGIB | DMG_CLUB);

						ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
#endif

						m_flDamageTime = gpGlobals->time;

						const char* pszSample;

						switch (RANDOM_LONG(0, 2))
						{
						default:
						case 0:
							pszSample = "barnacle/bcl_chew1.wav";
							break;
						case 1:
							pszSample = "barnacle/bcl_chew2.wav";
							break;
						case 2:
							pszSample = "barnacle/bcl_chew3.wav";
							break;
						}

						EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_VOICE, pszSample, VOL_NORM, ATTN_NORM, 0, 125);
					}
				}
			}
		}
	}

	const bool useNewGrapple =
#ifdef CLIENT_DLL
		gEngfuncs.pfnGetCvarFloat("sv_oldgrapple") != 1
#else
		oldgrapple.value != 1
#endif
		;

	if (UTIL_IsMultiplayer() && (UTIL_IsCTF() || useNewGrapple))
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase();
	}
	else
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
	}
}

void CGrapple::SecondaryAttack()
{
	if (m_pTip && m_pTip->IsStuck() &&
		(!m_pTip->GetGrappleTarget() || m_pTip->GetGrappleTarget()->IsPlayer()))
	{
		EndAttack();
	}
	else
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

		m_pPlayer->pev->movetype = MOVETYPE_WALK;
		ClearBits(m_pPlayer->pev->flags, FL_IMMUNE_LAVA);
	}
}

void CGrapple::Fire(const Vector& vecOrigin, const Vector& vecDir)
{
#ifndef CLIENT_DLL
	Vector vecSrc = vecOrigin;

	Vector vecEnd = vecSrc + vecDir * 2048.0;

	TraceResult tr;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr);

	if (0 == tr.fAllSolid)
	{
		auto pHit = Instance(tr.pHit);

		if (pHit)
		{
			UpdateEffect();

			m_flDamageTime = gpGlobals->time;
		}
	}
#endif
}

void CGrapple::EndAttack()
{
	m_FireState = FireState::OFF;

	SendWeaponAnim(BGRAPPLE_FIREREACHED);

	EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_release.wav", 0.98, ATTN_NORM, 0, 125);

	EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_pull.wav", 0.0, ATTN_NONE, SND_STOP, 100);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();

	const bool useNewGrapple =
#ifdef CLIENT_DLL
		gEngfuncs.pfnGetCvarFloat("sv_oldgrapple") != 1
#else
		oldgrapple.value != 1
#endif
		;

	if (UTIL_IsMultiplayer() && (UTIL_IsCTF() || useNewGrapple))
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase();
	}
	else
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
	}

	DestroyEffect();

	if (m_bGrappling && m_pPlayer->IsAlive())
	{
		m_pPlayer->SetAnimation(PLAYER_IDLE);
	}

	m_pPlayer->pev->movetype = MOVETYPE_WALK;
	ClearBits(m_pPlayer->pev->flags, FL_IMMUNE_LAVA);
}

void CGrapple::CreateEffect()
{
#ifndef CLIENT_DLL
	DestroyEffect();

	m_pTip = GetClassPtr<CGrappleTip>(nullptr);

	m_pTip->pev->classname = MAKE_STRING("grapple_tip");

	m_pTip->Spawn();

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	Vector vecOrigin =
		m_pPlayer->GetGunPosition() +
		gpGlobals->v_forward * 16.0 +
		gpGlobals->v_right * 8.0 +
		gpGlobals->v_up * -8.0;

	Vector vecAngles = m_pPlayer->pev->v_angle;

	vecAngles.x = -vecAngles.x;

	m_pTip->SetPosition(vecOrigin, vecAngles, m_pPlayer);

	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate("sprites/tongue.spr", 16);

		m_pBeam->EntsInit(m_pTip->entindex(), m_pPlayer->entindex());

		m_pBeam->SetFlags(BEAM_FSOLID);

		m_pBeam->SetBrightness(100.0);

		m_pBeam->SetEndAttachment(1);

		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	}
#endif
}

void CGrapple::UpdateEffect()
{
#ifndef CLIENT_DLL
	if (!m_pBeam || !m_pTip)
		CreateEffect();
#endif
}

void CGrapple::DestroyEffect()
{
	if (m_pBeam)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = nullptr;
	}

	if (m_pTip)
	{
		m_pTip->Killed(nullptr, GIB_NEVER);
		m_pTip = nullptr;
	}
}

int CGrapple::iItemSlot()
{
	return 1;
}

bool CGrapple::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = nullptr;
	p->iMaxAmmo1 = WEAPON_NOCLIP;
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_GRAPPLE;
	p->iFlags = 0;
	p->iWeight = 21;

	return true;
}
