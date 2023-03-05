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

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <EASTL/fixed_string.h>

#include <spdlog/logger.h>

#include "pm_materials.h"
#include "networking/NetworkDataSystem.h"
#include "utils/GameSystem.h"

using TextureName = eastl::fixed_string<char, CBTEXTURENAMEMAX>;

struct Material
{
	TextureName Name;
	char Type{};
};

/**
 *	@brief Used to detect the texture the player is standing on,
 *	map the texture name to a material type.
 *	Play footstep sound based on material type.
 */
class MaterialSystem final : public IGameSystem, public INetworkDataBlockHandler
{
public:
	MaterialSystem() = default;
	MaterialSystem(const MaterialSystem&) = delete;
	MaterialSystem& operator=(const MaterialSystem&) = delete;

	const char* GetName() const override { return "MaterialSystem"; }

	bool Initialize() override;
	void PostInitialize() override {}
	void Shutdown() override;

	void HandleNetworkDataBlock(NetworkDataBlock& block) override;

	void LoadMaterials(std::span<const std::string> fileNames);

	/**
	 *	@brief given texture name, find texture type.
	 *	If not found, return type 'concrete'.
	 */
	char FindTextureType(const char* name) const;

private:
	void ParseMaterialsFile(const char* fileName);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::vector<Material> m_Materials;
};

inline MaterialSystem g_MaterialSystem;
