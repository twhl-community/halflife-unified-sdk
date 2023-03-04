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

#include <cassert>

#include "networking/NetworkDataSystem.h"
#include "utils/GameSystem.h"
#include "utils/ReplacementMaps.h"

namespace sound
{
class ClientSoundReplacementSystem final : public IGameSystem, public INetworkDataBlockHandler
{
public:
	const char* GetName() const override { return "ClientSoundReplacement"; }

	bool Initialize() override
	{
		g_NetworkData.RegisterHandler("GlobalSoundReplacement", this);
		return true;
	}

	void PostInitialize() override {}

	void Shutdown() override {}

	void OnBeginNetworkDataProcessing() override
	{
		m_SoundReplacement.reset();
	}

	void OnEndNetworkDataProcessing() override
	{
		// If we didn't receive any data we should use a dummy replacement map to avoid crashes.
		if (!m_SoundReplacement)
		{
			m_SoundReplacement = std::make_unique<ReplacementMap>(Replacements{}, false);
		}
	}

	void HandleNetworkDataBlock(NetworkDataBlock& block) override
	{
		m_SoundReplacement = g_ReplacementMaps.Deserialize(block.Data);
	}

	const char* Lookup(const char* value) const noexcept
	{
		assert(m_SoundReplacement);
		return m_SoundReplacement->Lookup(value);
	}

private:
	// Make sure this is valid in case any calls come in before we load the data.
	std::unique_ptr<ReplacementMap> m_SoundReplacement = std::make_unique<ReplacementMap>(Replacements{}, false);
};

inline ClientSoundReplacementSystem g_ClientSoundReplacement;
}
