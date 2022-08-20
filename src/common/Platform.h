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
*	Platform abstractions, common header includes, workarounds for compiler warnings
*/

// Allow "DEBUG" in addition to default "_DEBUG"
#ifdef _DEBUG
#define DEBUG 1
#endif

// Silence certain warnings
#pragma warning(disable : 4244)	 // int or float down-conversion
#pragma warning(disable : 4305)	 // int or float data truncation
#pragma warning(disable : 4201)	 // nameless struct/union
#pragma warning(disable : 4514)	 // unreferenced inline function removed
#pragma warning(disable : 4100)	 // unreferenced formal parameter
#pragma warning(disable : 26495) // Variable is uninitialized
#pragma warning(disable : 26451) // Arithmetic overflow
#pragma warning(disable : 26812) // The enum type is unscoped

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

#include <EASTL/fixed_string.h>

using byte = unsigned char;
using string_t = unsigned int;
using qboolean = int;

#ifdef WIN32
//Avoid the ISO conformant warning
#define stricmp _stricmp
#define strnicmp _strnicmp
#define itoa _itoa
#define strupr _strupr
#define strdup _strdup

#define DLLEXPORT __declspec(dllexport)
#define DLLHIDDEN

#define stackalloc(size) _alloca(size)

//Note: an implementation of stackfree must safely ignore null pointers
#define stackfree(address)

#else // WIN32
#include <alloca.h>

#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _alloca alloca

#define DLLEXPORT __attribute__((visibility("default")))
#define DLLHIDDEN __attribute__((visibility("hidden")))

#define stackalloc(size) alloca(size)

//Note: an implementation of stackfree must safely ignore null pointers
#define stackfree(address)

#endif //WIN32

constexpr std::size_t MAX_PATH_LENGTH = 260;
constexpr std::size_t MAX_QPATH = 64; // Must match value in quakedefs.h
constexpr std::size_t MaxUserMessageLength = 192;

#define V_min(a, b) (((a) < (b)) ? (a) : (b))
#define V_max(a, b) (((a) > (b)) ? (a) : (b))

/**
*	@brief Type to efficiently store an absolute filename. Stores the string in a buffer with automatic lifetime if possible.
*/
using Filename = eastl::fixed_string<char, MAX_PATH_LENGTH>;

/**
*	@brief Type to efficiently store a relative filename. Stores the string in a buffer with automatic lifetime if possible.
*/
using RelativeFilename = eastl::fixed_string<char, MAX_QPATH>;
