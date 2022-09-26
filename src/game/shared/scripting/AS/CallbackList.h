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

#include <algorithm>
#include <cassert>

#include <angelscript.h>

#include "as_utils.h"

namespace scripting
{
struct CallbackListElement
{
	CallbackListElement(const asIScriptModule* module, as::UniquePtr<asIScriptFunction>&& function)
		: Module(module), Function(std::move(function))
	{
	}

	const asIScriptModule* const Module;
	const as::UniquePtr<asIScriptFunction> Function;
};

/**
 *	@brief Stores a list of callbacks using a user-defined element type, associated with a specific module.
 *	@tparam Container A container type containing a @c std::unique_ptr<CallbackListElement>.
 *		The pointed-to type can be a derived type or a type providing the same members.
 */
template <typename Container>
class CallbackList final : public Container
{
public:
	/**
	 *	@brief Finds a callback by function.
	 */
	auto find(asIScriptFunction* function)
	{
		assert(function);

		if (auto delegate = function->GetDelegateFunction(); delegate)
		{
			auto object = function->GetDelegateObject();

			return std::find_if(this->begin(), this->end(), [&](const auto& candidate)
				{
					auto candidateDelegate = candidate->Function->GetDelegateFunction();
					auto candidateObject = candidate->Function->GetDelegateObject();

					return candidateDelegate == delegate && candidateObject == object; });
		}
		else
		{
			return std::find_if(this->begin(), this->end(), [&](const auto& candidate)
				{ return candidate->Function.get() == function; });
		}
	}

	void RemoveCallbacksFromModule(asIScriptModule* module)
	{
		assert(module);

		for (auto it = this->begin(); it != this->end();)
		{
			if ((*it)->Module == module)
			{
				it = this->erase(it);
			}
			else
			{
				++it;
			}
		}
	}
};
}
