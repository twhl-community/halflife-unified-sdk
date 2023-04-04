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

#include <bit>

#include "cbase.h"
#include "DataMap.h"
#include "saverestore.h"

// Base class includes common SAVERESTOREDATA pointer, and manages the entity table
CSaveRestoreBuffer::CSaveRestoreBuffer(SAVERESTOREDATA& data)
	: m_data(data)
{
}

CSaveRestoreBuffer::~CSaveRestoreBuffer() = default;

int CSaveRestoreBuffer::EntityIndex(const CBaseEntity* pEntity)
{
	if (pEntity == nullptr)
		return -1;
	return EntityIndex(pEntity->edict());
}

int CSaveRestoreBuffer::EntityIndex(const edict_t* pentLookup)
{
	if (pentLookup == nullptr)
		return -1;

	for (int i = 0; i < m_data.tableCount; ++i)
	{
		if (m_data.pTable[i].pent == pentLookup)
			return i;
	}

	return -1;
}

edict_t* CSaveRestoreBuffer::EntityFromIndex(int entityIndex)
{
	if (entityIndex < 0)
		return nullptr;

	for (int i = 0; i < m_data.tableCount; ++i)
	{
		auto pTable = m_data.pTable + i;
		if (pTable->id == entityIndex)
			return pTable->pent;
	}

	return nullptr;
}

int CSaveRestoreBuffer::EntityFlagsSet(int entityIndex, int flags)
{
	if (entityIndex < 0)
		return 0;
	if (entityIndex > m_data.tableCount)
		return 0;

	m_data.pTable[entityIndex].flags |= flags;

	return m_data.pTable[entityIndex].flags;
}

void CSaveRestoreBuffer::BufferRewind(int size)
{
	if (m_data.size < size)
		size = m_data.size;

	m_data.pCurrentData -= size;
	m_data.size -= size;
}

unsigned int CSaveRestoreBuffer::HashString(const char* pszToken)
{
	unsigned int hash = 0;

	while ('\0' != *pszToken)
		hash = std::rotr(hash, 4) ^ *pszToken++;

	return hash;
}

unsigned short CSaveRestoreBuffer::TokenHash(const char* pszToken)
{
#if _DEBUG
	static int tokensparsed = 0;
	tokensparsed++;
#endif
	if (0 == m_data.tokenCount || nullptr == m_data.pTokens)
	{
		// if we're here it means trigger_changelevel is trying to actually save something when it's not supposed to.
		Logger->error("No token table array in TokenHash()!");
		return 0;
	}

	const unsigned short hash = (unsigned short)(HashString(pszToken) % (unsigned)m_data.tokenCount);

	for (int i = 0; i < m_data.tokenCount; i++)
	{
#if _DEBUG
		static bool beentheredonethat = false;
		if (i > 50 && !beentheredonethat)
		{
			beentheredonethat = true;
			Logger->error("CSaveRestoreBuffer :: TokenHash() is getting too full!");
		}
#endif

		int index = hash + i;
		if (index >= m_data.tokenCount)
			index -= m_data.tokenCount;

		if (!m_data.pTokens[index] || strcmp(pszToken, m_data.pTokens[index]) == 0)
		{
			m_data.pTokens[index] = (char*)pszToken;
			return index;
		}
	}

	// Token hash table full!!!
	// [Consider doing overflow table(s) after the main table & limiting linear hash table search]
	Logger->error("CSaveRestoreBuffer :: TokenHash() is COMPLETELY FULL!");
	return 0;
}

bool CSaveRestoreBuffer::IsValidSaveRestoreData(SAVERESTOREDATA* data)
{
	const bool isValid = nullptr != data && nullptr != data->pTokens && data->tokenCount > 0;

	ASSERT(isValid);

	return isValid;
}

bool CSave::WriteFields(void* baseData, const DataMap& dataMap)
{
	WriteHeader(dataMap.ClassName, 0);
	auto fieldCount = WriteValue(int(0));

	if (!fieldCount)
	{
		return false;
	}

	for (const auto& member : dataMap.Members)
	{
		auto field = std::get_if<DataFieldDescription>(&member);

		if (!field)
		{
			continue;
		}

		auto fields = reinterpret_cast<const std::byte*>(baseData) + field->fieldOffset;

		auto serializer = field->Serializer;

		if (!serializer)
		{
			Logger->error("Bad field type");
			continue;
		}

		if (DataEmpty(fields, field->fieldSize * serializer->GetFieldSize()))
			continue;

		auto fieldSize = WriteHeader(field->fieldName, 0);

		const auto startPosition = m_data.pCurrentData;

		serializer->Serialize(*this, fields, field->fieldSize);

		WriteCount(fieldSize, m_data.pCurrentData - startPosition);

		// Empty fields will not be written, write out the actual number of fields to be written
		++(*fieldCount);
	}

	return true;
}

std::byte* CSave::GetWriteAddress()
{
	return reinterpret_cast<std::byte*>(m_data.pCurrentData);
}

std::byte* CSave::WriteBytes(const std::byte* bytes, std::size_t sizeInBytes)
{
	const int size = int(sizeInBytes);

	if (m_data.size + size > m_data.bufferSize)
	{
		Logger->error("Save/Restore overflow!");
		m_data.size = m_data.bufferSize;
		return nullptr;
	}

	memcpy(m_data.pCurrentData, bytes, sizeInBytes);
	auto address = m_data.pCurrentData;
	m_data.pCurrentData += size;
	m_data.size += size;

	return reinterpret_cast<std::byte*>(address);
}

bool CSave::HasOverflowed() const
{
	return m_data.size >= m_data.bufferSize;
}

short* CSave::WriteHeader(const char* name, short size)
{
	if (size > (1 << (sizeof(short) * 8)))
	{
		Logger->error("CSave::WriteHeader() size parameter exceeds 'short'!");
	}

	const short hashvalue = TokenHash(name);

	auto address = reinterpret_cast<short*>(m_data.pCurrentData);

	WriteValue(size);
	WriteValue(hashvalue);

	return address;
}

void CSave::WriteCount(short* destination, int count)
{
	if (count > (1 << (sizeof(short) * 8)))
	{
		Logger->error("CSave::WriteCount() size parameter exceeds 'short'!");
	}

	*destination = count;
}

bool CSave::DataEmpty(const std::byte* pdata, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (std::byte{0} != pdata[i])
			return false;
	}
	return true;
}

bool CRestore::ReadFields(void* baseData, const DataMap& dataMap)
{
	const int headerSize = BufferReadValue<short>();

	ASSERT(headerSize == sizeof(int)); // First entry should be an int

	const int headerToken = BufferReadValue<short>();

	// Check the struct name
	if (headerToken != TokenHash(dataMap.ClassName)) // Field Set marker
	{
		// Logger->error("Expected {} found {}!", dataMap.ClassName, BufferPointer());
		BufferRewind(2 * sizeof(short));
		return false;
	}

	// Skip over the struct name
	const int fileCount = BufferReadValue<int>(); // Read field count

	int lastField = 0; // Make searches faster, most data is read/written in the same order

	// Clear out base data
	for (const auto& member : dataMap.Members)
	{
		auto field = std::get_if<DataFieldDescription>(&member);

		if (!field)
		{
			continue;
		}

		// Don't clear global fields
		if (!m_global || (field->flags & FTYPEDESC_GLOBAL) == 0)
		{
			auto serializer = field->Serializer;

			if (!serializer)
			{
				Logger->error("Bad field type");
				continue;
			}

			std::memset(reinterpret_cast<std::byte*>(baseData) + field->fieldOffset, 0,
				field->fieldSize * serializer->GetFieldSize());
		}
	}

	HEADER header;

	for (int field = 0; field < fileCount; ++field)
	{
		BufferReadHeader(header);
		lastField = ReadField(baseData, dataMap, m_data.pTokens[header.token], lastField, header.pData, header.size);
		lastField++;
	}

	return true;
}

int CRestore::ReadField(void* baseData, const DataMap& dataMap, const char* fieldName, int startField, std::byte* data, int size)
{
	for (std::size_t i = 0; i < dataMap.Members.size(); ++i)
	{
		const int fieldNumber = (i + startField) % int(dataMap.Members.size());
		const auto& member = dataMap.Members[fieldNumber];

		auto field = std::get_if<DataFieldDescription>(&member);

		if (!field)
		{
			continue;
		}

		if (!stricmp(field->fieldName, fieldName))
		{
			if (!m_global || (field->flags & FTYPEDESC_GLOBAL) == 0)
			{
				auto serializer = field->Serializer;

				if (!serializer)
				{
					Logger->error("Bad field type");
					continue;
				}

				m_ReadStartAddress = m_ReadAddress = data;
				m_ReadSize = size;
				m_HasOverflowed = false;

				serializer->Deserialize(*this, reinterpret_cast<std::byte*>(baseData) + field->fieldOffset, field->fieldSize);
			}
#if 0
			else
			{
				Logger->debug( "Skipping global field {}", pName);
			}
#endif
			return fieldNumber;
		}
	}

	return -1;
}

void CRestore::BufferReadHeader(HEADER& header)
{
	header.size = BufferReadValue<short>();	 // Read field size
	header.token = BufferReadValue<short>(); // Read field name token
	header.pData = BufferPointer();			 // Field Data is next
	BufferSkipBytes(header.size);			 // Advance to next field
}

bool CRestore::Empty()
{
	return (m_data.pCurrentData - m_data.pBaseData) >= m_data.bufferSize;
}

const std::byte* CRestore::GetReadAddress()
{
	return m_ReadAddress;
}

void CRestore::ReadBytes(std::byte* bytes, std::size_t sizeInBytes)
{
	if (((m_ReadAddress - m_ReadStartAddress) + sizeInBytes) > m_ReadSize)
	{
		Logger->error("Restore overflow!");
		m_ReadAddress = m_ReadStartAddress + m_ReadSize;
		m_HasOverflowed = true;
		return;
	}

	if (bytes)
		memcpy(bytes, m_ReadAddress, sizeInBytes);
	m_ReadAddress += sizeInBytes;
}

bool CRestore::HasOverflowed() const
{
	return m_HasOverflowed;
}

std::byte* CRestore::BufferPointer()
{
	return reinterpret_cast<std::byte*>(m_data.pCurrentData);
}

void CRestore::BufferReadBytes(std::byte* pOutput, int size)
{
	if (Empty())
		return;

	if ((m_data.size + size) > m_data.bufferSize)
	{
		Logger->error("Restore overflow!");
		m_data.size = m_data.bufferSize;
		return;
	}

	if (pOutput)
		memcpy(pOutput, m_data.pCurrentData, size);
	m_data.pCurrentData += size;
	m_data.size += size;
}

void CRestore::BufferSkipBytes(int bytes)
{
	BufferReadBytes(nullptr, bytes);
}
