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
#include <unordered_map>

#include <EASTL/fixed_string.h>

#include <spdlog/logger.h>

#include "pm_materials.h"
#include "networking/NetworkDataSystem.h"
#include "utils/GameSystem.h"
#include "utils/heterogeneous_lookup.h"
#include "utils/json_fwd.h"

constexpr std::size_t TextureNameMax = 16; // Must match texture name length in WAD and BSP file formats.

using TextureName = eastl::fixed_string<char, TextureNameMax>;

struct TextureNameHash : public TransparentStringHash
{
	using TransparentStringHash::operator();

	[[nodiscard]] size_t operator()(const TextureName& txt) const { return hash_type{}(txt.c_str()); }
};

struct Material
{
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

	static const char* StripTexturePrefix(const char* name);

	/**
	 *	@brief given texture name, find texture type.
	 *	If not found, return type 'concrete'.
	 */
	char FindTextureType(const char* name) const;

private:
	bool ParseConfiguration(const json& input);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::unordered_map<TextureName, Material, TextureNameHash, TransparentEqual> m_Materials;
};

inline MaterialSystem g_MaterialSystem;
