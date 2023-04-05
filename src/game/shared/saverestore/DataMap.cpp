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

#include "cbase.h"
#include "DataMap.h"

BASEPTR DataMap_FindFunctionAddress(const DataMap& dataMap, const char* name)
{
	for (auto map = &dataMap; map; map = map->BaseMap)
	{
		for (const auto& member : map->Members)
		{
			auto function = std::get_if<DataFunctionDescription>(&member);

			if (!function)
			{
				continue;
			}

			if (name == function->Name)
			{
				return function->Address;
			}
		}
	}

	return nullptr;
}

const char* DataMap_FindFunctionName(const DataMap& dataMap, BASEPTR address)
{
	for (auto map = &dataMap; map; map = map->BaseMap)
	{
		for (const auto& member : map->Members)
		{
			auto function = std::get_if<DataFunctionDescription>(&member);

			if (!function)
			{
				continue;
			}

			if (address == function->Address)
			{
				return function->Name.c_str();
			}
		}
	}

	return nullptr;
}
