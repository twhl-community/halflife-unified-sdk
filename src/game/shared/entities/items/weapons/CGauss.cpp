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
#include "CGauss.h"
#include "shake.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_gauss, CGauss);

BEGIN_DATAMAP(CGauss)
DEFINE_FIELD(m_fInAttack, FIELD_INTEGER),
	//	DEFINE_FIELD(m_flStartCharge, FIELD_TIME),
	//	DEFINE_FIELD(m_flPlayAftershock, FIELD_TIME),
	//	DEFINE_FIELD(m_flNextAmmoBurn, FIELD_TIME),
	DEFINE_FIELD(m_fPrimaryFire, FIELD_BOOLEAN),
	END_DATAMAP();

float CGauss::GetFullChargeTime()
{
	return g_Skill.GetValue("gauss_charge_time");
}

void CGauss::OnCreate()
{
	CBasePlayerWeapon::OnCreate();
	m_iId = WEAPON_GAUSS;
	m_iDefaultAmmo = GAUSS_DEFAULT_GIVE;
	m_WorldModel = pev->model = MAKE_STRING("models/w_gauss.mdl");
}

void CGauss::Precache()
{
	CBasePlayerWeapon::Precache();
	PrecacheModel(STRING(m_WorldModel));
	PrecacheModel("models/v_gauss.mdl");
	PrecacheModel("models/p_gauss.mdl");

	PrecacheSound("items/9mmclip1.wav");

	PrecacheSound("weapons/gauss2.wav");
	PrecacheSound("weapons/electro4.wav");
	PrecacheSound("weapons/electro5.wav");
	PrecacheSound("weapons/electro6.wav");
	PrecacheSound("ambience/pulsemachine.wav");

	m_iGlow = PrecacheModel("sprites/hotglow.spr");
	m_iBalls = PrecacheModel("sprites/hotglow.spr");
	m_iBeam = PrecacheModel("sprites/smoke.spr");

	m_usGaussFire = PRECACHE_EVENT(1, "events/gauss.sc");
	m_usGaussSpin = PRECACHE_EVENT(1, "events/gaussspin.sc");
}

bool CGauss::GetWeaponInfo(WeaponInfo& info)
{
	info.Name = STRING(pev->classname);
	info.AttackModeInfo[0].AmmoType = "uranium";
	info.AttackModeInfo[0].MagazineSize = WEAPON_NOCLIP;
	info.Slot = 3;
	info.Position = 1;
	info.Id = WEAPON_GAUSS;
	info.Weight = GAUSS_WEIGHT;

	return true;
}

void CGauss::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "uranium") >= 0)
	{
		pPlayer->EmitSound(CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

bool CGauss::Deploy()
{
	m_pPlayer->m_flPlayAftershock = 0.0;
	return DefaultDeploy("models/v_gauss.mdl", "models/p_gauss.mdl", GAUSS_DRAW, "gauss");
}

void CGauss::Holster()
{
	SendStopEvent(true);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim(GAUSS_HOLSTER);
	m_fInAttack = 0;
}

void CGauss::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == WaterLevel::Head)
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) < 2)
	{
		PlayEmptySound();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	m_fPrimaryFire = true;

	m_pPlayer->AdjustAmmoByIndex(m_iPrimaryAmmoType, -2);

	StartFire();
	m_fInAttack = 0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
}

void CGauss::SecondaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == WaterLevel::Head)
	{
		if (m_fInAttack != 0)
		{
			m_pPlayer->EmitSoundDyn(CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG(0, 0x3f));
			// Have to send to the host as well because the client will predict the frame with m_fInAttack == 0
			SendStopEvent(true);
			SendWeaponAnim(GAUSS_IDLE);
			m_fInAttack = 0;
		}
		else
		{
			PlayEmptySound();
		}

		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		return;
	}

	if (m_fInAttack == 0)
	{
		if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) <= 0)
		{
			m_pPlayer->EmitSound(CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
			return;
		}

		m_fPrimaryFire = false;

		m_pPlayer->AdjustAmmoByIndex(m_iPrimaryAmmoType, -1); // take one ammo just to start the spin
		m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase();

		// spin up
		m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;

		SendWeaponAnim(GAUSS_SPINUP);
		m_fInAttack = 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		m_pPlayer->m_flStartCharge = gpGlobals->time;
		m_pPlayer->m_flAmmoStartCharge = UTIL_WeaponTimeBase() + GetFullChargeTime();

		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usGaussSpin, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 110, 0, 0, 0);

		m_iSoundState = SND_CHANGE_PITCH;
	}
	else if (m_fInAttack == 1)
	{
		if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
		{
			SendWeaponAnim(GAUSS_SPIN);
			m_fInAttack = 2;
		}
	}
	else
	{
		if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) > 0)
		{
			// during the charging process, eat one bit of ammo every once in a while
			if (UTIL_WeaponTimeBase() >= m_pPlayer->m_flNextAmmoBurn && m_pPlayer->m_flNextAmmoBurn != 1000)
			{
				m_pPlayer->AdjustAmmoByIndex(m_iPrimaryAmmoType, -1);

				if (g_Skill.GetValue("gauss_fast_ammo_use") != 0)
				{
					m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.1;
				}
				else
				{
					m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.3;
				}
			}
		}

		if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) <= 0)
		{
			// out of ammo! force the gun to fire
			StartFire();
			m_fInAttack = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
			return;
		}

		if (UTIL_WeaponTimeBase() >= m_pPlayer->m_flAmmoStartCharge)
		{
			// don't eat any more ammo after gun is fully charged.
			m_pPlayer->m_flNextAmmoBurn = 1000;
		}

		int pitch = (gpGlobals->time - m_pPlayer->m_flStartCharge) * (150 / GetFullChargeTime()) + 100;
		if (pitch > 250)
			pitch = 250;

		// WeaponsLogger->debug("{} {} {}", m_fInAttack, m_iSoundState, pitch);

		if (m_iSoundState == 0)
			WeaponsLogger->debug("sound state {}", m_iSoundState);

		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usGaussSpin, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, pitch, 0, (m_iSoundState == SND_CHANGE_PITCH) ? 1 : 0, 0);

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions

		m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;

		// m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		if (m_pPlayer->m_flStartCharge < gpGlobals->time - 10)
		{
			// Player charged up too long. Zap him.
			m_pPlayer->EmitSoundDyn(CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG(0, 0x3f));
			m_pPlayer->EmitSoundDyn(CHAN_ITEM, "weapons/electro6.wav", 1.0, ATTN_NORM, 0, 75 + RANDOM_LONG(0, 0x3f));

			m_fInAttack = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;

			SendStopEvent(false);

#ifndef CLIENT_DLL
			m_pPlayer->TakeDamage(World, World, 50, DMG_SHOCK);
			UTIL_ScreenFade(m_pPlayer, Vector(255, 128, 0), 2, 0.5, 128, FFADE_IN);
#endif
			SendWeaponAnim(GAUSS_IDLE);

			// Player may have been killed and this weapon dropped, don't execute any more code after this!
			return;
		}
	}
}

void CGauss::StartFire()
{
	float flDamage;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition(); // + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;

	if (gpGlobals->time - m_pPlayer->m_flStartCharge > GetFullChargeTime())
	{
		flDamage = 200;
	}
	else
	{
		flDamage = 200 * ((gpGlobals->time - m_pPlayer->m_flStartCharge) / GetFullChargeTime());
	}

	if (m_fPrimaryFire)
	{
		// fixed damage on primary attack
#ifdef CLIENT_DLL
		flDamage = 20;
#else
		flDamage = GetSkillFloat("plr_gauss"sv);
#endif
	}

	if (m_fInAttack != 3)
	{
		// WeaponsLogger->debug("Time:{} Damage:{}", gpGlobals->time - m_pPlayer->m_flStartCharge, flDamage);

#ifndef CLIENT_DLL
		float flZVel = m_pPlayer->pev->velocity.z;

		if (!m_fPrimaryFire)
		{
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * flDamage * 5;
		}

		if (g_Skill.GetValue("gauss_vertical_force") == 0)
		{
			// Don't pop you up into the air.
			m_pPlayer->pev->velocity.z = flZVel;
		}
#endif
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	}

	// time until aftershock 'static discharge' sound
	m_pPlayer->m_flPlayAftershock = gpGlobals->time + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.3, 0.8);

	Fire(vecSrc, vecAiming, flDamage);
}

void CGauss::Fire(Vector vecOrigSrc, Vector vecDir, float flDamage)
{
	m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;

	Vector vecSrc = vecOrigSrc;

#ifdef CLIENT_DLL
	if (m_fPrimaryFire == false)
		g_irunninggausspred = true;
#endif

	// The main firing event is sent unreliably so it won't be delayed.
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usGaussFire, 0.0, m_pPlayer->pev->origin, m_pPlayer->pev->angles, flDamage, 0.0, 0, 0, m_fPrimaryFire ? 1 : 0, 0);

	SendStopEvent(false);

	// WeaponsLogger->debug("{}\n{}", vecSrc, vecDest);
	// WeaponsLogger->debug("{} {}", tr.flFraction, flMaxFrac);

#ifndef CLIENT_DLL
	Vector vecDest = vecSrc + vecDir * 8192;
	edict_t* pentIgnore = m_pPlayer->edict();
	TraceResult tr, beam_tr;
	float flMaxFrac = 1.0;
	int nTotal = 0;
	bool fHasPunched = false;
	bool fFirstBeam = true;
	int nMaxHits = 10;

	while (flDamage > 10 && nMaxHits > 0)
	{
		nMaxHits--;

		// WeaponsLogger->debug(".");
		UTIL_TraceLine(vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr);

		if (0 != tr.fAllSolid)
			break;

		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity == nullptr)
			break;

		if (fFirstBeam)
		{
			m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
			fFirstBeam = false;

			nTotal += 26;
		}

		if (0 != pEntity->pev->takedamage)
		{
			ClearMultiDamage();
			pEntity->TraceAttack(m_pPlayer, flDamage, vecDir, &tr, DMG_BULLET);
			ApplyMultiDamage(m_pPlayer, m_pPlayer);
		}

		if (pEntity->ReflectGauss())
		{
			float n;

			pentIgnore = nullptr;

			n = -DotProduct(tr.vecPlaneNormal, vecDir);

			if (n < 0.5) // 60 degrees
			{
				// WeaponsLogger->debug("reflect {}", n);
				// reflect
				Vector r;

				r = 2.0 * tr.vecPlaneNormal * n + vecDir;
				flMaxFrac = flMaxFrac - tr.flFraction;
				vecDir = r;
				vecSrc = tr.vecEndPos + vecDir * 8;
				vecDest = vecSrc + vecDir * 8192;

				// explode a bit
				m_pPlayer->RadiusDamage(tr.vecEndPos, this, m_pPlayer, flDamage * n, DMG_BLAST);

				nTotal += 34;

				// lose energy
				if (n == 0)
					n = 0.1;
				flDamage = flDamage * (1 - n);
			}
			else
			{
				nTotal += 13;

				// limit it to one hole punch
				if (fHasPunched)
					break;
				fHasPunched = true;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if (!m_fPrimaryFire)
				{
					UTIL_TraceLine(tr.vecEndPos + vecDir * 8, vecDest, dont_ignore_monsters, pentIgnore, &beam_tr);
					if (0 == beam_tr.fAllSolid)
					{
						// trace backwards to find exit point
						UTIL_TraceLine(beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, pentIgnore, &beam_tr);

						float n = (beam_tr.vecEndPos - tr.vecEndPos).Length();

						if (n < flDamage)
						{
							if (n == 0)
								n = 1;
							flDamage -= n;

							// WeaponsLogger->debug("punch {}", n);
							nTotal += 21;

							// exit blast damage
							// m_pPlayer->RadiusDamage(beam_tr.vecEndPos + vecDir * 8, this, m_pPlayer, flDamage, DMG_BLAST);
							const float damage_radius = flDamage * g_Skill.GetValue("gauss_damage_radius");

							::RadiusDamage(beam_tr.vecEndPos + vecDir * 8, this, m_pPlayer, flDamage, damage_radius, DMG_BLAST);

							CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);

							nTotal += 53;

							vecSrc = beam_tr.vecEndPos + vecDir;
						}
					}
					else
					{
						// WeaponsLogger->debug("blocked {}", n);
						flDamage = 0;
					}
				}
				else
				{
					// WeaponsLogger->debug("blocked solid");

					flDamage = 0;
				}
			}
		}
		else
		{
			vecSrc = tr.vecEndPos + vecDir;
			pentIgnore = pEntity->edict();
		}
	}
#endif
	// WeaponsLogger->debug("{} bytes", nTotal);
}

void CGauss::WeaponIdle()
{
	ResetEmptySound();

	// play aftershock static discharge
	if (0 != m_pPlayer->m_flPlayAftershock && m_pPlayer->m_flPlayAftershock < gpGlobals->time)
	{
		switch (RANDOM_LONG(0, 3))
		{
		case 0:
			m_pPlayer->EmitSound(CHAN_WEAPON, "weapons/electro4.wav", RANDOM_FLOAT(0.7, 0.8), ATTN_NORM);
			break;
		case 1:
			m_pPlayer->EmitSound(CHAN_WEAPON, "weapons/electro5.wav", RANDOM_FLOAT(0.7, 0.8), ATTN_NORM);
			break;
		case 2:
			m_pPlayer->EmitSound(CHAN_WEAPON, "weapons/electro6.wav", RANDOM_FLOAT(0.7, 0.8), ATTN_NORM);
			break;
		case 3:
			break; // no sound
		}
		m_pPlayer->m_flPlayAftershock = 0.0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fInAttack != 0)
	{
		StartFire();
		m_fInAttack = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
	}
	else
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.5)
		{
			iAnim = GAUSS_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		}
		else if (flRand <= 0.75)
		{
			iAnim = GAUSS_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		}
		else
		{
			iAnim = GAUSS_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
		}

		return;
		SendWeaponAnim(iAnim);
	}
}

void CGauss::SendStopEvent(bool sendToHost)
{
	// This reliable event is used to stop the spinning sound
	// It's delayed by a fraction of second to make sure it is delayed by 1 frame on the client
	// It's sent reliably anyway, which could lead to other delays

	int flags = FEV_RELIABLE | FEV_GLOBAL;

	if (!sendToHost)
	{
		flags |= FEV_NOTHOST;
	}

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usGaussFire, 0.01, m_pPlayer->pev->origin, m_pPlayer->pev->angles, 0.0, 0.0, 0, 0, 0, 1);
}

void CGauss::GetWeaponData(weapon_data_t& data)
{
	data.iuser2 = m_fInAttack;
	data.fuser1 = m_pPlayer->m_flStartCharge;
}

void CGauss::SetWeaponData(const weapon_data_t& data)
{
	m_fInAttack = data.iuser2;
	m_pPlayer->m_flStartCharge = data.fuser1;
}

class CGaussAmmo : public CBasePlayerAmmo
{
public:
	void OnCreate() override
	{
		CBasePlayerAmmo::OnCreate();
		m_AmmoAmount = AMMO_URANIUMBOX_GIVE;
		m_AmmoName = MAKE_STRING("uranium");
		pev->model = MAKE_STRING("models/w_gaussammo.mdl");
	}
};

LINK_ENTITY_TO_CLASS(ammo_gaussclip, CGaussAmmo);
