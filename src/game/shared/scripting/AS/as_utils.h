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
#include <limits>
#include <memory>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include <spdlog/logger.h>

#include <angelscript.h>

namespace as
{
struct ScriptEngineDeleter
{
	void operator()(asIScriptEngine* engine) const noexcept
	{
		if (engine)
		{
			engine->ShutDownAndRelease();
		}
	}
};

using EnginePtr = std::unique_ptr<asIScriptEngine, ScriptEngineDeleter>;

struct ScriptModuleDeleter
{
	void operator()(asIScriptModule* module) const noexcept
	{
		if (module)
		{
			module->Discard();
		}
	}
};

using ModulePtr = std::unique_ptr<asIScriptModule, ScriptModuleDeleter>;

/**
 *	@brief Custom deleter for Angelscript objects stored in @c std::unique_ptr
 */
template <typename T>
struct ObjectDeleter
{
	void operator()(T* object) const noexcept
	{
		if (object)
		{
			object->Release();
		}
	}
};

template <typename T>
using UniquePtr = std::unique_ptr<T, ObjectDeleter<T>>;

template <typename T>
UniquePtr<T> MakeUnique(T* object)
{
	UniquePtr<T> ptr{object};

	if (ptr)
	{
		ptr->AddRef();
	}

	return ptr;
}

class RefCountedClass
{
public:
	RefCountedClass() = default;
	virtual ~RefCountedClass() = default;

	void AddRef() const
	{
		++m_RefCount;
	}

	void Release() const
	{
		if (--m_RefCount == 0)
		{
			delete this;
		}
	}

private:
	mutable int m_RefCount = 1;
};

void RegisterRefCountedClass(asIScriptEngine& engine, const char* name);

template <typename TDerived, typename TBase>
TDerived* DownCast(TBase* base)
{
	return dynamic_cast<TDerived*>(base);
}

template <typename TBase, typename TDerived>
TBase* UpCast(TDerived* derived)
{
	return static_cast<TBase*>(derived);
}

/**
*	@brief Registers an implicit conversion from derived to base and an explicit cast from base to derived.
*/
template <typename TDerived, typename TBase>
void RegisterCasts(asIScriptEngine& engine, const char* derivedName, const char* baseName)
{
	engine.RegisterObjectMethod(derivedName, fmt::format("{}@ opImplCast()", baseName).c_str(),
		asFUNCTION((UpCast<TBase, TDerived>)), asCALL_CDECL_OBJFIRST);

		engine.RegisterObjectMethod(baseName, fmt::format("{}@ opCast()", derivedName).c_str(),
		asFUNCTION((DownCast<TDerived, TBase>)), asCALL_CDECL_OBJFIRST);
}

asITypeInfo* GetFuncDefByName(asIScriptEngine& engine, const char* name);

/**
 *	@brief Gets the actual script function that this function refers to.
 *	If this is a delegate it will return the function it wraps.
 */
const asIScriptFunction* GetUnderlyingFunction(const asIScriptFunction* function);

/**
 *	@copydoc GetUnderlyingFunction(const asIScriptFunction*)
 */
asIScriptFunction* GetUnderlyingFunction(asIScriptFunction* function);

std::string FormatFunctionName(const asIScriptFunction& function);
std::string FormatTypeName(const asITypeInfo& typeInfo);

std::string_view ReturnCodeToString(int code);
std::string_view ContextStateToString(int code);

/**
 *	@brief Gets a printable string for the function's module
 */
const char* GetModuleName(const asIScriptFunction& function);

/**
*	@brief If a script is calling native code, gets the module that's calling it.
*/
asIScriptModule* GetCallingModule();

struct SectionInfo
{
	std::string SectionName;
	int Line = 0;
	int Column = 0;
};

std::string GetSectionName(const asIScriptFunction& function);
std::string GetSectionName(const asIScriptFunction* function);

SectionInfo GetExecutingSectionInfo(asIScriptContext& context, asUINT stackLevel = 0);
SectionInfo GetExceptionSectionInfo(asIScriptContext& context);

void PrintStackTrace(
	spdlog::logger& logger, spdlog::level::level_enum level, asIScriptContext& context, asUINT maxDepth = std::numeric_limits<asUINT>::max());

struct PushContextState final
{
	explicit PushContextState(spdlog::logger* logger, asIScriptContext* context)
		: m_Logger(logger), m_Context(context)
	{
		assert(m_Logger);
		assert(m_Context);

		// TODO: probably needs a more robust check
		m_IsActive = m_Context->GetState() == asEXECUTION_ACTIVE;

		if (m_IsActive)
		{
			const int result = m_Context->PushState();

			if (result < 0)
			{
				m_Logger->error("Error \"{}\" ({}) while pushing context state", ReturnCodeToString(result), result);
				m_IsValid = false;
			}
		}
	}

	~PushContextState()
	{
		if (m_IsValid && m_IsActive)
		{
			const int result = m_Context->PopState();

			if (result < 0)
			{
				m_Logger->error("Error \"{}\" ({}) while popping context state", ReturnCodeToString(result), result);
			}
		}
	}

	PushContextState(const PushContextState&) = delete;
	PushContextState& operator=(const PushContextState&) = delete;

	operator bool() const { return m_IsValid; }

private:
	spdlog::logger* const m_Logger;
	asIScriptContext* const m_Context;
	bool m_IsActive;
	bool m_IsValid = true;
};
}

template <>
struct fmt::formatter<asIScriptModule>
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
	auto format(const asIScriptModule& module, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), "{}", module.GetName());
	}
};

template <>
struct fmt::formatter<asIScriptFunction>
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
	auto format(const asIScriptFunction& function, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), "{}", as::FormatFunctionName(function));
	}
};

template <>
struct fmt::formatter<asITypeInfo>
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
	auto format(const asITypeInfo& typeInfo, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), "{}", as::FormatTypeName(typeInfo));
	}
};
