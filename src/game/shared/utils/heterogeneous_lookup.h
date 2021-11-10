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

#include <functional>
#include <string>
#include <string_view>

/**
*	@file
*
*   Defines helper types to enable heterogenous lookup for strings in containers
*   based on https://www.codeproject.com/Tips/5255442/Cplusplus14-20-Heterogeneous-Lookup-Benchmark
*   See also https://github.com/microsoft/STL/issues/683
*/

struct TransparentEqual : public std::equal_to<>
{
    using is_transparent = void;
};

struct TransparentStringHash
{
    using is_transparent = void;
    using hash_type = std::hash<std::string_view>;  // just a helper local type

    [[nodiscard]] size_t operator()(std::string_view txt) const { return hash_type{}(txt); }
    [[nodiscard]] size_t operator()(const std::string& txt) const { return hash_type{}(txt); }
    [[nodiscard]] size_t operator()(const char* txt) const { return hash_type{}(txt); }
};
