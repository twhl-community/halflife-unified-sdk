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

#include <algorithm>

#include "hud.h"
#include "CampaignSelectSystem.h"

constexpr std::string_view CampaignSchemaName{"Campaign"sv};

static std::string GetCampaignSelectSchema()
{
	return R"(
{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Campaign Definition",
	"type": "object",
	"properties": {
		"Label": {
			"type": "string",
			"pattern": "^.+$"
		},
		"Description": {
			"type": "string"
		},
		"CampaignMap": {
			"type": "string",
			"pattern": "^.+$"
		},
		"TrainingMap": {
			"type": "string",
			"pattern": "^.+$"
		}
	},
	"required": [
		"Label"
	]
})";
}

bool CampaignSelectSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("ui.campaign");
	g_JSON.RegisterSchema(CampaignSchemaName, &GetCampaignSelectSchema);
	return true;
}

void CampaignSelectSystem::Shutdown()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

std::vector<CampaignInfo> CampaignSelectSystem::LoadCampaigns()
{
	std::vector<CampaignInfo> campaigns;

	FileFindHandle_t handle = FILESYSTEM_INVALID_FIND_HANDLE;

	if (auto fileName = g_pFileSystem->FindFirst("campaigns/*.json", &handle); fileName)
	{
		do
		{
			if (std::find_if(campaigns.begin(), campaigns.end(), [=](const auto& candidate)
				{ return candidate.FileName == fileName;
				}) != campaigns.end())
			{
				// The same file will be returned for every search path that includes the same directory so
				// we have to ignore them.
				continue;
			}

			auto campaign = g_JSON.ParseJSONFile(fmt::format("campaigns/{}", fileName).c_str(),
				{.SchemaName = CampaignSchemaName}, [=, this](const auto& input)
				{ return ParseCampaign(fileName, input); });

			if (campaign)
			{
				if (campaign->Label.empty())
				{
					m_Logger->error("Ignoring campaign file \"{}\": no label provided");
					continue;
				}

				if (campaign->CampaignMap.empty() && campaign->TrainingMap.empty())
				{
					m_Logger->error("Ignoring campaign file \"{}\": no maps provided", fileName);
					continue;
				}

				m_Logger->debug("Adding campaign \"{}\" from \"{}\"", campaign->Label, fileName);
				campaigns.push_back(std::move(*campaign));
			}

		} while ((fileName = g_pFileSystem->FindNext(handle)) != nullptr);

		g_pFileSystem->FindClose(handle);
	}

	std::sort(campaigns.begin(), campaigns.end(), [](const auto& lhs, const auto& rhs)
		{ return lhs.FileName < rhs.FileName; });

	return campaigns;
}

CampaignInfo CampaignSelectSystem::ParseCampaign(std::string&& fileName, const json& input)
{
	CampaignInfo info;

	info.FileName = std::move(fileName);
	info.Label = Trim(input.value("Label", ""));
	info.Description = input.value("Description", "No description provided.");
	info.CampaignMap = Trim(input.value("CampaignMap", ""));
	info.TrainingMap = Trim(input.value("TrainingMap", ""));

	return info;
}
