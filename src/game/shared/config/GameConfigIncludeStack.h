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
#include <unordered_set>

#include "utils/string_utils.h"

/**
*	@brief Stores a set of filenames included in a load context.
*/
class GameConfigIncludeStack final
{
public:

	/**
	*	@brief Adds a new filename to the set of included filenames.
	*	@return Whether this was a new filename.
	*/
	bool Add(std::string_view fileName)
	{
		//Use lowercase filenames so case-insensitive filesystems don't cause problems.
		auto lowered = ToLower(fileName);

		return m_Included.insert(std::move(lowered)).second;
	}

private:
	std::unordered_set<std::string> m_Included;
};
