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

#include "networking/NetworkDataSystem.h"

#include "utils/json_fwd.h"

namespace sound
{
/**
 *	@brief Strongly typed sound index, for disambiguating overloads between <tt>const char*</tt> and <tt>int</tt>.
 */
struct SoundIndex final
{
	static constexpr int InvalidIndex = 0;

	constexpr SoundIndex() noexcept = default;

	explicit constexpr SoundIndex(int index) noexcept
		: Index(index)
	{
	}

	constexpr bool IsValid() const
	{
		return Index != InvalidIndex;
	}

	constexpr auto operator<=>(const SoundIndex&) const = default;

	int Index = InvalidIndex;
};

/**
 *	@brief Provides sound playback for game systems.
 */
struct IGameSoundSystem : public INetworkDataBlockHandler
{
	virtual ~IGameSoundSystem() = default;

	virtual void StartSound(
		int entityIndex, int channelIndex, const char* soundOrSentence, const Vector& origin, float volume, float attenuation, int pitch, int flags) = 0;

	virtual void StopAllSounds() = 0;

	virtual void MsgFunc_EmitSound(const char* pszName, int iSize, void* pbuf) = 0;
};
}
