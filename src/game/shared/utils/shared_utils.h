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

#include <memory>
#include <string_view>

#include <spdlog/logger.h>

#include "entity_utils.h"
#include "filesystem_utils.h"
#include "mathlib.h"
#include "LogSystem.h"
#include "string_utils.h"
#include "extdll.h"
#include "enginecallback.h"

extern globalvars_t* gpGlobals;

inline cvar_t* g_pDeveloper;

inline std::shared_ptr<spdlog::logger> g_AssertLogger;
inline std::shared_ptr<spdlog::logger> g_PrecacheLogger;

// More explicit than "int"
typedef int EOFFSET;

//
// How did I ever live without ASSERT?
//
#ifdef DEBUG
void DBG_AssertFunction(bool fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage);
#define ASSERT(f) DBG_AssertFunction(f, #f, __FILE__, __LINE__, nullptr)
#define ASSERTSZ(f, sz) DBG_AssertFunction(f, #f, __FILE__, __LINE__, sz)
#else // !DEBUG
#define ASSERT(f)
#define ASSERTSZ(f, sz)
#endif // !DEBUG

//
// Conversion among the three types of "entity", including identity-conversions.
//
#ifdef DEBUG
edict_t* DBG_EntOfVars(const entvars_t* pev);
inline edict_t* ENT(const entvars_t* pev) { return DBG_EntOfVars(pev); }
#else
inline edict_t* ENT(const entvars_t* pev)
{
	return pev->pContainingEntity;
}
#endif
inline edict_t* ENT(edict_t* pent)
{
	return pent;
}
inline edict_t* ENT(EOFFSET eoffset) { return (*g_engfuncs.pfnPEntityOfEntOffset)(eoffset); }
inline EOFFSET OFFSET(const edict_t* pent)
{
	ASSERTSZ(pent, "Bad ent in OFFSET()");
	return (*g_engfuncs.pfnEntOffsetOfPEntity)(pent);
}
inline EOFFSET OFFSET(entvars_t* pev)
{
	ASSERTSZ(pev, "Bad pev in OFFSET()");
	return OFFSET(ENT(pev));
}

inline entvars_t* VARS(edict_t* pent)
{
	if (!pent)
		return nullptr;

	return &pent->v;
}

inline int ENTINDEX(edict_t* pEdict) { return (*g_engfuncs.pfnIndexOfEdict)(pEdict); }
inline edict_t* INDEXENT(int iEdictNum) { return (*g_engfuncs.pfnPEntityOfEntIndex)(iEdictNum); }

// Testing strings for nullity
inline constexpr bool FStringNull(string_t iString)
{
	return iString == string_t::Null;
}

inline const char* STRING(string_t offset)
{
	return gpGlobals->pStringBase + static_cast<unsigned int>(offset.m_Value);
}

/**
 *	@brief Use this instead of ALLOC_STRING on constant strings
 */
inline string_t MAKE_STRING(const char* str)
{
	return static_cast<string_t>(reinterpret_cast<uint64>(str) - reinterpret_cast<uint64>(gpGlobals->pStringBase));
}

string_t ALLOC_STRING(const char* str);

string_t ALLOC_STRING_VIEW(std::string_view str);

/**
 *	@brief Version of ALLOC_STRING that parses and converts escape characters
 */
string_t ALLOC_ESCAPED_STRING(const char* str);

void ClearStringPool();

bool Con_IsPrintBufferingEnabled();

void Con_SetPrintBufferingEnabled(bool enabled);

void Con_Printf(const char* format, ...);
void Con_DPrintf(const char* format, ...);

/**
 *	@brief Gets the command line value for the given key
 *	@return Whether the key was specified on the command line
 */
bool COM_GetParam(const char* name, const char** next);

/**
 *	@brief Checks whether the given key was specified on the command line
 */
bool COM_HasParam(const char* name);

/**
*	@brief Fixes bounds vectors so that the min components are <= than the max components.
*	This avoids the "backwards mins/maxs" engine error.
*/
bool UTIL_FixBoundsVectors(Vector& mins, Vector& maxs);

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

int UTIL_PrecacheModelDirect(const char* s);

int UTIL_PrecacheModel(const char* s);

int UTIL_PrecacheSoundDirect(const char* s);

int UTIL_PrecacheSound(const char* s);

int UTIL_PrecacheGenericDirect(const char* s);

template <>
struct fmt::formatter<Vector> : public fmt::formatter<float>
{
	template <typename FormatContext>
	auto format(const Vector& p, FormatContext& ctx) const -> decltype(ctx.out())
	{
		auto out = fmt::formatter<float>::format(p.x, ctx);
		*out++ = ' ';
		ctx.advance_to(out);
		out = fmt::formatter<float>::format(p.y, ctx);
		*out++ = ' ';
		ctx.advance_to(out);
		return fmt::formatter<float>::format(p.z, ctx);
	}
};

template <>
struct fmt::formatter<Vector2D> : public fmt::formatter<float>
{
	template <typename FormatContext>
	auto format(const Vector2D& p, FormatContext& ctx) const -> decltype(ctx.out())
	{
		auto out = fmt::formatter<float>::format(p.x, ctx);
		*out++ = ' ';
		ctx.advance_to(out);
		return fmt::formatter<float>::format(p.y, ctx);
	}
};

/**
 *	@brief Helper constant to allow the use of @c static_assert without an actual condition.
 *	Used mainly in <tt>if constexpr else</tt> branches.
 *	@details See https://en.cppreference.com/w/cpp/utility/variant/visit
 */
template <typename>
inline constexpr bool always_false_v = false;
