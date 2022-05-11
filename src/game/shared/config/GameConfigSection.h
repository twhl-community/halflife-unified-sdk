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

#include <string_view>
#include <tuple>

struct GameConfigContext;

/**
*	@brief Defines a section of a game configuration, and the logic to parse it.
*/
class GameConfigSection
{
protected:
	GameConfigSection() = default;

public:
	GameConfigSection(const GameConfigSection&) = delete;
	GameConfigSection& operator=(const GameConfigSection&) = delete;
	virtual ~GameConfigSection() = default;

	/**
	*	@brief Gets the name of this section
	*/
	virtual std::string_view GetName() const = 0;

	/**
	*	@brief Gets the partial schema for this section.
	*	@details The schema is the part of an object's "properties" definition.
	*	Define properties used by this section in it.
	*	@return A tuple containing the object definition
	*		and the contents of the \c required array applicable to the keys specified in the first element.
	*/
	virtual std::tuple<std::string, std::string> GetSchema() const = 0;

	/**
	*	@brief Tries to parse the given configuration.
	*/
	virtual bool TryParse(GameConfigContext& context) const = 0;
};
