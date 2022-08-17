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

#include <variant>

#include <EASTL/fixed_vector.h>

#include "Platform.h"

// TODO: move SoundIndex somewhere else
#include "IGameSoundSystem.h"
#include "OpenALUtils.h"

#include "sound/sentence_utils.h"

/**
 *	@brief The directory in the mod directory where sounds are stored.
 */
inline const Filename SoundDirectoryName{"sound"};

/**
 *	@brief Only try to load wave files to search for cue points if they have this extension.
 */
constexpr char CuePointExtension[]{".wav"};

/**
 *	@brief The file extension for sentence words.
 */
constexpr char SentenceWordExtension[]{".wav"};

/**
 *	@brief Maximum audible distance for a sound.
 */
constexpr vec_t NominalClippingDistance = 1000.0;

/**
 *	@brief Hard limit in the original engine, soft limit here. The vector will grow to accomodate more.
 */
constexpr std::size_t MaxWordsPerSentence = 32;

/**
 *	@brief The range, in seconds, to sample for mouths.
 */
constexpr float MouthSampleRange = 0.1f;

/**
 *	@brief Number of samples to sample before setting the mouthopen value.
 */
constexpr int MouthSamplesRequired = 10;

/**
 *	@brief Number of chunks to divide a sound in before applying time compression.
 */
constexpr std::size_t TimeCompressChunkCount = 8;

/**
 *	@brief The low pass high frequency gain to use when not underwater.
 */
constexpr float NormalLowPassHFGain = 0.38f;

/**
 *	@brief The low pass high frequency gain to use when underwater.
 */
constexpr float UnderwaterLowPassHFGain = 1.f;

namespace sound
{
/**
 *	@brief A single sound.
 *	@details Sounds should always be referred to using a @see SoundIndex.
 */
struct Sound
{
	RelativeFilename Name;
	OpenALBuffer Buffer;
	ALenum Format = 0;
	std::vector<float> Samples; // For sentences, to update mouths.
	bool IsLooping{false};

	explicit Sound(const RelativeFilename& filename)
		: Name(filename)
	{
	}

	Sound(Sound&&) = default;
	Sound& operator=(Sound&&) = default;
};

struct SentenceWord
{
	SoundIndex Index;
	sentences::SentenceWordParameters Parameters;
};

struct Sentence
{
	sentences::SentenceName Name;
	eastl::fixed_vector<SentenceWord, MaxWordsPerSentence> Words;
};

// Sentence playback needs to keep track of the current word.
struct SentenceChannel
{
	// Index into m_Sentences.
	std::size_t Sentence{0};
	std::size_t CurrentWord{0};

	OpenALBuffer TimeCompressBuffer;

	constexpr auto operator<=>(const SentenceChannel&) const = default;
};

using SoundData = std::variant<SoundIndex, SentenceChannel>;

struct Channel
{
	SoundData Sound;

	int EntityIndex{0};
	int ChannelIndex{0};
	int Pitch{PITCH_NORM};

	OpenALSource Source;
};
}
