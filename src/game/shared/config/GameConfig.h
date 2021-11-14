/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#pragma once

#include <any>
#include <memory>
#include <type_traits>
#include <vector>

/**
*	@brief Contains configuration data that can be applied to an object.
*	@details A section may create data if it needs to apply changes later on.
*/
class GameConfigData
{
protected:
	GameConfigData() = default;

public:
	GameConfigData(const GameConfigData&) = delete;
	GameConfigData& operator=(const GameConfigData&) = delete;
	virtual ~GameConfigData() = default;

	/**
	*	@brief Apply data to the current game instance.
	*/
	virtual void Apply(const std::any& userData) const = 0;
};

/**
*	@brief Stores game configuration data.
*/
class GameConfig final
{
public:
	/**
	*	@brief Gets a data object of type T.
	*/
	template<typename T, typename = std::enable_if_t<std::is_base_of_v<GameConfigData, T>>>
	T* Get()
	{
		for (const auto& object : m_Data)
		{
			if (auto candidate = dynamic_cast<T*>(object.get()); candidate)
			{
				return candidate;
			}
		}

		return nullptr;
	}

	/**
	*	@brief Adds a new data object to the configuration.
	*/
	template<typename T, typename = std::enable_if_t<std::is_base_of_v<GameConfigData, T>>>
	T* Add(std::unique_ptr<T>&& data)
	{
		if (!data)
		{
			return nullptr;
		}

		m_Data.emplace_back(std::move(data));

		return static_cast<T*>(m_Data.back().get());
	}

	/**
	*	@brief Creates a data object of type T.
	*	The given arguments will be forwarded to its constructor.
	*/
	template<typename T, typename... Args, typename = std::enable_if_t<std::is_base_of_v<GameConfigData, T>>>
	T* Create(Args&&... args)
	{
		return Add(std::make_unique<T>(std::forward<Args>(args)...));
	}

	/**
	*	@brief Gets or creates a data object of type T.
	*	If an object has to be created the given arguments will be forwarded to its constructor.
	*/
	template<typename T, typename... Args, typename = std::enable_if_t<std::is_base_of_v<GameConfigData, T>>>
	T* GetOrCreate(Args&&... args)
	{
		if (auto candidate = Get<T>(); candidate)
		{
			return candidate;
		}

		return Create<T>(std::forward<Args>(args)...);
	}

	/**
	*	@brief Applies data to current game instance.
	*	@param userData An object that configuration data may be applied to.
	*/
	void Apply(const std::any& userData) const
	{
		for (const auto& data : m_Data)
		{
			data->Apply(userData);
		}
	}

private:
	std::vector<std::unique_ptr<GameConfigData>> m_Data;
};
