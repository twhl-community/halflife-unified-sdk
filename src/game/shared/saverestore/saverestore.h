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
#include <memory>

#include <spdlog/logger.h>

class CBaseEntity;
struct DataFieldDescription;
struct DataMap;
struct SAVERESTOREDATA;

class CSaveRestoreBuffer
{
public:
	static inline std::shared_ptr<spdlog::logger> Logger;

	CSaveRestoreBuffer(SAVERESTOREDATA& data);
	~CSaveRestoreBuffer();

	int EntityIndex(const edict_t* pentLookup);
	int EntityIndex(const CBaseEntity* pEntity);

	int EntityFlags(int entityIndex, int flags) { return EntityFlagsSet(entityIndex, 0); }
	int EntityFlagsSet(int entityIndex, int flags);

	edict_t* EntityFromIndex(int entityIndex);

	unsigned short TokenHash(const char* pszToken);

	const SAVERESTOREDATA& GetData() const { return m_data; }

	// Data is only valid if it's a valid pointer and if it has a token list
	[[nodiscard]] static bool IsValidSaveRestoreData(SAVERESTOREDATA* data);

protected:
	SAVERESTOREDATA& m_data;
	void BufferRewind(int size);
	unsigned int HashString(const char* pszToken);
};


class CSave : public CSaveRestoreBuffer
{
public:
	using CSaveRestoreBuffer::CSaveRestoreBuffer;

	bool WriteFields(void* baseData, const DataMap& dataMap);

	std::byte* GetWriteAddress();

	std::byte* WriteBytes(const std::byte* bytes, std::size_t sizeInBytes);

	template <typename T>
	T* WriteValue(const T& value)
	{
		return reinterpret_cast<T*>(WriteBytes(reinterpret_cast<const std::byte*>(&value), sizeof(value)));
	}

	bool HasOverflowed() const;

private:
	short* WriteHeader(const char* name, short size);
	void WriteCount(short* destination, int count);

	static bool DataEmpty(const std::byte* pdata, int size);
};

struct HEADER
{
	unsigned short size;
	unsigned short token;
	std::byte* pData;
};

class CRestore : public CSaveRestoreBuffer
{
public:
	using CSaveRestoreBuffer::CSaveRestoreBuffer;

	bool ReadFields(void* baseData, const DataMap& dataMap);
	int ReadField(void* baseData, const DataMap& dataMap, const char* fieldName, int startField, std::byte* data, int size);
	bool Empty();
	void SetGlobalMode(bool global) { m_global = global; }

	bool ShouldPrecache() const { return m_precache; }

	void PrecacheMode(bool mode) { m_precache = mode; }

	const std::byte* GetReadAddress();

	void ReadBytes(std::byte* bytes, std::size_t sizeInBytes);

	template <typename T>
	T ReadValue()
	{
		T value{};
		ReadBytes(reinterpret_cast<std::byte*>(&value), sizeof(value));
		return value;
	}

	bool HasOverflowed() const;

private:
	std::byte* BufferPointer();
	void BufferReadBytes(std::byte* pOutput, int size);

	template <typename T>
	T BufferReadValue()
	{
		T value{};
		BufferReadBytes(reinterpret_cast<std::byte*>(&value), sizeof(T));
		return value;
	}

	void BufferSkipBytes(int bytes);

	void BufferReadHeader(HEADER& header);

	bool m_global = false; // Restoring a global entity?
	bool m_precache = true;

	std::byte* m_ReadStartAddress{};
	std::byte* m_ReadAddress{};
	std::size_t m_ReadSize{};
	bool m_HasOverflowed = false;
};
