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

/**
 *	@file
 *
 *	Platform abstractions, common header includes
 */

// Allow "DEBUG" in addition to default "_DEBUG"
#ifdef _DEBUG
#define DEBUG 1
#endif

#include "steam/steamtypes.h" // DAL
#include "common_types.h"

// Misc C-runtime library headers
#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>

#include <EASTL/fixed_string.h>

using namespace std::string_view_literals;

using byte = unsigned char;
using qboolean = int;

enum class string_t_value : unsigned int
{
	Null = 0
};

struct string_t
{
	static const string_t Null;

	constexpr string_t() = default;

	explicit constexpr string_t(unsigned int value)
		: m_Value(static_cast<string_t_value>(value))
	{
	}

	constexpr auto operator<=>(const string_t&) const = default;

	// Never write to this yourself.
	string_t_value m_Value = string_t_value::Null;
};

constexpr inline string_t string_t::Null{};

// Make sure string_t doesn't break any code.
// If these asserts fail then the compiler you're using doesn't support replacing typedefs with structs and enums.
static_assert(sizeof(string_t_value) == sizeof(unsigned int), "string_t_value must be the size of its underlying type");
static_assert(sizeof(string_t) == sizeof(string_t_value), "string_t must not contain any compiler-inserted padding");

#ifdef WIN32
// Avoid the ISO conformant warning
#define stricmp _stricmp
#define strnicmp _strnicmp
#define itoa _itoa
#define strupr _strupr
#define strdup _strdup

#define DLLEXPORT __declspec(dllexport)
#define DLLHIDDEN

#else // WIN32

#define stricmp strcasecmp
#define strnicmp strncasecmp

#define DLLEXPORT __attribute__((visibility("default")))
#define DLLHIDDEN __attribute__((visibility("hidden")))

#endif // WIN32

constexpr std::size_t MAX_PATH_LENGTH = 260;
constexpr std::size_t MAX_QPATH = 64; // Must match value in quakedefs.h
constexpr std::size_t MaxUserMessageLength = 192;

/**
 *	@brief Type to efficiently store an absolute filename. Stores the string in a buffer with automatic lifetime if possible.
 */
using Filename = eastl::fixed_string<char, MAX_PATH_LENGTH>;

/**
 *	@brief Type to efficiently store a relative filename. Stores the string in a buffer with automatic lifetime if possible.
 */
using RelativeFilename = eastl::fixed_string<char, MAX_QPATH>;
