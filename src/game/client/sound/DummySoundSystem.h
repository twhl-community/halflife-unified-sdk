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

#include "IGameSoundSystem.h"
#include "IMusicSystem.h"
#include "ISoundSystem.h"

namespace sound
{
struct DummyGameSoundSystem final : public IGameSoundSystem
{
	~DummyGameSoundSystem() override = default;

	void HandleNetworkDataBlock(NetworkDataBlock& block) override {}

	void StartSound(
		int entnum, int entchannel,
		const char* soundOrSentence, const Vector& origin, float fvol, float attenuation, int flags, int pitch) override {}

	void StopAllSounds() override {}

	void MsgFunc_EmitSound(const char* pszName, int iSize, void* pbuf) override {}
};

struct DummyMusicSystem final : public IMusicSystem
{
	~DummyMusicSystem() override = default;

	void Play(std::string&& fileName, bool looping) override {}
	void Stop() override {}
	void Pause() override {}
	void Resume() override {}
};

struct DummySoundSystem final : public ISoundSystem
{
	~DummySoundSystem() override = default;

	void Update() override {}

	void Block() override {}

	void Unblock() override {}

	void Pause() override {}

	void Resume() override {}

	IGameSoundSystem* GetGameSoundSystem() override
	{
		return m_GameSoundSystem.get();
	}

	IMusicSystem* GetMusicSystem() override
	{
		return m_MusicSystem.get();
	}

private:
	const std::unique_ptr<DummyGameSoundSystem> m_GameSoundSystem = std::make_unique<DummyGameSoundSystem>();
	const std::unique_ptr<IMusicSystem> m_MusicSystem = std::make_unique<DummyMusicSystem>();
};
}
