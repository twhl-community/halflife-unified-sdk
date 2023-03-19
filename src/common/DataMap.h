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

#include <span>
#include <type_traits>

#include "Platform.h"
#include "ClassData.h"

struct TYPEDESCRIPTION;

/**
 *	@brief Stores a list of type descriptions and a reference to a base class data map.
 */
struct DataMap final
{
	const char* ClassName{};

	/**
	 *	@brief If the type has a base class with a data map, this is that data map.
	 */
	const DataMap* BaseMap{};

	std::span<const TYPEDESCRIPTION> Descriptions;
};

#define DECLARE_DATAMAP_COMMON()             \
private:                                     \
	static const DataMap m_LocalDataMap;     \
                                             \
	static DataMap InternalCreateDataMap();  \
                                             \
public:                                      \
	static const DataMap* GetLocalDataMap(); \
	static const DataMap* GetBaseMap()

#define _DECLARE_VIRTUAL_DATAMAP_COMMON() \
	DECLARE_DATAMAP_COMMON()

/**
 *	@brief Declares a data map for a class with no base that has no virtual methods.
 *	Requires the @c DECLARE_CLASS_NOBASE macro to be used to define @c ThisClass.
 */
#define DECLARE_SIMPLE_DATAMAP() \
	DECLARE_DATAMAP_COMMON();    \
	const DataMap* GetDataMap() const

/**
 *	@brief Declares a data map for a class with no base that has virtual methods.
 *	Requires the @c DECLARE_CLASS_NOBASE macro to be used to define @c ThisClass.
 */
#define DECLARE_DATAMAP_NOBASE()       \
	_DECLARE_VIRTUAL_DATAMAP_COMMON(); \
	virtual const DataMap* GetDataMap() const

/**
 *	@brief Declares a data map for a class that inherits from a class that has virtual methods.
 *	Requires the @c DECLARE_CLASS macro to be used to define @c ThisClass and @c BaseClass.
 */
#define DECLARE_DATAMAP()              \
	_DECLARE_VIRTUAL_DATAMAP_COMMON(); \
	const DataMap* GetDataMap() const override

#define _BEGIN_DATAMAP_COMMON(thisClass)                               \
	const DataMap thisClass::m_LocalDataMap = InternalCreateDataMap(); \
                                                                       \
	const DataMap* thisClass::GetLocalDataMap()                        \
	{                                                                  \
		return &m_LocalDataMap;                                        \
	}                                                                  \
                                                                       \
	const DataMap* thisClass::GetDataMap() const                       \
	{                                                                  \
		return &m_LocalDataMap;                                        \
	}                                                                  \
                                                                       \
	DataMap thisClass::InternalCreateDataMap()                         \
	{                                                                  \
		const char* const className = #thisClass;                      \
		using ThisClass = thisClass;                                   \
                                                                       \
		static const TYPEDESCRIPTION descriptions[] =                  \
		{

/**
 *	@brief Begins a datamap definition for a class with no base class.
 */
#define BEGIN_DATAMAP_NOBASE(thisClass)    \
	const DataMap* thisClass::GetBaseMap() \
	{                                      \
		return nullptr;                    \
	}                                      \
                                           \
	_BEGIN_DATAMAP_COMMON(thisClass)

/**
 *	@brief Begins a datamap definition for a class with a base class.
 */
#define BEGIN_DATAMAP(thisClass)                        \
	const DataMap* thisClass::GetBaseMap()              \
	{                                                   \
		return thisClass::BaseClass::GetLocalDataMap(); \
	}                                                   \
                                                        \
	_BEGIN_DATAMAP_COMMON(thisClass)

/**
 *	@brief Ends the datamap definition. Append a semicolon to the end.
 *	@details Empty description because zero-length arrays are not allowed.
 */
#define END_DATAMAP()                                                         \
	{                                                                         \
		FIELD_TYPECOUNT, nullptr, -1, -1, 0                                   \
	}                                                                         \
	}                                                                         \
	;                                                                         \
                                                                              \
	return {                                                                  \
		.ClassName{className},                                                \
		.BaseMap{ThisClass::GetBaseMap()},                                    \
		.Descriptions{std::begin(descriptions), std::end(descriptions) - 1}}; \
	}

#define DEFINE_DUMMY_DATAMAP(thisClass) \
	BEGIN_DATAMAP(thisClass)            \
	END_DATAMAP()

#define RAW_DEFINE_FIELD(fieldName, fieldType, count, flags)                \
	{                                                                       \
		fieldType, #fieldName, offsetof(ThisClass, fieldName), count, flags \
	}

#define DEFINE_FIELD(fieldName, fieldType) \
	RAW_DEFINE_FIELD(fieldName, fieldType, 1, 0)

#define DEFINE_ARRAY(fieldName, fieldType, count) \
	RAW_DEFINE_FIELD(fieldName, fieldType, count, 0)

#define DEFINE_GLOBAL_FIELD(fieldName, fieldType) \
	RAW_DEFINE_FIELD(fieldName, fieldType, 1, FTYPEDESC_GLOBAL)
