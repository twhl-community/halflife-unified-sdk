/***
 *
 *	Copyright (c) 1999, Valve LLC. All rights reserved.
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

/**
 *	@file
 *	MDC - copying from cstrike\cl_dll so career-mode stuff can catch messages in this dll. (and C++ifying it)
 */

#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "Platform.h"
#include "palette.h"

class ByteBuffer
{
public:
	explicit ByteBuffer() = default;

	explicit ByteBuffer(std::span<std::byte> buffer)
		: m_Buffer(buffer)
	{
	}

	ByteBuffer(const ByteBuffer&) = default;
	ByteBuffer& operator=(const ByteBuffer&) = default;

	ByteBuffer(ByteBuffer&&) = delete;
	ByteBuffer& operator=(ByteBuffer&&) = delete;

	std::size_t GetOffset() const { return m_Index; }
	std::size_t GetRemaining() const { return m_Buffer.size() - m_Index; }
	std::size_t GetSize() const { return m_Buffer.size(); }

	bool HasOverflowed() const { return m_Overflowed; }

protected:
	std::byte* GetCurrentAddress();

protected:
	std::span<std::byte> m_Buffer;
	std::size_t m_Index{};
	bool m_Overflowed{false};
};

inline std::byte* ByteBuffer::GetCurrentAddress()
{
	assert(!m_Overflowed);
	return m_Buffer.data() + m_Index;
}

class BufferReader final : public ByteBuffer
{
public:
	using ByteBuffer::ByteBuffer;

	int ReadChar();
	int ReadByte();
	int ReadShort();
	int ReadLong();
	float ReadFloat();
	float ReadCoord();
	float ReadAngle();
	float ReadHiResAngle();
	char* ReadString();

	Vector ReadCoordVector();
	RGB24 ReadRGB24();

private:
	bool TryRead(std::size_t sizeInBytes);
};

inline int BufferReader::ReadChar()
{
	if (!TryRead(sizeof(signed char)))
	{
		return -1;
	}

	const auto i = *reinterpret_cast<signed char*>(GetCurrentAddress());

	m_Index += sizeof(signed char);

	return i;
}

inline int BufferReader::ReadByte()
{
	if (!TryRead(sizeof(byte)))
	{
		return -1;
	}

	const auto i = *reinterpret_cast<byte*>(GetCurrentAddress());

	m_Index += sizeof(byte);

	return i;
}

inline int BufferReader::ReadShort()
{
	if (!TryRead(sizeof(std::int16_t)))
	{
		return -1;
	}

	const auto address = reinterpret_cast<const byte*>(GetCurrentAddress());

	const std::int16_t i = *address + (*(address + 1) << 8);

	m_Index += sizeof(std::int16_t);

	return i;
}

inline int BufferReader::ReadLong()
{
	if (!TryRead(sizeof(std::int32_t)))
	{
		return -1;
	}

	const auto address = reinterpret_cast<const byte*>(GetCurrentAddress());

	const std::int32_t i = *address + (*(address + 1) << 8) + (*(address + 2) << 16) + (*(address + 3) << 24);

	m_Index += sizeof(std::int32_t);

	return i;
}

inline float BufferReader::ReadFloat()
{
	if (!TryRead(sizeof(float)))
	{
		return -1;
	}

	const auto address = reinterpret_cast<const byte*>(GetCurrentAddress());

	float result{};

	std::byte* const data = reinterpret_cast<std::byte*>(&result);

	data[0] = std::byte{address[0]};
	data[1] = std::byte{address[1]};
	data[2] = std::byte{address[2]};
	data[3] = std::byte{address[3]};

	m_Index += sizeof(float);

	return result;
}

inline float BufferReader::ReadCoord()
{
	return ReadShort() * (1.0f / 8);
}

inline float BufferReader::ReadAngle()
{
	return ReadChar() * (360.0f / 256);
}

inline float BufferReader::ReadHiResAngle()
{
	return ReadShort() * (360.0f / 65536);
}

inline char* BufferReader::ReadString()
{
	static char string[2048];

	std::size_t l = 0;

	do
	{
		if (m_Index + 1 > m_Buffer.size())
			break; // no more characters

		const int c = ReadChar();

		if (c == -1 || c == '\0')
			break;

		string[l] = c;
		++l;
	} while (l < sizeof(string) - 1);

	string[l] = '\0';

	return string;
}

inline Vector BufferReader::ReadCoordVector()
{
	Vector pos;

	pos.x = ReadCoord();
	pos.y = ReadCoord();
	pos.z = ReadCoord();

	return pos;
}

inline RGB24 BufferReader::ReadRGB24()
{
	RGB24 color;

	color.Red = ReadByte();
	color.Green = ReadByte();
	color.Blue = ReadByte();

	return color;
}

inline bool BufferReader::TryRead(std::size_t sizeInBytes)
{
	if (m_Index + sizeInBytes > m_Buffer.size())
	{
		m_Index = m_Buffer.size();
		m_Overflowed = true;
		return false;
	}

	return true;
}

class BufferWriter : public ByteBuffer
{
public:
	using ByteBuffer::ByteBuffer;

	void WriteByte(byte data);
	void WriteLong(int data);
	void WriteString(const char* str);

private:
	bool TryWrite(std::size_t sizeInBytes);
};

inline void BufferWriter::WriteByte(byte data)
{
	if (!TryWrite(sizeof(byte)))
	{
		return;
	}

	*GetCurrentAddress() = std::byte{data};

	++m_Index;
}

inline void BufferWriter::WriteLong(int data)
{
	if (!TryWrite(sizeof(std::int32_t)))
	{
		return;
	}

	auto address = GetCurrentAddress();

	address[0] = std::byte(data & 0xFF);
	address[1] = std::byte((data >> 8) & 0xFF);
	address[2] = std::byte((data >> 16) & 0xFF);
	address[3] = std::byte((data >> 24) & 0xFF);

	m_Index += sizeof(std::int32_t);
}

inline void BufferWriter::WriteString(const char* str)
{
	assert(str);

	static_assert(sizeof(std::byte) == sizeof(char));

	const std::size_t length = std::strlen(str) + 1;

	if (!TryWrite(length))
	{
		return;
	}

	std::memcpy(GetCurrentAddress(), str, length);

	m_Index += length;
}

inline bool BufferWriter::TryWrite(std::size_t sizeInBytes)
{
	if (m_Index + sizeInBytes >= m_Buffer.size())
	{
		m_Index = m_Buffer.size();
		m_Overflowed = true;
		return false;
	}

	return true;
}
