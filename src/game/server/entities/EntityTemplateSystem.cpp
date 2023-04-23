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
#include "EntityTemplateSystem.h"

constexpr std::string_view EntityTemplateSchemaName{"EntityTemplate"sv};

static std::string GetEntityTemplateSchema()
{
	return R"(
{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Entity Template",
	"type": "object",
	"properties": {
		"^.+$": {
			"title": "Entity KeyValue",
			"type": "string"
		}
	}
})";
}

bool EntityTemplateSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("ent.template");
	g_JSON.RegisterSchema(EntityTemplateSchemaName, &GetEntityTemplateSchema);
	return true;
}

void EntityTemplateSystem::Shutdown()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

void EntityTemplateSystem::LoadTemplates(const EntityTemplateMap& templateMap)
{
	m_Templates.clear();

	m_Logger->debug("Loading {} templates", templateMap.size());

	for (const auto& [key, value] : templateMap)
	{
		auto entityTemplate = g_JSON.ParseJSONFile(value.c_str(), {.SchemaName = EntityTemplateSchemaName},
			[this](const auto& input)
			{ return LoadTemplate(input); });

		if (entityTemplate)
		{
			m_Logger->debug("Loaded template \"{}\" for \"{}\" with {} keyvalues", value, key, entityTemplate->size());
			m_Templates.insert_or_assign(key, std::move(*entityTemplate));
		}
	}
}

void EntityTemplateSystem::MaybeApplyTemplate(CBaseEntity* entity)
{
	assert(entity);

	auto entityTemplate = m_Templates.find(entity->GetClassname());

	if (entityTemplate == m_Templates.end())
	{
		return;
	}

	m_Logger->debug("Applying template to \"{}\" with {} keyvalues",
		entityTemplate->first, entityTemplate->second.size());

	for (const auto& [key, value] : entityTemplate->second)
	{
		KeyValueData kvd{
			.szClassName = entity->GetClassname(),
			.szKeyName = key.c_str(),
			.szValue = value.c_str(),
			.fHandled = 0};

		DispatchKeyValue(entity->edict(), &kvd);
	}
}

std::unordered_map<std::string, std::string> EntityTemplateSystem::LoadTemplate(const json& input)
{
	std::unordered_map<std::string, std::string> keyValues;

	keyValues.reserve(input.size());

	for (const auto& [key, value] : input.items())
	{
		keyValues.insert_or_assign(key, value.get<std::string>());
	}

	return keyValues;
}
