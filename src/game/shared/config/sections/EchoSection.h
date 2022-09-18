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

#include "config/GameConfig.h"

#include "utils/JSONSystem.h"

/**
*	@brief Simple section that echoes a message provided in the config file.
*/
class EchoSection final : public GameConfigSection
{
public:
	explicit EchoSection() = default;

	std::string_view GetName() const override final { return "Echo"; }

	std::tuple<std::string, std::string> GetSchema() const override final
	{
		return {
			fmt::format(R"(
"Message": {{
	"type": "string"
}}
)"),
			{"\"Message\""}};
	}

	bool TryParse(GameConfigContext& context) const override final
	{
		using namespace std::literals;
		//If developer mode is on then this is a debug message, otherwise it's a trace message. Helps to prevent abuse.
		context.Loader.GetLogger()->log(g_pDeveloper->value > 0 ? spdlog::level::debug : spdlog::level::trace,
			"{}", context.Input.value("Message", "No message provided"sv));
		return true;
	}
};
