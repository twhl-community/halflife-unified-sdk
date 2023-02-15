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

#include <algorithm>
#include <cassert>

#include "cbase.h"
#include "MaterialSystem.h"

constexpr std::size_t MinimumMaterialsCount = 512; // original max number of textures loaded

std::optional<Material> TryParseMaterial(std::string_view line)
{
	line = RemoveComments(line);
	line = SkipWhitespace(line);

	if (line.empty())
		return {};

	if (0 == std::isalpha(line.front()))
		return {};

	const char type = std::toupper(line.front());

	line = SkipWhitespace(line.substr(1));

	if (line.empty())
		return {};

	const auto end = FindWhitespace(line);

	const std::string_view textureName{line.begin(), end};

	if (textureName.empty())
		return {};

	const std::size_t size = std::min(CBTEXTURENAMEMAX - 1, textureName.size());

	return Material{.Name{textureName.data(), size}, .Type = type};
}

void MaterialSystem::LoadMaterials()
{
	if (m_MaterialsInitialized)
		return;

	m_Materials.clear();
	m_Materials.reserve(MinimumMaterialsCount);

	ParseMaterialsFile("sound/materials.txt");

	std::stable_sort(m_Materials.begin(), m_Materials.end(), [](const auto& lhs, const auto& rhs)
		{ return stricmp(lhs.Name.c_str(), rhs.Name.c_str()) < 0; });

	m_MaterialsInitialized = true;
}

char MaterialSystem::FindTextureType(const char* name) const
{
	assert(m_MaterialsInitialized);

	const auto end = m_Materials.end();

	// TODO: multiple materials can have the same name, so this will not always return the right type.
	// Once full texture names are used duplicates should not be allowed which will solve this problem.
	if (auto it = std::lower_bound(m_Materials.begin(), end, name, [](const auto& material, auto name)
			{ return strnicmp(material.Name.c_str(), name, CBTEXTURENAMEMAX - 1) < 0; });
		it != end && strnicmp(it->Name.c_str(), name, CBTEXTURENAMEMAX - 1) == 0)
	{
		return it->Type;
	}

	return CHAR_TEX_CONCRETE;
}

void MaterialSystem::ParseMaterialsFile(const char* fileName)
{
	const auto fileContents = FileSystem_LoadFileIntoBuffer(fileName, FileContentFormat::Text);

	if (fileContents.empty())
		return;

	std::string_view materials{reinterpret_cast<const char*>(fileContents.data()), fileContents.size() - 1};

	while (true)
	{
		auto line = GetLine(materials);

		if (line.empty())
			break;

		auto material = TryParseMaterial(line);

		if (!material)
			continue;

		m_Materials.push_back(std::move(*material));
	}
}
