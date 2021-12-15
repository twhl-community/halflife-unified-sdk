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

#ifndef PLATFORM_H
#define PLATFORM_H

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
#pragma warning(disable : 4244)		// int or float down-conversion
#pragma warning(disable : 4305)		// int or float data truncation
#pragma warning(disable : 4201)		// nameless struct/union
#pragma warning(disable : 4514)		// unreferenced inline function removed
#pragma warning(disable : 4100)		// unreferenced formal parameter

#include "archtypes.h"     // DAL

// Misc C-runtime library headers
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using byte = unsigned char;
using word = unsigned short;
using func_t = unsigned int;
using string_t = unsigned int;
using qboolean = int;

#define ARRAYSIZE(p)		(sizeof(p)/sizeof(p[0]))

// Prevent tons of unused windows definitions
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOMINMAX
#include "winsani_in.h"
#include <Windows.h>
#include "winsani_out.h"
#include <malloc.h>

//Avoid the ISO conformant warning
#define stricmp _stricmp
#define strnicmp _strnicmp
#define itoa _itoa
#define strupr _strupr
#define strdup _strdup

#define DLLEXPORT __declspec( dllexport )

#define stackalloc(size) _alloca(size)

//Note: an implementation of stackfree must safely ignore null pointers
#define stackfree(address)

#else // _WIN32
#define FALSE 0
#define TRUE (!FALSE)
typedef uint32 ULONG;
typedef unsigned char BYTE;
typedef int BOOL;
#define MAX_PATH PATH_MAX
#include <limits.h>
#include <stdarg.h>
#include <alloca.h>
#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)

#define stricmp strcasecmp
#define _strnicmp strncasecmp
#define strnicmp strncasecmp
#define _snprintf snprintf
#define _alloca alloca

#define DLLEXPORT __attribute__ ( ( visibility( "default" ) ) )

#define stackalloc(size) alloca(size)

//Note: an implementation of stackfree must safely ignore null pointers
#define stackfree(address)

#endif //_WIN32

#define V_min(a,b)  (((a) < (b)) ? (a) : (b))
#define V_max(a,b)  (((a) > (b)) ? (a) : (b))

#endif //PLATFORM_H