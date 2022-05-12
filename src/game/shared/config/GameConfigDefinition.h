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

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class GameConfigSection;

/**
*	@brief Immutable definition of a game configuration.
*	Contains a set of sections that configuration files can use.
*	Instances should be created with GameConfigLoader::CreateDefinition.
*	@details Each section specifies what kind of data it supports, and provides a means of parsing it.
*/
class GameConfigDefinition
{
protected:
	GameConfigDefinition(std::string&& name, std::vector<std::unique_ptr<const GameConfigSection>>&& sections);

public:
	GameConfigDefinition(const GameConfigDefinition&) = delete;
	GameConfigDefinition& operator=(const GameConfigDefinition&) = delete;

	~GameConfigDefinition();

	std::string_view GetName() const { return m_Name; }

	const GameConfigSection* FindSection(std::string_view name) const;

	/**
	*	@brief Gets the complete JSON Schema for this definition.
	*	This includes all of the sections.
	*/
	std::string GetSchema() const;

private:
	std::string m_Name;
	std::vector<std::unique_ptr<const GameConfigSection>> m_Sections;
};
