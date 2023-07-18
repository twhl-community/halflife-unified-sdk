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
#include <vector>

#include <spdlog/logger.h>

#include "SoundDefs.h"

#include "utils/json_fwd.h"

namespace sound
{
class SoundCache;

/**
 *	@brief Manages sentences, sentence playback and entity mouth movement.
 */
class SentencesSystem final
{
public:
	static constexpr std::size_t InvalidIndex = std::numeric_limits<std::size_t>::max();

	SentencesSystem(std::shared_ptr<spdlog::logger> logger, SoundCache* soundCache);

	SentencesSystem(const SentencesSystem&) = delete;
	SentencesSystem& operator=(const SentencesSystem&) = delete;

	void HandleNetworkDataBlock(NetworkDataBlock& block);

	void Clear();

	std::size_t FindSentence(const char* name) const;

	const Sentence* GetSentence(std::size_t index) const;

	bool SetSentenceWord(Channel& channel, SentenceChannel& sentenceChannel);

	bool UpdateSentence(Channel& channel);

	void InitMouth(int entityIndex, int channelIndex);
	void CloseMouth(int entityIndex, int channelIndex);

private:
	bool CreateTimeCompressedBuffer(const Channel& channel, SentenceChannel& sentenceChannel, const Sound& sound);

	bool UpdateSentencePlayback(Channel& channel, SentenceChannel& sentenceChannel);

	void MoveMouth(Channel& channel, SentenceChannel& sentenceChannel);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	SoundCache* const m_SoundCache;

	std::vector<Sentence> m_Sentences;
};
}
