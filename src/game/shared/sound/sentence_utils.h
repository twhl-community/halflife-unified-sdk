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

#include <limits>
#include <optional>
#include <string_view>
#include <tuple>
#include <vector>

#include <spdlog/logger.h>

#include <EASTL/fixed_string.h>
#include <EASTL/fixed_vector.h>

#include "cbase.h"

namespace sentences
{
constexpr int CBSENTENCENAME_MAX = 32;

constexpr int EngineMaxSentences = 1536;

/**
 *	@brief The amount of sentences to reserve space for on startup.
 *	This is the original engine limit.
 */
constexpr int InitialSentencesReserveCount = EngineMaxSentences;

/**
 *	@brief max number of sentences in game. This limit is defined by ServerSoundSystem::EmitSound's use of fixed size integer writing.
 */
constexpr std::size_t MaxSentencesCount = std::numeric_limits<std::uint16_t>::max();

/**
 *	@brief max number of elements per sentence group
 */
constexpr int CSENTENCE_LRU_MAX = 32;

using SentenceName = eastl::fixed_string<char, CBSENTENCENAME_MAX>;

using SentenceIndexName = eastl::fixed_string<char, CBSENTENCENAME_MAX + 1>;

/**
 *	@brief group of related sentences
 */
struct SentenceGroup
{
	SentenceName GroupName;
	eastl::fixed_vector<int, CSENTENCE_LRU_MAX> Sentences{};
	eastl::fixed_vector<unsigned char, CSENTENCE_LRU_MAX> LeastRecentlyUsed{};
};

/**
 *	@brief Parses a single sentence out of a line of text.
 *	@return Tuple containing the sentence name and the sentence itself.
 */
std::tuple<std::string_view, std::string_view> ParseSentence(std::string_view text);

/**
 *	@brief Given a sentence name, parses out the group index if present.
 *	@return If the name is correctly formatted, a tuple containing the group name and group index.
 */
std::optional<std::tuple<std::string_view, int>> ParseGroupData(std::string_view name);

/**
 *	@brief Parses the diectory out of a sentence.
 *	@details The directory starts at the beginning up to the last @c / character.
 *	@return Tuple containing the directory and the sentence after the directory.
 */
std::tuple<std::string_view, std::string_view> ParseDirectory(std::string_view sentence);

class SentencesListParser final
{
public:
	SentencesListParser(std::string_view contents, spdlog::logger& logger)
		: m_Contents(contents),
		  m_Logger(logger)
	{
	}

	std::optional<std::tuple<std::string_view, std::string_view>> Next();

private:
	std::string_view m_Contents;
	spdlog::logger& m_Logger;
};

struct SentenceWordParameters
{
	int Volume{100};
	int Pitch{PITCH_NORM};
	int Start{};
	int End{100};
	int TimeCompress{};
};

/**
 *	@brief Given a sentence, parses out the words and parameters.
 */
struct SentenceWordParser final
{
	const std::string_view Sentence;

	eastl::fixed_string<char, 256> Word;
	SentenceWordParameters Parameters;

	explicit SentenceWordParser(std::string_view sentence)
		: Sentence(sentence),
		  m_Next(Sentence.begin())
	{
	}

	bool Parse();

private:
	enum class WordParseResult
	{
		Done = 0,
		ParsedWord,
		SkipWord
	};

	WordParseResult ParseCore();

	bool CheckForSpecialCharacters();

	WordParseResult ParseParameters(SentenceWordParameters& params);

private:
	std::string_view::const_iterator m_Next;
	SentenceWordParameters m_GlobalParameters;
};
}
