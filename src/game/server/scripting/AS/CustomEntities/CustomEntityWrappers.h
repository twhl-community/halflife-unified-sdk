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
#include <string>

#include "cbase.h"

#include "ICustomEntity.h"

#include "scripting/AS/as_utils.h"
#include "scripting/AS/ASGenericCall.h"
#include "scripting/AS/ASManager.h"
#include "scripting/AS/ASScriptingSystem.h"

namespace scripting
{
/**
*	@brief Base class for all custom entity wrappers.
*/
template<std::derived_from<CBaseEntity> TBaseClass>
class CustomBaseEntity : public TBaseClass, public ICustomEntity
{
public:
	ICustomEntity* MyCustomEntityPointer() override final { return this; }

	CBaseEntity* GetEntityPointer() override final { return this; }

	void SetScriptObject(as::UniquePtr<asIScriptObject>&& object) override final
	{
		m_Object = std::move(object);

		auto type = m_Object->GetObjectType();

		// TODO: instead of caching these in every entity, cache them on a per-script-type basis.
		// Any given script class will have the same functions so this will simplify things a bit.
		m_OnCreate = as::MakeUnique(type->GetMethodByDecl("void OnCreate()"));
		m_KeyValue = as::MakeUnique(type->GetMethodByDecl("bool KeyValue(const string& in key, const string& in value)"));
		m_Precache = as::MakeUnique(type->GetMethodByDecl("void Precache()"));
		m_Spawn = as::MakeUnique(type->GetMethodByDecl("void Spawn()"));
		m_IsAlive = as::MakeUnique(type->GetMethodByDecl("bool IsAlive()"));
	}

	void SetThinkFunction(asIScriptFunction* function) override final
	{
		SetDelegateFunction(m_ThinkFunction, function);
	}

	void SetTouchFunction(asIScriptFunction* function) override final
	{
		SetDelegateFunction(m_TouchFunction, function);
	}

	void SetBlockedFunction(asIScriptFunction* function) override final
	{
		SetDelegateFunction(m_BlockedFunction, function);
	}

	void SetUseFunction(asIScriptFunction* function) override final
	{
		SetDelegateFunction(m_UseFunction, function);
	}

	void Think() override final
	{
		if (!m_ThinkFunction)
		{
			TBaseClass::Think();
			return;
		}

		ExecuteFunction<call::ReturnVoid>(m_ThinkFunction);
	}

	void Touch(CBaseEntity* other) override final
	{
		if (!m_TouchFunction)
		{
			TBaseClass::Touch(other);
			return;
		}

		ExecuteFunction<call::ReturnVoid>(m_TouchFunction, call::Object{other});
	}

	void Blocked(CBaseEntity* other) override final
	{
		if (!m_BlockedFunction)
		{
			TBaseClass::Blocked(other);
			return;
		}

		ExecuteFunction<call::ReturnVoid>(m_BlockedFunction, call::Object{other});
	}

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override final
	{
		if (!m_UseFunction)
		{
			TBaseClass::Use(pActivator, pCaller, useType, value);
			return;
		}

		ExecuteFunction<call::ReturnVoid>(m_UseFunction, call::Object{pActivator}, call::Object{pCaller}, useType, value);
	}

	void OnCreate() override final
	{
		if (!m_OnCreate)
		{
			TBaseClass::OnCreate();
			return;
		}

		ExecuteFunction<call::ReturnVoid>(m_OnCreate);
	}

	bool KeyValue(KeyValueData* pkvd) override final
	{
		if (!m_KeyValue)
		{
			return TBaseClass::KeyValue(pkvd);
		}

		const std::string key{pkvd->szKeyName};
		const std::string value{pkvd->szValue};

		return ExecuteFunction<bool>(m_KeyValue, &key, &value).value_or(false);
	}

	void Precache() override final
	{
		if (!m_Spawn)
		{
			TBaseClass::Precache();
			return;
		}

		ExecuteFunction<call::ReturnVoid>(m_Precache);
	}

	void Spawn() override final
	{
		if (!m_Spawn)
		{
			TBaseClass::Spawn();
			return;
		}

		ExecuteFunction<call::ReturnVoid>(m_Spawn);
	}

	bool IsAlive() override final
	{
		if (!m_IsAlive)
		{
			return TBaseClass::IsAlive();
		}

		return ExecuteFunction<bool>(m_IsAlive).value_or(false);
	}

protected:
	template<typename TReturn, typename... Args>
	std::optional<TReturn> ExecuteFunction(const as::UniquePtr<asIScriptFunction>& function, Args&&... args)
	{
		const as::PushContextState pushState{g_Scripting.GetLogger(), g_Scripting.GetContext()};

		if (!pushState)
		{
			return {};
		}

		return call::ExecuteObjectMethod<TReturn>(*g_Scripting.GetContext(), m_Object.get(), function.get(), std::forward<Args>(args)...);
	}

	/**
	*	@brief Given a delegate function, checks to make sure its part of our script type and
	*		sets it up in @p actualFunction for execution.
	*/
	void SetDelegateFunction(as::UniquePtr<asIScriptFunction>& actualFunction, asIScriptFunction* delegateFunction)
	{
		if (!delegateFunction)
		{
			actualFunction.reset();
			return;
		}

		as::UniquePtr<asIScriptFunction> cleanup{delegateFunction};

		if (cleanup->GetFuncType() != asFUNC_DELEGATE || cleanup->GetDelegateObject() != m_Object.get())
		{
			asGetActiveContext()->SetException("Invalid think function passed to SetThink");
			actualFunction.reset();
			return;
		}

		// We'll call it directly so get the underlying function.
		actualFunction.reset(delegateFunction->GetDelegateFunction());
		actualFunction->AddRef();
	}

protected:
	as::UniquePtr<asIScriptObject> m_Object;

	as::UniquePtr<asIScriptFunction> m_ThinkFunction;
	as::UniquePtr<asIScriptFunction> m_TouchFunction;
	as::UniquePtr<asIScriptFunction> m_BlockedFunction;
	as::UniquePtr<asIScriptFunction> m_UseFunction;

	as::UniquePtr<asIScriptFunction> m_OnCreate;
	as::UniquePtr<asIScriptFunction> m_KeyValue;
	as::UniquePtr<asIScriptFunction> m_Precache;
	as::UniquePtr<asIScriptFunction> m_Spawn;
	as::UniquePtr<asIScriptFunction> m_IsAlive;
};

template<typename TBaseClass>
inline std::unique_ptr<CBaseEntity> CreateCustomEntityWrapper()
{
	return std::make_unique<TBaseClass>();
}
}
