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
#include <array>
#include <optional>

#include "cbase.h"
#include "EntityClassificationSystem.h"

constexpr std::string_view EntityClassificationsSchemaName{"EntityClassifications"sv};

static constexpr std::array RelationshipStrings =
	{
		std::pair{"ally"sv, Relationship::Ally},
		std::pair{"fear"sv, Relationship::Fear},
		std::pair{"none"sv, Relationship::None},
		std::pair{"dislike"sv, Relationship::Dislike},
		std::pair{"hate"sv, Relationship::Hate},
		std::pair{"nemesis"sv, Relationship::Nemesis}};

static std::optional<Relationship> ParseRelationship(const std::string_view name)
{
	for (const auto& relationship : RelationshipStrings)
	{
		if (0 == UTIL_CompareI(relationship.first, name))
		{
			return relationship.second;
		}
	}

	return std::nullopt;
}

static std::string GetEntityClassificationsSchema()
{
	const auto relationships = []()
	{
		std::string relationships;

		bool first = true;

		for (const auto& relationship : RelationshipStrings)
		{
			if (!first)
			{
				relationships += ',';
			}
			else
			{
				first = false;
			}

			relationships += '"';
			relationships += relationship.first;
			relationships += '"';
		}

		return relationships;
	}();

	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Entity Classifications And Relationships",
	"type": "object",
	"patternProperties": {{
		"^.+$": {{
			"title": "Classification",
			"type": "object",
			"properties": {{
				"DefaultRelationshipToTarget": {{
					"title": "Default Relationship To Target",
					"type": "string",
					"enum": [{0}]
				}},
				"Relationships": {{
					"title": "Relationships To Targets",
					"type": "object",
					"patternProperties": {{
						"^.+$": {{
							"title": "Relationship To Target",
							"description": "Key = Target Class; Value = Relationship",
							"type": "string",
							"enum": [{0}]
						}}
					}}
				}}
			}}
		}}
	}}
}}
)",
		relationships);
}

bool EntityClassificationSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("ent.classify");

	g_JSON.RegisterSchema(EntityClassificationsSchemaName, &GetEntityClassificationsSchema);

	// Initialize classifications to default (for client side).
	Clear();

#ifdef CLIENT_DLL
	AddClassification("player");
#endif

	for (auto& clazz : m_Classifications)
	{
		clazz.Relationships.insert(clazz.Relationships.end(), m_Classifications.size(), Relationship::None);
	}

	return true;
}

void EntityClassificationSystem::Shutdown()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

void EntityClassificationSystem::Load(const std::string& fileName)
{
	Clear();

	const auto result = g_JSON.ParseJSONFile(fileName.c_str(), {.SchemaName = EntityClassificationsSchemaName},
		[this](const auto& input)
		{ return ParseConfiguration(input); });
}

EntityClassification EntityClassificationSystem::GetClass(std::string_view name)
{
	if (auto clazz = FindClassification(name); clazz)
	{
		return static_cast<EntityClassification>(clazz - m_Classifications.data());
	}

	m_Logger->error("Unknown entity classification \"{}\"", name);

	return ENTCLASS_NONE;
}

Relationship EntityClassificationSystem::GetRelationship(EntityClassification source, EntityClassification target) const
{
	const auto sourceIndex = static_cast<std::size_t>(source);
	const auto targetIndex = static_cast<std::size_t>(target);

	if (sourceIndex >= m_Classifications.size() || targetIndex >= m_Classifications.size())
	{
		return Relationship::None;
	}

	const auto& clazz = m_Classifications[sourceIndex];

	if (targetIndex >= clazz.Relationships.size())
	{
		return Relationship::None;
	}

	return clazz.Relationships[targetIndex];
}

bool EntityClassificationSystem::ClassNameIs(EntityClassification classification, std::initializer_list<const char*> classNames) const
{
	const auto classIndex = static_cast<std::size_t>(classification);

	if (classIndex >= m_Classifications.size())
	{
		return false;
	}

	const auto& clazz = m_Classifications[classIndex];

	for (const auto className : classNames)
	{
		if (0 == UTIL_CompareI(clazz.Name, className))
		{
			return true;
		}
	}

	return false;
}

EntityClassificationSystem::Classification* EntityClassificationSystem::FindClassification(std::string_view name)
{
	if (auto it = std::find_if(m_Classifications.begin(), m_Classifications.end(), [&](const auto& candidate)
			{ return 0 == UTIL_CompareI(candidate.Name, name); });
		it != m_Classifications.end())
	{
		return &(*it);
	}

	return nullptr;
}

void EntityClassificationSystem::AddClassification(std::string&& name)
{
	if (FindClassification(name))
	{
		// Already added.
		return;
	}

	m_Classifications.emplace_back(std::move(name));
}

void EntityClassificationSystem::Clear()
{
	m_Classifications.clear();

	// Add none as the first classification regardless of configuration settings.
	AddClassification(ENTCLASS_NONE_NAME);
}

bool EntityClassificationSystem::ParseConfiguration(const json& input)
{
	// Add classifications first so we can define circular relationships.
	for (const auto& [key, value] : input.items())
	{
		AddClassification(std::string{key});
	}

	for (const auto& [key, value] : input.items())
	{
		auto clazz = FindClassification(key);

		assert(clazz);

		Relationship relationship = Relationship::None;

		if (auto defaultRelationshipTo = value.find("DefaultRelationshipToTarget"); defaultRelationshipTo != value.end())
		{
			const auto relationshipString = defaultRelationshipTo->get<std::string>();

			if (auto result = ParseRelationship(relationshipString); result)
			{
				relationship = *result;
			}
			else
			{
				m_Logger->error("Invalid relationship \"{}\" for class \"{}\" DefaultRelationshipToTarget key",
					relationshipString, key);
			}
		}

		clazz->Relationships.insert(clazz->Relationships.end(), m_Classifications.size(), relationship);

		auto relationships = value.find("Relationships");

		if (relationships == value.end())
		{
			continue;
		}

		for (const auto& [targetClass, relationshipString] : relationships->items())
		{
			const auto targetClassId = static_cast<std::size_t>(GetClass(targetClass));

			const auto relationshipStringValue = relationshipString.get<std::string>();

			if (auto result = ParseRelationship(relationshipStringValue); result)
			{
				clazz->Relationships[targetClassId] = *result;
			}
			else
			{
				m_Logger->error("Invalid relationship \"{}\" for class \"{}\" relationship to \"{}\"",
					relationshipStringValue, key, targetClass);
			}
		}
	}

	return true;
}
