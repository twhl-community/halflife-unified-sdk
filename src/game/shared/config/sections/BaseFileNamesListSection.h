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

#include "cbase.h"

#include "config/GameConfig.h"

#include "utils/JSONSystem.h"

/**
 *	@brief Base class for sections that specify a list of filenames.
 *	Derived classes must provide the GetListName and GetFileNamesList functions.
 */
template <typename DataContext>
class BaseFileNamesListSection : public GameConfigSection<DataContext>
{
public:
	json::value_t GetType() const override final { return json::value_t::object; }

	std::string GetSchema() const override
	{
		return fmt::format(R"(
"properties": {{
	"FileNames": {{
		"type": "array",
		"items": {{
			"type": "string"
		}}
	}}
}})");
	}

	bool TryParse(GameConfigContext<DataContext>& context) const override final
	{
		auto& fileNamesList = GetFileNamesList(context.Data);

		if (const auto resetList = context.Input.find("ResetList");
			resetList != context.Input.end() && resetList->is_boolean() && resetList->get<bool>())
		{
			context.Logger.debug("Resetting {} filename list", GetListName());
			fileNamesList.clear();
		}

		const auto fileNames = context.Input.find("FileNames");

		if (fileNames == context.Input.end() || !fileNames->is_array())
		{
			return false;
		}

		const auto begin = fileNames->begin();

		for (auto it = fileNames->begin(), end = fileNames->end(); it != end; ++it)
		{
			const auto& fileName = *it;

			if (!fileName.is_string())
			{
				continue;
			}

			auto fileNameString = fileName.get<std::string>();

			const auto trimmedString = Trim(fileNameString);

			if (trimmedString.empty())
			{
				context.Logger.warn("Ignoring empty {} filename (index {})", GetListName(), it - begin);
				continue;
			}

			context.Logger.debug("Adding {} file \"{}\"", GetListName(), trimmedString);

			fileNamesList.push_back(std::string{trimmedString});
		}

		return true;
	}

protected:
	virtual std::string_view GetListName() const = 0;

	virtual std::vector<std::string>& GetFileNamesList(DataContext& context) const = 0;
};
