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

#include <string>

#include "cbase.h"
#include "shared_utils.h"
#include "CStringPool.h"

CStringPool g_StringPool;

string_t ALLOC_STRING(const char* str)
{
	return MAKE_STRING(g_StringPool.Allocate(str));
}

string_t ALLOC_ESCAPED_STRING(const char* str)
{
	if (!str)
	{
		ALERT(at_warning, "NULL string passed to ALLOC_ESCAPED_STRING\n");
		return MAKE_STRING("");
	}

	std::string converted{str};

	for (std::size_t index = 0; index < converted.length();)
	{
		if (converted[index] == '\\')
		{
			if (index + 1 >= converted.length())
			{
				ALERT(at_warning, "Incomplete escape character encountered in ALLOC_ESCAPED_STRING\n\tOriginal string: \"%s\"\n", str);
				break;
			}

			const char next = converted[index + 1];

			converted.erase(index, 1);

			//TODO: support all escape characters
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
	//This clears the pool and frees memory
	g_StringPool = CStringPool{};
}

void Con_Printf(const char* format, ...)
{
	static char buffer[8192];

	va_list list;

	va_start(list, format);

	const int result = vsnprintf(buffer, std::size(buffer), format, list);

	if (result >= 0 && static_cast<std::size_t>(result) < std::size(buffer))
	{
		g_engfuncs.pfnServerPrint(buffer);
	}
	else
	{
		g_engfuncs.pfnServerPrint("Error logging message\n");
	}

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

const char* COM_Parse(const char* data)
{
	int c;
	int len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return NULL; // end of file;
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
