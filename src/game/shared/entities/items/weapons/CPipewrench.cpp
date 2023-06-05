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
#include "CPipewrench.h"

#define PIPEWRENCH_BODYHIT_VOLUME 128
#define PIPEWRENCH_WALLHIT_VOLUME 512

BEGIN_DATAMAP(CPipewrench)
DEFINE_FIELD(m_flBigSwingStart, FIELD_TIME),
	DEFINE_FIELD(m_iSwing, FIELD_INTEGER),
	DEFINE_FIELD(m_iSwingMode, FIELD_INTEGER),
	DEFINE_FUNCTION(SwingAgain),
	DEFINE_FUNCTION(Smack),
	DEFINE_FUNCTION(BigSwing),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(weapon_pipewrench, CPipewrench);

void CPipewrench::OnCreate()
{
	BaseClass::OnCreate();
	m_iId = WEAPON_PIPEWRENCH;
	SetMagazine1(WEAPON_NOCLIP);
	m_WorldModel = pev->model = MAKE_STRING("models/w_pipe_wrench.mdl");
}

void CPipewrench::Precache()
{
	CBasePlayerWeapon::Precache();
	PrecacheModel("models/v_pipe_wrench.mdl");
	PrecacheModel(STRING(m_WorldModel));
	PrecacheModel("models/p_pipe_wrench.mdl");
	// Shepard - The commented sounds below are unused
	// in Opposing Force, if you wish to use them,
	// uncomment all the appropriate lines.
	/*PrecacheSound("weapons/pwrench_big_hit1.wav");
	PrecacheSound("weapons/pwrench_big_hit2.wav");*/
	PrecacheSound("weapons/pwrench_big_hitbod1.wav");
	PrecacheSound("weapons/pwrench_big_hitbod2.wav");
	PrecacheSound("weapons/pwrench_big_miss.wav");
	PrecacheSound("weapons/pwrench_hit1.wav");
	PrecacheSound("weapons/pwrench_hit2.wav");
	PrecacheSound("weapons/pwrench_hitbod1.wav");
	PrecacheSound("weapons/pwrench_hitbod2.wav");
	PrecacheSound("weapons/pwrench_hitbod3.wav");
	PrecacheSound("weapons/pwrench_miss1.wav");
	PrecacheSound("weapons/pwrench_miss2.wav");

	m_usPipewrench = PRECACHE_EVENT(1, "events/pipewrench.sc");
}

bool CPipewrench::Deploy()
{
	return DefaultDeploy("models/v_pipe_wrench.mdl", "models/p_pipe_wrench.mdl", PIPEWRENCH_DRAW, "crowbar");
}

void CPipewrench::Holster()
{
	// Cancel any swing in progress.
	m_iSwingMode = SWING_NONE;
	SetThink(nullptr);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(PIPEWRENCH_HOLSTER);
}

void CPipewrench::PrimaryAttack()
{
	if (m_iSwingMode == SWING_NONE && !Swing(true))
	{
#ifndef CLIENT_DLL
		SetThink(&CPipewrench::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
#endif
	}
}

void CPipewrench::SecondaryAttack()
{
	if (m_iSwingMode != SWING_START_BIG)
	{
		SendWeaponAnim(PIPEWRENCH_BIG_SWING_START);
		m_flBigSwingStart = gpGlobals->time;
	}

	m_iSwingMode = SWING_START_BIG;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.1);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2;
}

void CPipewrench::Smack()
{
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}

void CPipewrench::SwingAgain()
{
	Swing(false);
}

bool CPipewrench::Swing(const bool bFirst)
{
	bool bDidHit = false;

	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &tr);
		if (tr.flFraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer);
			vecEnd = tr.vecEndPos; // This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	if (bFirst)
	{
		PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usPipewrench,
			0.0, g_vecZero, g_vecZero, 0, 0, 0,
			0.0, 0, static_cast<int>(tr.flFraction < 1));
	}


	if (tr.flFraction >= 1.0)
	{
		if (bFirst)
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay(0.75);
			m_flNextSecondaryAttack = GetNextAttackDelay(0.75);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;

			// Shepard - In Opposing Force, the "miss" sound is
			// played twice (maybe it's a mistake from Gearbox or
			// an intended feature), if you only want a single
			// sound, comment this "switch" or the one in the
			// event (EV_Pipewrench)
			/*
			switch ( ((m_iSwing++) % 1) )
			{
			case 0: m_pPlayer->EmitSound(CHAN_ITEM, "weapons/pwrench_miss1.wav", 1, ATTN_NORM); break;
			case 1: m_pPlayer->EmitSound(CHAN_ITEM, "weapons/pwrench_miss2.wav", 1, ATTN_NORM); break;
			}
			*/

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
	}
	else
	{
		switch (((m_iSwing++) % 2) + 1)
		{
		case 0:
			SendWeaponAnim(PIPEWRENCH_ATTACK1HIT);
			break;
		case 1:
			SendWeaponAnim(PIPEWRENCH_ATTACK2HIT);
			break;
		case 2:
			SendWeaponAnim(PIPEWRENCH_ATTACK3HIT);
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		bDidHit = true;
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity)
		{
			ClearMultiDamage();

			if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase()) || g_Skill.GetValue("pipewrench_full_damage") != 0)
			{
				// first swing does full damage
				pEntity->TraceAttack(m_pPlayer, GetSkillFloat("plr_pipewrench"sv), gpGlobals->v_forward, &tr, DMG_CLUB);
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack(m_pPlayer, GetSkillFloat("plr_pipewrench"sv) / 2, gpGlobals->v_forward, &tr, DMG_CLUB);
			}

			ApplyMultiDamage(m_pPlayer, m_pPlayer);
		}

#endif

		if (GetSkillFloat("chainsaw_melee") == 0)
		{
			m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
			m_flNextSecondaryAttack = GetNextAttackDelay(0.5);
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;

#ifndef CLIENT_DLL

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool bHitWorld = true;

		if (pEntity)
		{
			if (pEntity->Classify() != ENTCLASS_NONE && !pEntity->IsMachine())
			{
				// play thwack or smack sound
				switch (RANDOM_LONG(0, 2))
				{
				case 0:
					m_pPlayer->EmitSound(CHAN_ITEM, "weapons/pwrench_hitbod1.wav", 1, ATTN_NORM);
					break;
				case 1:
					m_pPlayer->EmitSound(CHAN_ITEM, "weapons/pwrench_hitbod2.wav", 1, ATTN_NORM);
					break;
				case 2:
					m_pPlayer->EmitSound(CHAN_ITEM, "weapons/pwrench_hitbod3.wav", 1, ATTN_NORM);
					break;
				}
				m_pPlayer->m_iWeaponVolume = PIPEWRENCH_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return true;
				else
					flVol = 0.1;

				bHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (bHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play pipe wrench strike
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				m_pPlayer->EmitSoundDyn(CHAN_ITEM, "weapons/pwrench_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				m_pPlayer->EmitSoundDyn(CHAN_ITEM, "weapons/pwrench_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * PIPEWRENCH_WALLHIT_VOLUME;

		SetThink(&CPipewrench::Smack);
		pev->nextthink = gpGlobals->time + 0.2;
#endif

		if (GetSkillFloat("chainsaw_melee") != 0)
		{
			m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
			m_flNextSecondaryAttack = GetNextAttackDelay(0.5);
		}
	}
	return bDidHit;
}

void CPipewrench::BigSwing()
{
	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, m_pPlayer->edict(), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &tr);
		if (tr.flFraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer);
			vecEnd = tr.vecEndPos; // This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usPipewrench,
		0.0, g_vecZero, g_vecZero, 0, 0, 0,
		0.0, 1, static_cast<int>(tr.flFraction < 1));

	EmitSoundDyn(CHAN_WEAPON, "weapons/pwrench_big_miss.wav", VOL_NORM, ATTN_NORM, 0, 94 + RANDOM_LONG(0, 15));

	if (tr.flFraction >= 1.0)
	{
		// miss
		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = GetNextAttackDelay(1.0);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;

		SendWeaponAnim(PIPEWRENCH_BIG_SWING_MISS);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	}
	else
	{
		SendWeaponAnim(PIPEWRENCH_BIG_SWING_HIT);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity)
		{
			ClearMultiDamage();

			float flDamage = (gpGlobals->time - m_flBigSwingStart) * GetSkillFloat("plr_pipewrench"sv) + 25.0f;
			if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase()) || g_Skill.GetValue("pipewrench_full_damage") != 0)
			{
				// first swing does full damage
				pEntity->TraceAttack(m_pPlayer, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB);
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack(m_pPlayer, flDamage / 2, gpGlobals->v_forward, &tr, DMG_CLUB);
			}
			ApplyMultiDamage(m_pPlayer, m_pPlayer);
		}

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool bHitWorld = true;

		if (pEntity)
		{
			if (pEntity->Classify() != ENTCLASS_NONE && !pEntity->IsMachine())
			{
				// play thwack or smack sound
				switch (RANDOM_LONG(0, 1))
				{
				case 0:
					m_pPlayer->EmitSound(CHAN_ITEM, "weapons/pwrench_big_hitbod1.wav", 1, ATTN_NORM);
					break;
				case 1:
					m_pPlayer->EmitSound(CHAN_ITEM, "weapons/pwrench_big_hitbod2.wav", 1, ATTN_NORM);
					break;
				}
				m_pPlayer->m_iWeaponVolume = PIPEWRENCH_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return;
				else
					flVol = 0.1;

				bHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (bHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play pipe wrench strike
			// Shepard - The commented sounds below are unused
			// in Opposing Force, if you wish to use them,
			// uncomment all the appropriate lines.
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				m_pPlayer->EmitSoundDyn(CHAN_ITEM, "weapons/pwrench_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				// m_pPlayer->EmitSoundDyn(CHAN_ITEM, "weapons/pwrench_big_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			case 1:
				m_pPlayer->EmitSoundDyn(CHAN_ITEM, "weapons/pwrench_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				// m_pPlayer->EmitSoundDyn(CHAN_ITEM, "weapons/pwrench_big_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * PIPEWRENCH_WALLHIT_VOLUME;

		// Shepard - The original Opposing Force's pipe wrench
		// doesn't make a bullet hole decal when making a big
		// swing. If you want that decal, just uncomment the
		// 2 lines below.
		/*SetThink( &CPipewrench::Smack );
		SetNextThink( UTIL_WeaponTimeBase() + 0.2 );*/
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = GetNextAttackDelay(1.0);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	}
}

void CPipewrench::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_iSwingMode == SWING_START_BIG)
	{
		if (gpGlobals->time > m_flBigSwingStart + 1)
		{
			m_iSwingMode = SWING_DOING_BIG;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.2;
			SetThink(&CPipewrench::BigSwing);
			pev->nextthink = gpGlobals->time + 0.1;
		}
	}
	else
	{
		m_iSwingMode = SWING_NONE;
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		if (flRand <= 0.5)
		{
			iAnim = PIPEWRENCH_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
		}
		else if (flRand <= 0.9)
		{
			iAnim = PIPEWRENCH_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
		}
		else
		{
			iAnim = PIPEWRENCH_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
		}

		SendWeaponAnim(iAnim);
	}
}

void CPipewrench::GetWeaponData(weapon_data_t& data)
{
	BaseClass::GetWeaponData(data);

	data.m_fInSpecialReload = static_cast<int>(m_iSwingMode);
}

void CPipewrench::SetWeaponData(const weapon_data_t& data)
{
	BaseClass::SetWeaponData(data);

	m_iSwingMode = data.m_fInSpecialReload;
}

bool CPipewrench::GetWeaponInfo(WeaponInfo& info)
{
	info.Name = STRING(pev->classname);
	info.AttackModeInfo[0].MagazineSize = WEAPON_NOCLIP;
	info.Slot = 0;
	info.Position = 1;
	info.Id = WEAPON_PIPEWRENCH;
	info.Weight = PIPEWRENCH_WEIGHT;

	return true;
}
