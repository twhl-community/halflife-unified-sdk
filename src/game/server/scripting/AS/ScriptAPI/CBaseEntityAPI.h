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

#include <concepts>
#include <string>

#include <angelscript.h>

#include "cbase.h"

namespace scripting
{
template <typename T>
bool CBaseEntity_KeyValue(T* entity, const std::string& key, const std::string& value)
{
	KeyValueData kvd{
		STRING(entity->pev->classname),
		key.c_str(),
		value.c_str(),
		0};

	return entity->KeyValue(&kvd);
}

inline void CBaseEntity_PrecacheModel(CBaseEntity* entity, const std::string& value)
{
	entity->PrecacheModel(STRING(ALLOC_STRING(value.c_str())));
}

inline void CBaseEntity_SetModel(CBaseEntity* entity, const std::string& value)
{
	entity->SetModel(STRING(ALLOC_STRING(value.c_str())));
}

inline std::string CBaseEntity_GetClassName(CBaseEntity* entity)
{
	return entity->GetClassname();
}

inline int CBaseEntity_GetSolid(CBaseEntity* entity)
{
	return entity->pev->solid;
}

inline void CBaseEntity_SetSolid(CBaseEntity* entity, int value)
{
	entity->pev->solid = value;
}

inline int CBaseEntity_GetMovetype(CBaseEntity* entity)
{
	return entity->pev->movetype;
}

inline void CBaseEntity_SetMovetype(CBaseEntity* entity, int value)
{
	entity->pev->movetype = value;
}

inline std::string CBaseEntity_GetModelName(CBaseEntity* entity)
{
	return entity->GetModelName();
}

inline float CBaseEntity_GetNextThink(CBaseEntity* entity)
{
	return entity->pev->nextthink;
}

inline void CBaseEntity_SetNextThink(CBaseEntity* entity, float value)
{
	entity->pev->nextthink = value;
}

/**
 *	@brief Registers the @c CBaseEntity members for direct access.
 */
template <std::derived_from<CBaseEntity> T>
void RegisterCBaseEntityMembers(asIScriptEngine& engine, const char* name)
{
	engine.RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOCOUNT);

	// Don't register OnCreate, OnDestroy, etc because those are called by internal systems only.

	engine.RegisterObjectMethod(name, "bool KeyValue(const string& in key, const string& in value)",
		asFUNCTION(CBaseEntity_KeyValue<T>), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "void Precache()",
		asMETHOD(T, Precache), asCALL_THISCALL);

	engine.RegisterObjectMethod(name, "void Spawn()",
		asMETHOD(T, Spawn), asCALL_THISCALL);

	engine.RegisterObjectMethod(name, "void PrecacheModel(const string& in value)",
		asFUNCTION(CBaseEntity_PrecacheModel), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "void SetModel(const string& in value)",
		asFUNCTION(CBaseEntity_SetModel), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "void SetSize(const Vector& in min, const Vector& in max)",
		asMETHOD(T, SetSize), asCALL_THISCALL);

	engine.RegisterObjectMethod(name, "bool IsAlive()",
		asMETHOD(T, IsAlive), asCALL_THISCALL);

	// These are non-virtual functions that are overridden in derived classes.
	// They'll use the most derived version because we're using type T here to register them.
	engine.RegisterObjectMethod(name, "void SUB_UseTargets(CBaseEntity@ pActivator, USE_TYPE useType, float value)",
		asMETHOD(T, SUB_UseTargets), asCALL_THISCALL);

	// Properties
	engine.RegisterObjectMethod(name, "string get_ClassName() const property",
		asFUNCTION(CBaseEntity_GetClassName), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "SOLID get_solid() const property",
		asFUNCTION(CBaseEntity_GetSolid), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "void set_solid(SOLID value) property",
		asFUNCTION(CBaseEntity_SetSolid), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "MOVETYPE get_movetype() const property",
		asFUNCTION(CBaseEntity_GetMovetype), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "void set_movetype(MOVETYPE value) property",
		asFUNCTION(CBaseEntity_SetMovetype), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "string get_modelname() const property",
		asFUNCTION(CBaseEntity_GetModelName), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "float get_nextthink() const property",
		asFUNCTION(CBaseEntity_GetNextThink), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "void set_nextthink(float value) property",
		asFUNCTION(CBaseEntity_SetNextThink), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "dictionary@ get_UserData() property",
		asMETHOD(T, GetUserData), asCALL_THISCALL);
}

template <typename T>
void BaseEntity_OnCreate(T* entity)
{
	return entity->T::OnCreate();
}

template <typename T>
bool BaseEntity_KeyValue(T* entity, const std::string& key, const std::string& value)
{
	KeyValueData kvd{
		STRING(entity->pev->classname),
		key.c_str(),
		value.c_str(),
		0};

	return entity->T::KeyValue(&kvd);
}

template <typename T>
void BaseEntity_Precache(T* entity)
{
	return entity->T::Precache();
}

template <typename T>
void BaseEntity_Spawn(T* entity)
{
	return entity->T::Spawn();
}

template <typename T>
bool BaseEntity_IsAlive(T* entity)
{
	return entity->T::IsAlive();
}

/**
 *	@brief Registers the @c CBaseEntity methods for base class access.
 */
template <std::derived_from<CBaseEntity> T>
void RegisterCBaseEntityBaseClassMethods(asIScriptEngine& engine, const char* name)
{
	engine.RegisterObjectType(name, 0, asOBJ_REF | asOBJ_NOCOUNT);

	engine.RegisterObjectMethod(name, "void OnCreate()",
		asFUNCTION(BaseEntity_OnCreate<T>), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "bool KeyValue(const string& in key, const string& in value)",
		asFUNCTION(BaseEntity_KeyValue<T>), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "void Precache()",
		asFUNCTION(BaseEntity_Precache<T>), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "void Spawn()",
		asFUNCTION(BaseEntity_Spawn<T>), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "bool IsAlive()",
		asFUNCTION(BaseEntity_IsAlive<T>), asCALL_CDECL_OBJFIRST);
}

inline void RegisterCBaseEntity(asIScriptEngine& engine)
{
	RegisterCBaseEntityMembers<CBaseEntity>(engine, "CBaseEntity");
	RegisterCBaseEntityBaseClassMethods<CBaseEntity>(engine, "BaseEntity");
}
}
