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
#include "AmmoTypeSystem.h"
#include "CWeaponBox.h"

LINK_ENTITY_TO_CLASS(weaponbox, CWeaponBox);

BEGIN_DATAMAP(CWeaponBox)
DEFINE_ARRAY(m_rgAmmo, FIELD_INTEGER, MAX_AMMO_TYPES),
	DEFINE_ARRAY(m_rgiszAmmo, FIELD_STRING, MAX_AMMO_TYPES),
	DEFINE_ARRAY(m_rgpPlayerWeapons, FIELD_CLASSPTR, MAX_WEAPON_SLOTS),
	DEFINE_FIELD(m_cAmmoTypes, FIELD_INTEGER),
	DEFINE_FUNCTION(Kill),
	END_DATAMAP();

void CWeaponBox::OnCreate()
{
	CBaseEntity::OnCreate();

	pev->model = MAKE_STRING("models/w_weaponbox.mdl");
}

void CWeaponBox::Precache()
{
	PrecacheModel(STRING(pev->model));
}

bool CWeaponBox::KeyValue(KeyValueData* pkvd)
{
	if (m_cAmmoTypes < MAX_AMMO_TYPES)
	{
		PackAmmo(ALLOC_STRING(pkvd->szKeyName), atoi(pkvd->szValue));
		m_cAmmoTypes++; // count this new ammo type.

		return true;
	}
	else
	{
		CBaseEntity::Logger->error("WeaponBox too full! only {} ammotypes allowed", MAX_AMMO_TYPES);
	}

	return false;
}

void CWeaponBox::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	SetSize(g_vecZero, g_vecZero);

	SetModel(STRING(pev->model));
}

void CWeaponBox::UpdateOnRemove()
{
	RemoveWeapons();
	CBaseEntity::UpdateOnRemove();
}

void CWeaponBox::RemoveWeapons()
{
	CBasePlayerWeapon* weapon;

	// destroy the weapons
	for (int i = 0; i < MAX_WEAPON_SLOTS; ++i)
	{
		weapon = m_rgpPlayerWeapons[i];

		while (weapon)
		{
			weapon->SetThink(&CBasePlayerWeapon::SUB_Remove);
			weapon->pev->nextthink = gpGlobals->time + 0.1;
			weapon = weapon->m_pNext;
		}
	}
}

void CWeaponBox::Kill()
{
	RemoveWeapons();

	// remove the box
	UTIL_Remove(this);
}

void CWeaponBox::Touch(CBaseEntity* pOther)
{
	if ((pev->flags & FL_ONGROUND) == 0)
	{
		return;
	}

	if (!pOther->IsPlayer())
	{
		// only players may touch a weaponbox.
		return;
	}

	if (!pOther->IsAlive())
	{
		// no dead guys.
		return;
	}

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;
	int i;

	// dole out ammo
	for (i = 0; i < MAX_AMMO_TYPES; i++)
	{
		if (!FStringNull(m_rgiszAmmo[i]))
		{
			// there's some ammo of this type.
			pPlayer->GiveAmmo(m_rgAmmo[i], STRING(m_rgiszAmmo[i]));

			// Logger->trace("Gave {} rounds of {}", m_rgAmmo[i], STRING(m_rgiszAmmo[i]));

			// now empty the ammo from the weaponbox since we just gave it to the player
			m_rgiszAmmo[i] = string_t::Null;
			m_rgAmmo[i] = 0;
		}
	}

	// go through my weapons and try to give the usable ones to the player.
	// it's important the the player be given ammo first, so the weapons code doesn't refuse
	// to deploy a better weapon that the player may pick up because he has no ammo for it.
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		if (m_rgpPlayerWeapons[i])
		{
			CBasePlayerWeapon* weapon;

			// have at least one weapon in this slot
			while (m_rgpPlayerWeapons[i])
			{
				// Logger->debug("trying to give {}", STRING(m_rgpPlayerWeapons[i]->pev->classname));

				weapon = m_rgpPlayerWeapons[i];
				m_rgpPlayerWeapons[i] = m_rgpPlayerWeapons[i]->m_pNext; // unlink this weapon from the box

				if (pPlayer->AddPlayerWeapon(weapon) == ItemAddResult::Added)
				{
					weapon->AttachToPlayer(pPlayer);
				}
			}
		}
	}

	pOther->EmitSound(CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
	SetTouch(nullptr);
	UTIL_Remove(this);
}

bool CWeaponBox::PackWeapon(CBasePlayerWeapon* weapon)
{
	// is one of these weapons already packed in this box?
	if (HasWeapon(weapon))
	{
		return false; // box can only hold one of each weapon type
	}

	if (weapon->m_pPlayer)
	{
		if (!weapon->m_pPlayer->RemovePlayerWeapon(weapon))
		{
			// failed to unhook the weapon from the player!
			return false;
		}
	}

	int iWeaponSlot = weapon->iItemSlot();

	if (m_rgpPlayerWeapons[iWeaponSlot])
	{
		// there's already one weapon in this slot, so link this into the slot's column
		weapon->m_pNext = m_rgpPlayerWeapons[iWeaponSlot];
		m_rgpPlayerWeapons[iWeaponSlot] = weapon;
	}
	else
	{
		// first weapon we have for this slot
		m_rgpPlayerWeapons[iWeaponSlot] = weapon;
		weapon->m_pNext = nullptr;
	}

	weapon->m_RespawnDelay = ITEM_NEVER_RESPAWN_DELAY;
	weapon->pev->movetype = MOVETYPE_NONE;
	weapon->pev->solid = SOLID_NOT;
	weapon->pev->effects = EF_NODRAW;
	weapon->pev->modelindex = 0;
	weapon->pev->model = string_t::Null;
	weapon->pev->owner = edict();
	weapon->SetThink(nullptr); // crowbar may be trying to swing again, etc.
	weapon->SetTouch(nullptr);
	weapon->m_pPlayer = nullptr;

	// Logger->debug("packed {}", STRING(pWeapon->pev->classname));

	return true;
}

bool CWeaponBox::PackAmmo(string_t iszName, int iCount)
{
	if (FStringNull(iszName))
	{
		// error here
		Logger->error("NULL String in PackAmmo!");
		return false;
	}

	const auto type = g_AmmoTypes.GetByName(STRING(iszName));

	if (type)
	{
		if (iCount == RefillAllAmmoAmount)
		{
			iCount = type->MaximumCapacity;
		}

		if (iCount > 0)
		{
			// Logger->debug("Packed {} rounds of {}", iCount, STRING(iszName));
			GiveAmmo(iCount, type);
			return true;
		}
	}

	return false;
}

int CWeaponBox::GiveAmmo(int iCount, const char* szName, int* pIndex)
{
	const auto type = g_AmmoTypes.GetByName(szName);

	if (!type)
	{
		CBasePlayerWeapon::WeaponsLogger->error("CWeaponBox::GiveAmmo: Unknown ammo type \"{}\"", szName);
		return -1;
	}

	return GiveAmmo(iCount, type, pIndex);
}

int CWeaponBox::GiveAmmo(int iCount, const AmmoType* type, int* pIndex)
{
	if (!type)
	{
		return -1;
	}

	int i;

	for (i = 1; i < MAX_AMMO_TYPES && !FStringNull(m_rgiszAmmo[i]); i++)
	{
		if (type->Name.comparei(STRING(m_rgiszAmmo[i])) == 0)
		{
			if (pIndex)
				*pIndex = i;

			int iAdd = std::min(iCount, type->MaximumCapacity - m_rgAmmo[i]);
			if (iCount == 0 || iAdd > 0)
			{
				m_rgAmmo[i] += iAdd;

				return i;
			}
			return -1;
		}
	}
	if (i < MAX_AMMO_TYPES)
	{
		if (pIndex)
			*pIndex = i;

		m_rgiszAmmo[i] = ALLOC_STRING(type->Name.c_str());
		m_rgAmmo[i] = iCount;

		return i;
	}
	CBaseEntity::Logger->error("out of named ammo slots");
	return i;
}

bool CWeaponBox::HasWeapon(CBasePlayerWeapon* checkWeapon)
{
	CBasePlayerWeapon* weapon = m_rgpPlayerWeapons[checkWeapon->iItemSlot()];

	while (weapon)
	{
		if (weapon->ClassnameIs(STRING(checkWeapon->pev->classname)))
		{
			return true;
		}
		weapon = weapon->m_pNext;
	}

	return false;
}

bool CWeaponBox::IsEmpty()
{
	int i;

	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		if (m_rgpPlayerWeapons[i])
		{
			return false;
		}
	}

	for (i = 0; i < MAX_AMMO_TYPES; i++)
	{
		if (!FStringNull(m_rgiszAmmo[i]))
		{
			// still have a bit of this type of ammo
			return false;
		}
	}

	return true;
}

void CWeaponBox::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 16);
}
