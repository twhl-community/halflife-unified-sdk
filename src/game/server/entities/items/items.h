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

#pragma once

#include "items/CBaseItem.h"

class CItem : public CBaseItem
{
public:
	ItemType GetType() const override final { return ItemType::Consumable; }

	void Accept(IItemVisitor& visitor) override
	{
		visitor.Visit(this);
	}

	ItemAddResult Apply(CBasePlayer* player) override
	{
		return AddItem(player) ? ItemAddResult::Added : ItemAddResult::NotAdded;
	}

protected:
	virtual bool AddItem(CBasePlayer* player) = 0;
};
