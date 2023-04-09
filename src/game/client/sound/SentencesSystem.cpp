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

#include <limits>

#include <AL/alext.h>

#include "cbase.h"
#include "SentencesSystem.h"
#include "SoundCache.h"
#include "view.h"

#include "sound/sentence_utils.h"
#include "utils/JSONSystem.h"

namespace sound
{
SentencesSystem::SentencesSystem(std::shared_ptr<spdlog::logger> logger, SoundCache* soundCache)
	: m_Logger(logger),
	  m_SoundCache(soundCache)
{
}

void SentencesSystem::HandleNetworkDataBlock(NetworkDataBlock& block)
{
	assert(m_Sentences.empty());

	if (!block.Data.is_array())
	{
		block.ErrorMessage = "Sentences network data has invalid format (must be array of strings)";
		return;
	}

	RelativeFilename wordFileName;

	for (const auto& inputSentence : block.Data)
	{
		if (!inputSentence.is_string())
		{
			block.ErrorMessage = "Sentences network data has invalid format (array elements must be strings)";
			return;
		}

		if (m_Sentences.size() >= sentences::MaxSentencesCount)
		{
			m_Logger->error("Too many sentences in list!");
			break;
		}

		const auto sentenceContents = inputSentence.get<std::string>();

		const auto sentence = sentences::ParseSentence(sentenceContents);

		const std::string_view name = std::get<0>(sentence);
		const std::string_view value = std::get<1>(sentence);

		// This should never happen since the server validates input at parse time.
		if (name.empty() || value.empty())
		{
			block.ErrorMessage = "Invalid sentence in list";
			return;
		}

		// Parse sentence contents.
		Sentence sentenceToAdd;

		sentenceToAdd.Name = sentences::SentenceName{name.data(), name.size()};

		const auto directoryInfo = sentences::ParseDirectory(value);
		const std::string_view directoryName{std::get<0>(directoryInfo)};

		sentences::SentenceWordParser wordParser{std::get<1>(directoryInfo)};

		while (wordParser.Parse())
		{
			SentenceWord word;

			word.Parameters = wordParser.Parameters;

			wordFileName.clear();
			wordFileName.append(directoryName.data(), directoryName.size());
			wordFileName.append(wordParser.Word.data(), wordParser.Word.size());
			wordFileName.append(SentenceWordExtension);

			word.Index = m_SoundCache->FindName(wordFileName);

			sentenceToAdd.Words.push_back(word);
		}

		m_Sentences.push_back(sentenceToAdd);
	}

	g_NetworkData.GetLogger()->debug("Parsed {} sentences from network data", m_Sentences.size());
}

void SentencesSystem::Clear()
{
	m_Sentences.clear();
}

std::size_t SentencesSystem::FindSentence(const char* name) const
{
	for (const auto& sentence : m_Sentences)
	{
		if (sentence.Name == name)
		{
			return &sentence - m_Sentences.data();
		}
	}

	return InvalidIndex;
}

const Sentence* SentencesSystem::GetSentence(std::size_t index) const
{
	if (index < m_Sentences.size())
	{
		return &m_Sentences[index];
	}

	return nullptr;
}

bool SentencesSystem::SetSentenceWord(Channel& channel, SentenceChannel& sentenceChannel)
{
	const auto& sentence = m_Sentences[sentenceChannel.Sentence];

	if (sentenceChannel.CurrentWord >= sentence.Words.size())
	{
		return false;
	}

	const auto& word = sentence.Words[sentenceChannel.CurrentWord];

	if (auto wordSound = m_SoundCache->GetSound(word.Index); wordSound && m_SoundCache->LoadSound(*wordSound))
	{
		if (word.Parameters.TimeCompress != 0)
		{
			if (!sentenceChannel.TimeCompressBuffer.IsValid())
			{
				sentenceChannel.TimeCompressBuffer = OpenALBuffer::Create();
			}

			// Detach buffer first so it can be modified.
			alSourcei(channel.Source.Id, AL_BUFFER, NullBuffer);

			if (!CreateTimeCompressedBuffer(channel, sentenceChannel, *wordSound))
			{
				return false;
			}

			alSourcei(channel.Source.Id, AL_BUFFER, sentenceChannel.TimeCompressBuffer.Id);
		}
		else
		{
			alSourcei(channel.Source.Id, AL_BUFFER, wordSound->Buffer.Id);

			if (word.Parameters.Start != 0)
			{
				ALint size = 0;
				alGetBufferi(wordSound->Buffer.Id, AL_SIZE, &size);

				const ALint startOffset = static_cast<ALint>(size * (word.Parameters.Start / 100.f));

				alSourcei(channel.Source.Id, AL_BYTE_OFFSET, startOffset);
			}
		}

		// Mix sentence-wide and word-specific pitch values.
		const float pitch = std::clamp(channel.Pitch + (word.Parameters.Pitch - PITCH_NORM), 1, 255) / 100.f;

		alSourcef(channel.Source.Id, AL_GAIN, word.Parameters.Volume / 100.f);
		alSourcef(channel.Source.Id, AL_PITCH, pitch);

		return true;
	}

	return false;
}

bool SentencesSystem::UpdateSentence(Channel& channel)
{
	auto& sentenceChannel = std::get<SentenceChannel>(channel.Sound);

	const bool shouldRemove = UpdateSentencePlayback(channel, sentenceChannel);

	if (!shouldRemove)
	{
		MoveMouth(channel, sentenceChannel);
	}

	return shouldRemove;
}

void SentencesSystem::InitMouth(int entityIndex, int channelIndex)
{
	if ((channelIndex == CHAN_VOICE || channelIndex == CHAN_STREAM) && entityIndex > 0)
	{
		auto entity = gEngfuncs.GetEntityByIndex(entityIndex);

		if (entity)
		{
			auto& mouth = entity->mouth;

			mouth.mouthopen = 0;
			mouth.sndavg = 0;
			mouth.sndcount = 0;
		}
	}
}

void SentencesSystem::CloseMouth(int entityIndex, int channelIndex)
{
	if ((channelIndex == CHAN_VOICE || channelIndex == CHAN_STREAM) && entityIndex > 0)
	{
		auto entity = gEngfuncs.GetEntityByIndex(entityIndex);

		if (entity)
		{
			auto& mouth = entity->mouth;

			mouth.mouthopen = 0;
		}
	}
}

void SentencesSystem::MoveMouth(Channel& channel, SentenceChannel& sentenceChannel)
{
	if (channel.ChannelIndex != CHAN_VOICE && channel.ChannelIndex != CHAN_STREAM)
	{
		return;
	}

	const auto& sentence = m_Sentences[sentenceChannel.Sentence];
	const auto& word = sentence.Words[sentenceChannel.CurrentWord];
	const auto& sound = *m_SoundCache->GetSound(word.Index);

	ALint frequency = 0;
	alGetBufferi(sound.Buffer.Id, AL_FREQUENCY, &frequency);

	ALint sampleOffset = 0;
	alGetSourcei(channel.Source.Id, AL_SAMPLE_OFFSET, &sampleOffset);

	const std::size_t maxSamples = static_cast<std::size_t>(frequency * MouthSampleRange);

	const std::size_t samplesToCheck = std::min(maxSamples, sound.Samples.size() - static_cast<std::size_t>(sampleOffset));

	const auto entity = gEngfuncs.GetEntityByIndex(channel.EntityIndex);

	if (!entity)
	{
		// Probably no map loaded.
		return;
	}

	auto& mouth = entity->mouth;

	int average = 0;

	for (std::size_t sampleIndex = 0; sampleIndex < samplesToCheck && mouth.sndcount < MouthSamplesRequired; ++mouth.sndcount)
	{
		const float sample = sound.Samples[static_cast<std::size_t>(sampleOffset) + sampleIndex];

		// Rescale the sample range from [-1, 1] to [-128, 127].
		const int scaledSample = std::clamp(
			static_cast<int>(sample * 128),
			static_cast<int>(std::numeric_limits<std::int8_t>::min()),
			static_cast<int>(std::numeric_limits<std::int8_t>::max()));

		average += std::abs(scaledSample);

		// Find another sample to use.
		sampleIndex += 80 + (scaledSample & 0x1F);
	}

	mouth.sndavg += average;

	if (mouth.sndcount >= MouthSamplesRequired)
	{
		mouth.mouthopen = mouth.sndavg / MouthSamplesRequired;
		mouth.sndavg = 0;
		mouth.sndcount = 0;
	}
}

bool SentencesSystem::CreateTimeCompressedBuffer(const Channel& channel, SentenceChannel& sentenceChannel, const Sound& sound)
{
	const auto& sentence = m_Sentences[sentenceChannel.Sentence];
	const auto& word = sentence.Words[sentenceChannel.CurrentWord];
	const auto& wordSound = *m_SoundCache->GetSound(word.Index);

	m_Logger->trace("Creating time compressed buffer for sentence {}, word {} (index {})",
		sentence.Name.c_str(), wordSound.Name.c_str(), sentenceChannel.CurrentWord);

	const float skipFraction = word.Parameters.TimeCompress / 100.f;
	const float writeFraction = 1.f - skipFraction;

	const std::size_t channelCount = wordSound.Format == AL_FORMAT_MONO_FLOAT32 ? 1 : 2;

	ALint size = 0;
	alGetBufferi(sound.Buffer.Id, AL_SIZE, &size);

	const ALint startOffset = static_cast<ALint>(size * (word.Parameters.Start / 100.f));
	const ALint endOffsetFromEnd = static_cast<ALint>(size * ((100 - word.Parameters.End) / 100.f));

	const std::size_t numberOfActualSamples = (size - startOffset - endOffsetFromEnd) / sizeof(float);

	// Use the logical number of samples to minimize loss of samples due to fractional calculations.
	const std::size_t numberOfLogicalSamples = numberOfActualSamples / channelCount;

	std::vector<float> compressedData;
	compressedData.reserve(static_cast<std::size_t>(numberOfActualSamples * writeFraction));

	const std::size_t chunkSize = numberOfLogicalSamples / TimeCompressChunkCount;

	const std::size_t startIndex = static_cast<std::size_t>((wordSound.Samples.size() / static_cast<float>(channelCount)) * (word.Parameters.Start / 100.f));

	bool isFirstIteration = true;

	for (std::size_t readIndex = startIndex; readIndex < numberOfLogicalSamples;)
	{
		const std::size_t currentChunkSize = std::min(chunkSize, numberOfLogicalSamples - readIndex);

		const std::size_t chunkEndPos = readIndex + currentChunkSize;

		// The first iteration uses the full chunk, all others skip the skip fraction.
		if (isFirstIteration)
		{
			isFirstIteration = false;
		}
		else
		{
			readIndex += static_cast<std::size_t>(currentChunkSize * skipFraction);
		}

		compressedData.insert(
			compressedData.end(),
			wordSound.Samples.begin() + (readIndex * channelCount),
			wordSound.Samples.begin() + (chunkEndPos * channelCount));

		readIndex = chunkEndPos;
	}

	ALint frequency = -1;
	alGetBufferi(wordSound.Buffer.Id, AL_FREQUENCY, &frequency);

	// Clear error state.
	alGetError();

	alBufferData(sentenceChannel.TimeCompressBuffer.Id, wordSound.Format, compressedData.data(), compressedData.size() * sizeof(float), frequency);

	if (const auto error = alGetError(); error != AL_NO_ERROR)
	{
		m_Logger->error("OpenAL error {} ({}) while initializing time compressed buffer", alGetString(error), error);
		return false;
	}

	return true;
}

bool SentencesSystem::UpdateSentencePlayback(Channel& channel, SentenceChannel& sentenceChannel)
{
	const auto& sentence = m_Sentences[sentenceChannel.Sentence];
	const auto& word = sentence.Words[sentenceChannel.CurrentWord];

	ALint state = AL_STOPPED;
	alGetSourcei(channel.Source.Id, AL_SOURCE_STATE, &state);

	if (state != AL_STOPPED)
	{
		// Time compressed sounds have already been modified to account for end clipping.
		if (word.Parameters.End == 100 || word.Parameters.TimeCompress != 0)
		{
			return false;
		}

		const auto wordSound = m_SoundCache->GetSound(word.Index);

		ALint size = 0;
		alGetBufferi(wordSound->Buffer.Id, AL_SIZE, &size);

		ALint offset = 0;
		alGetSourcei(channel.Source.Id, AL_BYTE_OFFSET, &offset);

		const ALint endOffset = static_cast<ALint>(size * (word.Parameters.End / 100.f));

		if (offset < endOffset)
		{
			return false;
		}

		alSourceStop(channel.Source.Id);
	}

	do
	{
		++sentenceChannel.CurrentWord;
	} while (sentenceChannel.CurrentWord < sentence.Words.size() && !SetSentenceWord(channel, sentenceChannel));

	if (sentenceChannel.CurrentWord < sentence.Words.size())
	{
		alSourcePlay(channel.Source.Id);
		return false;
	}

	return true;
}
}
