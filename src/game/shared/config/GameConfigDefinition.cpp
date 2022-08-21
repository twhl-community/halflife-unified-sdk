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

#include <sstream>
#include <stdexcept>
#include <string_view>

#include <fmt/format.h>

#include "cbase.h"

#include "GameConfigDefinition.h"
#include "GameConfigSection.h"

using namespace std::literals;

GameConfigDefinition::GameConfigDefinition(std::string&& name, std::vector<std::unique_ptr<const GameConfigSection>>&& sections)
	: m_Name(std::move(name)), m_Sections(std::move(sections))
{
}

GameConfigDefinition::~GameConfigDefinition() = default;

const GameConfigSection* GameConfigDefinition::FindSection(std::string_view name) const
{
	if (auto it = std::find_if(m_Sections.begin(), m_Sections.end(), [&](const auto& section)
			{ return section->GetName() == name; });
		it != m_Sections.end())
	{
		return it->get();
	}

	return nullptr;
}

std::string GameConfigDefinition::GetSchema() const
{
	//A configuration is structured like this:
	/*
	{
		"Includes": [
			"some_filename.json"
		],
		"Sections": [
			{
				"Name": "SectionName",
				Section-specific properties here
			}
		]
	}
	*/

	//This structure allows for future expansion without making breaking changes

	const auto sections = [this]()
	{
		std::ostringstream stream;

		bool first = true;

		//Each section is comprised of a reserved key "Name" whose value is required to be the name of the section,
		//and keys specified by each section
		for (const auto& section : m_Sections)
		{
			if (!first)
			{
				stream << ',';
			}

			first = false;

			const auto [definition, required] = section->GetSchema();

			stream << fmt::format(R"(
{{
	"title": "{0} Section",
	"type": "object",
	"properties": {{
		"Name": {{
			"type": "string",
			"const": "{0}",
			"default": "{0}",
			"readOnly": true
		}},
		"Condition": {{
			"description": "If specified, this section will only be used if the condition evaluates true",
			"type": "string"
		}},
		{1}
	}},
	"required": [
		"Name"{2}
		{3}
	]
}}
)",
				section->GetName(), definition, required.empty() ? ""sv : ","sv, required);
		}

		return stream.str();
	}();

	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "{} Game Configuration",
	"type": "object",
	"properties": {{
		"Includes": {{
			"title": "Included Files",
			"description": "List of JSON files to include before this file.",
			"type": "array",
			"items": {{
				"title": "Include",
				"type": "string",
				"pattern": "^[\\w/]+.json$"
			}}
		}},
		"Sections": {{
			"title": "Sections",
			"type": "array",
			"items": {{
				"anyOf": [
					{}
				]
			}}
		}}
	}}
}}
)",
		this->GetName(), sections);
}
