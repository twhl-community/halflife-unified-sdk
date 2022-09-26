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
#include <cassert>
#include <iterator>

#include "cbase.h"

#include "Scheduler.h"

namespace scripting
{
Scheduler::Scheduler(std::shared_ptr<spdlog::logger> logger)
	: m_Logger(logger)
{
}

void Scheduler::Think(const float currentTime, asIScriptContext& context)
{
	if (!m_Functions.empty())
	{
		const as::PushContextState pushState{m_Logger.get(), &context};

		m_ExecutingFunctions = true;

		for (auto it = m_Functions.begin(); it != m_Functions.end();)
		{
			auto& function = **it;

			if (function.ExecutionTime <= currentTime &&
				ExecuteFunction(function, context))
			{
				it = m_Functions.erase(it);
			}
			else
			{
				++it;
			}
		}

		m_ExecutingFunctions = false;
	}

	// Add new functions
	if (!m_FunctionsToAdd.empty())
	{
		if (m_Functions.empty())
		{
			m_Functions = std::move(m_FunctionsToAdd);
		}
		else
		{
			m_Functions.reserve(m_Functions.size() + m_FunctionsToAdd.size());
			std::move(m_FunctionsToAdd.begin(), m_FunctionsToAdd.end(), std::back_inserter(m_Functions));
			m_FunctionsToAdd.clear();
		}
	}

	// Remove functions flagged for removal
	// This can include functions that were added from m_FunctionsToAdd, so perform this operation after adding those
	for (auto& function : m_FunctionsToRemove)
	{
		ClearCallbackCore(function);
	}

	m_FunctionsToRemove.clear();
}

void Scheduler::AdjustExecutionTimes(float adjustAmount)
{
	for (auto& function : m_Functions)
	{
		function->ExecutionTime += adjustAmount;
	}
}

void Scheduler::RemoveFunctionsFromModule(asIScriptModule& module)
{
	m_Logger->trace("Removing all scheduled callbacks from module \"{}\"", module);
	m_Functions.RemoveCallbacksFromModule(&module);
}

Scheduler::ScheduledFunctionHandle Scheduler::Schedule(asIScriptFunction* callback, float repeatInterval, int repeatCount)
{
	auto context = asGetActiveContext();

	const auto result = [&, this]() -> ScheduledFunctionHandle
	{
		if (!callback)
		{
			m_Logger->error("Scheduler::Schedule: Null callback passed");
			return {};
		}

		as::UniquePtr<asIScriptFunction> function{callback};

		if (callback->GetFuncType() == asFUNC_SYSTEM)
		{
			m_Logger->error("Scheduler::Schedule: Cannot pass system function \"{}\" as callback", *function);
			return {};
		}

		if (repeatCount == 0 || repeatCount < REPEAT_INFINITE_TIMES)
		{
			m_Logger->error("Scheduler::Schedule: Repeat count must be larger than zero or REPEAT_INFINITE_TIMES");
			return {};
		}

		// Allow 0 to execute a function every frame
		if (repeatInterval < 0)
		{
			m_Logger->error("Scheduler::Schedule: Repeat interval must be a positive value");
			return {};
		}

		auto& list = m_ExecutingFunctions ? m_FunctionsToAdd : m_Functions;

		// Store off the module that's calling us so we can remove this callback when that module is destroyed.
		const auto callingFunction = context->GetFunction();
		assert(callingFunction);
		const auto callingModule = callingFunction->GetModule();
		assert(callingModule);

		list.push_back(std::make_unique<ScheduledFunction>(
			callingModule, std::move(function), m_CurrentTime + repeatInterval, repeatInterval, repeatCount));

		auto scheduledFunction = list.back().get();

		auto context = asGetActiveContext();

		m_Logger->trace(
			"Scheduler::Schedule: Scheduled callback \"{}\" from module \"{}\" at {} seconds to start at {} seconds with interval {} and repeat count {}",
			*scheduledFunction->Function, context->GetFunction()->GetModuleName(),
			m_CurrentTime, scheduledFunction->ExecutionTime, repeatInterval, repeatCount);

		return {scheduledFunction, scheduledFunction->CreationTime};
	}();

	if (!result.Function)
	{
		context->SetException("Error scheduling callback", false);
	}

	return result;
}

void Scheduler::ClearCallback(ScheduledFunctionHandle handle)
{
	if (!handle.Function)
	{
		asGetActiveContext()->SetException("Null handle passed to Scheduler::ClearCallback", false);
		return;
	}

	if (m_ExecutingFunctions)
	{
		// Delay until functions have finished executing
		m_FunctionsToRemove.emplace_back(handle);
		return;
	}

	ClearCallbackCore(handle);
}

void Scheduler::OnModuleDestroying(asIScriptModule& module)
{
	RemoveFunctionsFromModule(module);
}

void Scheduler::ClearCallbackCore(const ScheduledFunctionHandle& handle)
{
	if (auto it = std::find_if(m_Functions.begin(), m_Functions.end(), [&](const auto& candidate)
			{ return candidate.get() == handle.Function && candidate->CreationTime == handle.CreationTime; });
		it != m_Functions.end())
	{
		m_Functions.erase(it);
	}
	else
	{
		// It is not an error to clear a callback multiple times, so don't log this.
		//m_Logger->error("Scheduler::ClearCallbackCore: Tried to clear callback that was already cleared");
	}
}

bool Scheduler::ExecuteFunction(ScheduledFunction& function, asIScriptContext& context)
{
	// Call will log any errors if the context has logging enabled
	auto result = context.Prepare(function.Function.get());

	if (result >= 0)
	{
		result = context.Execute();
	}

	if (function.RepeatCount != REPEAT_INFINITE_TIMES)
	{
		--function.RepeatCount;
	}

	const auto shouldRemove = function.RepeatCount == 0;

	if (!shouldRemove)
	{
		// This must not use m_CurrentTime as a base because otherwise additional delays will creep in over time
		// This is because m_ExecutionTime is <= currentTime, not ==
		function.ExecutionTime += function.RepeatInterval;
	}

	return shouldRemove;
}

static void ConstructScheduledFunctionHandle(void* memory)
{
	new (memory) Scheduler::ScheduledFunctionHandle();
}

static void DestructScheduledFunctionHandle(Scheduler::ScheduledFunctionHandle* handle)
{
	handle->~ScheduledFunctionHandle();
}

static void CopyConstructScheduledFunctionHandle(void* memory, const Scheduler::ScheduledFunctionHandle& other)
{
	new (memory) Scheduler::ScheduledFunctionHandle(other);
}

static bool ScheduledFunction_opImplConv(const Scheduler::ScheduledFunctionHandle* handle)
{
	return handle->Function != nullptr;
}

static void RegisterScheduledFunction(asIScriptEngine& engine)
{
	const char* const name = "ScheduledFunction";

	engine.RegisterObjectType(name, sizeof(Scheduler::ScheduledFunctionHandle), asOBJ_VALUE | asGetTypeTraits<Scheduler::ScheduledFunctionHandle>());

	engine.RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, "void f()",
		asFUNCTION(ConstructScheduledFunctionHandle), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectBehaviour(name, asBEHAVE_DESTRUCT, "void f()",
		asFUNCTION(DestructScheduledFunctionHandle), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, "void f(const ScheduledFunction& in other)",
		asFUNCTION(CopyConstructScheduledFunctionHandle), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectMethod(name, "ScheduledFunction& opAssign(const ScheduledFunction& in other)",
		asMETHODPR(Scheduler::ScheduledFunctionHandle, operator=, (const Scheduler::ScheduledFunctionHandle&), Scheduler::ScheduledFunctionHandle&),
		asCALL_THISCALL);

	engine.RegisterObjectMethod(name, "bool opImplConv() const",
		asFUNCTION(ScheduledFunction_opImplConv), asCALL_CDECL_OBJFIRST);
}

void RegisterScheduler(asIScriptEngine& engine, Scheduler* scheduler)
{
	assert(scheduler);

	const std::string previousNamespace = engine.GetDefaultNamespace();
	engine.SetDefaultNamespace("");

	engine.RegisterFuncdef("void ScheduledCallback()");

	RegisterScheduledFunction(engine);

	const auto className = "ScriptScheduler";

	engine.RegisterObjectType(className, 0, asOBJ_REF | asOBJ_NOCOUNT);

	engine.RegisterObjectProperty(className, "const int REPEAT_INFINITE_TIMES", asOFFSET(Scheduler, REPEAT_INFINITE_TIMES));

	engine.RegisterObjectMethod(className,
		"ScheduledFunction Schedule(ScheduledCallback@ callback, float repeatInterval, int repeatCount = 1)",
		asMETHOD(Scheduler, Schedule), asCALL_THISCALL);

	engine.RegisterObjectMethod(className, "void ClearCallback(ScheduledFunction function)",
		asMETHOD(Scheduler, ClearCallback), asCALL_THISCALL);

	engine.RegisterGlobalProperty("ScriptScheduler Scheduler", scheduler);

	engine.SetDefaultNamespace(previousNamespace.c_str());
}
}
