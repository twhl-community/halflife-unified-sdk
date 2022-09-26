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

#include <string>

#include "scripting/AS/EventSystem.h"

class asIScriptEngine;

namespace scripting
{
class MapInitEventArgs : public EventArgs
{
public:
	MapInitEventArgs() = default;
};

class SayTextEventArgs : public EventArgs
{
public:
	SayTextEventArgs(std::string&& allText, std::string&& command)
		: AllText(std::move(allText)), Command(std::move(command))
	{
	}

	const std::string AllText;
	const std::string Command;

	bool Suppress = false;
};

void RegisterEventTypes(asIScriptEngine& engine, EventSystem& eventSystem);
}
