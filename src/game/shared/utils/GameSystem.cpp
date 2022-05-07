#include <algorithm>

#include "cbase.h"
#include "GameSystem.h"

bool GameSystemRegistry::Contains(IGameSystem* system) const
{
	return std::find(m_GameSystems.begin(), m_GameSystems.end(), system) != m_GameSystems.end();
}

void GameSystemRegistry::Add(IGameSystem* system)
{
	if (!system)
	{
		return;
	}

	if (Contains(system))
	{
		return;
	}

	m_GameSystems.push_back(system);
}

void GameSystemRegistry::Remove(IGameSystem* system)
{
	m_GameSystems.erase(std::find(m_GameSystems.begin(), m_GameSystems.end(), system));
}

bool GameSystemRegistry::Initialize()
{
	for (auto system : m_GameSystems)
	{
		if (!system->Initialize())
		{
			Con_Printf("Could not initialize %s system\n", system->GetName());
			return false;
		}
	}

	return true;
}
