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

#include "IMusicSystem.h"
#include "ISoundSystem.h"

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

	void Block() override {}

	void Unblock() override {}

	void Pause() override {}

	void Resume() override {}

	IMusicSystem* GetMusicSystem() override
	{
		return m_MusicSystem.get();
	}

private:
	const std::unique_ptr<IMusicSystem> m_MusicSystem = std::make_unique<DummyMusicSystem>();
};
