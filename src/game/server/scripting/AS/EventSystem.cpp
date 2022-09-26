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
#include "EventSystem.h"

#include "scripting/AS/ASGenericCall.h"
#include "scripting/AS/ASManager.h"

namespace scripting
{
EventSystem::EventSystem(std::shared_ptr<spdlog::logger> logger, asIScriptEngine* engine, asIScriptContext* context)
	: m_Logger(logger), m_Engine(engine), m_Context(context)
{
	assert(m_Logger);
	assert(m_Engine);
	assert(m_Context);
}

void EventSystem::Subscribe(asIScriptFunction* callback)
{
	as::UniquePtr<asIScriptFunction> cleanup{callback};

	if (auto typeData = TryGetTypeDataForFuncdef(callback); typeData)
	{
		const auto module = as::GetCallingModule();

		m_Logger->trace("Module \"{}\": subscribing callback \"{}\" to event \"{}\"",
			*module, *callback, typeData->Type->GetName());

		typeData->Callbacks.push_back(std::make_unique<CallbackListElement>(module, std::move(cleanup)));
	}
	else
	{
		asGetActiveContext()->SetException(fmt::format("Couldn't subscribe event handler \"{}\"", *callback).c_str(), false);
	}
}

void EventSystem::Unsubscribe(asIScriptFunction* callback)
{
	as::UniquePtr<asIScriptFunction> cleanup{callback};

	if (auto typeData = TryGetTypeDataForFuncdef(callback); typeData)
	{
		// TODO: need to filter by module in case callback is shared.
		// It might be easier to block the use of shared functions as callbacks though.
		// Note that when checking functions for shared you need to account for delegates as well.
		if (auto it = typeData->Callbacks.find(callback); it != typeData->Callbacks.end())
		{
			const auto module = as::GetCallingModule();

			m_Logger->trace("Module \"{}\": unsubscribing callback \"{}\" from event \"{}\"",
				*module, *callback, typeData->Type->GetName());

			typeData->Callbacks.erase(it);
		}
	}
	else
	{
		asGetActiveContext()->SetException(fmt::format("Couldn't unsubscribe event handler \"{}\"", *callback).c_str(), false);
	}
}

void EventSystem::UnsubscribeAllFromModule(asIScriptModule& module)
{
	m_Logger->trace("Removing all event callbacks from module \"{}\"", module);

	for (auto& typeData : m_TypeData)
	{
		typeData.Callbacks.RemoveCallbacksFromModule(&module);
	}
}

void EventSystem::OnModuleDestroying(asIScriptModule& module)
{
	UnsubscribeAllFromModule(module);
}

EventSystem::TypeData* EventSystem::TryGetTypeDataForFuncdef(asIScriptFunction* callback)
{
	if (!callback)
	{
		m_Logger->error("EventSystem: null callback passed");
		return nullptr;
	}

	asIScriptContext* const ctx = asGetActiveContext();

	// Determine which event we're subscribing to based on which (Un)Subscribe overload was called.
	const auto function = ctx->GetSystemFunction();

	int typeId;
	int result = function->GetParam(0, &typeId);

	if (result < 0)
	{
		m_Logger->error("EventSystem: Couldn't get funcdef parameter type: {} ({})", as::ReturnCodeToString(result), result);
		return nullptr;
	}

	auto type = m_Engine->GetTypeInfoById(typeId);

	if (!type)
	{
		m_Logger->error("EventSystem: Couldn't get funcdef type");
		return nullptr;
	}

	if ((type->GetFlags() & asOBJ_FUNCDEF) == 0)
	{
		m_Logger->error("EventSystem: Funcdef parameter \"{}::{}\" isn't a funcdef", type->GetNamespace(), type->GetName());
		return nullptr;
	}

	auto funcdef = type->GetFuncdefSignature();

	auto it = m_FunctionToTypeData.find(funcdef);

	if (it == m_FunctionToTypeData.end())
	{
		m_Logger->error("EventSystem: Funcdef \"{}\" not supported", funcdef->GetName());
		return nullptr;
	}

	return &m_TypeData[it->second];
}

void EventSystem::PublishCore(const TypeData& typeData, EventArgs* args)
{
	m_Logger->trace("Publishing event \"{}\"", typeData.Type->GetName());

	call::Caller caller{m_Context};

	for (const auto& callback : typeData.Callbacks)
	{
		if (!caller.ExecuteFunction<call::ReturnVoid>(callback->Function.get(), args))
		{
			m_Logger->error("Terminating event callback execution because an error occurred");
			break;
		}
	}
}

void RegisterEventSystem(asIScriptEngine& engine, EventSystem& eventSystem)
{
	// Create the Events namespace.
	const std::string previousNamespace = engine.GetDefaultNamespace();
	engine.SetDefaultNamespace("Events");

	engine.SetDefaultNamespace(previousNamespace.c_str());
}
}
