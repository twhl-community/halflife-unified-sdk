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
#include "BspLoader.h"

#define vec3_t Vector

// Pull in tools header for definitions. Don't call any of its functions!
#include "../../utils/common/bspfile.h"

std::optional<BspData> BspLoader::Load(const char* fileName)
{
	const auto contents = FileSystem_LoadFileIntoBuffer(fileName, FileContentFormat::Binary);

	if (contents.size() < sizeof(dheader_t))
	{
		return {};
	}

	dheader_t header;

	std::memcpy(&header, contents.data(), sizeof(dheader_t));

	if (header.version != BSPVERSION)
	{
		return {};
	}

	BspData data;

	const auto& modelLump = header.lumps[LUMP_MODELS];

	data.SubModelCount = static_cast<std::size_t>(modelLump.filelen) / sizeof(dmodel_t);

	return data;
}
