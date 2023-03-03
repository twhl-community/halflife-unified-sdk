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
 *	@brief Allows a configuration file to specify one or more global model replacement files.
 */
class GlobalModelReplacementSection final : public BaseFileNamesListSection<ServerConfigContext>
{
public:
	explicit GlobalModelReplacementSection() = default;

	std::string_view GetName() const override final { return "GlobalModelReplacement"; }

protected:
	std::string_view GetListName() const override final { return "global model replacement"; }

	std::vector<std::string>& GetFileNamesList(ServerConfigContext& context) const override final
	{
		return context.GlobalModelReplacementFiles;
	}
};
