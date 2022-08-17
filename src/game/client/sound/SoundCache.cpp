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

#include <AL/alext.h>

#include "cbase.h"
#include "RiffChunkReader.h"
#include "SoundCache.h"

namespace sound
{
SoundCache::SoundCache(std::shared_ptr<spdlog::logger> logger)
	: m_Logger(logger), m_Loader(std::make_unique<nqr::NyquistIO>())
{
}

SoundIndex SoundCache::FindName(const RelativeFilename& fileName)
{
	for (const auto& sound : m_Sounds)
	{
		if (sound.Name.comparei(fileName) == 0)
		{
			return MakeSoundIndex(&sound);
		}
	}

	if (m_Sounds.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max()))
	{
		// Chances are you'll run out of memory before this happens.
		m_Logger->warn("Out of sound slots, cannot load {} (maximum {})", fileName.c_str(), std::numeric_limits<int>::max());
		return {};
	}

	m_Sounds.emplace_back(fileName);

	return MakeSoundIndex(&m_Sounds.back());
}

Sound* SoundCache::GetSound(SoundIndex index)
{
	const int i = index.Index - 1;

	if (i < 0 || static_cast<std::size_t>(i) >= m_Sounds.size())
	{
		return nullptr;
	}

	return &m_Sounds[i];
}

bool SoundCache::LoadSound(Sound& sound)
{
	if (sound.Buffer.IsValid())
	{
		return true;
	}

	m_Logger->trace("Loading sound {}", sound.Name.c_str());

	if (sound.Name.empty())
	{
		m_Logger->error("Sound has no name");
		return false;
	}

	Filename completeFileName = SoundDirectoryName;

	if (sound.Name.front() != '/')
	{
		completeFileName += '/';
	}

	completeFileName.append(sound.Name.begin(), sound.Name.end());

	std::string absolutePath;
	absolutePath.resize(MAX_PATH_LENGTH);

	if (!g_pFileSystem->GetLocalPath(completeFileName.c_str(), absolutePath.data(), absolutePath.size()))
	{
		// TODO: add support for EASTL types to fmt formatting.
		m_Logger->error("Could not find sound file {}", sound.Name.c_str());
		return false;
	}

	// Trim the size to the actual string's size.
	absolutePath.resize(strlen(absolutePath.c_str()));

	nqr::AudioData data;

	try
	{
		m_Loader->Load(&data, absolutePath);
	}
	catch (const std::exception& e)
	{
		m_Logger->error("Error loading sound file {}: {}", sound.Name.c_str(), e.what());
		return false;
	}

	const ALenum format = data.channelCount == 1 ? AL_FORMAT_MONO_FLOAT32 : AL_FORMAT_STEREO_FLOAT32;

	sound.Buffer = OpenALBuffer::Create();

	m_Logger->trace("Loading sound {} into buffer {}", absolutePath, sound.Buffer.Id);

	alBufferData(sound.Buffer.Id, format, data.samples.data(), data.samples.size() * sizeof(float), data.sampleRate);

	// See https://openal-soft.org/openal-extensions/SOFT_loop_points.txt
	auto cuePoints = TryLoadCuePoints(absolutePath, data.samples.size());

	sound.IsLooping = cuePoints.has_value();

	if (cuePoints)
	{
		const ALint loopPoints[2] = {std::get<0>(*cuePoints), std::get<1>(*cuePoints)};
		alBufferiv(sound.Buffer.Id, AL_LOOP_POINTS_SOFT, loopPoints);
	}

	sound.Format = format;

	// Cache the samples for future use.
	sound.Samples = std::move(data.samples);

	return true;
}

void SoundCache::ClearBuffers()
{
	for (auto& sound : m_Sounds)
	{
		sound.Samples.clear();
		sound.Buffer.Delete();
	}
}

SoundIndex SoundCache::MakeSoundIndex(const Sound* sound) const
{
	return SoundIndex{(sound - m_Sounds.data()) + 1};
}

std::optional<std::tuple<ALint, ALint>> SoundCache::TryLoadCuePoints(const std::string& fileName, ALint sampleCount)
{
	m_Logger->trace("Trying to load loop info from file \"{}\"", fileName);

	if (!fileName.ends_with(CuePointExtension))
	{
		m_Logger->trace("File \"{}\" does not have the {} extension, ignoring", fileName, CuePointExtension);
		return {};
	}

	std::unique_ptr<FILE, decltype(fclose)*> file{fopen(fileName.c_str(), "rb"), &fclose};

	if (!file)
	{
		m_Logger->error("Couldn't open file \"{}\" for reading", fileName);
		return {};
	}

	fseek(file.get(), 0, SEEK_END);
	const long size = ftell(file.get());

	if (size < 0)
	{
		m_Logger->error("Error seeking file \"{}\"", fileName);
		return {};
	}

	fseek(file.get(), 0, SEEK_SET);

	std::vector<std::byte> data;

	data.resize(size);

	if (fread(data.data(), 1, size, file.get()) != size)
	{
		m_Logger->error("Error reading file \"{}\"", fileName);
		return {};
	}

	file.reset();

	RiffFileReader fileReader{data.data(), data.size()};

	if (!fileReader)
	{
		m_Logger->error("Not a wave file: \"{}\"", fileName);
		return {};
	}

	auto chunkReader = fileReader.GetChunkReader();

	// Find the fmt chunk so we can get the original bit depth.
	const auto fmtChunk = chunkReader.FindChunk("fmt ");

	if (!fmtChunk)
	{
		// Should never happen since the file was already loaded before.
		m_Logger->error("Wave file \"{}\" missing fmt chunk", fileName);
		return {};
	}

	if (fmtChunk->Size < 16)
	{
		m_Logger->error("Wave file \"{}\" has bad fmt chunk", fileName);
		return {};
	}

	const std::int16_t bitDepth = *reinterpret_cast<const std::int16_t*>(fmtChunk->Data + 14);

	if (bitDepth < 0)
	{
		m_Logger->error("Wave file \"{}\" has bad bit depth {}", fileName, bitDepth);
		return {};
	}

	const int width = bitDepth / 8;

	chunkReader = fileReader.GetChunkReader();

	// Search for a cue chunk.
	// See https://www.recordingblogs.com/wiki/cue-chunk-of-a-wave-file for more information.
	const auto cueChunk = chunkReader.FindChunk("cue ");

	if (!cueChunk)
	{
		m_Logger->debug("No cue chunk in file \"{}\"", fileName);
		return {};
	}

	const int numberOfCuePoints = *reinterpret_cast<const int*>(cueChunk->Data);

	// Note: this check does not exist in Quake and GoldSource.
	if (numberOfCuePoints <= 0)
	{
		return {};
	}

	const auto convertRawSampleCount = [&](int rawSampleCount)
	{
		return rawSampleCount / width;
	};

	// There is at least one cue point in this file. Use the first one to denote the loop start point.
	const int cuePositionInBytes = *reinterpret_cast<const int*>(cueChunk->Data + 4 + 20);

	// Convert loop start from bytes to 32 bit float samples.
	std::tuple<ALint, ALint> loopPoints = {convertRawSampleCount(cuePositionInBytes), sampleCount};

	m_Logger->trace("Loaded loop start point {} from file \"{}\"", std::get<0>(loopPoints), fileName);

	// See if there's a list chunk containing an associated data list following the cue chunk.
	// See https://www.recordingblogs.com/wiki/associated-data-list-chunk-of-a-wave-file for more information.
	const auto listChunk = chunkReader.FindChunk("LIST");

	if (!listChunk)
	{
		return loopPoints;
	}

	if (0 != strncmp("adtl", reinterpret_cast<const char*>(listChunk->Data), 4))
	{
		return loopPoints;
	}

	RiffChunkReader adtlChunkReader{listChunk->Data + 4, listChunk->Size};

	// Note: Quake and GoldSource assume this chunk exists and is present as the first sub chunk.
	auto ltxtChunk = adtlChunkReader.FindChunk("ltxt");

	if (!ltxtChunk)
	{
		return loopPoints;
	}

	if (ltxtChunk->Size < 12)
	{
		m_Logger->error("Wave file \"{}\" has bad ltxt subchunk", fileName);
		return loopPoints;
	}

	const std::string_view purposeId{reinterpret_cast<const char*>(ltxtChunk->Data + 8), 4};

	if (purposeId != "mark")
	{
		m_Logger->debug("Wave file \"{}\" has ltxt subchunk with purpose ID \"{}\" (expected \"mark\")", fileName, purposeId);
		return loopPoints;
	}

	const int remainingSampleCount = *reinterpret_cast<const int*>(ltxtChunk->Data + 4);
	const int sampleEndInBytes = cuePositionInBytes + remainingSampleCount;

	std::get<1>(loopPoints) = convertRawSampleCount(sampleEndInBytes);

	m_Logger->trace("Loaded loop end point {} from file \"{}\"", std::get<1>(loopPoints), fileName);

	return loopPoints;
}
}
