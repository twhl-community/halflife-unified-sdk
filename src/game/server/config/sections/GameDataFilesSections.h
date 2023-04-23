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

#include "ServerConfigContext.h"

#include "config/GameConfig.h"
#include "config/sections/BaseFileNamesListSection.h"

/**
 *	@brief Allows a configuration file to specify one or more sentences files.
 */
class SentencesSection final : public BaseFileNamesListSection<ServerConfigContext>
{
public:
	explicit SentencesSection() = default;

	std::string_view GetName() const override final { return "Sentences"; }

protected:
	std::string_view GetListName() const override final { return "sentences"; }

	std::vector<std::string>& GetFileNamesList(ServerConfigContext& context) const override final
	{
		return context.SentencesFiles;
	}
};

/**
 *	@brief Allows a configuration file to specify one or more materials files.
 */
class MaterialsSection final : public BaseFileNamesListSection<ServerConfigContext>
{
public:
	explicit MaterialsSection() = default;

	std::string_view GetName() const override final { return "Materials"; }

protected:
	std::string_view GetListName() const override final { return "materials"; }

	std::vector<std::string>& GetFileNamesList(ServerConfigContext& context) const override final
	{
		return context.MaterialsFiles;
	}
};

/**
 *	@brief Allows a configuration file to specify one or more skill files.
 */
class SkillSection final : public BaseFileNamesListSection<ServerConfigContext>
{
public:
	explicit SkillSection() = default;

	std::string_view GetName() const override final { return "Skill"; }

protected:
	std::string_view GetListName() const override final { return "skill"; }

	std::vector<std::string>& GetFileNamesList(ServerConfigContext& context) const override final
	{
		return context.SkillFiles;
	}
};
