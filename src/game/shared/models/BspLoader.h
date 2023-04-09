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

#include <cstddef>
#include <optional>

struct BspData
{
	std::size_t SubModelCount{};
};

/**
 *	@brief Loads BSP data into memory for use.
 *	Extend as needed if more data is required.
 */
class BspLoader final
{
public:
	BspLoader() = delete;

	static std::optional<BspData> Load(const char* fileName);
};
