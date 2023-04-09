/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include <cstring>
#include <optional>
#include <string_view>

struct RiffChunk
{
	std::string_view ChunkId{};
	std::size_t Size{};
	const std::byte* Data{};
};

/**
 *	@brief Scans for RIFF chunks in the given data.
 */
class RiffChunkReader final
{
public:
	RiffChunkReader() = default;
	RiffChunkReader(const std::byte* data, std::size_t size);

	constexpr operator bool() const { return m_Data != nullptr; }

	std::optional<RiffChunk> Next();

	std::optional<RiffChunk> FindChunk(std::string_view chunkId);

private:
	const std::byte* m_Data{};
	std::size_t m_Size{};
	const std::byte* m_ReadPosition{};
};

/**
 *	@brief Determines whether the given data is a RIFF header and allows scanning for RIFF chunks.
 *	@details See https://www.recordingblogs.com/wiki/wave-file-format for more information.
 */
class RiffFileReader final
{
public:
	RiffFileReader(const std::byte* potentialRiffData, std::size_t sizeInBytes);

	constexpr operator bool() const { return m_ChunkReader; }

	RiffChunkReader GetChunkReader() const { return m_ChunkReader; }

private:
	RiffChunkReader m_ChunkReader;
};

inline RiffChunkReader::RiffChunkReader(const std::byte* data, std::size_t size)
	: m_Data(data),
	  m_Size(size),
	  m_ReadPosition(data)
{
}

inline RiffFileReader::RiffFileReader(const std::byte* potentialRiffData, std::size_t sizeInBytes)
{
	// A valid RIFF header must be at least 12 bytes large.
	if (!potentialRiffData || sizeInBytes <= 12)
	{
		return;
	}

	if (0 != strncmp("RIFF", reinterpret_cast<const char*>(potentialRiffData), 4))
	{
		return;
	}

	const int remainingSizeInBytes = *reinterpret_cast<const int*>(potentialRiffData + 4);

	// Don't bother if it only has the type id.
	if (remainingSizeInBytes <= 4)
	{
		return;
	}

	if (0 != strncmp("WAVE", reinterpret_cast<const char*>(potentialRiffData + 8), 4))
	{
		return;
	}

	// Store the size of the data section.
	m_ChunkReader = {potentialRiffData + 12, static_cast<std::size_t>(remainingSizeInBytes - 4)};
}

inline std::optional<RiffChunk> RiffChunkReader::Next()
{
	const auto end = m_Data + m_Size;

	// No valid chunks left.
	if ((end - m_ReadPosition) < 8)
	{
		return {};
	}

	const char* chunkId = reinterpret_cast<const char*>(m_ReadPosition);
	const int size = *reinterpret_cast<const int*>(m_ReadPosition + 4);

	if (size < 0)
	{
		return {};
	}

	const RiffChunk chunk{{chunkId, 4}, static_cast<std::size_t>(size), m_ReadPosition + 8};

	// Align to 2 byte boundary.
	m_ReadPosition += (size + 8 + 1) & ~1;

	return chunk;
}

inline std::optional<RiffChunk> RiffChunkReader::FindChunk(std::string_view chunkId)
{
	for (auto chunk = Next(); chunk; chunk = Next())
	{
		if (chunk->ChunkId == chunkId)
		{
			return chunk;
		}
	}

	return {};
}
