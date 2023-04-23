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

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include <spdlog/logger.h>

#include "GameSystem.h"

#include "scripting/AS/as_utils.h"

class asIScriptContext;

/**
 *	@brief Evaluates strings containing conditonal statements.
 */
struct ConditionEvaluator final : public IGameSystem
{
	const char* GetName() const override { return "ConditionEvaluator"; }

	bool Initialize() override;
	void PostInitialize() override {}
	void Shutdown() override;

	/**
	 *	@brief Evaluate a conditional expression
	 *	@return If the conditional was successfully evaluated, returns the result.
	 *		Otherwise, returns an empty optional.
	 */
	std::optional<bool> Evaluate(std::string_view conditional);

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	as::EnginePtr m_ScriptEngine;
	as::UniquePtr<asIScriptContext> m_ScriptContext;
};

inline ConditionEvaluator g_ConditionEvaluator;
