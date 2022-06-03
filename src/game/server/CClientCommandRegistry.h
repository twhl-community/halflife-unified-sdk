#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "cbase.h"
#include "command_utils.h"

class CBasePlayer;
class CClientCommandRegistry;

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

/**
*	@brief Encapsulates a client command.
*	Created using CClientCommandRegistry::Create or CClientCommandRegistry::CreateScoped.
*/
class CClientCommand final
{
public:
	CClientCommand(CClientCommandRegistry* registry,
		std::string_view name, ClientCommandFlags flags, std::function<void(CBasePlayer*, const CCommandArgs&)>&& function)
		: Registry(registry), Name(name), Flags(flags), Function(std::move(function))
	{
	}

	CClientCommandRegistry* const Registry;
	const std::string_view Name;
	const ClientCommandFlags Flags;
	const std::function<void(CBasePlayer*, const CCommandArgs&)> Function;
};

/**
*	@brief Automatically removes the client command on destruction.
*/
class CScopedClientCommand final
{
public:
	CScopedClientCommand() = default;

	explicit CScopedClientCommand(std::shared_ptr<const CClientCommand> command)
		: Command(command)
	{
	}

	CScopedClientCommand(const CScopedClientCommand&) = delete;
	CScopedClientCommand& operator=(const CScopedClientCommand&) = delete;

	CScopedClientCommand(CScopedClientCommand&& other) noexcept = default;
	CScopedClientCommand& operator=(CScopedClientCommand&& other) noexcept = default;

	~CScopedClientCommand() noexcept
	{
		Remove();
	}

	void Remove() noexcept;

private:
	std::shared_ptr<const CClientCommand> Command;
};

struct CClientCommandCreateArguments
{
	const ClientCommandFlags Flags = ClientCommandFlag::None;
};

/**
*	@brief Manages the set of client commands.
*/
class CClientCommandRegistry final
{
public:
	std::shared_ptr<const CClientCommand> Find(std::string_view name) const;

	std::shared_ptr<const CClientCommand> Create(
		std::string_view name, std::function<void(CBasePlayer*, const CCommandArgs&)>&& function, const CClientCommandCreateArguments arguments = {});

	/**
	*	@brief Creates a client command that is automatically removed from the registry when the scoped command is destroyed.
	*/
	CScopedClientCommand CreateScoped(
		std::string_view name, std::function<void(CBasePlayer*, const CCommandArgs&)>&& function, const CClientCommandCreateArguments arguments = {})
	{
		return CScopedClientCommand{Create(name, std::move(function), arguments)};
	}

	void Remove(const std::shared_ptr<const CClientCommand>& command);

private:
	std::unordered_map<std::string_view, std::shared_ptr<const CClientCommand>> m_Commands;
};

inline CClientCommandRegistry g_ClientCommands;

inline void CScopedClientCommand::Remove() noexcept
{
	if (Command)
	{
		Command->Registry->Remove(Command);
		Command.reset();
	}
}