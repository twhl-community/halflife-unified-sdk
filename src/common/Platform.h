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
#pragma warning(disable : 4244)		// int or float down-conversion
#pragma warning(disable : 4305)		// int or float data truncation
#pragma warning(disable : 4201)		// nameless struct/union
#pragma warning(disable : 4514)		// unreferenced inline function removed
#pragma warning(disable : 4100)		// unreferenced formal parameter

#include "archtypes.h"     // DAL
#include "common_types.h"

// Misc C-runtime library headers
#include <algorithm>
#include <cmath>
#include <cstdint>
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

//Disable all Windows 10 and older APIs otherwise pulled in by Windows.h
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
//#define NOUSER //Need GetCursorPos in the mouse thread code
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

//Disable additional stuff not covered by the Windows.h list
#define NOWINRES
#define NOIME

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
#define _alloca alloca

#define DLLEXPORT __attribute__ ( ( visibility( "default" ) ) )

#define stackalloc(size) alloca(size)

//Note: an implementation of stackfree must safely ignore null pointers
#define stackfree(address)

#endif //_WIN32

#define V_min(a,b)  (((a) < (b)) ? (a) : (b))
#define V_max(a,b)  (((a) > (b)) ? (a) : (b))