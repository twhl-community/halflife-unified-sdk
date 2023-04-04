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

#include <cstddef>
#include <cstdint>

#include "Platform.h"
#include "saverestore.h"

/**
 *	@brief Provides a means of serializing and deserializing an array of fields of a specific type.
 */
class IDataFieldSerializer
{
public:
	virtual ~IDataFieldSerializer() = default;

	virtual std::size_t GetFieldSize() const = 0;

	/**
	 *	@brief Reads fields from the source and writes them to the buffer.
	 */
	virtual void Serialize(CSave& save, const std::byte* fields, std::size_t count) const = 0;

	/**
	 *	@brief Reads fields from the buffer and writes them to the destination.
	 */
	virtual void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const = 0;
};

/**
 *	@brief Reads and writes fields directly.
 */
template <typename T>
class DataFieldValueSerializer : public IDataFieldSerializer
{
public:
	std::size_t GetFieldSize() const override
	{
		return sizeof(T);
	}

	void Serialize(CSave& save, const std::byte* fields, std::size_t count) const override
	{
		save.WriteBytes(fields, count * sizeof(T));
	}

	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override
	{
		restore.ReadBytes(fields, count * sizeof(T));
	}
};

/**
 *	@brief Base class for serializers that convert between the in-memory and on-disk type.
 */
template <typename T, typename Derived>
class DataFieldConvertingSerializer : public IDataFieldSerializer
{
public:
	std::size_t GetFieldSize() const override
	{
		return sizeof(T);
	}

	void Serialize(CSave& save, const std::byte* fields, std::size_t count) const override
	{
		auto address = reinterpret_cast<const T*>(fields);

		for (std::size_t i = 0; i < count; ++i)
		{
			const auto& value = Derived::ToDisk(*address);

			save.WriteBytes(reinterpret_cast<const std::byte*>(&value), sizeof(value));

			++address;
		}
	}

	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override
	{
		auto address = reinterpret_cast<T*>(fields);

		for (std::size_t i = 0; i < count; ++i)
		{
			decltype(Derived::ToDisk({})) value{};

			restore.ReadBytes(reinterpret_cast<std::byte*>(&value), sizeof(value));

			*address = Derived::ToMemory(value);

			++address;
		}
	}
};

/**
 *	@brief Serializes booleans as bytes.
 */
class DataFieldBooleanSerializer : public DataFieldConvertingSerializer<bool, DataFieldBooleanSerializer>
{
public:
	static std::uint8_t ToDisk(bool value)
	{
		return std::uint8_t(value);
	}

	static bool ToMemory(std::uint8_t value)
	{
		return bool(value);
	}
};

class DataFieldTimeSerializer : public DataFieldValueSerializer<float>
{
public:
	void Serialize(CSave& save, const std::byte* fields, std::size_t count) const override;
	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override;
};

class DataFieldStringOffsetSerializer : public IDataFieldSerializer
{
public:
	std::size_t GetFieldSize() const override;
	void Serialize(CSave& save, const std::byte* fields, std::size_t count) const override;
	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override;
};

class DataFieldModelStringOffsetSerializer : public DataFieldStringOffsetSerializer
{
public:
	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override;
};

class DataFieldSoundStringOffsetSerializer : public DataFieldStringOffsetSerializer
{
public:
	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override;
};

class DataFieldEdictSerializer : public IDataFieldSerializer
{
public:
	std::size_t GetFieldSize() const override;
	void Serialize(CSave& save, const std::byte* fields, std::size_t count) const override;
	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override;
};

class DataFieldClassPointerSerializer : public IDataFieldSerializer
{
public:
	std::size_t GetFieldSize() const override;
	void Serialize(CSave& save, const std::byte* fields, std::size_t count) const override;
	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override;
};

class DataFieldEntityHandleSerializer : public IDataFieldSerializer
{
public:
	std::size_t GetFieldSize() const override;
	void Serialize(CSave& save, const std::byte* fields, std::size_t count) const override;
	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override;
};

class DataFieldVectorSerializer : public DataFieldValueSerializer<Vector>
{
};

class DataFieldPositionVectorSerializer : public DataFieldVectorSerializer
{
public:
	void Serialize(CSave& save, const std::byte* fields, std::size_t count) const override;
	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override;
};

class DataFieldFunctionPointerSerializer : public IDataFieldSerializer
{
public:
	std::size_t GetFieldSize() const override;
	void Serialize(CSave& save, const std::byte* fields, std::size_t count) const override;
	void Deserialize(CRestore& restore, std::byte* fields, std::size_t count) const override;
};
