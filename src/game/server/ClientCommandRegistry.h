#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "cbase.h"
#include "ConCommandSystem.h"

class CBasePlayer;
class ClientCommandRegistry;

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
*	Created using ClientCommandRegistry::Create or ClientCommandRegistry::CreateScoped.
*/
class ClientCommand final
{
public:
	ClientCommand(ClientCommandRegistry* registry,
		std::string_view name, ClientCommandFlags flags, std::function<void(CBasePlayer*, const CommandArgs&)>&& function)
		: Registry(registry), Name(name), Flags(flags), Function(std::move(function))
	{
	}

	ClientCommandRegistry* const Registry;
	const std::string_view Name;
	const ClientCommandFlags Flags;
	const std::function<void(CBasePlayer*, const CommandArgs&)> Function;
};

/**
*	@brief Automatically removes the client command on destruction.
*/
class ScopedClientCommand final
{
public:
	ScopedClientCommand() = default;

	explicit ScopedClientCommand(std::shared_ptr<const ClientCommand> command)
		: Command(command)
	{
	}

	ScopedClientCommand(const ScopedClientCommand&) = delete;
	ScopedClientCommand& operator=(const ScopedClientCommand&) = delete;

	ScopedClientCommand(ScopedClientCommand&& other) noexcept = default;
	ScopedClientCommand& operator=(ScopedClientCommand&& other) noexcept = default;

	~ScopedClientCommand() noexcept
	{
		Remove();
	}

	void Remove() noexcept;

private:
	std::shared_ptr<const ClientCommand> Command;
};

struct CClientCommandCreateArguments
{
	const ClientCommandFlags Flags = ClientCommandFlag::None;
};

/**
*	@brief Manages the set of client commands.
*/
class ClientCommandRegistry final
{
public:
	std::shared_ptr<const ClientCommand> Find(std::string_view name) const;

	std::shared_ptr<const ClientCommand> Create(
		std::string_view name, std::function<void(CBasePlayer*, const CommandArgs&)>&& function, const CClientCommandCreateArguments arguments = {});

	/**
	*	@brief Creates a client command that is automatically removed from the registry when the scoped command is destroyed.
	*/
	ScopedClientCommand CreateScoped(
		std::string_view name, std::function<void(CBasePlayer*, const CommandArgs&)>&& function, const CClientCommandCreateArguments arguments = {})
	{
		return ScopedClientCommand{Create(name, std::move(function), arguments)};
	}

	void Remove(const std::shared_ptr<const ClientCommand>& command);

private:
	std::unordered_map<std::string_view, std::shared_ptr<const ClientCommand>> m_Commands;
};

inline ClientCommandRegistry g_ClientCommands;

inline void ScopedClientCommand::Remove() noexcept
{
	if (Command)
	{
		Command->Registry->Remove(Command);
		Command.reset();
	}
}