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

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <spdlog/logger.h>

#include "utils/GameSystem.h"
#include "utils/json_fwd.h"

enum class EntityClassification : std::uint32_t
{
};

constexpr EntityClassification ENTCLASS_NONE{0};

constexpr char ENTCLASS_NONE_NAME[] = "none";

/**
 *	@brief Monster to monster relationship types
 */
enum class Relationship : std::int8_t
{
	Ally = -2,	 //!< pals. Good alternative to None when applicable.
	Fear = -1,	 //!< will run
	None = 0,	 //!< disregard
	Dislike = 1, //!< will attack
	Hate = 2,	 //!< will attack this character instead of any visible DISLIKEd characters
	Nemesis = 3, //!< A monster Will ALWAYS attack its nemsis, no matter what
};

/**
 *	@brief Manages the set of entity classifications and the relations between them.
 */
class EntityClassificationSystem final : public IGameSystem
{
private:
	struct Classification
	{
		std::string Name;
		std::vector<Relationship> Relationships{};
	};

public:
	const char* GetName() const override { return "EntityClassifications"; }

	bool Initialize() override;

	void PostInitialize() override {}

	void Shutdown() override;

	void Load(const std::string& fileName);

	EntityClassification GetClass(std::string_view name);

	Relationship GetRelationship(EntityClassification source, EntityClassification target) const;

	/**
	 *	@brief Returns true if the given classification is one of the given class names.
	 */
	bool ClassNameIs(EntityClassification classification, std::initializer_list<const char*> classNames) const;

private:
	Classification* FindClassification(std::string_view name);

	void AddClassification(std::string&& name);

	void Clear();

	bool ParseConfiguration(const json& input);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::vector<Classification> m_Classifications;
};

inline EntityClassificationSystem g_EntityClassifications;
