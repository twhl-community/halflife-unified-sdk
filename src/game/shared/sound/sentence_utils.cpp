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

namespace sentences
{
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
}
