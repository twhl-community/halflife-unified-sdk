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

#include <string_view>

#include "filesystem_utils.h"
#include "logging_utils.h"
#include "string_utils.h"
#include "extdll.h"
#include "enginecallback.h"

extern globalvars_t* gpGlobals;

inline cvar_t* g_pDeveloper;

inline const char* STRING(string_t offset)
{
	return ((const char*)(gpGlobals->pStringBase + (unsigned int)(offset)));
}

/**
 *	@brief Use this instead of ALLOC_STRING on constant strings
 */
inline string_t MAKE_STRING(const char* str)
{
	return ((uint64)(str) - (uint64)(STRING(0)));
}

string_t ALLOC_STRING(const char* str);

/**
 *	@brief Version of ALLOC_STRING that parses and converts escape characters
 */
string_t ALLOC_ESCAPED_STRING(const char* str);

void ClearStringPool();

void Con_Printf(const char* format, ...);

/**
 *	@brief Gets the command line value for the given key
 *	@return Whether the key was specified on the command line
 */
bool COM_GetParam(const char* name, const char** next);

/**
 *	@brief Checks whether the given key was specified on the command line
 */
bool COM_HasParam(const char* name);

constexpr bool UTIL_IsServer()
{
#ifdef CLIENT_DLL
	return false;
#else
	return true;
#endif
}

constexpr std::string_view GetShortLibraryPrefix()
{
	using namespace std::literals;

	if constexpr (UTIL_IsServer())
	{
		return "sv"sv;
	}
	else
	{
		return "cl"sv;
	}
}

constexpr std::string_view GetLongLibraryPrefix()
{
	using namespace std::literals;

	if constexpr (UTIL_IsServer())
	{
		return "server"sv;
	}
	else
	{
		return "client"sv;
	}
}

inline char com_token[1500];

/**
 *	@brief Parse a token out of a string
 */
const char* COM_Parse(const char* data);

/**
 *	@brief Returns true if additional data is waiting to be processed on this line
 */
bool COM_TokenWaiting(const char* buffer);

int UTIL_SharedRandomLong(unsigned int seed, int low, int high);
float UTIL_SharedRandomFloat(unsigned int seed, float low, float high);

// The client also provides these functions, so use them to make this cross-library.
inline float CVAR_GET_FLOAT(const char* x) { return g_engfuncs.pfnCVarGetFloat(x); }
inline const char* CVAR_GET_STRING(const char* x) { return g_engfuncs.pfnCVarGetString(x); }

const char* UTIL_CheckForGlobalModelReplacement(const char* s);

int UTIL_PrecacheModel(const char* s);

/**
 *	@brief Helper constant to allow the use of @c static_assert without an actual condition.
 *	Used mainly in <tt>if constexpr else</tt> branches.
 *	@details See https://en.cppreference.com/w/cpp/utility/variant/visit
 */
template <typename>
inline constexpr bool always_false_v = false;
