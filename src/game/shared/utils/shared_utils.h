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

#include "filesystem_shared.h"

extern globalvars_t* gpGlobals;

inline const char* STRING(string_t offset)
{
	return ((const char*)(gpGlobals->pStringBase + (unsigned int)(offset)));
}

/**
*	@brief Use this instead of ALLOC_STRING on constant strings
*/
inline string_t MAKE_STRING(const char* str)
{
	return ((uint64)(str)-(uint64)(STRING(0)));
}

string_t ALLOC_STRING(const char* str);

/**
*	@brief Version of ALLOC_STRING that parses and converts escape characters
*/
string_t ALLOC_ESCAPED_STRING(const char* str);

void ClearStringPool();
