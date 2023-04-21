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
#include "CSatchel.h"

LINK_ENTITY_TO_CLASS(weapon_satchel, CSatchel);

BEGIN_DATAMAP(CSatchel)
DEFINE_FIELD(m_chargeReady, FIELD_INTEGER),
	END_DATAMAP();

void CSatchel::OnCreate()
{
	CBasePlayerWeapon::OnCreate();
	m_iId = WEAPON_SATCHEL;
	m_iDefaultAmmo = SATCHEL_DEFAULT_GIVE;
	m_WorldModel = pev->model = MAKE_STRING("models/w_satchel.mdl");
}

bool CSatchel::AddDuplicate(CBasePlayerWeapon* original)
{
	CSatchel* pSatchel;

	if (UTIL_IsMultiplayer())
	{
		pSatchel = (CSatchel*)original;

		if (pSatchel->m_chargeReady != 0)
		{
			// player has some satchels deployed. Refuse to add more.
			return false;
		}
	}

	return CBasePlayerWeapon::AddDuplicate(original);
}

void CSatchel::AddToPlayer(CBasePlayer* pPlayer)
{
	m_chargeReady = 0; // this satchel charge weapon now forgets that any satchels are deployed by it.
	CBasePlayerWeapon::AddToPlayer(pPlayer);
}

void CSatchel::Precache()
{
	CBasePlayerWeapon::Precache();
	PrecacheModel("models/v_satchel.mdl");
	PrecacheModel("models/v_satchel_radio.mdl");
	PrecacheModel(STRING(m_WorldModel));
	PrecacheModel("models/p_satchel.mdl");
	PrecacheModel("models/p_satchel_radio.mdl");

	UTIL_PrecacheOther("monster_satchel");
}

bool CSatchel::GetWeaponInfo(WeaponInfo& info)
{
	info.Name = STRING(pev->classname);
	info.AttackModeInfo[0].AmmoType = "Satchel Charge";
	info.AttackModeInfo[0].MagazineSize = WEAPON_NOCLIP;
	info.Slot = 4;
	info.Position = 1;
	info.Flags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	info.Id = WEAPON_SATCHEL;
	info.Weight = SATCHEL_WEIGHT;

	return true;
}

bool CSatchel::IsUseable()
{
	if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) > 0)
	{
		// player is carrying some satchels
		return true;
	}

	if (m_chargeReady != 0)
	{
		// player isn't carrying any satchels, but has some out
		return true;
	}

	return false;
}

bool CSatchel::CanDeploy()
{
	if (m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) > 0)
	{
		// player is carrying some satchels
		return true;
	}

	if (m_chargeReady != 0)
	{
		// player isn't carrying any satchels, but has some out
		return true;
	}

	return false;
}

bool CSatchel::Deploy()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	// m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	bool result;

	if (0 != m_chargeReady)
		result = DefaultDeploy("models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl", SATCHEL_RADIO_DRAW, "hive");
	else
		result = DefaultDeploy("models/v_satchel.mdl", "models/p_satchel.mdl", SATCHEL_DRAW, "trip");

	if (result)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	}

	return result;
}

void CSatchel::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (0 != m_chargeReady)
	{
		SendWeaponAnim(SATCHEL_RADIO_HOLSTER);
	}
	else
	{
		SendWeaponAnim(SATCHEL_DROP);
	}
	m_pPlayer->EmitSound(CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);

	if (0 == m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType) && 0 == m_chargeReady)
	{
		m_pPlayer->ClearWeaponBit(m_iId);
		SetThink(&CSatchel::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CSatchel::PrimaryAttack()
{
	switch (m_chargeReady)
	{
	case 0:
	{
		Throw();
	}
	break;
	case 1:
	{
		SendWeaponAnim(SATCHEL_RADIO_FIRE);

		CBaseEntity* pSatchel = nullptr;

		while ((pSatchel = UTIL_FindEntityInSphere(pSatchel, m_pPlayer->pev->origin, 4096)) != nullptr)
		{
			if (pSatchel->ClassnameIs("monster_satchel"))
			{
				if (pSatchel->GetOwner() == m_pPlayer)
				{
					pSatchel->Use(m_pPlayer, m_pPlayer, USE_ON, 0);
					m_chargeReady = 2;
				}
			}
		}

		m_chargeReady = 2;
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		break;
	}

	case 2:
		// we're reloading, don't allow fire
		{
		}
		break;
	}
}

void CSatchel::SecondaryAttack()
{
	if (m_chargeReady != 2)
	{
		Throw();
	}
}

void CSatchel::Throw()
{
	if (0 != m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType))
	{
#ifndef CLIENT_DLL
		Vector vecSrc = m_pPlayer->pev->origin;
		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;

		CBaseEntity* pSatchel = Create("monster_satchel", vecSrc, Vector(0, 0, 0), m_pPlayer);
		pSatchel->pev->velocity = vecThrow;
		pSatchel->pev->avelocity.y = 400;

		SetWeaponModels("models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl");
#else
		LoadVModel("models/v_satchel_radio.mdl", m_pPlayer);
#endif

		SendWeaponAnim(SATCHEL_RADIO_DRAW);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		m_chargeReady = 1;

		m_pPlayer->AdjustAmmoByIndex(m_iPrimaryAmmoType, -1);

		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
}

void CSatchel::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	switch (m_chargeReady)
	{
	case 0:
		SendWeaponAnim(SATCHEL_FIDGET1);
		// use tripmine animations
		strcpy(m_pPlayer->m_szAnimExtention, "trip");
		break;
	case 1:
		SendWeaponAnim(SATCHEL_RADIO_FIDGET1);
		// use hivehand animations
		strcpy(m_pPlayer->m_szAnimExtention, "hive");
		break;
	case 2:
		if (0 == m_pPlayer->GetAmmoCountByIndex(m_iPrimaryAmmoType))
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

#ifndef CLIENT_DLL
		SetWeaponModels("models/v_satchel.mdl", "models/p_satchel.mdl");
#else
		LoadVModel("models/v_satchel.mdl", m_pPlayer);
#endif

		SendWeaponAnim(SATCHEL_DRAW);

		// use tripmine animations
		strcpy(m_pPlayer->m_szAnimExtention, "trip");

		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_chargeReady = 0;
		break;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}

void CSatchel::GetWeaponData(weapon_data_t& data)
{
	data.iuser1 = m_chargeReady;
}

void CSatchel::SetWeaponData(const weapon_data_t& data)
{
	m_chargeReady = data.iuser1;
}
