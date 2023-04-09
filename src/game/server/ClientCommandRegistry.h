#pragma once

#include <algorithm>
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
	ClientCommand(std::string_view name, ClientCommandFlags flags, std::function<void(CBasePlayer*, const CommandArgs&)>&& function)
		: Name(name),
		  Flags(flags),
		  Function(std::move(function))
	{
	}

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

	ScopedClientCommand(ClientCommandRegistry* registry, const ClientCommand* command)
		: Registry(registry),
		  Command(command)
	{
	}

	ScopedClientCommand(const ScopedClientCommand&) = delete;
	ScopedClientCommand& operator=(const ScopedClientCommand&) = delete;

	ScopedClientCommand(ScopedClientCommand&& other) noexcept;
	ScopedClientCommand& operator=(ScopedClientCommand&& other) noexcept;

	~ScopedClientCommand() noexcept
	{
		Remove();
	}

	void Remove() noexcept;

private:
	ClientCommandRegistry* Registry = nullptr;
	const ClientCommand* Command = nullptr;
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
	ClientCommandRegistry() = default;
	ClientCommandRegistry(const ClientCommandRegistry&) = delete;
	ClientCommandRegistry& operator=(const ClientCommandRegistry&) = delete;

	const ClientCommand* Find(std::string_view name) const;

	const ClientCommand* Create(
		std::string_view name, std::function<void(CBasePlayer*, const CommandArgs&)>&& function, const CClientCommandCreateArguments arguments = {});

	/**
	 *	@brief Creates a client command that is automatically removed from the registry when the scoped command is destroyed.
	 */
	ScopedClientCommand CreateScoped(
		std::string_view name, std::function<void(CBasePlayer*, const CommandArgs&)>&& function, const CClientCommandCreateArguments arguments = {})
	{
		return ScopedClientCommand{this, Create(name, std::move(function), arguments)};
	}

	void Remove(const ClientCommand* command);

private:
	std::unordered_map<std::string_view, std::unique_ptr<const ClientCommand>> m_Commands;
};

inline ClientCommandRegistry g_ClientCommands;

inline ScopedClientCommand::ScopedClientCommand(ScopedClientCommand&& other) noexcept
	: Registry(other.Registry),
	  Command(other.Command)
{
	other.Registry = nullptr;
	other.Command = nullptr;
}

inline ScopedClientCommand& ScopedClientCommand::operator=(ScopedClientCommand&& other) noexcept
{
	if (this != &other)
	{
		Remove();
		std::swap(Registry, other.Registry);
		std::swap(Command, other.Command);
	}

	return *this;
}

inline void ScopedClientCommand::Remove() noexcept
{
	if (Registry && Command)
	{
		Registry->Remove(Command);
		Command = nullptr;
		Registry = nullptr;
	}
}
