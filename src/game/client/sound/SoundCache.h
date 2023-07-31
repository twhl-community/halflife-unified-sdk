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

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <spdlog/logger.h>

#include <libnyquist/Decoders.h>

#include "IGameSoundSystem.h"
#include "OpenALUtils.h"
#include "SoundDefs.h"
#include "utils/string_utils.h"

namespace sound
{
/**
 *	@brief Maintains a cache of sounds and provides a means of loading them.
 */
class SoundCache final
{
private:
	// Comparer that allows us to access the sound name in the set without making copies.
	struct LookupComparer
	{
		using is_transparent = void;

		explicit LookupComparer(std::vector<Sound>* cache)
			: m_Cache(cache)
		{
		}

		bool operator()(std::size_t lhs, std::size_t rhs) const
		{
			return UTIL_CompareI(ToStringView((*m_Cache)[lhs].Name), ToStringView((*m_Cache)[rhs].Name)) < 0;
		}

		bool operator()(std::string_view lhs, std::size_t rhs) const
		{
			return UTIL_CompareI(lhs, ToStringView((*m_Cache)[rhs].Name)) < 0;
		}

		bool operator()(std::size_t lhs, std::string_view rhs) const
		{
			return UTIL_CompareI(ToStringView((*m_Cache)[lhs].Name), rhs) < 0;
		}

	private:
		std::vector<Sound>* const m_Cache;
	};

public:
	explicit SoundCache(std::shared_ptr<spdlog::logger> logger);

	SoundIndex FindName(const RelativeFilename& fileName);

	Sound* GetSound(SoundIndex index);

	bool LoadSound(Sound& sound);

	void ClearBuffers();

	void Clear();

private:
	std::optional<std::tuple<ALint, ALint>> TryLoadCuePoints(
		const std::string& fileName, ALint sampleCount, int channelCount);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::vector<Sound> m_Sounds;

	std::set<std::size_t, LookupComparer> m_SoundLookup{LookupComparer{&m_Sounds}};

	std::unique_ptr<nqr::NyquistIO> m_Loader;
};
}
