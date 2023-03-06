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
#include <string>

#include "cbase.h"
#include "shared_utils.h"
#include "StringPool.h"

#ifndef CLIENT_DLL
#include "MapState.h"
#include "ServerLibrary.h"
#include "sound/ServerSoundSystem.h"
#endif

StringPool g_StringPool;

#ifdef DEBUG
void DBG_AssertFunction(
	bool fExpr,
	const char* szExpr,
	const char* szFile,
	int szLine,
	const char* szMessage)
{
	if (fExpr)
		return;

	// Log as critical so it's enabled by default in debug builds.
	if (szMessage != nullptr)
		g_AssertLogger->critical("ASSERT FAILED:\n {} \n({}@{})\n{}", szExpr, szFile, szLine, szMessage);
	else
		g_AssertLogger->critical("ASSERT FAILED:\n {} \n({}@{})", szExpr, szFile, szLine);
}
#endif // DEBUG

#ifdef DEBUG
edict_t* DBG_EntOfVars(const entvars_t* pev)
{
	if (pev->pContainingEntity != nullptr)
		return pev->pContainingEntity;
	CBaseEntity::Logger->debug("entvars_t pContainingEntity is nullptr, calling into engine");
	edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
	if (pent == nullptr)
		CBaseEntity::Logger->debug("DAMN!  Even the engine couldn't FindEntityByVars!");
	((entvars_t*)pev)->pContainingEntity = pent;
	return pent;
}
#endif // DEBUG

string_t ALLOC_STRING(const char* str)
{
	return MAKE_STRING(g_StringPool.Allocate(str));
}

string_t ALLOC_STRING_VIEW(std::string_view str)
{
	return MAKE_STRING(g_StringPool.Allocate(str));
}

string_t ALLOC_ESCAPED_STRING(const char* str)
{
	if (!str)
	{
		CBaseEntity::Logger->warn("NULL string passed to ALLOC_ESCAPED_STRING");
		return MAKE_STRING("");
	}

	std::string converted{str};

	for (std::size_t index = 0; index < converted.length();)
	{
		if (converted[index] == '\\')
		{
			if (index + 1 >= converted.length())
			{
				CBaseEntity::Logger->warn("Incomplete escape character encountered in ALLOC_ESCAPED_STRING\n\tOriginal string: \"{}\"", str);
				break;
			}

			const char next = converted[index + 1];

			converted.erase(index, 1);

			// TODO: support all escape characters
			if (next == 'n')
			{
				converted[index] = '\n';
			}
		}

		++index;
	}

	return ALLOC_STRING(converted.c_str());
}

void ClearStringPool()
{
	// This clears the pool and frees memory
	g_StringPool = StringPool{};
}

static bool g_PrintBufferingEnabled = false;

std::string g_PrintBuffer;

bool Con_IsPrintBufferingEnabled()
{
	return g_PrintBufferingEnabled;
}

void Con_SetPrintBufferingEnabled(bool enabled)
{
	if (g_PrintBufferingEnabled == enabled)
	{
		return;
	}

	g_PrintBufferingEnabled = enabled;

	if (!g_PrintBufferingEnabled)
	{
		// Flush buffer to console. Send it one line at a time to minimize the chances of truncation.
		if (!g_PrintBuffer.empty())
		{
			// Append a newline so we're sure every line ends with one.
			// This newline will be temporarily converted to a null terminator below for the last line.
			g_PrintBuffer += '\n';

			std::size_t startIndex = 0;

			while (true)
			{
				// Don't print last newline.
				if (startIndex == std::string::npos || startIndex == (g_PrintBuffer.size() - 1))
				{
					break;
				}

				std::size_t endIndex = g_PrintBuffer.find('\n', startIndex + 1);

				const std::size_t actualEndIndex = endIndex != std::string::npos ? endIndex : g_PrintBuffer.size();

				const std::size_t count = actualEndIndex - startIndex;

				g_PrintBuffer[actualEndIndex] = '\0';

				// Print substring.
				g_engfuncs.pfnServerPrint(g_PrintBuffer.data() + startIndex);

				g_PrintBuffer[actualEndIndex] = '\n';

				// If there was a valid newline it'll be printed next iteration.
				startIndex = endIndex;
			}
		}

		g_PrintBuffer.clear();
		g_PrintBuffer.shrink_to_fit();
	}
}

void Con_VPrintf(const char* format, va_list list)
{
	static char buffer[8192];

	const int result = vsnprintf(buffer, std::size(buffer), format, list);

	if (result >= 0 && static_cast<std::size_t>(result) < std::size(buffer))
	{
		if (g_PrintBufferingEnabled)
		{
			g_PrintBuffer += buffer;
		}
		else
		{
			g_engfuncs.pfnServerPrint(buffer);
		}
	}
	else
	{
		g_engfuncs.pfnServerPrint("Error logging message\n");
	}
}

void Con_VDPrintf(const char* format, va_list list)
{
	static char buffer[8192];

	const int result = vsnprintf(buffer, std::size(buffer), format, list);

	if (result >= 0 && static_cast<std::size_t>(result) < std::size(buffer))
	{
		g_engfuncs.pfnAlertMessage(at_console, "%s", buffer);
	}
	else
	{
		g_engfuncs.pfnAlertMessage(at_console, "Error logging message\n");
	}
}

void Con_Printf(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	Con_VPrintf(format, list);
	va_end(list);
}

void Con_DPrintf(const char* format, ...)
{
	if (g_pDeveloper->value == 0)
	{
		return;
	}

	va_list list;
	va_start(list, format);
	Con_VDPrintf(format, list);
	va_end(list);
}

bool COM_GetParam(const char* name, const char** next)
{
	return g_engfuncs.pfnCheckParm(name, next) != 0;
}

bool COM_HasParam(const char* name)
{
	return g_engfuncs.pfnCheckParm(name, nullptr) != 0;
}

bool UTIL_FixBoundsVectors(Vector& mins, Vector& maxs)
{
	bool neededFixing = false;

	for (std::size_t i = 0; i < 3; ++i)
	{
		if (mins[i] > maxs[i])
		{
			neededFixing = true;
			std::swap(mins[i], maxs[i]);
		}
	}

	return neededFixing;
}

const char* COM_Parse(const char* data)
{
	int c;
	int len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return nullptr;

	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return nullptr; // end of file;
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while ('\0' != *data && *data != '\n')
			data++;
		goto skipwhite;
	}


	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (true)
		{
			c = *data++;
			if (c == '\"' || '\0' == c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

	// parse single characters
	if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',')
			break;
	} while (c > 32);

	com_token[len] = 0;
	return data;
}

bool COM_TokenWaiting(const char* buffer)
{
	for (const char* p = buffer; '\0' != *p && *p != '\n'; ++p)
	{
		if (0 == isspace(*p) || 0 != isalnum(*p))
			return true;
	}

	return false;
}

static unsigned int glSeed = 0;

unsigned int seed_table[256] =
	{
		28985, 27138, 26457, 9451, 17764, 10909, 28790, 8716, 6361, 4853, 17798, 21977, 19643, 20662, 10834, 20103,
		27067, 28634, 18623, 25849, 8576, 26234, 23887, 18228, 32587, 4836, 3306, 1811, 3035, 24559, 18399, 315,
		26766, 907, 24102, 12370, 9674, 2972, 10472, 16492, 22683, 11529, 27968, 30406, 13213, 2319, 23620, 16823,
		10013, 23772, 21567, 1251, 19579, 20313, 18241, 30130, 8402, 20807, 27354, 7169, 21211, 17293, 5410, 19223,
		10255, 22480, 27388, 9946, 15628, 24389, 17308, 2370, 9530, 31683, 25927, 23567, 11694, 26397, 32602, 15031,
		18255, 17582, 1422, 28835, 23607, 12597, 20602, 10138, 5212, 1252, 10074, 23166, 19823, 31667, 5902, 24630,
		18948, 14330, 14950, 8939, 23540, 21311, 22428, 22391, 3583, 29004, 30498, 18714, 4278, 2437, 22430, 3439,
		28313, 23161, 25396, 13471, 19324, 15287, 2563, 18901, 13103, 16867, 9714, 14322, 15197, 26889, 19372, 26241,
		31925, 14640, 11497, 8941, 10056, 6451, 28656, 10737, 13874, 17356, 8281, 25937, 1661, 4850, 7448, 12744,
		21826, 5477, 10167, 16705, 26897, 8839, 30947, 27978, 27283, 24685, 32298, 3525, 12398, 28726, 9475, 10208,
		617, 13467, 22287, 2376, 6097, 26312, 2974, 9114, 21787, 28010, 4725, 15387, 3274, 10762, 31695, 17320,
		18324, 12441, 16801, 27376, 22464, 7500, 5666, 18144, 15314, 31914, 31627, 6495, 5226, 31203, 2331, 4668,
		12650, 18275, 351, 7268, 31319, 30119, 7600, 2905, 13826, 11343, 13053, 15583, 30055, 31093, 5067, 761,
		9685, 11070, 21369, 27155, 3663, 26542, 20169, 12161, 15411, 30401, 7580, 31784, 8985, 29367, 20989, 14203,
		29694, 21167, 10337, 1706, 28578, 887, 3373, 19477, 14382, 675, 7033, 15111, 26138, 12252, 30996, 21409,
		25678, 18555, 13256, 23316, 22407, 16727, 991, 9236, 5373, 29402, 6117, 15241, 27715, 19291, 19888, 19847};

unsigned int U_Random()
{
	glSeed *= 69069;
	glSeed += seed_table[glSeed & 0xff];

	return (++glSeed & 0x0fffffff);
}

void U_Srand(unsigned int seed)
{
	glSeed = seed_table[seed & 0xff];
}

/*
=====================
UTIL_SharedRandomLong
=====================
*/
int UTIL_SharedRandomLong(unsigned int seed, int low, int high)
{
	unsigned int range;

	U_Srand((int)seed + low + high);

	range = high - low + 1;
	if (0 == (range - 1))
	{
		return low;
	}
	else
	{
		int offset;
		int rnum;

		rnum = U_Random();

		offset = rnum % range;

		return (low + offset);
	}
}

/*
=====================
UTIL_SharedRandomFloat
=====================
*/
float UTIL_SharedRandomFloat(unsigned int seed, float low, float high)
{
	//
	unsigned int range;

	U_Srand((int)seed + *(int*)&low + *(int*)&high);

	U_Random();
	U_Random();

	range = high - low;
	if (0 == range)
	{
		return low;
	}
	else
	{
		int tensixrand;
		float offset;

		tensixrand = U_Random() & 65535;

		offset = (float)tensixrand / 65536.0;

		return (low + offset * range);
	}
}

const char* UTIL_CheckForGlobalModelReplacement(const char* s)
{
#ifndef CLIENT_DLL
	s = g_Server.GetMapState()->m_GlobalModelReplacement->Lookup(s);
#endif

	return s;
}

static void UTIL_PrecacheLog(const char* type, const char* fileName)
{
	if (g_PrecacheLogger->should_log(spdlog::level::trace))
	{
		g_PrecacheLogger->trace("[{}] \"{}\"{}", type, fileName, fileName == gpGlobals->pStringBase ? " (Null string_t)" : "");
	}
}

int UTIL_PrecacheModelDirect(const char* s)
{
	UTIL_PrecacheLog("model", s);
	return g_engfuncs.pfnPrecacheModel(s);
}

int UTIL_PrecacheModel(const char* s)
{
	ASSERT(s != nullptr);
	ASSERT(s[0] != '\0');

	if (!s || s[0] == '\0')
	{
		return 0;
	}

	s = UTIL_CheckForGlobalModelReplacement(s);

	return UTIL_PrecacheModelDirect(s);
}

int UTIL_PrecacheSoundDirect(const char* s)
{
	UTIL_PrecacheLog("sound", s);
	return g_engfuncs.pfnPrecacheSound(s);
}

int UTIL_PrecacheSound(const char* s)
{
	ASSERT(s != nullptr);
	ASSERT(s[0] != '\0');

	if (!s || s[0] == '\0')
	{
		return 0;
	}

#ifndef CLIENT_DLL
	s = sound::g_ServerSound.CheckForSoundReplacement(s);
#endif

	return UTIL_PrecacheSoundDirect(s);
}

int UTIL_PrecacheGenericDirect(const char* s)
{
	UTIL_PrecacheLog("generic", s);
	return g_engfuncs.pfnPrecacheGeneric(s);
}
