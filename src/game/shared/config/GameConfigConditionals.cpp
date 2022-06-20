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

#include <angelscript.h>

#include <angelscript/scriptstdstring/scriptstdstring.h>

#include "GameConfigConditionals.h"

void RegisterGameConfigConditionalsScriptAPI(asIScriptEngine& engine, GameConfigConditionals& conditionals)
{
	//So we can use strings in conditionals
	RegisterStdString(&engine);

	//Register everything as globals to keep conditional strings short and easy to read
	engine.RegisterGlobalProperty("const bool Singleplayer", &conditionals.Singleplayer);
	engine.RegisterGlobalProperty("const bool Multiplayer", &conditionals.Multiplayer);
	engine.RegisterGlobalProperty("const bool ListenServer", &conditionals.ListenServer);
	engine.RegisterGlobalProperty("const bool DedicatedServer", &conditionals.DedicatedServer);
}
