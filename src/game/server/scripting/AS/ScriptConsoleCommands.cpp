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

#include "ScriptConsoleCommands.h"

#include "scripting/AS/ASGenericCall.h"
#include "scripting/AS/ASScriptingSystem.h"

namespace scripting
{
static const std::string EMPTY_STRING{};

ScriptCommandArgs::ScriptCommandArgs(const CommandArgs& args, int firstArgument)
{
	const int count = args.Count();

	m_Arguments.reserve(static_cast<std::size_t>(count - firstArgument));

	for (int i = firstArgument; i < count; ++i)
	{
		m_Arguments.push_back(args.Argument(i));
	}
}

int ScriptCommandArgs::Count() const
{
	return static_cast<int>(m_Arguments.size());
}

const std::string& ScriptCommandArgs::Argument(int index) const
{
	if (index < 0 || static_cast<std::size_t>(index) >= m_Arguments.size())
	{
		asGetActiveContext()->SetException("Index out of range");
		return EMPTY_STRING;
	}

	return m_Arguments[index];
}

BaseScriptConsoleCommand::BaseScriptConsoleCommand(asIScriptModule* module, std::string&& name)
	: m_Module(module),
	  m_Name(std::move(name))
{
	g_ScriptConCommands->Add(this);
}

BaseScriptConsoleCommand::~BaseScriptConsoleCommand()
{
}

void ScriptConsoleCommand::OnCommand(const ScriptCommandArgs& args)
{
	const as::PushContextState pusher{g_Scripting.GetLogger(), g_Scripting.GetContext()};

	if (!pusher)
	{
		return;
	}

	call::ExecuteFunction<call::ReturnVoid>(*g_Scripting.GetContext(), m_Callback.get(), call::Object{&args});
}

void ScriptConsoleVariable::OnCommand(const ScriptCommandArgs& args)
{
	if (args.Count() == 1)
	{
		Con_Printf("\"%s\" is \"%s\"\n", GetName().c_str(), m_String.c_str());
		return;
	}

	SetString(args.Argument(1));
}

void ScriptConsoleVariable::SetString(const std::string& value)
{
	if (m_String == value)
	{
		return;
	}

	m_String = value;
	m_Value = std::atof(m_String.c_str());

	Con_Printf("\"%s\" changed to \"%s\"\n", GetName().c_str(), m_String.c_str());

	if (m_Callback)
	{
		const as::PushContextState pusher{g_Scripting.GetLogger(), g_Scripting.GetContext()};

		if (!pusher)
		{
			return;
		}

		call::ExecuteFunction<call::ReturnVoid>(*g_Scripting.GetContext(), m_Callback.get(), call::Object{this});
	}
}

bool ScriptConsoleCommandSystem::Initialize()
{
	g_ConCommands.CreateCommand("script_command", [this](const auto& args)
		{ OnCommand(args); }, CommandLibraryPrefix::No);

	return true;
}

void ScriptConsoleCommandSystem::OnModuleDestroying(asIScriptModule& module)
{
	for (auto it = m_Commands.begin(); it != m_Commands.end();)
	{
		if (it->second->GetModule() == &module)
		{
			it = m_Commands.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void ScriptConsoleCommandSystem::Add(BaseScriptConsoleCommand* command)
{
	if (Find(command->GetName()))
	{
		g_Scripting.GetLogger()->error("Attempted to register console command with name \"{}\" that is already in use",
			command->GetName());
		return;
	}

	m_Commands.insert_or_assign(command->GetName(), command);
}

BaseScriptConsoleCommand* ScriptConsoleCommandSystem::Find(std::string_view name)
{
	if (auto it = m_Commands.find(name); it != m_Commands.end())
	{
		return it->second;
	}

	return nullptr;
}

void ScriptConsoleCommandSystem::OnCommand(const CommandArgs& args)
{
	const int count = args.Count();

	if (count <= 0)
	{
		return;
	}

	if (args.Count() < 2)
	{
		Con_Printf("Usage: script_command <command_name> [arguments]\n");
		return;
	}

	if (auto command = Find(args.Argument(1)); command)
	{
		command->OnCommand(ScriptCommandArgs(args, 1));
	}
}

static void RegisterCommandArgs(asIScriptEngine& engine)
{
	const char* const name = "CommandArgs";

	engine.RegisterObjectType(name, sizeof(ScriptCommandArgs), asOBJ_REF | asOBJ_NOCOUNT);

	engine.RegisterObjectMethod(name, "int Count() const",
		asMETHOD(ScriptCommandArgs, Count), asCALL_THISCALL);

	engine.RegisterObjectMethod(name, "const string& Argument(int index) const",
		asMETHOD(ScriptCommandArgs, Argument), asCALL_THISCALL);
}

static ScriptConsoleCommand* CreateConsoleCommand(std::string name, asIScriptFunction* callback)
{
	name = Trim(name);

	// TODO: more robust name validation.
	if (name.empty())
	{
		asGetActiveContext()->SetException("Command name must be valid");
		return nullptr;
	}

	if (!callback)
	{
		asGetActiveContext()->SetException("Null callback");
		return nullptr;
	}

	return new ScriptConsoleCommand(as::GetCallingModule(), std::move(name), as::UniquePtr<asIScriptFunction>{callback});
}

static void RegisterConCommand(asIScriptEngine& engine)
{
	engine.RegisterFuncdef("void ConCommandCallback(CommandArgs@ args)");

	const char* const name = "ConCommand";

	engine.RegisterObjectType(name, 0, asOBJ_REF);

	as::RegisterRefCountedClass(engine, name);

	engine.RegisterObjectBehaviour(name, asBEHAVE_FACTORY, "ConCommand@ f(string name, ConCommandCallback@ callback)",
		asFUNCTION(CreateConsoleCommand), asCALL_CDECL);
}

static ScriptConsoleVariable* CreateConsoleVariable(std::string name, std::string value, asIScriptFunction* callback)
{
	name = Trim(name);

	// TODO: more robust name validation.
	if (name.empty())
	{
		asGetActiveContext()->SetException("Command name must be valid");
		return nullptr;
	}

	return new ScriptConsoleVariable(as::GetCallingModule(), std::move(name), std::move(value),
		as::UniquePtr<asIScriptFunction>(callback));
}

static void RegisterConVar(asIScriptEngine& engine)
{
	const char* const name = "ConVar";

	engine.RegisterObjectType(name, 0, asOBJ_REF);

	engine.RegisterFuncdef("void ConVarCallback(ConVar@ var)");

	as::RegisterRefCountedClass(engine, name);

	engine.RegisterObjectBehaviour(name, asBEHAVE_FACTORY, "ConVar@ f(string name, string value, ConVarCallback@ callback = null)",
		asFUNCTION(CreateConsoleVariable), asCALL_CDECL);

	engine.RegisterObjectMethod(name, "const string& get_String() const property",
		asMETHOD(ScriptConsoleVariable, GetString), asCALL_THISCALL);
}

void RegisterScriptConsoleCommands(asIScriptEngine& engine)
{
	RegisterCommandArgs(engine);
	RegisterConCommand(engine);
	RegisterConVar(engine);
}
}
