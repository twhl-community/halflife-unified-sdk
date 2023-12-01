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

#include <unordered_set>

#include <fmt/format.h>

#include "cbase.h"
#include "MapState.h"
#include "SentencesSystem.h"
#include "ServerLibrary.h"
#include "networking/NetworkDataSystem.h"
#include "sound/sentence_utils.h"
#include "sound/ServerSoundSystem.h"
#include "utils/heterogeneous_lookup.h"
#include "utils/JSONSystem.h"

namespace sentences
{
constexpr std::string_view SentencesSchemaName{"Sentences"sv};

static std::string GetSentencesSchema()
{
	return fmt::format(R"(
{{
	"$schema": "http://json-schema.org/draft-07/schema#",
	"title": "Sentences and Sentence Groups Definition",
	"type": "object",
	"properties": {{
		"Sentences": {{
			"description": "List of sentences to add or replace",
			"type": "array",
			"items": {{
				"title": "Sentence",
				"type": "string",
				"pattern": "^\\w\\S+\\s+.+$"
			}}
		}},
		"Groups": {{
			"description": "Sentence Groups to add or replace",
			"type": "object",
			"patternProperties": {{
				"^\\w\\S+$": {{
					"type": "array",
					"items": {{
						"title": "Sentence Name",
						"type": "string",
						"pattern": "^\\w\\S+$"
					}}
				}}
			}}
		}}
	}}
}}
)");
}


class SentencesParser final
{
public:
	explicit SentencesParser(std::shared_ptr<spdlog::logger> logger)
		: m_Logger(logger)
	{
	}

	bool ParseSentences(const json& input);

	std::vector<Sentence> m_Sentences;

	std::unordered_map<std::string, SentenceGroup, TransparentStringHash, TransparentEqual> m_Groups;

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	std::unordered_map<std::string, std::size_t, TransparentStringHash, TransparentEqual> m_SentenceToIndexMap;
};

bool SentencesParser::ParseSentences(const json& input)
{
	if (auto sentences = input.find("Sentences"); sentences != input.end())
	{
		std::unordered_set<std::string> duplicateNameCheck;

		for (const auto& sentence : *sentences)
		{
			const std::string sentenceData = sentence.get<std::string>();

			const auto parsedSentence = ParseSentence(sentenceData);

			const std::string_view name = std::get<0>(parsedSentence);

			if (name.empty())
			{
				continue;
			}

			if (0 == std::isalpha(name.front()))
			{
				m_Logger->warn("Sentence {} must start with an alphabetic character", name);
				continue;
			}

			if (m_Sentences.size() >= MaxSentencesCount)
			{
				m_Logger->error("Too many sentences in file! (maximum {})", MaxSentencesCount);
				break;
			}

			if (!duplicateNameCheck.insert(std::string{name}).second)
			{
				// Need to ignore duplicates in the same file to match original behavior.
				m_Logger->warn("Encountered duplicate sentence name \"{}\", ignoring", name);
				continue;
			}

			if (auto it = m_SentenceToIndexMap.find(name); it != m_SentenceToIndexMap.end())
			{
				auto& originalSentence = m_Sentences[it->second];
				originalSentence.Value = std::get<1>(parsedSentence);
			}
			else
			{
				m_Sentences.emplace_back(SentenceName{name.data(), name.size()}, std::string{std::get<1>(parsedSentence)});
				m_SentenceToIndexMap.insert_or_assign(std::string{name}, m_Sentences.size() - 1);
			}
		}
	}

	if (auto groups = input.find("Groups"); groups != input.end())
	{
		for (const auto& [groupName, groupContents] : groups->items())
		{
			auto it = m_Groups.find(groupName);

			if (it != m_Groups.end())
			{
				it->second.Sentences.clear();
			}
			else
			{
				it = m_Groups.insert_or_assign(groupName, SentenceGroup{SentenceName{groupName.data(), groupName.size()}}).first;
			}

			for (const auto& sentenceName : groupContents)
			{
				const std::string sentenceNameString = sentenceName.get<std::string>();
				if (auto sentenceIndex = m_SentenceToIndexMap.find(sentenceNameString);
					sentenceIndex != m_SentenceToIndexMap.end())
				{
					it->second.Sentences.push_back(static_cast<int>(sentenceIndex->second));
				}
				else
				{
					m_Logger->warn("Group \"{}\": Sentence \"{}\" not found", groupName, sentenceNameString);
				}
			}
		}
	}

	return true;
}

bool SentencesSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("sentences");
	g_JSON.RegisterSchema(SentencesSchemaName, &GetSentencesSchema);
	g_NetworkData.RegisterHandler("Sentences", this);
	return true;
}

void SentencesSystem::Shutdown()
{
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

void SentencesSystem::LoadSentences(std::span<const std::string> fileNames)
{
	m_Sentences.clear();
	m_SentenceGroups.clear();

	m_Sentences.reserve(InitialSentencesReserveCount);

	{
		SentencesParser parser{m_Logger};

		for (const auto& fileName : fileNames)
		{
			g_JSON.ParseJSONFile(fileName.c_str(), {.SchemaName = SentencesSchemaName}, [&](const auto& input)
				{ return parser.ParseSentences(input); });
		}

		m_Sentences = std::move(parser.m_Sentences);

		std::transform(parser.m_Groups.begin(), parser.m_Groups.end(), std::back_inserter(m_SentenceGroups), [](auto&& group)
			{ return std::move(group.second); });
	}

	m_Logger->debug("Loaded {} out of max {} sentences with {} sentence groups", m_Sentences.size(), MaxSentencesCount, m_SentenceGroups.size());

	// init lru lists
	for (auto& group : m_SentenceGroups)
	{
		group.LeastRecentlyUsed.resize(group.Sentences.size());
		InitLRU(group.LeastRecentlyUsed);
	}
}

void SentencesSystem::NewMapStarted()
{
	// Check if the replacement map has any bad data.
	if (m_Logger->should_log(spdlog::level::debug))
	{
		const auto isSentence = [this](const auto& name)
		{
			return std::find_if(m_Sentences.begin(), m_Sentences.end(), [&](const auto& candidate)
					   { return 0 == stricmp(candidate.Name.c_str(), name.c_str()); }) != m_Sentences.end();
		};

		const auto isGroup = [this](const auto& name)
		{
			return std::find_if(m_SentenceGroups.begin(), m_SentenceGroups.end(), [&](const auto& candidate)
					   { return 0 == strcmp(candidate.GroupName.c_str(), name.c_str()); }) != m_SentenceGroups.end();
		};

		for (const auto& [original, replacement] : g_Server.GetMapState()->m_GlobalSentenceReplacement->GetAll())
		{
			if (!isSentence(original) && !isGroup(original))
			{
				m_Logger->debug("Global sentence replacement key \"{} => {}\" refers to nonexistent sentence or group", original, replacement);
			}

			if (!isSentence(replacement) && !isGroup(replacement))
			{
				m_Logger->debug("Global sentence replacement value in \"{} => {}\" refers to nonexistent sentence or group", original, replacement);
			}
		}
	}
}

void SentencesSystem::HandleNetworkDataBlock(NetworkDataBlock& block)
{
	block.Data = json::array();

	for (const auto& sentence : m_Sentences)
	{
		block.Data.push_back(fmt::format("{} {}", sentence.Name.c_str(), sentence.Value));
	}

	g_NetworkData.GetLogger()->debug("Wrote {} sentences to network data", m_Sentences.size());
}

const char* SentencesSystem::GetSentenceNameByIndex(int index) const
{
	if (index < 0 || static_cast<std::size_t>(index) >= m_Sentences.size())
	{
		ASSERT(!"Invalid index passed to SentencesSystem::GetSentenceNameByIndex");
		return nullptr;
	}

	return m_Sentences[index].Name.c_str();
}

int SentencesSystem::GetGroupIndex(CBaseEntity* entity, const char* szgroupname) const
{
	if (!szgroupname)
		return -1;

	// See if the group was replaced.
	szgroupname = CheckForSentenceReplacement(entity, szgroupname);

	// search rgsentenceg for match on szgroupname
	int i = 0;

	for (const auto& group : m_SentenceGroups)
	{
		if (szgroupname == group.GroupName)
			return i;
		++i;
	}

	return -1;
}

int SentencesSystem::LookupSentence(CBaseEntity* entity, const char* sample, SentenceIndexName* sentencenum) const
{
	// this is a sentence name; lookup sentence number
	// and give to engine as string.

	// Skip ! prefix
	++sample;

	// Handle sentence replacement.
	sample = CheckForSentenceReplacement(entity, sample);

	for (size_t i = 0; i < m_Sentences.size(); i++)
	{
		if (!stricmp(m_Sentences[i].Name.c_str(), sample))
		{
			if (sentencenum)
			{
				fmt::format_to(std::back_inserter(*sentencenum), "!{}", i);
			}
			return static_cast<int>(i);
		}
	}

	// sentence name not found!
	return -1;
}

int SentencesSystem::PlayRndSz(CBaseEntity* entity, const char* szgroupname,
	float volume, float attenuation, int flags, int pitch)
{
	const int isentenceg = GetGroupIndex(entity, szgroupname);
	if (isentenceg < 0)
	{
		m_Logger->debug("No such sentence group {}", szgroupname);
		return -1;
	}

	SentenceIndexName name;

	const int ipick = Pick(isentenceg, name);
	if (ipick >= 0 && !name.empty())
		sound::g_ServerSound.EmitSound(entity, CHAN_VOICE, name.c_str(), volume, attenuation, flags, pitch);

	return ipick;
}

int SentencesSystem::PlaySequentialSz(CBaseEntity* entity, const char* szgroupname,
	float volume, float attenuation, int flags, int pitch, int ipick, bool freset)
{
	const int isentenceg = GetGroupIndex(entity, szgroupname);
	if (isentenceg < 0)
		return -1;

	SentenceIndexName name;

	const int ipicknext = PickSequential(isentenceg, name, ipick, freset);
	if (ipicknext >= 0 && !name.empty())
		sound::g_ServerSound.EmitSound(entity, CHAN_VOICE, name.c_str(), volume, attenuation, flags, pitch);
	return ipicknext;
}

void SentencesSystem::Stop(CBaseEntity* entity, int isentenceg, int ipick)
{
	if (isentenceg < 0 || ipick < 0)
		return;

	const int sentenceIndex = m_SentenceGroups[isentenceg].Sentences[ipick];

	SentenceIndexName name;
	fmt::format_to(std::back_inserter(name), "!{}", m_Sentences[sentenceIndex].Name.c_str());

	entity->StopSound(CHAN_VOICE, name.c_str());
}

void SentencesSystem::InitLRU(eastl::fixed_vector<unsigned char, CSENTENCE_LRU_MAX>& lru)
{
	for (std::size_t i = 0; i < lru.size(); ++i)
		lru[i] = (unsigned char)i;

	const int count = static_cast<int>(lru.size());

	// randomize array
	for (int i = 0; i < (count * 4); i++)
	{
		const int j = RANDOM_LONG(0, count - 1);
		const int k = RANDOM_LONG(0, count - 1);
		std::swap(lru[j], lru[k]);
	}
}

int SentencesSystem::Pick(int isentenceg, SentenceIndexName& found)
{
	if (isentenceg < 0)
		return -1;

	auto& group = m_SentenceGroups[isentenceg];

	// Make 2 attempts to find a sentence: if we don't find it the first time, reset the LRU array and try again.
	for (int iteration = 0; iteration < 2; ++iteration)
	{
		for (unsigned char& index : group.LeastRecentlyUsed)
		{
			if (index != 0xFF)
			{
				const int ipick = index;
				index = 0xFF;

				const int sentenceIndex = group.Sentences[ipick];

				found.clear();
				fmt::format_to(std::back_inserter(found), "!{}", m_Sentences[sentenceIndex].Name.c_str());
				return ipick;
			}
		}

		InitLRU(group.LeastRecentlyUsed);
	}

	return -1;
}

int SentencesSystem::PickSequential(int isentenceg, SentenceIndexName& found, int ipick, bool freset) const
{
	if (isentenceg < 0)
		return -1;

	const auto& group = m_SentenceGroups[isentenceg];

	if (group.Sentences.empty())
		return -1;

	ipick = std::min(ipick, static_cast<int>(group.Sentences.size() - 1));

	const int sentenceIndex = group.Sentences[ipick];

	found.clear();
	fmt::format_to(std::back_inserter(found), "!{}", m_Sentences[sentenceIndex].Name.c_str());

	// Note: this check is never true because ipick is clamped above.
	if (ipick >= static_cast<int>(group.Sentences.size()))
	{
		if (freset)
			// reset at end of list
			return 0;
		else
			return static_cast<int>(group.Sentences.size());
	}

	return ipick + 1;
}

const char* SentencesSystem::CheckForSentenceReplacement(CBaseEntity* entity, const char* sentenceName) const
{
	if (entity->m_SentenceReplacement)
	{
		sentenceName = entity->m_SentenceReplacement->Lookup(sentenceName);
	}

	return g_Server.GetMapState()->m_GlobalSentenceReplacement->Lookup(sentenceName);
}
}
