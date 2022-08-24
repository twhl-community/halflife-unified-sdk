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
ConvertToLowercase = {}
LoadFromAllPaths = {}
)",
			options.ConvertToLowercase, options.LoadFromAllPaths);
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

const char* ReplacementMap::Lookup(const char* value, bool lowercase) const noexcept
{
	if (lowercase)
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

ReplacementMap ReplacementMapSystem::Parse(const json& input, const ReplacementMapOptions& options) const
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

			if (options.ConvertToLowercase)
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

	return ReplacementMap{std::move(map)};
}

const ReplacementMap* ReplacementMapSystem::Load(const std::string& fileName, const ReplacementMapOptions& options)
{
	std::string normalizedFileName{fileName};

	ToLower(normalizedFileName);

	if (auto it = m_ReplacementMaps.find(normalizedFileName); it != m_ReplacementMaps.end())
	{
		return it->second.get();
	}

	const auto pathID = options.LoadFromAllPaths ? nullptr : "GAMECONFIG";

	auto map = g_JSON.ParseJSONFile(
					 fileName.c_str(),
					 {.SchemaName = ReplacementMapSchemaName, .PathID = pathID},
					 [&, this](const json& input)
					 { return Parse(input, options); })
		.value_or(ReplacementMap{});

	const auto result = m_ReplacementMaps.emplace(std::move(normalizedFileName), std::make_unique<ReplacementMap>(std::move(map)));

	return result.first->second.get();
}
