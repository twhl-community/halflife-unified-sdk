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

#include <new>

#include "cbase.h"
#include "CBaseEntityAPI.h"
#include "MathAPIs.h"

namespace scripting
{
static void DefaultConstructVector(void* memory)
{
	new (memory) Vector();
}

static void CopyConstructVector(void* memory, const Vector& other)
{
	new (memory) Vector(other);
}

static void FullConstructVector(void* memory, float x, float y, float z)
{
	new (memory) Vector(x, y, z);
}

static void DestructVector(void* memory)
{
	reinterpret_cast<Vector*>(memory)->~Vector();
}

static void RegisterVector(asIScriptEngine& engine)
{
	const char* const name = "Vector";

	engine.RegisterObjectType(name, sizeof(Vector), asOBJ_VALUE | asGetTypeTraits<Vector>());

	engine.RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, "void f()",
		asFUNCTION(DefaultConstructVector), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, "void f(const Vector& in other)",
		asFUNCTION(CopyConstructVector), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectBehaviour(name, asBEHAVE_CONSTRUCT, "void f(float x, float y, float z)",
		asFUNCTION(FullConstructVector), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectBehaviour(name, asBEHAVE_DESTRUCT, "void f()",
		asFUNCTION(DestructVector), asCALL_CDECL_OBJFIRST);

	engine.RegisterObjectProperty(name, "float x", asOFFSET(Vector, x));
	engine.RegisterObjectProperty(name, "float y", asOFFSET(Vector, y));
	engine.RegisterObjectProperty(name, "float z", asOFFSET(Vector, z));

	engine.RegisterGlobalProperty("const Vector vec3_origin", const_cast<Vector*>(&vec3_origin));
}

void RegisterMathAPI(asIScriptEngine& engine)
{
	RegisterVector(engine);
}
}
