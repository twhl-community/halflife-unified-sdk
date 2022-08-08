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

#include "cbase.h"
#include "CSentencesSystem.h"
#include "sound/sentence_utils.h"

bool CSentencesSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("sentences");
	return true;
}

void CSentencesSystem::PostInitialize()
{
	LoadSentences();
}

void CSentencesSystem::Shutdown()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

void CSentencesSystem::LoadSentences()
{
	m_Logger->trace("Loading sentences.txt");

	sentences::g_SentenceNames.clear();
	sentences::g_SentenceGroups.clear();

	sentences::g_SentenceNames.reserve(sentences::CVOXFILESENTENCEMAX);

	const auto fileContents = FileSystem_LoadFileIntoBuffer("sound/sentences.txt", FileContentFormat::Text);

	if (fileContents.empty())
	{
		m_Logger->debug("No sentences to load, file does not exist or is empty");
		return;
	}

	// for each line in the file...
	sentences::SentencesListParser parser{{reinterpret_cast<const char*>(fileContents.data())}, *m_Logger};

	for (auto sentence = parser.Next(); sentence; sentence = parser.Next())
	{
		const std::string_view name = std::get<0>(*sentence);

		sentences::g_SentenceNames.push_back(sentences::SentenceName{name.data(), name.size()});

		const auto groupData = sentences::ParseGroupData(name);

		if (!groupData)
		{
			continue;
		}

		const std::string_view groupName = std::get<0>(*groupData);

		// if new name doesn't match previous group name,
		// make a new group.

		if (!sentences::g_SentenceGroups.empty() && sentences::g_SentenceGroups.back().GroupName.c_str() == groupName)
		{
			// name matches with previous, increment group count
			++sentences::g_SentenceGroups.back().count;
		}
		else
		{
			// name doesn't match with prev name,
			// copy name into group, init count to 1

			sentences::SENTENCEG group;

			group.GroupName.assign(groupName.data(), groupName.size());

			sentences::g_SentenceGroups.push_back(group);
		}
	}

	m_Logger->debug("Loaded {} sentences with {} sentence groups", sentences::g_SentenceNames.size(), sentences::g_SentenceGroups.size());

	// init lru lists
	for (auto& group : sentences::g_SentenceGroups)
	{
		USENTENCEG_InitLRU(group.rgblru, group.count);
	}
}