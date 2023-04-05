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
#include <string>
#include <type_traits>
#include <variant>

#include "Platform.h"
#include "ClassData.h"
#include "DataFieldSerializers.h"

enum USE_TYPE;

class SINGLE_INHERITANCE CBaseEntity;

template <typename T>
using TBASEPTR = void (T::*)();

template <typename T>
using TENTITYFUNCPTR = void (T::*)(CBaseEntity* pOther);

template <typename T>
using TUSEPTR = void (T::*)(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

using BASEPTR = TBASEPTR<CBaseEntity>;
using ENTITYFUNCPTR = TENTITYFUNCPTR<CBaseEntity>;
using USEPTR = TUSEPTR<CBaseEntity>;

enum FIELDTYPE
{
	FIELD_CHARACTER = 0,   // a byte
	FIELD_BOOLEAN,		   // boolean, implemented as an int, I may use this as a hint for compression
	FIELD_SHORT,		   // 2 byte integer
	FIELD_INTEGER,		   // Any integer or enum
	FIELD_INT64,		   // 64 bit integer
	FIELD_FLOAT,		   // Any floating point value
	FIELD_TIME,			   // a floating point time (these are fixed up automatically too!)
	FIELD_STRING,		   // A string ID (return from ALLOC_STRING)
	FIELD_MODELNAME,	   // Engine string that is a model name (needs precache)
	FIELD_SOUNDNAME,	   // Engine string that is a sound name (needs precache)
	FIELD_EDICT,		   // edict_t*. Deprecated, only used by the engine and entvars_t. Do not use.
	FIELD_CLASSPTR,		   // CBaseEntity *
	FIELD_EHANDLE,		   // Entity handle
	FIELD_VECTOR,		   // Any vector
	FIELD_POSITION_VECTOR, // A world coordinate (these are fixed up across level transitions automagically)
	FIELD_FUNCTIONPOINTER, // A class function pointer (Think, Use, etc)

	FIELD_TYPECOUNT, // MUST BE LAST. Not a valid type.
};

// Specialize this type for each field type to map it to its serializer.
template <FIELDTYPE Type>
struct FieldTypeToSerializerMapper;

template <>
struct FieldTypeToSerializerMapper<FIELD_CHARACTER>
{
	static const inline DataFieldValueSerializer<std::byte> Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_BOOLEAN>
{
	static const inline DataFieldBooleanSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_SHORT>
{
	static const inline DataFieldValueSerializer<short> Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_INTEGER>
{
	static const inline DataFieldValueSerializer<int> Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_INT64>
{
	static const inline DataFieldValueSerializer<std::uint64_t> Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_FLOAT>
{
	static const inline DataFieldValueSerializer<float> Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_TIME>
{
	static const inline DataFieldTimeSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_STRING>
{
	static const inline DataFieldStringOffsetSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_MODELNAME>
{
	static const inline DataFieldModelStringOffsetSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_SOUNDNAME>
{
	static const inline DataFieldSoundStringOffsetSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_EDICT>
{
	static const inline DataFieldEdictSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_CLASSPTR>
{
	static const inline DataFieldClassPointerSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_EHANDLE>
{
	static const inline DataFieldEntityHandleSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_VECTOR>
{
	static const inline DataFieldVectorSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_POSITION_VECTOR>
{
	static const inline DataFieldPositionVectorSerializer Serializer;
};

template <>
struct FieldTypeToSerializerMapper<FIELD_FUNCTIONPOINTER>
{
	static const inline DataFieldFunctionPointerSerializer Serializer;
};

struct DataFieldDescription
{
	FIELDTYPE fieldType;
	const IDataFieldSerializer* Serializer;
	const char* fieldName;
	int fieldOffset;
	short fieldSize;
	short flags;
};

struct DataFunctionDescription
{
	std::string Name;
	BASEPTR Address;
};

using DataMember = std::variant<DataFieldDescription, DataFunctionDescription>;

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

	std::span<const DataMember> Members;
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
		static const DataMember members[] =                            \
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
#define END_DATAMAP()                                          \
	{                                                          \
	}                                                          \
	}                                                          \
	;                                                          \
                                                               \
	return {                                                   \
		.ClassName{className},                                 \
		.BaseMap{ThisClass::GetBaseMap()},                     \
		.Members{std::begin(members), std::end(members) - 1}}; \
	}

#define DEFINE_DUMMY_DATAMAP(thisClass) \
	BEGIN_DATAMAP(thisClass)            \
	END_DATAMAP()

#define RAW_DEFINE_FIELD(fieldName, fieldType, serializer, count, flags)                \
	DataFieldDescription                                                                \
	{                                                                                   \
		fieldType, serializer, #fieldName, offsetof(ThisClass, fieldName), count, flags \
	}

#define DEFINE_FIELD(fieldName, fieldType) \
	RAW_DEFINE_FIELD(fieldName, fieldType, &FieldTypeToSerializerMapper<fieldType>::Serializer, 1, 0)

#define DEFINE_ARRAY(fieldName, fieldType, count) \
	RAW_DEFINE_FIELD(fieldName, fieldType, &FieldTypeToSerializerMapper<fieldType>::Serializer, count, 0)

#define DEFINE_GLOBAL_FIELD(fieldName, fieldType) \
	RAW_DEFINE_FIELD(fieldName, fieldType, &FieldTypeToSerializerMapper<fieldType>::Serializer, 1, FTYPEDESC_GLOBAL)

// Normally, converting one pointer to member function type to another is not allowed.
// Converting a pointer to pointer does work, so this works as expected on the platforms we support.
template <typename TFunctionPointer>
BASEPTR DataMap_ConvertFunctionPointer(TFunctionPointer pointer)
{
	// If these aren't the same size then the cast will produce an invalid pointer.
	static_assert(sizeof(BASEPTR) == sizeof(TFunctionPointer));

	return *reinterpret_cast<BASEPTR*>(&pointer);
}

#define DEFINE_FUNCTION(functionName)                                \
	DataFunctionDescription                                          \
	{                                                                \
		std::string{className} + #functionName,                      \
			DataMap_ConvertFunctionPointer(&ThisClass::functionName) \
	}

BASEPTR DataMap_FindFunctionAddress(const DataMap& dataMap, const char* name);
const char* DataMap_FindFunctionName(const DataMap& dataMap, BASEPTR address);
