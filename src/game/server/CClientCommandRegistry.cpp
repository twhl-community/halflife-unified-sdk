#include "CClientCommandRegistry.h"

const CClientCommand* CClientCommandRegistry::Find(std::string_view name) const
{
	if (auto it = m_Commands.find(name); it != m_Commands.end())
	{
		return it->second;
	}

	return nullptr;
}

void CClientCommandRegistry::Add(const CClientCommand* command)
{
	if (!command)
	{
		return;
	}

	if (Find(command->Name))
	{
		ASSERT(!"Tried to register multiple commands with the same name");
		return;
	}

	m_Commands.emplace(command->Name, command);
}

void CClientCommandRegistry::Remove(const CClientCommand* command)
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
