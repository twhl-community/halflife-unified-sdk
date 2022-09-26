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
#include "Events.h"

namespace scripting
{
static void RegisterMapInitEventArgs(asIScriptEngine& engine, EventSystem& eventSystem)
{
	const char* const name = "MapInitEventArgs";

	eventSystem.RegisterEvent<MapInitEventArgs>(name);
}

static void RegisterSayTextEventArgs(asIScriptEngine& engine, EventSystem& eventSystem)
{
	const char* const name = "SayTextEventArgs";

	eventSystem.RegisterEvent<SayTextEventArgs>(name);

	engine.RegisterObjectProperty(name, "const string AllText", asOFFSET(SayTextEventArgs, AllText));
	engine.RegisterObjectProperty(name, "const string Command", asOFFSET(SayTextEventArgs, Command));
	engine.RegisterObjectProperty(name, "bool Suppress", asOFFSET(SayTextEventArgs, Suppress));
}

void RegisterEventTypes(asIScriptEngine& engine, EventSystem& eventSystem)
{
	const std::string previousNamespace = engine.GetDefaultNamespace();
	engine.SetDefaultNamespace("");

	RegisterMapInitEventArgs(engine, eventSystem);
	RegisterSayTextEventArgs(engine, eventSystem);

	engine.SetDefaultNamespace(previousNamespace.c_str());
}
}
