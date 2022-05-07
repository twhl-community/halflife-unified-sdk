#pragma once

#include <type_traits>
#include <vector>

/**
*	@brief Represents a system that hooks into game events like initialization, shutdown, etc.
*/
struct IGameSystem
{
	virtual ~IGameSystem() {}

	/**
	*	@brief Gets a short, descriptive name of this system
	*/
	virtual const char* GetName() const = 0;

	/**
	*	@brief Called once on startup to initialize the system.
	*	@return @c true if initialization succeeded, @c false to abort.
	*/
	virtual bool Initialize() = 0;

	/**
	*	@brief Called when all systems have initialized.
	*/
	virtual void PostInitialize() = 0;

	/**
	*	@brief Called on shutdown, even if a system aborted initialization.
	*/
	virtual void Shutdown() = 0;
};

struct GameSystemRegistry final
{
	bool Contains(IGameSystem* system) const;

	void Add(IGameSystem* system);
	void Remove(IGameSystem* system);
	void RemoveAll();

	/**
	*	@brief Invokes @c func on each registered system in the order that the systems were registered.
	*/
	template <typename Func, typename... Args>
	void Invoke(Func func, Args&&... args);

	/**
	*	@brief Invokes @c func on each registered system in the reverse order that the systems were registered.
	*/
	template <typename Func, typename... Args>
	void InvokeReverse(Func func, Args&&... args);

	/**
	*	@brief Initializes all registerd systems.
	*	@return @c true if all systems initialized, @c false otherwise.
	*/
	bool Initialize();

private:
	std::vector<IGameSystem*> m_GameSystems;
};

inline GameSystemRegistry g_GameSystems;

inline void GameSystemRegistry::RemoveAll()
{
	m_GameSystems.clear();
}

template <typename Func, typename... Args>
void GameSystemRegistry::Invoke(Func func, Args&&... args)
{
	for (auto system : m_GameSystems)
	{
		(system->*func)(std::forward<Args>(args)...);
	}
}

template <typename Func, typename... Args>
void GameSystemRegistry::InvokeReverse(Func func, Args&&... args)
{
	for (auto it = m_GameSystems.rbegin(), end = m_GameSystems.rend(); it != end; ++it)
	{
		auto system = *it;
		(system->*func)(std::forward<Args>(args)...);
	}
}
