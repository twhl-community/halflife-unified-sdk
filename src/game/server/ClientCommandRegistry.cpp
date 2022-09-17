#include "ClientCommandRegistry.h"

const ClientCommand* ClientCommandRegistry::Find(std::string_view name) const
{
	if (auto it = m_Commands.find(name); it != m_Commands.end())
	{
		return it->second.get();
	}

	return {};
}

const ClientCommand* ClientCommandRegistry::Create(
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

	auto command = std::make_unique<ClientCommand>(name, arguments.Flags, std::move(function));

	const auto result = m_Commands.emplace(command->Name, std::move(command));

	return result.first->second.get();
}

void ClientCommandRegistry::Remove(const ClientCommand* command)
{
	if (!command)
	{
		return;
	}

	// Can't rely on the command since it may be a double delete, in which case command is a dangling pointer.
	// Search by address to be safe.
	if (auto it = std::find_if(m_Commands.begin(), m_Commands.end(), [&](const auto& candidate)
			{ return candidate.second.get() == command; });
		it != m_Commands.end())
	{
		m_Commands.erase(it);
	}
	else
	{
		ASSERT(!"Tried to delete a command that has already been deleted");
	}
}
