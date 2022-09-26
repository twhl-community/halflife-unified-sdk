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

#include <angelscript.h>

#include "cbase.h"

#include "scripting/AS/as_utils.h"

class CBaseEntity;

namespace scripting
{
/**
*	@brief Interface to a custom entity.
*/
class ICustomEntity
{
public:
	virtual ~ICustomEntity() = default;

	/**
	*	@brief Gets the underlying entity.
	*/
	virtual CBaseEntity* GetEntityPointer() = 0;

	virtual void SetScriptObject(as::UniquePtr<asIScriptObject>&& object) = 0;

	virtual void SetThinkFunction(asIScriptFunction* function) = 0;
	virtual void SetTouchFunction(asIScriptFunction* function) = 0;
	virtual void SetBlockedFunction(asIScriptFunction* function) = 0;
	virtual void SetUseFunction(asIScriptFunction* function) = 0;
};
}
