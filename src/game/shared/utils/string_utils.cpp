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

#include <charconv>

#include <EASTL/string.h>

#include "Platform.h"

#include "string_utils.h"

std::string_view TrimStart(std::string_view text)
{
	std::size_t index = 0;

	while (index < text.size() && 0 != std::isspace(text[index]))
	{
		++index;
	}

	if (index < text.size())
	{
		return text.substr(index);
	}

	// text was empty or consisted only of whitespace
	return {};
}

std::string_view TrimEnd(std::string_view text)
{
	// Avoid relying on overflow
	std::size_t index = text.size();

	while (index > 0 && std::isspace(text[index - 1]) != 0)
	{
		--index;
	}

	// index is the number of characters left in the string
	if (index > 0)
	{
		return text.substr(0, index);
	}

	// text was empty or consisted only of whitespace
	return {};
}

std::string_view Trim(std::string_view text)
{
	return TrimEnd(TrimStart(text));
}

// TODO: need to use ICU for Unicode-compliant conversion

void ToLower(std::string& text)
{
	std::transform(text.begin(), text.end(), text.begin(), [](auto c)
		{ return std::tolower(c); });
}

void ToUpper(std::string& text)
{
	std::transform(text.begin(), text.end(), text.begin(), [](auto c)
		{ return std::toupper(c); });
}

std::string ToLower(std::string_view text)
{
	std::string result{text};

	ToLower(result);

	return result;
}

std::string ToUpper(std::string_view text)
{
	std::string result{text};

	ToUpper(result);

	return result;
}

int UTIL_CompareI(std::string_view lhs, std::string_view rhs)
{
	return eastl::string::comparei(lhs.data(), lhs.data() + lhs.size(), rhs.data(), rhs.data() + rhs.size());
}

void UTIL_StringToVector(Vector& destination, std::string_view pString)
{
	std::size_t index = 0;

	int j;

	const char* const end = pString.data() + pString.size();

	for (j = 0; j < 3; j++) // lifted from pr_edict.c
	{
		const auto result = std::from_chars(pString.data() + index, end, destination[j]);

		if (result.ec != std::errc())
		{
			break;
		}

		// Continue after parsed value.
		index = result.ptr - pString.data();

		// Skip all whitespace.
		while (index < pString.size() && std::isspace(pString[index]) != 0)
			++index;
		if (index >= pString.size())
			break;
	}
	if (j < 2)
	{
		/*
		CBaseEntity::Logger->error("Bad field in entity!! {}:{} == \"{}\"",
			pkvd->szClassName, pkvd->szKeyName, pkvd->szValue );
		*/
		for (j = j + 1; j < 3; j++)
			destination[j] = 0;
	}
}

int UTIL_StringToInteger(std::string_view str)
{
	int value = 0;
	std::from_chars(str.data(), str.data() + str.size(), value);
	return value;
}

IntegerString UTIL_ToString(int iValue)
{
	IntegerString buffer;
	fmt::format_to(std::back_inserter(buffer), "{}", iValue);
	return buffer;
}


bool UTIL_ParseStringWithArrayIndex(std::string_view input, std::string_view& name, int& index)
{
	const std::size_t endOfName = input.find_last_not_of("0123456789");

	if (endOfName == std::string_view::npos)
	{
		// No name.
		return false;
	}

	if (endOfName == (input.length() - 1))
	{
		// No index.
		return false;
	}

	const auto indexPart = input.substr(endOfName + 1);

	const auto end = indexPart.data() + indexPart.length();

	int possibleIndex;
	const auto result = std::from_chars(indexPart.data(), end, possibleIndex);

	if (result.ec != std::errc())
	{
		return false;
	}

	if (result.ptr != end)
	{
		// Shouldn't happen since we've already verified that there is at least one number at the end.
		return false;
	}

	if (possibleIndex < 0)
	{
		// Shouldn't happen since from_chars isn't supposed to parse numbers too big to be stored correctly.
		return false;
	}

	name = input.substr(0, endOfName + 1);
	index = possibleIndex;

	return true;
}

std::string_view GetLine(std::string_view& text)
{
	auto end = text.find('\n');

	if (end == text.npos)
	{
		end = text.size();
	}
	else
	{
		++end;
	}

	const auto line = text.substr(0, end);

	text = text.substr(end);

	return line;
}

std::string_view::const_iterator FindWhitespace(std::string_view text)
{
	return std::find_if(text.begin(), text.end(), [](auto c)
		{ return 0 != std::isspace(c); });
}

std::string_view SkipWhitespace(std::string_view text)
{
	const auto end = text.end();

	const auto it = std::find_if(text.begin(), text.end(), [](auto c)
		{ return 0 == std::isspace(c); });

	if (it == end)
	{
		return {};
	}

	return {it, end};
}

std::string_view RemoveComments(std::string_view text)
{
	const auto commentStart = text.find("//");

	if (commentStart != text.npos)
	{
		text = text.substr(0, commentStart);
	}

	return text;
}
