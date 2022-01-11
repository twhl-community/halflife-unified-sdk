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

class CBaseEntity;

/**
*	@brief Safe way to point to CBaseEntities who may die between frames
*/
class EHANDLE
{
private:
	edict_t* m_pent = nullptr;
	int m_serialnumber = 0;

public:
	edict_t* Get();
	edict_t* Set(edict_t* pent);

	operator CBaseEntity*();

	CBaseEntity* operator=(CBaseEntity* pEntity);
	CBaseEntity* operator->();

	template <typename T>
	T* Entity()
	{
		return static_cast<T*>(operator CBaseEntity*());
	}
};
