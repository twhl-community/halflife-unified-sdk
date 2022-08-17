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
#include <string>
#include <tuple>
#include <vector>

#include <spdlog/logger.h>

#include <libnyquist/Decoders.h>

#include "IGameSoundSystem.h"
#include "OpenALUtils.h"
#include "SoundInternalDefs.h"

namespace sound
{
/**
 *	@brief Maintains a cache of sounds and provides a means of loading them.
 */
class SoundCache final
{
public:
	explicit SoundCache(std::shared_ptr<spdlog::logger> logger);

	SoundIndex FindName(const RelativeFilename& fileName);

	Sound* GetSound(SoundIndex index);

	bool LoadSound(Sound& sound);

	void ClearBuffers();

private:
	SoundIndex MakeSoundIndex(const Sound* sound) const;

	std::optional<std::tuple<ALint, ALint>> TryLoadCuePoints(const std::string& fileName, ALint sampleCount);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::vector<Sound> m_Sounds;

	std::unique_ptr<nqr::NyquistIO> m_Loader;
};
}
