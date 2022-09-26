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
#include <cstdint>
#include <optional>
#include <type_traits>

#include <angelscript.h>

#include "as_utils.h"
#include "ASManager.h"

/**
 *	@file
 *
 *	Provides functions for calling script functions with parameters with automatic argument type detection.
 */

namespace scripting
{
namespace call
{
/**
 *	@brief Wrapper around an object address to mark it as an object to be passed to @c asIScriptContext::SetArgObject.
 */
struct Object
{
	const void* const Address;
};

namespace detail
{
struct ReturnObjectBase
{
};
}

/**
*	@brief Used when calling functions that return void.
*/
struct ReturnVoid
{
};

/**
 *	@brief Indicates that a return type is an object to be retrieved using @c asIScriptContext::GetReturnObject.
 */
template <typename T>
struct ReturnObject final : public detail::ReturnObjectBase
{
	using TReturn = T;
};

namespace detail
{
struct FunctionArgumentSetter
{
	asUINT Index{0};

	template <typename TArg>
	bool operator()(asIScriptContext& context, TArg&& value)
	{
		using T = std::remove_reference_t<TArg>;

		int result;

		if constexpr (std::is_enum_v<T>)
		{
			result = SetArgument(context, static_cast<std::underlying_type_t<T>>(value));
		}
		else
		{
			result = SetArgument(context, value);
		}

		if (result < 0)
		{
			g_ASManager.GetLogger()->error("Error setting function \"{}\" argument {}", *context.GetFunction(), Index);
			return false;
		}

		++Index;

		return true;
	}

private:
	template<typename TArg>
	int SetArgument(asIScriptContext& context, TArg&& value)
	{
		using T = std::remove_reference_t<TArg>;

		if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, std::uint8_t> || std::is_same_v<T, std::int8_t>)
		{
			return context.SetArgByte(Index, static_cast<asBYTE>(value));
		}
		else if constexpr (std::is_same_v<T, std::uint16_t> || std::is_same_v<T, std::int16_t>)
		{
			return context.SetArgWord(Index, static_cast<asWORD>(value));
		}
		else if constexpr (std::is_same_v<T, std::uint32_t> || std::is_same_v<T, std::int32_t>)
		{
			return context.SetArgDWord(Index, static_cast<asDWORD>(value));
		}
		else if constexpr (std::is_same_v<T, std::uint64_t> || std::is_same_v<T, std::int64_t>)
		{
			return context.SetArgQWord(Index, static_cast<asQWORD>(value));
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return context.SetArgFloat(Index, value);
		}
		else if constexpr (std::is_same_v<T, double>)
		{
			return context.SetArgDouble(Index, value);
		}
		else if constexpr (std::is_same_v<T, Object>)
		{
			return context.SetArgObject(Index, const_cast<void*>(value.Address));
		}
		else if constexpr (std::is_pointer_v<T>)
		{
			return context.SetArgAddress(Index, const_cast<std::remove_const_t<std::remove_pointer_t<T>>*>(value));
		}
		else
		{
			static_assert(always_false_v<T>, "Invalid argument type");
			return -1;
		}
	}
};
}

template <typename... Args>
bool SetFunctionArguments(asIScriptContext& context, Args&&... args)
{
	assert(context.GetFunction()->GetParamCount() == sizeof...(args));

	detail::FunctionArgumentSetter setter;

	return (setter(context, args) && ...);
}

template <typename T>
std::optional<T> GetReturnValue(asIScriptContext& context)
{
	if constexpr (std::is_same_v<T, ReturnVoid>)
	{
		return ReturnVoid{};
	}
	else if constexpr (std::is_same_v<T, bool>)
	{
		return context.GetReturnByte() != 0;
	}
	else if constexpr (std::is_same_v<T, std::uint8_t> || std::is_same_v<T, std::int8_t>)
	{
		return static_cast<T>(context.GetReturnByte());
	}
	else if constexpr (std::is_same_v<T, std::uint16_t> || std::is_same_v<T, std::int16_t>)
	{
		return static_cast<T>(context.GetReturnWord());
	}
	else if constexpr (std::is_same_v<T, std::uint32_t> || std::is_same_v<T, std::int32_t>)
	{
		return static_cast<T>(context.GetReturnDWord());
	}
	else if constexpr (std::is_same_v<T, std::uint64_t> || std::is_same_v<T, std::int64_t>)
	{
		return static_cast<T>(context.GetReturnQWord());
	}
	else if constexpr (std::is_same_v<T, float>)
	{
		return static_cast<T>(context.GetReturnFloat());
	}
	else if constexpr (std::is_same_v<T, double>)
	{
		return static_cast<T>(context.GetReturnDouble());
	}
	else if constexpr (std::is_base_of_v<detail::ReturnObjectBase, T>)
	{
		return reinterpret_cast<T::TReturn>(context.GetReturnObject());
	}
	else if constexpr (std::is_pointer_v<T>)
	{
		return reinterpret_cast<T>(context.GetReturnAddress());
	}
	else
	{
		static_assert(always_false_v<T>, "Invalid return type");
		return {};
	}
}

struct Caller
{
	Caller(asIScriptContext* context)
		: m_Context(context), m_PushState(g_ASManager.GetLogger().get(), context)
	{
		assert(m_Context);
	}

	~Caller() = default;

	Caller(const Caller&) = delete;
	Caller& operator=(const Caller&) = delete;

	template <typename TReturn, typename... Args>
	std::optional<TReturn> ExecuteFunction(asIScriptFunction* function, Args&&... args)
	{
		assert(function);

		if (!function)
		{
			return {};
		}

		return InternalExecuteFunction<TReturn>(nullptr, function, std::forward<Args>(args)...);
	}

	template <typename TReturn, typename... Args>
	std::optional<TReturn> ExecuteObjectMethod(asIScriptObject* object, asIScriptFunction* function, Args&&... args)
	{
		assert(object);
		assert(function);

		if (!object || !function)
		{
			return {};
		}

		return InternalExecuteFunction<TReturn>(object, function, std::forward<Args>(args)...);
	}

private:
	template <typename TReturn, typename... Args>
	std::optional<TReturn> InternalExecuteFunction(asIScriptObject* object, asIScriptFunction* function, Args&&... args)
	{
		if (!m_PushState)
		{
			return {};
		}

		if (!g_ASManager.PrepareContext(*m_Context, function))
		{
			return {};
		}

		if (object)
		{
			const int result = m_Context->SetObject(object);

			if (result < 0)
			{
				g_ASManager.GetLogger()->error("Error setting object for method execution: {} ({})", as::ReturnCodeToString(result), result);
				return {};
			}
		}

		if (!SetFunctionArguments(*m_Context, std::forward<Args>(args)...))
		{
			return {};
		}

		if (!g_ASManager.ExecuteContext(*m_Context))
		{
			return {};
		}

		return GetReturnValue<TReturn>(*m_Context);
	}

private:
	asIScriptContext* const m_Context;
	as::PushContextState m_PushState;
};

/**
 *	@brief Calls a global function with the given parameters, returning the value of type @c TReturn.
 *	@details To indicate that a pointer argument needs to be passed as an object, wrap the pointer with @see Object.
 *	To indicate that a return pointer needs to be retrieved as an object, wrap the type with @see ReturnObject.
 */
template <typename TReturn, typename... Args>
std::optional<TReturn> ExecuteFunction(asIScriptContext& context, asIScriptFunction* function, Args&&... args)
{
	return Caller{&context}.ExecuteFunction<TReturn>(function, std::forward<Args>(args)...);
}

/**
 *	@brief Calls an object method with the given parameters, returning the value of type @c TReturn.
 *	@details To indicate that a pointer argument needs to be passed as an object, wrap the pointer with @see Object.
 *	To indicate that a return pointer needs to be retrieved as an object, wrap the type with @see ReturnObject.
 */
template <typename TReturn, typename... Args>
std::optional<TReturn> ExecuteObjectMethod(asIScriptContext& context, asIScriptObject* object, asIScriptFunction* function, Args&&... args)
{
	return Caller{&context}.ExecuteObjectMethod<TReturn>(object, function, std::forward<Args>(args)...);
}
}
}
