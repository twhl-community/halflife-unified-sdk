/***
 *
 *	Copyright (c) 1999, Valve LLC. All rights reserved.
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
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

/**
 *	@brief Handles the registration and delegation of user message handlers.
 *	Allows the use of member functions.
 */
class ClientUserMessages final
{
public:
	using MessageHandler = std::function<void(const char* pszName, int iSize, void* pbuf)>;

private:
	struct Element
	{
		std::unique_ptr<char[]> Name;
		MessageHandler Handler;
	};

public:
	ClientUserMessages() = default;

	ClientUserMessages(const ClientUserMessages&) = delete;
	ClientUserMessages& operator=(const ClientUserMessages&) = delete;

	bool IsRegistered(std::string_view name) const;

	void RegisterHandler(std::string_view name, MessageHandler&& handler);

	/**
	 *	@brief Registers an object member function as a handler.
	 */
	template <typename T>
	void RegisterHandler(std::string_view name, void (T::*handler)(const char* pszName, int iSize, void* pbuf), T* instance)
	{
		assert(handler);
		assert(instance);

		RegisterHandler(name, [handler, instance](const char* pszName, int iSize, void* pbuf)
			{ return (instance->*handler)(pszName, iSize, pbuf); });
	}

	/**
	 *	@brief Registers an object member function as a handler.
	 *	The handler does not take the message name as a parameter.
	 */
	template <typename T>
	void RegisterHandler(std::string_view name, void (T::*handler)(int iSize, void* pbuf), T* instance)
	{
		assert(handler);
		assert(instance);

		RegisterHandler(name, [handler, instance](const char* pszName, int iSize, void* pbuf)
			{ return (instance->*handler)(iSize, pbuf); });
	}

	void Clear();

	bool DispatchUserMessage(const char* pszName, int iSize, void* pbuf);

private:
	std::unordered_map<std::string_view, Element> m_Handlers;
};

inline ClientUserMessages g_ClientUserMessages;
