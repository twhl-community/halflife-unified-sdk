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
#include "DataFieldSerializers.h"
#include "DataMap.h"

void DataFieldTimeSerializer::Serialize(CSave& save, const std::byte* fields, std::size_t count) const
{
	auto values = reinterpret_cast<float*>(save.GetWriteAddress());

	DataFieldValueSerializer<float>::Serialize(save, fields, count);

	if (save.HasOverflowed())
	{
		return;
	}

	// Always encode time as a delta from the current time so it can be re-based if loaded in a new level
	// Times of 0 are never written to the file, so they will be restored as 0, not a relative time
	const float offset = save.GetData().time;

	for (std::size_t i = 0; i < count; ++i, ++values)
	{
		*values -= offset;
	}
}

void DataFieldTimeSerializer::Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const
{
	DataFieldValueSerializer<float>::Deserialize(restore, fields, count);

	// Re-base time variables
	auto values = reinterpret_cast<float*>(fields);

	const float offset = restore.GetData().time;

	for (std::size_t i = 0; i < count; ++i, ++values)
	{
		*values += offset;
	}
}

std::size_t DataFieldStringOffsetSerializer::GetFieldSize() const
{
	return sizeof(string_t);
}

void DataFieldStringOffsetSerializer::Serialize(CSave& save, const std::byte* fields, std::size_t count) const
{
	auto address = reinterpret_cast<const string_t*>(fields);

	for (std::size_t i = 0; i < count; ++i)
	{
		auto string = STRING(*address);

		save.WriteBytes(reinterpret_cast<const std::byte*>(string), std::strlen(string) + 1);

		++address;
	}
}

template <typename Callback>
void DeserializeStringOffset(CRestore& restore, std::byte* fields, std::size_t count, Callback&& callback)
{
	auto address = reinterpret_cast<string_t*>(fields);
	const char* string = reinterpret_cast<const char*>(restore.GetReadAddress());

	for (std::size_t i = 0; i < count; ++i)
	{
		const std::size_t length = std::strlen(string);

		if (length == 0)
		{
			*address = string_t::Null;
		}
		else
		{
			*address = ALLOC_STRING_VIEW({string, length});

			// Must pass the allocated string since it's also stored in precache lists!
			callback(STRING(*address));
		}

		++address;
		string += length + 1;
	}
}

void DataFieldStringOffsetSerializer::Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const
{
	// Regular strings don't need additional work.
	DeserializeStringOffset(restore, fields, count, [](auto) {});
}

void DataFieldModelStringOffsetSerializer::Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const
{
	// Don't use UTIL_PrecacheModel here because we're restoring an already-replaced name.
	DeserializeStringOffset(restore, fields, count, [&](auto string)
		{
			if (restore.ShouldPrecache())
			{
				UTIL_PrecacheModelDirect(string);
			} });
}

void DataFieldSoundStringOffsetSerializer::Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const
{
	// Don't use UTIL_PrecacheSound here because we're restoring an already-replaced name.
	DeserializeStringOffset(restore, fields, count, [&](auto string)
		{
			if (restore.ShouldPrecache())
			{
				UTIL_PrecacheSoundDirect(string);
			} });
}

std::size_t DataFieldEdictSerializer::GetFieldSize() const
{
	return sizeof(edict_t*);
}

void DataFieldEdictSerializer::Serialize(CSave& save, const std::byte* fields, std::size_t count) const
{
	auto address = reinterpret_cast<const edict_t* const*>(fields);

	for (std::size_t i = 0; i < count; ++i)
	{
		auto index = save.EntityIndex(*address);

		save.WriteValue(index);

		++address;
	}
}

void DataFieldEdictSerializer::Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const
{
	auto address = reinterpret_cast<edict_t**>(fields);
	const int* indices = reinterpret_cast<const int*>(restore.GetReadAddress());

	for (std::size_t i = 0; i < count; ++i)
	{
		*address = restore.EntityFromIndex(*indices);

		++address;
		++indices;
	}
}

std::size_t DataFieldClassPointerSerializer::GetFieldSize() const
{
	return sizeof(CBaseEntity*);
}

void DataFieldClassPointerSerializer::Serialize(CSave& save, const std::byte* fields, std::size_t count) const
{
	auto address = reinterpret_cast<const CBaseEntity* const*>(fields);

	for (std::size_t i = 0; i < count; ++i)
	{
		auto index = save.EntityIndex(*address);

		save.WriteValue(index);

		++address;
	}
}

void DataFieldClassPointerSerializer::Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const
{
	auto address = reinterpret_cast<CBaseEntity**>(fields);
	const int* indices = reinterpret_cast<const int*>(restore.GetReadAddress());

	for (std::size_t i = 0; i < count; ++i)
	{
		auto entity = restore.EntityFromIndex(*indices);

		*address = entity ? CBaseEntity::Instance(entity) : nullptr;

		++address;
		++indices;
	}
}

std::size_t DataFieldEntityHandleSerializer::GetFieldSize() const
{
	return sizeof(BaseEntityHandle);
}

void DataFieldEntityHandleSerializer::Serialize(CSave& save, const std::byte* fields, std::size_t count) const
{
	auto address = reinterpret_cast<const BaseEntityHandle*>(fields);

	for (std::size_t i = 0; i < count; ++i)
	{
		auto index = save.EntityIndex(address->InternalGetEntity());

		save.WriteValue(index);

		++address;
	}
}

void DataFieldEntityHandleSerializer::Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const
{
	auto address = reinterpret_cast<BaseEntityHandle*>(fields);
	const int* indices = reinterpret_cast<const int*>(restore.GetReadAddress());

	for (std::size_t i = 0; i < count; ++i)
	{
		auto entity = restore.EntityFromIndex(*indices);

		address->InternalSetEntity(entity ? CBaseEntity::Instance(entity) : nullptr);

		++address;
		++indices;
	}
}

void DataFieldPositionVectorSerializer::Serialize(CSave& save, const std::byte* fields, std::size_t count) const
{
	auto address = reinterpret_cast<const Vector*>(fields);

	const auto& data = save.GetData();
	const Vector offset = 0 != data.fUseLandmark ? data.vecLandmarkOffset : vec3_origin;

	for (std::size_t i = 0; i < count; ++i)
	{
		save.WriteValue(*address - offset);
		++address;
	}
}

void DataFieldPositionVectorSerializer::Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const
{
	auto address = reinterpret_cast<Vector*>(fields);

	const auto& data = restore.GetData();
	const Vector offset = 0 != data.fUseLandmark ? data.vecLandmarkOffset : vec3_origin;

	for (std::size_t i = 0; i < count; ++i)
	{
		*address = restore.ReadValue<Vector>() + offset;
		++address;
	}
}

// These sizes must be the same.
static_assert(sizeof(BASEPTR) == sizeof(USEPTR) && sizeof(BASEPTR) == sizeof(ENTITYFUNCPTR));

std::size_t DataFieldFunctionPointerSerializer::GetFieldSize() const
{
	return sizeof(BASEPTR);
}

void DataFieldFunctionPointerSerializer::Serialize(CSave& save, const std::byte* fields, std::size_t count) const
{
	const auto dataMap = save.GetCurrentDataMap();

	auto address = reinterpret_cast<const BASEPTR*>(fields);

	for (std::size_t i = 0; i < count; ++i)
	{
		const char* functionName = DataMap_FindFunctionName(*dataMap, *address);

		if (functionName)
		{
			save.WriteBytes(reinterpret_cast<const std::byte*>(functionName), std::strlen(functionName) + 1);
		}
		else
		{
			CSaveRestoreBuffer::Logger->error("Invalid function pointer in entity!");
		}

		++address;
	}
}

void DataFieldFunctionPointerSerializer::Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const
{
	const auto dataMap = restore.GetCurrentDataMap();

	auto readAddress = reinterpret_cast<const char*>(restore.GetReadAddress());
	auto address = reinterpret_cast<BASEPTR*>(fields);

	for (std::size_t i = 0; i < count; ++i)
	{
		const std::size_t length = std::strlen(readAddress);

		if (length == 0)
		{
			*address = nullptr;
		}
		else
		{
			// HACK: get around language rules to convert the pointer.
			// TODO: get rid of this when the function table is set up.
			auto functionAddress = DataMap_FindFunctionAddress(*dataMap, readAddress);
			*address = *reinterpret_cast<BASEPTR*>(&functionAddress);
		}

		readAddress += length + 1;
		++address;
	}
}
