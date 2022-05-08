#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>

#include "cbase.h"
#include "command_utils.h"

class CBasePlayer;
class CClientCommand;

/**
*	@brief Manages the set of client commands.
*/
class CClientCommandRegistry final
{
public:
	const CClientCommand* Find(std::string_view name) const;

	void Add(const CClientCommand* command);
	void Remove(const CClientCommand* command);

private:
	std::unordered_map<std::string_view, const CClientCommand*> m_Commands;
};

inline CClientCommandRegistry* GetClientCommandRegistry()
{
	static CClientCommandRegistry registry;
	return &registry;
}

using ClientCommandFlags = std::uint32_t;

namespace ClientCommandFlag
{
enum ClientCommandFlag
{
	None = 0,

	/**
	*	@brief Only allow this command to execute if cheats are enabled.
	*/
	Cheat = 1 << 0,
};
}

enum class AutoRegister
{
	No = 0,
	Yes
};

/**
*	@brief Encapsulates a client command. Unless otherwise specified, will be automatically registered.
*/
class CClientCommand
{
public:
	//Complete case
	CClientCommand(std::string_view name, std::function<void(CBasePlayer*, const CCommandArgs&)>&& function,
		ClientCommandFlags flags, AutoRegister autoRegister = AutoRegister::Yes)
		: Name(name)
		, Function(std::move(function))
		, Flags(flags)
	{
		if (autoRegister == AutoRegister::Yes)
		{
			GetClientCommandRegistry()->Add(this);
		}
	}

	//Simple case
	CClientCommand(std::string_view name, std::function<void(CBasePlayer*, const CCommandArgs&)>&& function,
		AutoRegister autoRegister = AutoRegister::Yes)
		: CClientCommand(name, std::move(function), ClientCommandFlag::None, autoRegister)
	{
	}

	~CClientCommand()
	{
		GetClientCommandRegistry()->Remove(this);
	}

	const std::string_view Name;
	const std::function<void(CBasePlayer*, const CCommandArgs&)> Function;
	const ClientCommandFlags Flags;
};
