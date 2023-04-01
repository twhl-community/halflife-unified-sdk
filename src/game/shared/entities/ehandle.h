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

#include <cassert>

class CBaseEntity;

/**
 *	@brief Base class for entity handles.
 *	Should not be used directly. Use EntityHandle instead.
 */
class BaseEntityHandle
{
public:
	edict_t* GetEdict() const;

	CBaseEntity* InternalGetEntity() const;
	void InternalSetEntity(CBaseEntity* entity);

private:
	edict_t* m_Edict = nullptr;
	int m_SerialNumber = 0;
};

/**
 *	@brief Safe way to point to CBaseEntities who may die between frames
 *	@tparam TBaseEntity A class type, either CBaseEntity or a type deriving from it
 *		that all entities assigned to this handle must derive from.
 */
template <typename TBaseEntity>
class EntityHandle : protected BaseEntityHandle
{
public:
	TBaseEntity* operator=(TBaseEntity* entity);

	/**
	 *	@brief Gets the entity this handle points to,
	 *	or @c nullptr if it does not point to a valid entity.
	 */
	TBaseEntity* Get() const;

	/**
	 *	@brief Gets the entity this handle points to as @c TOtherBaseEntity,
	 *	or @c nullptr if it does not point to a valid entity.
	 */
	template <typename TOtherBaseEntity>
	TOtherBaseEntity* Entity();

	operator TBaseEntity*();
	TBaseEntity* operator->();

	bool operator==(const EntityHandle<TBaseEntity>& other) { return Get() == other.Get(); }
	bool operator!=(const EntityHandle<TBaseEntity>& other) { return !(*this == other); }
};

template <typename TBaseEntity>
typename TBaseEntity* EntityHandle<TBaseEntity>::operator=(TBaseEntity* entity)
{
	InternalSetEntity(entity);
	return entity;
}

template <typename TBaseEntity>
typename TBaseEntity* EntityHandle<TBaseEntity>::Get() const
{
	return static_cast<TBaseEntity*>(InternalGetEntity());
}

template <typename TBaseEntity>
template <typename TOtherBaseEntity>
typename TOtherBaseEntity* EntityHandle<TBaseEntity>::Entity()
{
	auto entity = Get();

	// In debug builds this verifies that the dynamic type is the expected type.
	assert(!entity || dynamic_cast<TOtherBaseEntity*>(entity));

	return static_cast<TOtherBaseEntity*>(entity);
}

template <typename TBaseEntity>
EntityHandle<TBaseEntity>::operator TBaseEntity*()
{
	return Get();
}

template <typename TBaseEntity>
typename TBaseEntity* EntityHandle<TBaseEntity>::operator->()
{
	return Get();
}

using EHANDLE = EntityHandle<CBaseEntity>;
