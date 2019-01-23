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
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

// Prevent tons of unused windows definitions
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
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

#define DLLEXPORT __declspec( dllexport )

inline void* stackalloc( size_t size )
{
	return _alloca( size );
}

//Note: an implementation of stackfree must safely ignore null pointers
inline void stackfree( void* pAddress )
{
	//Nothing
}

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

#define DLLEXPORT __attribute__ ( ( visibility( "default" ) ) )

inline void* stackalloc( size_t size )
{
	return alloca( size );
}

inline void stackfree( void* pAddress )
{
	//Nothing
}
#endif //_WIN32

#define V_min(a,b)  (((a) < (b)) ? (a) : (b))
#define V_max(a,b)  (((a) > (b)) ? (a) : (b))

/**
*	@brief Overload to allocate arrays on the stack
*/
template<typename T, std::enable_if_t< std::is_array_v<T> &&  std::extent_v<T> == 0, int> = 0>
inline std::remove_extent_t<typename T>* stackalloc( size_t elementCount )
{
	return reinterpret_cast< std::remove_extent_t<T>* >( stackalloc( elementCount * sizeof( std::remove_extent_t<T> ) ) );
}

#endif //PLATFORM_H
