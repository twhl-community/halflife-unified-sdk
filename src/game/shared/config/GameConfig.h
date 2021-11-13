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
	void Add(std::unique_ptr<GameConfigData>&& data)
	{
		if (!data)
		{
			return;
		}

		m_Data.emplace_back(std::move(data));
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
