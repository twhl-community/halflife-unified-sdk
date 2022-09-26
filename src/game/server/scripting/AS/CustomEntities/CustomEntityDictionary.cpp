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
#include <cctype>

#include "cbase.h"
#include "CustomEntityConstants.h"
#include "CustomEntityDictionary.h"
#include "CustomEntityScriptClasses.h"
#include "ICustomEntity.h"

#include "scripting/AS/ASGenericCall.h"
#include "scripting/AS/ASManager.h"

namespace scripting
{
CustomEntityDictionary::CustomEntityDictionary(std::shared_ptr<spdlog::logger> logger,
	asIScriptEngine* engine, asIScriptContext* context, CustomEntityScriptClasses* customEntityScriptClasses)
	: m_Logger(logger), m_Context(context), m_CustomEntityScriptClasses(customEntityScriptClasses)
{
	assert(m_Logger);
	assert(engine);
	assert(m_Context);
	assert(m_CustomEntityScriptClasses);

	RegisterCustomEntityDictionary(*engine, this);

	if (auto module = m_CustomEntityScriptClasses->GetModule(); module)
	{
		m_BaseClassType = as::MakeUnique(module->GetTypeInfoByName(ScriptBaseClass));
	}

	if (!m_BaseClassType)
	{
		m_Logger->error("Internal error getting base class \"{}\" type", ScriptBaseClass);
	}
}

bool CustomEntityDictionary::RegisterType(const std::string& className, const std::string& typeName)
{
	auto context = asGetActiveContext();
	assert(context);

	auto function = context->GetFunction();
	assert(function);

	auto module = function->GetModule();
	assert(module);

	return RegisterTypeCore(className, typeName, *context, *module);
}

bool CustomEntityDictionary::RegisterTypeCore(
	const std::string& className, const std::string& typeName, asIScriptContext& context, asIScriptModule& module)
{
	m_Logger->trace("Registering custom entity classname \"{}\" with type \"{}\"", className, typeName);

	const bool success = [&, this]()
	{
		if (className.empty() || std::find_if(className.begin(), className.end(), [](auto c)
									 { return std::isalnum(c) == 0 && c != '_'; }) != className.end())
		{
			m_Logger->error("Can't register custom class: the classname \"{}\" is invalid", className);
			return false;
		}

		auto type = module.GetTypeInfoByDecl(typeName.c_str());

		if (!type)
		{
			m_Logger->error("Can't register custom class: the type \"{}\" does not exist", typeName);
			return false;
		}

		if ((type->GetFlags() & asOBJ_SCRIPT_OBJECT) == 0)
		{
			m_Logger->error("Can't register custom class: the type \"{}\" is not a script class", typeName);
			return false;
		}

		if ((type->GetFlags() & asOBJ_ABSTRACT) != 0)
		{
			m_Logger->error("Can't register custom class: the type \"{}\" is abstract", typeName);
			return false;
		}

		if (!m_BaseClassType)
		{
			m_Logger->error("Can't register custom class: internal error getting base class \"{}\" type", ScriptBaseClass);
			return false;
		}

		if (m_BaseClassType.get() == type)
		{
			m_Logger->error("Can't register custom class: the type \"{}\" cannot be the script base class \"{}\"", typeName, ScriptBaseClass);
			return false;
		}

		if (!type->DerivesFrom(m_BaseClassType.get()))
		{
			m_Logger->error("Can't register custom class: the type \"{}\" does not inherit from the script base class \"{}\"",
				typeName, ScriptBaseClass);
			return false;
		}

		auto constructor = type->GetFactoryByDecl(fmt::format("{}@ f()", as::FormatTypeName(*type)).c_str());

		if (!constructor)
		{
			m_Logger->error("Can't register custom class: the type \"{}\" does not have a parameterless constructor", typeName);
			return false;
		}

		const auto wrapperFactory = [&]() -> const std::function<std::unique_ptr<CBaseEntity>()>*
		{
			for (auto baseType = type->GetBaseType(); baseType; baseType = baseType->GetBaseType())
			{
				m_Logger->trace("Checking class \"{}\" for a C++ wrapper", *baseType);

				if (auto factory = m_CustomEntityScriptClasses->FindWrapperFactory(baseType->GetName()); factory)
				{
					return factory;
				}
			}

			return nullptr;
		}();

		if (!wrapperFactory)
		{
			m_Logger->error("Can't register custom class: internal error getting C++ wrapper factory for type \"{}\" (did you inherit directly from {}?)",
				typeName, ScriptBaseClass);
			return false;
		}

		// Do this after validating the type so plugin authors get all validation errors first.
		if (auto it = m_Dictionary.find(className); it != m_Dictionary.end())
		{
			m_Logger->error("Can't register custom class: the classname \"{}\" is already in use with type \"{}\" in module \"{}\"",
				className, *it->second.Type, *it->second.Type->GetModule());
			return false;
		}

		m_Dictionary.insert_or_assign(
			className,
			CustomTypeInfo{wrapperFactory, as::MakeUnique(type), as::MakeUnique(constructor)});

		m_Logger->debug("Registered custom entity classname \"{}\" with type \"{}\"", className, typeName);

		return true;
	}();

	if (!success)
	{
		as::PrintStackTrace(*m_Logger, spdlog::level::err, context);
	}

	return success;
}

std::unique_ptr<CBaseEntity> CustomEntityDictionary::TryCreateCustomEntity(const char* className, entvars_t* pev)
{
	assert(className);
	assert(pev);

	if (auto it = m_Dictionary.find(className); it != m_Dictionary.end())
	{
		auto& typeInfo = it->second;

		call::Caller caller{m_Context};

		if (!caller.ExecuteFunction<call::ReturnVoid>(typeInfo.Constructor.get()))
		{
			m_Logger->error("Couldn't create custom entity due to context error");
			return {};
		}

		as::UniquePtr<asIScriptObject> obj = as::MakeUnique(*reinterpret_cast<asIScriptObject**>(m_Context->GetAddressOfReturnValue()));

		if (!obj)
		{
			m_Logger->error("Error calling custom entity constructor");
			return {};
		}

		auto wrapperEntity = (*typeInfo.WrapperFactory)();

		if (!wrapperEntity)
		{
			assert(!"This should never happen");
			m_Logger->error("Internal error creating wrapper entity for custom entity \"{}\"", className);
			return {};
		}

		auto customEntity = wrapperEntity->MyCustomEntityPointer();

		if (!customEntity)
		{
			assert(!"This should never happen");
			m_Logger->error("Internal error getting custom entity pointer for custom entity \"{}\"", className);
			return {};
		}

		// Directly set the three private members that glue the script instance to the C++ instance.
		const asUINT count = obj->GetPropertyCount();

		const auto getPropertyAddress = [&](const char* name) -> void*
		{
			for (asUINT i = 0; i < count; ++i)
			{
				if (0 == strcmp(name, obj->GetPropertyName(i)))
				{
					return obj->GetAddressOfProperty(i);
				}
			}

			return nullptr;
		};

		const auto selfAddress = getPropertyAddress("m_Self");
		const auto baseClassAddress = getPropertyAddress("m_BaseClass");
		const auto callbackAddress = getPropertyAddress("m_CallbackHandler");

		if (!selfAddress || !baseClassAddress || !callbackAddress)
		{
			assert(!"This should never happen");
			m_Logger->error("Internal error getting Self, BaseClass and/or CallbackHandler for custom entity \"{}\"", className);
			return {};
		}

		// Though the script types are all reference counted,
		// the wrapper doesn't have a ref count so we don't have to do extra work.
		*reinterpret_cast<CBaseEntity**>(selfAddress) = wrapperEntity.get();
		*reinterpret_cast<CBaseEntity**>(baseClassAddress) = wrapperEntity.get();
		// Important! The address of ICustomEntity* is different from that of CBaseEntity* here so they are not interchangeable!
		*reinterpret_cast<ICustomEntity**>(callbackAddress) = wrapperEntity->MyCustomEntityPointer();

		customEntity->SetScriptObject(std::move(obj));

		// Transfer the edict to the actual entity object.
		wrapperEntity->pev = pev;
		wrapperEntity->edict()->pvPrivateData = wrapperEntity.get();
		wrapperEntity->pev->classname = ALLOC_STRING(className);

		wrapperEntity->OnCreate();

		return wrapperEntity;
	}

	return {};
}

void CustomEntityDictionary::RemoveAllFromModule(asIScriptModule& module)
{
	//TODO: if there are entities using these types they will still exist. Need to either remove them or delay plugin removal.

	m_Logger->trace("Removing all custom entity types from module \"{}\"", module);

	for (auto it = m_Dictionary.begin(); it != m_Dictionary.end();)
	{
		if (it->second.Type->GetModule() == &module)
		{
			it = m_Dictionary.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void CustomEntityDictionary::RemoveAll()
{
	m_Dictionary.clear();
}

void CustomEntityDictionary::OnModuleDestroying(asIScriptModule& module)
{
	RemoveAllFromModule(module);
}

void RegisterCustomEntityDictionary(asIScriptEngine& engine, CustomEntityDictionary* customEntityDictionary)
{
	assert(customEntityDictionary);

	const std::string previousNamespace = engine.GetDefaultNamespace();
	engine.SetDefaultNamespace("CustomEntities");

	engine.RegisterGlobalFunction("bool RegisterType(const string& in className, const string& in typeName)",
		asMETHOD(CustomEntityDictionary, RegisterType), asCALL_THISCALL_ASGLOBAL, customEntityDictionary);

	engine.SetDefaultNamespace(previousNamespace.c_str());
}
}
