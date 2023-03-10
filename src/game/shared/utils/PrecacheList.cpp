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

#include <algorithm>

#include "cbase.h"

#include "PrecacheList.h"

int PrecacheList::IndexOf(const char* str) const
{
	if (auto it = std::find_if(m_Precaches.begin(), m_Precaches.end(), [&](const auto candidate)
			{ return 0 == stricmp(candidate, str); });
		it != m_Precaches.end())
	{
		return static_cast<int>(it - m_Precaches.begin());
	}

	return -1;
}

int PrecacheList::Add(const char* str)
{
	assert(str);
	assert(str[0] > ' ');

	if (m_ValidationFunction)
	{
		str = m_ValidationFunction(str, this);
	}

	if (!str)
	{
		return 0;
	}

	if (auto index = IndexOf(str); index != -1)
	{
		LogString(spdlog::level::trace, "existing", str, index);
		return index;
	}

	const int index = static_cast<int>(m_Precaches.size());

	m_Precaches.push_back(str);

	// This should never happen since the engine doesn't precache anything until after the game has finished.
	// If it does happen it's because something else is calling precache functions directly.
	if (m_EnginePrecacheFunction)
	{
		// This call could cause a host error and may not return.
		if (const int engineIndex = m_EnginePrecacheFunction(str); engineIndex != index)
		{
			m_Logger->error("Precached file \"{}\" has mismatched index (engine: {}, local: {})", str, engineIndex, index);
			assert(!"Mismatched precache file index");
		}
	}

	LogString(spdlog::level::debug, "new", str, index);

	return index;
}

void PrecacheList::AddUnchecked(const char* str)
{
	assert(str);
	m_Precaches.push_back(str);
}

void PrecacheList::Clear()
{
	m_Precaches.clear();

	// First entry is the empty string (invalid).
	m_Precaches.push_back("");
}

void PrecacheList::LogString(spdlog::level::level_enum level, const char* state, const char* str, int index)
{
	m_Logger->log(level, "[{}] {} \"{}\"{} ({})", m_Type, state, str, str == gpGlobals->pStringBase ? " (Null string_t)" : "", index);
}
