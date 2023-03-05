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

#include "networking/NetworkDataSystem.h"

constexpr std::size_t MinimumMaterialsCount = 512; // original max number of textures loaded

bool MaterialSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("materials");

	g_NetworkData.RegisterHandler("MaterialSystem", this);

	return true;
}

void MaterialSystem::Shutdown()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

void MaterialSystem::HandleNetworkDataBlock(NetworkDataBlock& block)
{
	if (block.Mode == NetworkDataMode::Serialize)
	{
		block.Data = json::array();

		for (const auto& material : m_Materials)
		{
			json mat = json::object();

			mat.emplace("Texture", material.Name.c_str());
			mat.emplace("Type", std::string{material.Type});

			block.Data.push_back(std::move(mat));
		}

		g_NetworkData.GetLogger()->debug("Wrote {} materials to network data", m_Materials.size());
	}
	else
	{
		m_Materials.clear();

		if (!block.Data.is_array())
		{
			block.ErrorMessage = "Material network data must be an array of objects";
			return;
		}

		for (const auto& mat : block.Data)
		{
			if (!mat.is_object())
			{
				block.ErrorMessage = "Material network data must be an array of objects";
				return;
			}

			const auto texture = mat.value<std::string>("Texture", {});
			const auto type = mat.value<std::string>("Type", {});

			if (texture.empty() || type.empty())
			{
				block.ErrorMessage = "Material network data entry must contain a texture name and material type";
				return;
			}

			m_Materials.emplace_back(texture.c_str(), type.front());
		}

		g_NetworkData.GetLogger()->debug("Parsed {} materials from network data", m_Materials.size());
	}
}

void MaterialSystem::LoadMaterials(std::span<const std::string> fileNames)
{
	m_Materials.clear();
	m_Materials.reserve(MinimumMaterialsCount);

	for (const auto& fileName : fileNames)
	{
		ParseMaterialsFile(fileName.c_str());
	}

	std::stable_sort(m_Materials.begin(), m_Materials.end(), [](const auto& lhs, const auto& rhs)
		{ return stricmp(lhs.Name.c_str(), rhs.Name.c_str()) < 0; });

	m_Logger->debug("Loaded {} materials", m_Materials.size());
}

const char* MaterialSystem::StripTexturePrefix(const char* name)
{
	// strip leading '-0' or '+0~' or '{' or '!'
	if (*name == '-' || *name == '+')
	{
		name += 2;
	}

	if (*name == '{' || *name == '!' || *name == '~' || *name == ' ')
	{
		++name;
	}

	return name;
}

char MaterialSystem::FindTextureType(const char* name) const
{
	const auto end = m_Materials.end();

	// TODO: multiple materials can have the same name, so this will not always return the right type.
	// Once full texture names are used duplicates should not be allowed which will solve this problem.
	if (auto it = std::lower_bound(m_Materials.begin(), end, name, [](const auto& material, auto name)
			{ return stricmp(material.Name.c_str(), name) < 0; });
		it != end && stricmp(it->Name.c_str(), name) == 0)
	{
		return it->Type;
	}

	return CHAR_TEX_CONCRETE;
}

void MaterialSystem::ParseMaterialsFile(const char* fileName)
{
	m_Logger->trace("Loading {}", fileName);

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

std::optional<Material> MaterialSystem::TryParseMaterial(std::string_view line)
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

	if (textureName.size() >= TextureNameMax)
	{
		m_Logger->warn("Texture name \"{}\" exceeds {} byte maximum, ignoring", textureName, TextureNameMax);
		return {};
	}

	return Material{.Name{textureName.data(), textureName.size()}, .Type = type};
}
