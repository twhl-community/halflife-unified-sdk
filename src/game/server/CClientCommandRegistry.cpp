#include "CClientCommandRegistry.h"

std::shared_ptr<const CClientCommand> CClientCommandRegistry::Find(std::string_view name) const
{
	if (auto it = m_Commands.find(name); it != m_Commands.end())
	{
		return it->second;
	}

	return {};
}

std::shared_ptr<const CClientCommand> CClientCommandRegistry::Create(
	std::string_view name, std::function<void(CBasePlayer*, const CCommandArgs&)>&& function, const CClientCommandCreateArguments arguments)
{
	if (Find(name))
	{
		ASSERT(!"Tried to register multiple commands with the same name");
		return {};
	}

	auto command = std::make_shared<CClientCommand>(this, name, arguments.Flags, std::move(function));

	m_Commands.emplace(command->Name, command);

	return command;
}

void CClientCommandRegistry::Remove(const std::shared_ptr<const CClientCommand>& command)
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
