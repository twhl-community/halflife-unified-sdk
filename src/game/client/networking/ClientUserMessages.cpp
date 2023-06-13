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

#include <algorithm>

#include "hud.h"
#include "ClientLibrary.h"
#include "ClientUserMessages.h"

static int DispatchUserMessageInternal(const char* pszName, int iSize, void* pbuf)
{
	return static_cast<int>(g_ClientUserMessages.DispatchUserMessage(pszName, iSize, pbuf));
}

bool ClientUserMessages::IsRegistered(std::string_view name) const
{
	return m_Handlers.contains(name);
}

void ClientUserMessages::RegisterHandler(std::string_view name, MessageHandler&& handler)
{
	if (name.empty() || std::find_if_not(name.begin(), name.end(), [](auto c)
							{ return std::isalpha(c); }) != name.end())
	{
		assert(!"User message handler names must contain only alphabetic characters");
		return;
	}

	if (!handler)
	{
		assert(!"Handler must be valid");
		return;
	}

	if (IsRegistered(name))
	{
		assert(!"Cannot register multiple handlers to the same user message");
		return;
	}

	// Ensure key address remains the same.
	Element element{.Name = std::make_unique<char[]>(name.size() + 1), .Handler = std::move(handler)};

	std::strncpy(element.Name.get(), name.data(), name.size());
	element.Name[name.size()] = '\0';

	const std::string_view key{element.Name.get(), name.size()};

	m_Handlers.emplace(key, std::move(element));

	gEngfuncs.pfnHookUserMsg(key.data(), &DispatchUserMessageInternal);
}

void ClientUserMessages::Clear()
{
	m_Handlers.clear();
}

bool ClientUserMessages::DispatchUserMessage(const char* pszName, int iSize, void* pbuf)
{
	g_Client.OnUserMessageReceived();

	if (auto it = m_Handlers.find(pszName); it != m_Handlers.end())
	{
		BufferReader reader{{reinterpret_cast<std::byte*>(pbuf), static_cast<std::size_t>(iSize)}};
		it->second.Handler(pszName, reader);
		return true;
	}

	// Don't spawn the console with error messages; this is a developer error.
	g_GameLogger->debug("No user message handler for message \"{}\"", pszName);

	assert(!"No user message handler for message");

	return false;
}
