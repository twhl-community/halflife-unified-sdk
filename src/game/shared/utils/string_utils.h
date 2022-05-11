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

#include <string>
#include <string_view>

/**
*	@brief Trims whitespace at the start of the given text.
*/
std::string_view TrimStart(std::string_view text);

/**
*	@brief Trims whitespace at the end of the given text.
*/
std::string_view TrimEnd(std::string_view text);

/**
*	@brief Trims whitespace at the start and end of the given text.
*/
std::string_view Trim(std::string_view text);

std::string ToLower(std::string_view text);

std::string ToUpper(std::string_view text);

void UTIL_StringToVector(float* pVector, std::string_view pString);
