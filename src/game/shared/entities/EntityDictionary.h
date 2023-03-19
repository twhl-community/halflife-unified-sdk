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
#include <concepts>
#include <ranges>
#include <string_view>
#include <unordered_map>

#include "CBaseEntity.h"
#include "weapons.h"
#include "items/CBaseItem.h"

/**
 *	@file
 *
 *	Entity dictionary that allows classname-based lookup and type-safe construction.
 *	Entities should be registered only using the @see LINK_ENTITY_TO_CLASS macro.
 */

template <typename TEntity>
class EntityDescriptor;

template <typename TEntity>
void RegisterEntityDescriptor(EntityDescriptor<TEntity>* descriptor);

/**
 *	@brief Contains all of the information needed to identify and create entities.
 */
class BaseEntityDescriptor
{
protected:
	BaseEntityDescriptor(std::string_view className)
		: m_ClassName(className)
	{
	}

public:
	std::string_view GetClassName() const { return m_ClassName; }

	virtual CBaseEntity* Create() const = 0;

private:
	const std::string_view m_ClassName;
};

template <typename TEntity>
class EntityDescriptor final : public BaseEntityDescriptor
{
public:
	EntityDescriptor(std::string_view className)
		: BaseEntityDescriptor(className)
	{
		RegisterEntityDescriptor(this);
	}

	CBaseEntity* Create() const override
	{
		return new TEntity();
	}
};

class BaseEntityDictionary
{
protected:
	BaseEntityDictionary() = default;

public:
	auto GetClassNames() const
	{
		return m_Dictionary | std::views::transform([](const auto& e)
								  { return e.first; });
	}

	const BaseEntityDescriptor* Find(std::string_view className) const
	{
		if (auto it = m_Dictionary.find(className); it != m_Dictionary.end())
		{
			return it->second;
		}

		return {};
	}

	CBaseEntity* Create(std::string_view className, entvars_t* pev) const
	{
		assert(pev);
		return CreateCore(className, pev);
	}

	CBaseEntity* Create(std::string_view className) const
	{
		return CreateCore(className, nullptr);
	}

protected:
	CBaseEntity* CreateCore(std::string_view className, entvars_t* pev) const
	{
		assert(!className.empty());

		auto descriptor = Find(className);

		if (!descriptor)
		{
			return nullptr;
		}

		return CreateCore(className, descriptor, pev);
	}

	CBaseEntity* CreateCore(std::string_view className, const BaseEntityDescriptor* descriptor, entvars_t* pev) const
	{
		assert(!className.empty());

		auto entity = descriptor->Create();

		// allocate entity if necessary
		if (pev == nullptr)
		{
			pev = VARS(CREATE_ENTITY());
		}

		// Replicate the ALLOC_PRIVATE engine function's behavior.
		pev->pContainingEntity->pvPrivateData = entity;

		entity->pev = pev;
		entity->pev->classname = ALLOC_STRING_VIEW(className);

		entity->OnCreate();

		return entity;
	}

protected:
	std::unordered_map<std::string_view, const BaseEntityDescriptor*> m_Dictionary;
};

/**
 *	@brief Dictionary of entity classes deriving from @p TBaseEntity.
 */
template <std::derived_from<CBaseEntity> TBaseEntity>
class EntityDictionary final : public BaseEntityDictionary
{
public:
	TBaseEntity* Create(std::string_view className, entvars_t* pev) const
	{
		return static_cast<TBaseEntity*>(BaseEntityDictionary::Create(className, pev));
	}

	TBaseEntity* Create(std::string_view className) const
	{
		return static_cast<TBaseEntity*>(BaseEntityDictionary::Create(className));
	}

	template <std::derived_from<TBaseEntity> TConcrete>
	TConcrete* Create(std::string_view className, entvars_t* pev) const
	{
		return Cast<TConcrete>(BaseEntityDictionary::Create(className, pev));
	}

	template <std::derived_from<TBaseEntity> TConcrete>
	TConcrete* Create(std::string_view className) const
	{
		return Cast<TConcrete>(BaseEntityDictionary::Create(className));
	}

	/**
	*	@brief Destroys the CBaseEntity object. Does not free the associated edict.
	*/
	void Destroy(CBaseEntity* entity)
	{
		if (!entity)
		{
			return;
		}

		entity->OnDestroy();
		delete entity;
	}

	template <std::derived_from<TBaseEntity> TConcrete>
	void Add(const EntityDescriptor<TConcrete>* descriptor)
	{
		assert(descriptor);

		if (!m_Dictionary.try_emplace(descriptor->GetClassName(), descriptor).second)
		{
			assert(!"Tried to register multiple entities with the same classname");
		}
	}

	void Add(const BaseEntityDescriptor*)
	{
		// Overload to ignore addition if it doesn't derive from base type.
	}

private:
	template <typename TConcrete>
	static TConcrete* Cast(TBaseEntity* entity)
	{
#if DEBUG
		// In debug builds you'll be warned if the type doesn't match what's been created.
		auto concrete = dynamic_cast<TConcrete*>(entity);

		assert(concrete);

		return concrete;
#else
		return static_cast<TConcrete*>(entity);
#endif
	}
};

template <typename TBaseEntity>
struct EntityDictionaryLocator final
{
	static EntityDictionary<TBaseEntity>* Get()
	{
		// Force construction to occur on first use.
		static EntityDictionary<TBaseEntity> dictionary;
		return &dictionary;
	}
};

// Used at runtime to look up entities of specific types.
inline EntityDictionary<CBaseEntity>* g_EntityDictionary = EntityDictionaryLocator<CBaseEntity>::Get();
inline EntityDictionary<CBaseItem>* g_ItemDictionary = EntityDictionaryLocator<CBaseItem>::Get();
inline EntityDictionary<CBasePlayerWeapon>* g_WeaponDictionary = EntityDictionaryLocator<CBasePlayerWeapon>::Get();

namespace detail
{
template <typename TEntity, typename... Dictionaries>
void RegisterEntityDescriptorToDictionaries(EntityDescriptor<TEntity>* descriptor, Dictionaries... dictionaries)
{
	// Attempt to add descriptor to each dictionary.
	// This uses fold expressions coupled with operator, to chain the calls.
	(dictionaries->Add(descriptor), ...);
}
}

template <typename TEntity>
void RegisterEntityDescriptor(EntityDescriptor<TEntity>* descriptor)
{
	assert(descriptor);

	// This is where each dictionary is initially constructed and built to add all entity classes.
	// Ideally some form of reflection would be used to build these dictionaries after the initial dictionary has been created,
	// but since we don't have reflection available and existing libraries require loads of refactoring this will have to do for now.
	detail::RegisterEntityDescriptorToDictionaries(descriptor,
		EntityDictionaryLocator<CBaseEntity>::Get(),
		EntityDictionaryLocator<CBaseItem>::Get(),
		EntityDictionaryLocator<CBasePlayerWeapon>::Get());
}

// This is the glue that hooks .MAP entity class names to our CPP classes
// The _declspec forces them to be exported by name so we can do a lookup with GetProcAddress()
// The function is used to intialize / allocate the object for the entity
#define LINK_ENTITY_TO_CLASS(mapClassName, DLLClassName)                                     \
	namespace                                                                                \
	{                                                                                        \
	static const EntityDescriptor<DLLClassName> g_##mapClassName##Descriptor{#mapClassName}; \
	}                                                                                        \
	extern "C" DLLEXPORT void mapClassName(entvars_t* pev);                                  \
	void mapClassName(entvars_t* pev)                                                        \
	{                                                                                        \
		g_EntityDictionary->Create(#mapClassName, pev);                                      \
	}
