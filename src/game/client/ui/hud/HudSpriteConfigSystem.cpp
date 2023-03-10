/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#include "ClientLibrary.h"
#include "HudSpriteConfigSystem.h"
#include "JSONSystem.h"

constexpr std::string_view HudSpriteConfigSchemaName{"HudSpriteConfig"sv};

static std::string GetHudSpriteConfigSchema()
{
	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Hud Sprite Configuration",
	"type": "object",
	"patternProperties": {{
		"^\\w+$": {{
			"type": "object",
			"properties": {{
				"SpriteName": {{
					"type": "string",
					"pattern": "^\\w+$"
				}},
				"Left": {{
					"type": "integer"
				}},
				"Top": {{
					"type": "integer"
				}},
				"Width": {{
					"type": "integer"
				}},
				"Height": {{
					"type": "integer"
				}}
			}},
			"required": [
				"SpriteName",
				"Left",
				"Top",
				"Width",
				"Height"
			]
		}}
	}},
	"additionalProperties": false
}}
)");
}

bool HudSpriteConfigSystem::Initialize()
{
	g_JSON.RegisterSchema(HudSpriteConfigSchemaName, &GetHudSpriteConfigSchema);
	return true;
}

static std::vector<HudSprite> ParseHudSpriteConfiguration(const json& input)
{
	std::vector<HudSprite> sprites;
	sprites.reserve(input.size());

	for (const auto& [hudSpriteName, data] : input.items())
	{
		HudSprite sprite;

		std::string trimmedHudSpriteName{Trim(hudSpriteName)};
		std::string trimmedSpriteName{Trim(data.value("SpriteName"sv, ""sv))};

		if (trimmedHudSpriteName.empty() || trimmedSpriteName.empty())
		{
			continue;
		}

		sprite.Name = trimmedHudSpriteName.c_str();
		sprite.SpriteName = trimmedSpriteName.c_str();
		sprite.Rectangle.left = data.value<int>("Left"sv, 0);
		sprite.Rectangle.top = data.value<int>("Top"sv, 0);
		sprite.Rectangle.right = sprite.Rectangle.left + data.value<int>("Width"sv, 0);
		sprite.Rectangle.bottom = sprite.Rectangle.top + data.value<int>("Height"sv, 0);

		sprites.push_back(std::move(sprite));
	}

	return sprites;
}

std::vector<HudSprite> HudSpriteConfigSystem::Load(const char* fileName)
{
	auto sprites = g_JSON.ParseJSONFile(fileName, {.SchemaName = HudSpriteConfigSchemaName}, ParseHudSpriteConfiguration)
					   .value_or(std::vector<HudSprite>{});

	// TODO: use a logger for hud messages?
	g_GameLogger->debug("Loaded {} hud sprites from \"{}\"", sprites.size(), fileName);

	return sprites;
}

/**
 *	@brief Finds and returns the matching sprite name @p spriteName in the given sprite list @p sprites
 */
const HudSprite* GetSpriteList(const std::vector<HudSprite>& sprites, const char* spriteName)
{
	if (auto it = std::find_if(sprites.begin(), sprites.end(), [&](const auto& candidate)
			{ return candidate.Name == spriteName; });
		it != sprites.end())
	{
		return &(*it);
	}

	return nullptr;
}
