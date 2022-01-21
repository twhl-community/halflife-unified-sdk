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

#include "Platform.h"

#include "string_utils.h"

std::string_view TrimStart(std::string_view text)
{
	std::size_t index = 0;

	while (index < text.size() && std::isspace(text[index]))
	{
		++index;
	}

	if (index < text.size())
	{
		return text.substr(index);
	}

	//text was empty or consisted only of whitespace
	return {};
}

std::string_view TrimEnd(std::string_view text)
{
	//Avoid relying on overflow
	std::size_t index = text.size();

	while (index > 0 && std::isspace(text[index - 1]))
	{
		--index;
	}

	//index is the number of characters left in the string
	if (index > 0)
	{
		return text.substr(0, index);
	}

	//text was empty or consisted only of whitespace
	return {};
}

std::string_view Trim(std::string_view text)
{
	return TrimEnd(TrimStart(text));
}

//TODO: need to use ICU for Unicode-compliant conversion

std::string ToLower(std::string_view text)
{
	std::string result{text};

	std::transform(result.begin(), result.end(), result.begin(), [](auto c) { return std::tolower(c); });

	return result;
}

std::string ToUpper(std::string_view text)
{
	std::string result{text};

	std::transform(result.begin(), result.end(), result.begin(), [](auto c) { return std::toupper(c); });

	return result;
}
