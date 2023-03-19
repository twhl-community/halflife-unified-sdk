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

class CAmmoGeneric final : public CBasePlayerAmmo
{
	DECLARE_CLASS(CAmmoGeneric, CBasePlayerAmmo);
	DECLARE_DATAMAP();

public:
	bool KeyValue(KeyValueData* pkvd) override
	{
		if (FStrEq(pkvd->szKeyName, "ammo_name"))
		{
			m_AmmoName = ALLOC_STRING(pkvd->szValue);
			return true;
		}

		return CBasePlayerAmmo::KeyValue(pkvd);
	}
};

LINK_ENTITY_TO_CLASS(ammo_generic, CAmmoGeneric);

BEGIN_DATAMAP(CAmmoGeneric)
DEFINE_FIELD(m_AmmoName, FIELD_STRING),
	END_DATAMAP();