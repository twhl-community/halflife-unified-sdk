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

#include <algorithm>
#include <charconv>
#include <iterator>

#include "cbase.h"
#include "sentence_utils.h"

using namespace std::string_view_literals;

namespace sentences
{
constexpr std::string_view DefaultSentenceDirectory{"vox/"sv};
constexpr char voxperiod[]{"_period"};
constexpr char voxcomma[]{"_comma"};

struct SentenceWordParameterInfo
{
	const char Type;
	const int Min;
	const int Max;
	int SentenceWordParameters::*const Member;
};

static constexpr SentenceWordParameterInfo SentenceWordParameterInfos[] =
	{
		{'v', 0, 100, &SentenceWordParameters::Volume},
		{'p', 1, 255, &SentenceWordParameters::Pitch},
		{'t', 0, 100, &SentenceWordParameters::TimeCompress},
		{'s', 0, 100, &SentenceWordParameters::Start},
		{'e', 0, 100, &SentenceWordParameters::End}};

static const SentenceWordParameterInfo* FindParameterInfo(char type)
{
	for (const auto& info : SentenceWordParameterInfos)
	{
		if (info.Type == type)
		{
			return &info;
		}
	}

	return nullptr;
}

std::tuple<std::string_view, std::string_view> ParseSentence(std::string_view text)
{
	// First, see if there's a comment in there somewhere and trim to that.
	const auto commentStart = text.find("//");

	if (commentStart != text.npos)
	{
		text = text.substr(0, commentStart);
	}

	// Skip whitespace.
	const auto nameBegin = std::find_if(text.begin(), text.end(), [](auto c)
		{ return 0 == std::isspace(c); });

	if (nameBegin == text.end())
	{
		return {};
	}

	// Get sentence name.
	const auto it = std::find_if(nameBegin, text.end(), [](auto c)
		{ return 0 != std::isspace(c); });

	if (it == text.end())
	{
		return {};
	}

	const std::string_view name{nameBegin, it};

	// Skip whitespace.
	const auto sentenceBegin = std::find_if(it, text.end(), [](auto c)
		{ return 0 == std::isspace(c); });

	if (sentenceBegin == text.end())
	{
		return {};
	}

	std::string_view::reverse_iterator rev{sentenceBegin};

	// Find end of sentence.
	const auto sentenceEnd = std::find_if(text.rbegin(), std::make_reverse_iterator(sentenceBegin), [](auto c)
		{ return 0 == std::isspace(c); });

	// Should never happen.
	if (sentenceEnd == text.rend())
	{
		return {};
	}

	const std::string_view sentence{sentenceBegin, sentenceEnd.base()};

	return {name, sentence};
}

std::optional<std::tuple<std::string_view, int>> ParseGroupData(std::string_view name)
{
	const auto indexStart = std::find_if(name.rbegin(), name.rend(), [](auto c)
		{ return 0 == std::isdigit(c); });

	if (indexStart == name.rend())
	{
		return {};
	}

	// A name can't consist solely of a number.
	if (indexStart == name.rbegin())
	{
		return {};
	}

	const auto nameLength = indexStart.base() - name.begin();

	int index = 0;
	const auto result = std::from_chars(name.data() + nameLength, name.data() + name.size(), index);

	if (result.ec != std::errc())
	{
		return {};
	}

	return std::tuple{name.substr(0, nameLength), index};
}

std::tuple<std::string_view, std::string_view> ParseDirectory(std::string_view sentence)
{
	// Parse from back to front, stopping at the first slash. This allows directories with multiple subdirectories to be used.
	for (auto it = sentence.rbegin(), end = sentence.rend(); it != end; ++it)
	{
		if (*it == '/')
		{
			const auto offset = it.base() - sentence.begin();
			return {sentence.substr(0, offset), sentence.substr(offset)};
		}
	}

	// No directory found, use default.
	return {DefaultSentenceDirectory, sentence};
}

std::optional<std::tuple<std::string_view, std::string_view>> SentencesListParser::Next()
{
	if (m_Contents.empty())
	{
		return {};
	}

	// Find the next valid sentence entry.
	while (true)
	{
		const auto line = GetLine(m_Contents);

		if (line.empty())
		{
			break;
		}

		const auto sentence = ParseSentence(line);

		const std::string_view name = std::get<0>(sentence);

		if (name.empty())
		{
			// Comment, empty line or whitespace.
			continue;
		}

		if (0 == std::isalpha(name.front()))
		{
			m_Logger.warn("Sentence {} must start with an alphabetic character", name);
			continue;
		}

		if (name.size() >= sentences::CBSENTENCENAME_MAX)
		{
			m_Logger.warn("Sentence {} longer than {} letters", name, sentences::CBSENTENCENAME_MAX - 1);
			continue;
		}

		return sentence;
	}

	return {};
}

bool SentenceWordParser::Parse()
{
	while (true)
	{
		switch (ParseCore())
		{
		case WordParseResult::Done: return false;
		case WordParseResult::ParsedWord:
			return true;

			// We parsed the global parameters or there was a syntax error, need to parse the next word.
		case WordParseResult::SkipWord: break;
		}
	}
}

SentenceWordParser::WordParseResult SentenceWordParser::ParseCore()
{
	Parameters = m_GlobalParameters;

	const auto end = Sentence.end();

	// Skip whitespace.
	m_Next = std::find_if(m_Next, end, [](auto c)
		{ return 0 == std::isspace(c); });

	if (m_Next == end)
	{
		return WordParseResult::Done;
	}

	// A word is a line of text containing no whitespace, optionally followed by (), which contains zero or more parameters.
	// A parameter is a single character indicating the type followed by the value, which is a number.
	// Special handling is needed for the characters '.' and ',', which translate to _period and _comma respectively.

	if (CheckForSpecialCharacters())
	{
		++m_Next;
		// Ignore parameters for these.
		// TODO: probably ignored in the engine because they didn't want to allocate memory, so this could be enabled.
		SentenceWordParameters dummy;
		return ParseParameters(dummy);
	}

	// Parse word until whitespace or parameters.
	const auto start = m_Next;

	for (; m_Next != end; ++m_Next)
	{
		if (std::isspace(*m_Next) != 0 || *m_Next == '(' || *m_Next == ')' || *m_Next == '.' || *m_Next == ',')
		{
			break;
		}
	}

	const std::string_view word{start, m_Next};
	Word.assign(word.data(), word.size());

	const WordParseResult result = ParseParameters(Parameters);

	if (Word.empty())
	{
		// It's a set of parameters by itself, apply to all subsequent words.
		m_GlobalParameters = Parameters;
		return WordParseResult::SkipWord;
	}

	return result;
}

bool SentenceWordParser::CheckForSpecialCharacters()
{
	switch (*m_Next)
	{
	case '.':
		Word = voxperiod;
		return true;
	case ',':
		Word = voxcomma;
		return true;

	default: return false;
	}
}

SentenceWordParser::WordParseResult SentenceWordParser::ParseParameters(SentenceWordParameters& params)
{
	const auto end = Sentence.end();

	if (m_Next == end)
	{
		// End of sentence.
		return WordParseResult::ParsedWord;
	}

	if (*m_Next == ')')
	{
		// Malformed parameter section.
		++m_Next;
		return WordParseResult::SkipWord;
	}

	if (*m_Next != '(')
	{
		// Word has no parameters.
		return WordParseResult::ParsedWord;
	}

	++m_Next;

	while (m_Next != end)
	{
		const char type = *m_Next++;

		if (type == ')')
		{
			break;
		}

		if (const auto info = FindParameterInfo(type); info)
		{
			if (m_Next == end || 0 == std::isdigit(*m_Next))
			{
				// Invalid syntax, skip to end of parameter section.
				m_Next = std::find(m_Next, end, ')');

				if (m_Next != end)
				{
					++m_Next;
				}

				break;
			}

			// Parse out up to 7 digits.
			eastl::fixed_string<char, 8, false> number;

			while (m_Next != end)
			{
				number.push_back(*m_Next);

				// Don't swallow non-digit characters.
				if (0 == std::isdigit(number.back()) || number.size() >= (number.capacity() - 1))
				{
					break;
				}

				++m_Next;
			}

			const int parsedValue = atoi(number.c_str());

			const int clampedValue = std::clamp(parsedValue, info->Min, info->Max);

			params.*info->Member = clampedValue;
		}
	}

	return WordParseResult::ParsedWord;
}
}
