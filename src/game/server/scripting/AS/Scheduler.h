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

#include <chrono>
#include <memory>
#include <vector>

#include <spdlog/logger.h>

#include <angelscript.h>

#include "ScriptPluginList.h"

#include "scripting/AS/as_utils.h"
#include "scripting/AS/CallbackList.h"

namespace scripting
{
/**
 *	@brief Provides a means of scheduling functions for delayed execution
 */
class Scheduler final : public IPluginListener
{
private:
	/**
	 *	@brief Contains all of the information to make a scheduled call
	 */
	struct ScheduledFunction final : public CallbackListElement
	{
		ScheduledFunction(
			const asIScriptModule* module, as::UniquePtr<asIScriptFunction>&& function, float executionTime, float repeatInterval, int repeatCount)
			: CallbackListElement(module, std::move(function)), ExecutionTime(executionTime), RepeatInterval(repeatInterval), RepeatCount(repeatCount)
		{
		}

		ScheduledFunction(const ScheduledFunction&) = delete;
		ScheduledFunction& operator=(const ScheduledFunction&) = delete;

		// Used to avoid cases where the same memory address is used for a handle.
		// Stale handles won't be able to remove unrelated callbacks.
		const std::chrono::system_clock::time_point CreationTime{std::chrono::system_clock::now()};

		float ExecutionTime;
		const float RepeatInterval;
		int RepeatCount;
	};

public:
	const int REPEAT_INFINITE_TIMES = -1;

	struct ScheduledFunctionHandle
	{
		ScheduledFunction* Function{};
		std::chrono::system_clock::time_point CreationTime{};
	};

public:
	explicit Scheduler(std::shared_ptr<spdlog::logger> logger);
	~Scheduler() = default;

	Scheduler(const Scheduler&) = delete;
	Scheduler& operator=(const Scheduler&) = delete;

	/**
	 *	@brief Sets the time used for timer base
	 */
	void SetCurrentTime(float value)
	{
		m_CurrentTime = value;
	}

	void Think(const float currentTime, asIScriptContext& context);

	/**
	 *	@brief Adjusts the execution times for every scheduled function by the given amount
	 */
	void AdjustExecutionTimes(float adjustAmount);

	/**
	 *	@brief Removes all functions that are part of the given module
	 */
	void RemoveFunctionsFromModule(asIScriptModule& module);

	ScheduledFunctionHandle Schedule(asIScriptFunction* callback, float repeatInterval, int repeatCount);

	void ClearCallback(ScheduledFunctionHandle handle);

	void OnModuleDestroying(asIScriptModule& module) override;

private:
	void ClearCallbackCore(const ScheduledFunctionHandle& handle);

	/**
	 *	@brief Executes the given function with the given context
	 *	@return Whether the function should be removed from the list
	 */
	bool ExecuteFunction(ScheduledFunction& function, asIScriptContext& context);

private:
	const std::shared_ptr<spdlog::logger> m_Logger;

	float m_CurrentTime = 0;

	bool m_ExecutingFunctions = false;

	CallbackList<std::vector<std::unique_ptr<ScheduledFunction>>> m_Functions;
	CallbackList<std::vector<std::unique_ptr<ScheduledFunction>>> m_FunctionsToAdd;
	std::vector<ScheduledFunctionHandle> m_FunctionsToRemove;
};

/**
 *	@brief Registers the Scheduler reference type
 */
void RegisterScheduler(asIScriptEngine& engine, Scheduler* scheduler);
}
