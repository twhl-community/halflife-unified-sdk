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

#include "extdll.h"
#include "enginecallback.h"
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
