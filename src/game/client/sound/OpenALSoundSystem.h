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

#include <spdlog/logger.h>

#include <AL/alc.h>

#include <libnyquist/Decoders.h>

#include "ISoundSystem.h"
#include "OpenALUtils.h"

class OpenALMusicSystem;

class OpenALSoundSystem final : public ISoundSystem
{
public:
	~OpenALSoundSystem() override;

	bool Create();

	void Block() override;

	void Unblock() override;

	void Pause() override;

	void Resume() override;

	IMusicSystem* GetMusicSystem() override;

	spdlog::logger* GetLogger() { return m_Logger.get(); }

	ALCdevice* GetDevice() { return m_Device.get(); }

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	std::unique_ptr<ALCdevice, DeleterWrapper<alcCloseDevice>> m_Device;

	std::unique_ptr<OpenALMusicSystem> m_MusicSystem;

	bool m_Blocked{false};
	bool m_Paused{false};
};
