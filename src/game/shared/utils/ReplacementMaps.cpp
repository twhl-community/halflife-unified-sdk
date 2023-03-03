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

#include <iterator>

#include <fmt/format.h>

#include "cbase.h"
#include "JSONSystem.h"
#include "ReplacementMaps.h"

using namespace std::literals;

constexpr std::string_view ReplacementMapSchemaName{"ReplacementMap"sv};

static std::string GetReplacementMapSchema()
{
	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Replacement Map File",
	"type": "object",
	"patternProperties": {{
		".*": {{
			"type": "string"
		}}
	}}
}}
)");
}

template <>
struct fmt::formatter<ReplacementMapOptions>
{
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		auto it = ctx.begin();

		if (it != ctx.end() && *it != '}')
		{
			throw format_error("invalid format");
		}

		return it;
	}

	template <typename FormatContext>
	auto format(const ReplacementMapOptions& options, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), R"(
CaseSensitive = {}
LoadFromAllPaths = {}
)",
			options.CaseSensitive, options.LoadFromAllPaths);
	}
};

static const char* LookupReplacement(const Replacements& map, const char* searchString, const char* originalValue)
{
	if (auto it = map.find(searchString); it != map.end())
	{
		return it->second.c_str();
	}

	return originalValue;
}

const char* ReplacementMap::Lookup(const char* value) const noexcept
{
	if (!m_CaseSensitive)
	{
		Filename searchString{value};
		searchString.make_lower();
		return LookupReplacement(m_Replacements, searchString.c_str(), value);
	}

	return LookupReplacement(m_Replacements, value, value);
}

bool ReplacementMapSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("replacementmap");

	g_JSON.RegisterSchema(ReplacementMapSchemaName, &GetReplacementMapSchema);

	return true;
}

void ReplacementMapSystem::Shutdown()
{
	m_Logger.reset();
}

void ReplacementMapSystem::Clear()
{
	m_ReplacementMaps.clear();
}

Replacements ReplacementMapSystem::Parse(const json& input, const ReplacementMapOptions& options) const
{
	m_Logger->trace("Parsing replacement map with options:\n{}", options);

	Replacements map;

	if (input.is_object())
	{
		map.reserve(input.size());

		for (const auto& kv : input.items())
		{
			std::string key = kv.key();
			auto value = kv.value().get<std::string>();

			if (!options.CaseSensitive)
			{
				ToLower(key);
				ToLower(value);
			}

			if (const auto result = map.emplace(std::move(key), value); !result.second)
			{
				if (value != result.first->second)
				{
					m_Logger->warn("Ignoring duplicate replacement \"{}\" => \"{}\" (existing replacement is \"{}\")",
						kv.key(), value, result.first->second);
				}
				else
				{
					m_Logger->debug("Ignoring duplicate replacement \"{}\"", kv.key());
				}
			}
		}
	}

	return map;
}

Replacements ReplacementMapSystem::ParseFile(const char* fileName, const ReplacementMapOptions& options) const
{
	const auto pathID = options.LoadFromAllPaths ? nullptr : "GAMECONFIG";

	return g_JSON.ParseJSONFile(
						 fileName,
						 {.SchemaName = ReplacementMapSchemaName, .PathID = pathID},
						 [&, this](const json& input)
						 { return Parse(input, options); })
		.value_or(Replacements{});
}

const ReplacementMap* ReplacementMapSystem::Load(const std::string& fileName, const ReplacementMapOptions& options)
{
	std::string normalizedFileName{fileName};

	ToLower(normalizedFileName);

	if (auto it = m_ReplacementMaps.find(normalizedFileName); it != m_ReplacementMaps.end())
	{
		if (it->second->IsCaseSensitive() != options.CaseSensitive)
		{
			m_Logger->warn(
				"Replacement file \"{}\" was previously loaded with case sensitivity {} and is now being loaded from cache with case sensitivity {}",
				fileName, it->second->IsCaseSensitive() ? "on" : "off", options.CaseSensitive ? "on" : "off");
		}

		return it->second.get();
	}

	auto map = ParseFile(fileName.c_str(), options);

	auto replacementMap = std::make_unique<ReplacementMap>(std::move(map), options.CaseSensitive);

	const auto result = m_ReplacementMaps.emplace(std::move(normalizedFileName), std::move(replacementMap));

	return result.first->second.get();
}

std::unique_ptr<ReplacementMap> ReplacementMapSystem::LoadMultiple(const std::span<std::string> fileNames, const ReplacementMapOptions& options) const
{
	Replacements map;

	// Load each file and insert existing entries into the new map.
	// Only entries not in the new map will be added.
	for (const auto& fileName : fileNames)
	{
		auto next = ParseFile(fileName.c_str(), options);

		next.insert(std::make_move_iterator(map.begin()), std::make_move_iterator(map.end()));

		map = std::move(next);
	}

	return std::make_unique<ReplacementMap>(std::move(map), options.CaseSensitive);
}
