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

#include <optional>
#include <string_view>
#include <tuple>
#include <vector>

#include <EASTL/fixed_string.h>

#include "cbase.h"

namespace sentences
{
constexpr int CBSENTENCENAME_MAX = 16;

/**
*	@brief max number of sentences in game. NOTE: this must match CVOXFILESENTENCEMAX in engine\sound.h!!!
*/
constexpr int CVOXFILESENTENCEMAX = 1536;

/**
*	@brief max number of elements per sentence group
*/
constexpr int CSENTENCE_LRU_MAX = 32;

using SentenceName = eastl::fixed_string<char, CBSENTENCENAME_MAX>;

inline std::vector<SentenceName> g_SentenceNames;

/**
*	@brief group of related sentences
*/
struct SENTENCEG
{
	SentenceName GroupName;
	int count = 1;
	unsigned char rgblru[CSENTENCE_LRU_MAX]{};
};

inline std::vector<SENTENCEG> g_SentenceGroups;

/**
 *	@brief Parses a single sentence out of a line of text.
 */
std::tuple<std::string_view, std::string_view> ParseSentence(std::string_view text);

/**
*	@brief Given a sentence name, parses out the group index if present.
*/
std::optional<std::tuple<std::string_view, int>> ParseGroupData(std::string_view name);
}
