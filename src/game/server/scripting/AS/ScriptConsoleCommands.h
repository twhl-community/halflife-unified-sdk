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

#include <memory>
#include <string>
#include <unordered_map>

#include <EASTL/fixed_vector.h>

#include <angelscript.h>

#include "cbase.h"

#include "scripting/AS/as_utils.h"
#include "scripting/AS/ScriptPluginList.h"

#include "utils/GameSystem.h"
#include "utils/heterogeneous_lookup.h"

namespace scripting
{
// TODO: can probably just wrap CommandArgs with an offset, unless there's a way to call OnCommand from somewhere else.
class ScriptCommandArgs
{
public:
	static constexpr std::size_t MaxArguments = 80;

	ScriptCommandArgs() = default;

	explicit ScriptCommandArgs(eastl::fixed_vector<std::string, MaxArguments>&& arguments) noexcept
		: m_Arguments(std::move(arguments))
	{
	}

	explicit ScriptCommandArgs(const CommandArgs& args, int firstArgument);

	~ScriptCommandArgs() = default;
	ScriptCommandArgs(const ScriptCommandArgs&) = delete;
	ScriptCommandArgs& operator=(const ScriptCommandArgs&) = delete;
	ScriptCommandArgs(ScriptCommandArgs&&) = delete;
	ScriptCommandArgs& operator=(ScriptCommandArgs&&) = delete;

	int Count() const;

	const std::string& Argument(int index) const;

private:
	eastl::fixed_vector<std::string, MaxArguments> m_Arguments;
};

class BaseScriptConsoleCommand : public as::RefCountedClass
{
protected:
	explicit BaseScriptConsoleCommand(asIScriptModule* module, std::string&& name);

public:
	BaseScriptConsoleCommand(BaseScriptConsoleCommand&&) = delete;
	BaseScriptConsoleCommand& operator=(BaseScriptConsoleCommand&&) = delete;
	BaseScriptConsoleCommand(const BaseScriptConsoleCommand&) = delete;
	BaseScriptConsoleCommand& operator=(const BaseScriptConsoleCommand&) = delete;
	virtual ~BaseScriptConsoleCommand();

	asIScriptModule* GetModule() { return m_Module; }

	const std::string& GetName() const { return m_Name; }

	virtual void OnCommand(const ScriptCommandArgs& args) = 0;

private:
	asIScriptModule* m_Module;
	const std::string m_Name;
};

class ScriptConsoleCommand final : public BaseScriptConsoleCommand
{
public:
	explicit ScriptConsoleCommand(asIScriptModule* module, std::string&& name, as::UniquePtr<asIScriptFunction>&& callback)
		: BaseScriptConsoleCommand(module, std::move(name)),
		  m_Callback(std::move(callback))
	{
	}

	void OnCommand(const ScriptCommandArgs& args) override;

private:
	as::UniquePtr<asIScriptFunction> m_Callback;
};

class ScriptConsoleVariable final : public BaseScriptConsoleCommand
{
public:
	explicit ScriptConsoleVariable(asIScriptModule* module, std::string&& name, std::string&& value,
		as::UniquePtr<asIScriptFunction>&& callback)
		: BaseScriptConsoleCommand(module, std::move(name)),
		  m_String(std::move(value)),
		  m_Value(std::atof(m_String.c_str())),
		  m_Callback(std::move(callback))
	{
	}

	void OnCommand(const ScriptCommandArgs& args) override;

	const std::string& GetString() const { return m_String; }

	float GetFloat() const { return m_Value; }

	int GetInteger() const { return static_cast<int>(m_Value); }

	bool GetBool() const { return GetInteger() != 0; }

	void SetString(const std::string& value);

private:
	std::string m_String;
	float m_Value;
	as::UniquePtr<asIScriptFunction> m_Callback;
};

class ScriptConsoleCommandSystem final : public IGameSystem, public IPluginListener
{
public:
	const char* GetName() const override { return "ScriptConCommands"; }

	bool Initialize() override;
	void PostInitialize() override {}
	void Shutdown() override {}

	void OnModuleDestroying(asIScriptModule& module) override;

	void Add(BaseScriptConsoleCommand* command);

	BaseScriptConsoleCommand* Find(std::string_view name);

	void OnCommand(const CommandArgs& args);

private:
	std::unordered_map<std::string, BaseScriptConsoleCommand*, TransparentStringHash, TransparentEqual> m_Commands;
};

inline const std::shared_ptr<ScriptConsoleCommandSystem> g_ScriptConCommands = std::make_shared<ScriptConsoleCommandSystem>();

void RegisterScriptConsoleCommands(asIScriptEngine& engine);
}
