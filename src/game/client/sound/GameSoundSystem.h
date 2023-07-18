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

#include <array>
#include <memory>
#include <string_view>
#include <vector>

#include <spdlog/logger.h>

#include "HalfLifeSoundEffects.h"
#include "IGameSoundSystem.h"
#include "SentencesSystem.h"
#include "SoundCache.h"
#include "SoundDefs.h"
#include "OpenALUtils.h"

struct cvar_t;

namespace sound
{
class GameSoundSystem final : public IGameSoundSystem
{
public:
	~GameSoundSystem() override;

	bool Create(std::shared_ptr<spdlog::logger> logger);

	void OnBeginNetworkDataProcessing() override;

	void HandleNetworkDataBlock(NetworkDataBlock& block) override;

	void Update();

	void Block();

	void Unblock();

	void Pause();

	void Resume();

	void StartSound(int entityIndex, int channelIndex, const char* soundOrSentence,
		const Vector& origin, float volume, float attenuation, int flags, int pitch) override;

	void StartSound(int entityIndex, int channelIndex, SoundData&& sound,
		const Vector& origin, float volume, float attenuation, int pitch, int flags) override;

	void StopAllSounds() override;

	void MsgFunc_EmitSound(const char* pszName, BufferReader& reader) override;

private:
	bool MakeCurrent();

	void PrintHRTFImplementations();
	void ConfigureHRTF(bool enabled);

	void SetVolume();

	std::string_view GetSoundName(const SoundData& sound) const;

	Channel* CreateChannel();

	/**
	 *	@brief Clears a channel in preparation to be reused.
	 */
	void ClearChannel(Channel& channel);

	void RemoveChannel(Channel& channel);

	template <typename FilterFunction>
	Channel* FindDynamicChannel(int entityIndex, FilterFunction&& filterFunction);

	Channel* FindOrCreateChannel(int entityIndex, int channelIndex);

	bool SetupChannel(Channel& channel, int entityIndex, int channelIndex,
		SoundData&& sound, const Vector& origin, float volume, int pitch, float attenuation, bool isRelative);

	bool AlterChannel(int entityIndex, int channelIndex, const SoundData& sound, float volume, int pitch, int flags);

	void UpdateRoomEffect();

	void UpdateSourceEffect(OpenALSource& source);

	void UpdateSounds();

	bool UpdateSound(Channel& channel);

	void Spatialize(Channel& channel, int messagenum);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::shared_ptr<spdlog::logger> m_CacheLogger;
	std::shared_ptr<spdlog::logger> m_SentencesLogger;

	cvar_t* m_Volume{};
	cvar_t* m_RoomOff{};
	cvar_t* m_RoomType{};
	cvar_t* m_WaterRoomType{};

	cvar_t* m_HRTFEnabled{};
	cvar_t* m_HRTFImplementation{};

	bool m_SupportsHRTF{false};
	bool m_CachedHRTFEnabled{false};

	std::unique_ptr<ALCdevice, DeleterWrapper<alcCloseDevice>> m_Device;
	std::unique_ptr<ALCcontext, DeleterWrapper<alcDestroyContext>> m_Context;

	std::array<ALuint, RoomEffectCount> m_RoomEffects{};

	ALuint m_AuxiliaryEffectSlot{0};

	ALuint m_Filter{0};

	int m_CurrentGameFrame{0};

	// Force a room type update on startup.
	bool m_PreviousRoomOn{true};
	int m_PreviousRoomType{-1};
	float m_PreviousHFGain{-1.f};

	std::unique_ptr<SoundCache> m_SoundCache;
	std::unique_ptr<SentencesSystem> m_Sentences;
	std::vector<Channel> m_Channels;

	bool m_Blocked{false};
	bool m_Paused{false};

	float m_LastKnownVolume{-1};

	std::vector<SoundIndex> m_PrecacheMap;
};
}
