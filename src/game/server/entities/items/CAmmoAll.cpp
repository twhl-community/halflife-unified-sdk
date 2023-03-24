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

class CAmmoAll final : public CBasePlayerAmmo
{
public:
	void OnCreate() override
	{
		CBasePlayerAmmo::OnCreate();
		m_AmmoName = MAKE_STRING("all"); // Just give it a name, doesn't need to be a valid one.
	}

	bool AddAmmo(CBasePlayer* player) override
	{
		bool addedAmmo = false;

		for (int i = 0; i < g_AmmoTypes.GetCount(); ++i)
		{
			auto type = g_AmmoTypes.GetByIndex(i + 1);

			addedAmmo |= DefaultGiveAmmo(player, m_AmmoAmount, type->Name.c_str(), false);
		}

		if (addedAmmo)
		{
			PlayPickupSound(DefaultItemPickupSound);
		}

		return addedAmmo;
	}
};

LINK_ENTITY_TO_CLASS(ammo_all, CAmmoAll);
