#include "ClientCommandRegistry.h"

std::shared_ptr<const ClientCommand> ClientCommandRegistry::Find(std::string_view name) const
{
	if (auto it = m_Commands.find(name); it != m_Commands.end())
	{
		return it->second;
	}

	return {};
}

std::shared_ptr<const ClientCommand> ClientCommandRegistry::Create(
	std::string_view name, std::function<void(CBasePlayer*, const CommandArgs&)>&& function, const CClientCommandCreateArguments arguments)
{
	if (Find(name))
	{
		ASSERT(!"Tried to register multiple commands with the same name");
		return {};
	}

	if (std::find_if(name.begin(), name.end(), [](int c)
			{ return 0 == std::islower(static_cast<unsigned char>(c)) && c != '_'; }) != name.end())
	{
		ASSERT(!"Client command name has invalid characters");
		return {};
	}

	auto command = std::make_shared<ClientCommand>(this, name, arguments.Flags, std::move(function));

	m_Commands.emplace(command->Name, command);

	return command;
}

void ClientCommandRegistry::Remove(const std::shared_ptr<const ClientCommand>& command)
{
	if (!command)
	{
		return;
	}

	if (auto it = m_Commands.find(command->Name); it != m_Commands.end())
	{
		m_Commands.erase(it);
	}
}
