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

#include <cassert>
#include <concepts>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>

#include <spdlog/logger.h>

#include <angelscript.h>

#include "ScriptPluginList.h"

#include "scripting/AS/as_utils.h"
#include "scripting/AS/CallbackList.h"

namespace scripting
{
/**
 *	@brief Base class for script event argument types.
 */
class EventArgs : public as::RefCountedClass
{
public:
	EventArgs() = default;

	EventArgs(const EventArgs&) = delete;
	EventArgs& operator=(const EventArgs&) = delete;
};

/**
 *	@brief Handles the registration of Angelscript event types and script side event interfacing.
 */
class EventSystem final : public IPluginListener
{
private:
	struct TypeData
	{
		as::UniquePtr<asITypeInfo> Type;
		CallbackList<std::vector<std::unique_ptr<CallbackListElement>>> Callbacks;
	};

	struct TransparentEqual : public std::equal_to<>
	{
		using is_transparent = void;

		using std::equal_to<>::operator();

		[[nodiscard]] auto operator()(const as::UniquePtr<asIScriptFunction>& left, const asIScriptFunction* right) const noexcept
		{
			return left.get() == right;
		}

		[[nodiscard]] auto operator()(const asIScriptFunction* left, const as::UniquePtr<asIScriptFunction>& right) const noexcept
		{
			return left == right.get();
		}
	};

	struct TransparentHash : public std::hash<as::UniquePtr<asIScriptFunction>>
	{
		using is_transparent = void;

		using std::hash<as::UniquePtr<asIScriptFunction>>::operator();

		[[nodiscard]] size_t operator()(const asIScriptFunction* func) const { return std::hash<const asIScriptFunction*>{}(func); }
	};

public:
	EventSystem(std::shared_ptr<spdlog::logger> logger, asIScriptEngine* engine, asIScriptContext* context);
	~EventSystem() = default;

	EventSystem(const EventSystem&) = delete;
	EventSystem& operator=(const EventSystem&) = delete;

	/**
	 *	@brief Registers an event type for use in scripts.
	 *	@details Registers a reference type with name @p name.
	 *	Registers a funcdef with signature <tt>void &lt;name&gt;Handler(&lt;name&gt;@@)</tt>.
	 *	Registers an @p Event::Subscribe and @p Event::Unsubscribe overload that take the funcdef.
	 *	@tparam TEvent Type of the event arguments class. Must inherit from @c EventArgs.
	 */
	template <std::derived_from<EventArgs> TEvent>
	void RegisterEvent(const char* name);

	/**
	 *	@brief Publishes the event of type @c TEvent.
	 *	@details All callbacks registered to the given event type will be executed in the order that they were subscribed.
	 */
	template <std::derived_from<EventArgs> TEvent, typename... Args>
	as::UniquePtr<TEvent> Publish(Args&&... args);

	void Subscribe(asIScriptFunction* callback);

	void Unsubscribe(asIScriptFunction* callback);

	void UnsubscribeAllFromModule(asIScriptModule& module);

	void OnModuleDestroying(asIScriptModule& module) override;

private:
	TypeData* TryGetTypeDataForFuncdef(asIScriptFunction* callback);

	void PublishCore(const TypeData& typeData, EventArgs* args);

private:
	const std::shared_ptr<spdlog::logger> m_Logger;
	asIScriptEngine* const m_Engine;
	asIScriptContext* const m_Context;

	std::vector<TypeData> m_TypeData;

	std::unordered_map<as::UniquePtr<asIScriptFunction>, std::size_t, TransparentHash, TransparentEqual> m_FunctionToTypeData;
	std::unordered_map<std::type_index, std::size_t> m_TypeToTypeData;
};

template <std::derived_from<EventArgs> TEvent>
void EventSystem::RegisterEvent(const char* name)
{
	assert(name);

	if (auto it = std::find_if(m_TypeData.begin(), m_TypeData.end(), [&](const auto& candidate)
			{ return 0 == strcmp(name, candidate.Type->GetName()); });
		it != m_TypeData.end())
	{
		m_Logger->error("Event type \"{}\" already registered", name);
		return;
	}

	const std::string previousNamespace = m_Engine->GetDefaultNamespace();
	m_Engine->SetDefaultNamespace("");

	m_Engine->RegisterObjectType(name, sizeof(TEvent), asOBJ_REF);

	as::RegisterRefCountedClass(*m_Engine, name);

	m_Engine->RegisterFuncdef(fmt::format("void {0}Handler({0}@)", name).c_str());

	auto type = as::MakeUnique(m_Engine->GetTypeInfoByName(name));
	auto funcdef = as::GetFuncDefByName(*m_Engine, fmt::format("{}Handler", name).c_str());

	assert(type);
	assert(funcdef);

	auto funcdefFunction = as::MakeUnique(funcdef->GetFuncdefSignature());

	assert(funcdefFunction);

	m_TypeData.emplace_back(TypeData{std::move(type), {}});

	m_FunctionToTypeData.insert_or_assign(std::move(funcdefFunction), m_TypeData.size() - 1);
	m_TypeToTypeData.insert_or_assign(typeid(TEvent), m_TypeData.size() - 1);

	m_Engine->SetDefaultNamespace("Events");

	m_Engine->RegisterGlobalFunction(fmt::format("void Subscribe({}Handler@)", name).c_str(),
		asMETHOD(EventSystem, Subscribe), asCALL_THISCALL_ASGLOBAL, this);

	m_Engine->RegisterGlobalFunction(fmt::format("void Unsubscribe({}Handler@)", name).c_str(),
		asMETHOD(EventSystem, Unsubscribe), asCALL_THISCALL_ASGLOBAL, this);

	m_Engine->SetDefaultNamespace(previousNamespace.c_str());
}

template <std::derived_from<EventArgs> TEvent, typename... Args>
as::UniquePtr<TEvent> EventSystem::Publish(Args&&... args)
{
	as::UniquePtr<TEvent> event{new TEvent(std::forward<Args>(args)...)};

	if (auto it = m_TypeToTypeData.find(typeid(TEvent)); it != m_TypeToTypeData.end())
	{
		const auto& typeData = m_TypeData[it->second];

		// Don't do any work if there are no callbacks for this event.
		if (!typeData.Callbacks.empty())
		{
			PublishCore(typeData, event.get());
		}
	}

	return event;
}

void RegisterEventSystem(asIScriptEngine& engine, EventSystem& eventSystem);
}
