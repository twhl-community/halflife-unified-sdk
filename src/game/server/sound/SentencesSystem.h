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
#include <span>
#include <string>
#include <vector>

#include <spdlog/logger.h>

#include "networking/NetworkDataSystem.h"
#include "sound/sentence_utils.h"
#include "utils/json_fwd.h"
#include "utils/GameSystem.h"

class CBaseEntity;

namespace sentences
{
struct Sentence
{
	SentenceName Name;
	std::string Value;
};

class SentencesSystem final : public IGameSystem, public INetworkDataBlockHandler
{
public:
	const char* GetName() const override { return "Sentences"; }

	bool Initialize() override;
	void PostInitialize() override {}
	void Shutdown() override;

	void LoadSentences(std::span<const std::string> fileNames);

	void NewMapStarted();

	void HandleNetworkDataBlock(NetworkDataBlock& block) override;

	const char* GetSentenceNameByIndex(int index) const;

	/**
	 *	@brief Given sentence group rootname (name without number suffix), get sentence group index (isentenceg).
	 *	@return -1 if no such name.
	 */
	int GetGroupIndex(CBaseEntity* entity, const char* szrootname) const;

	/**
	 *	@brief convert sentence (sample) name to !sentencenum.
	 *	@return Sentence index, or -1 if the sentence could not be found.
	 */
	int LookupSentence(CBaseEntity* entity, const char* sample, SentenceIndexName* sentencenum) const;

	/**
	 *	@brief given sentence group index, play random sentence for given entity.
	 *	@return ipick - which sentence was picked to play from the group.
	 *		Ipick is only needed if you plan on stopping the sound before playback is done (@see Stop).
	 */
	int PlayRndI(CBaseEntity* entity, int isentenceg, float volume, float attenuation, int flags, int pitch);

	/**
	 *	@brief same as PlayRndI, but takes sentence group name instead of index.
	 */
	int PlayRndSz(CBaseEntity* entity, const char* szrootname, float volume, float attenuation, int flags, int pitch);

	/**
	 *	@brief play sentences in sequential order from sentence group. Reset after last sentence.
	 */
	int PlaySequentialSz(CBaseEntity* entity, const char* szrootname, float volume, float attenuation, int flags, int pitch, int ipick, bool freset);

	/**
	 *	@brief for this entity, for the given sentence within the sentence group, stop the sentence.
	 */
	void Stop(CBaseEntity* entity, int isentenceg, int ipick);

private:
	void LoadSentences(const std::string& fileName);

	/**
	 *	@brief randomize list of sentence name indices
	 */
	void InitLRU(unsigned char* plru, int count) const;

	/**
	 *	@brief pick a random sentence from rootname0 to rootnameX.
	 *		picks from the rgsentenceg[isentenceg] least recently used, modifies lru array.
	 *	@details lru must be seeded with 0-n randomized sentence numbers, with the rest of the lru filled with -1.
	 *		The first integer in the lru is actually the size of the list.
	 *	@param[out] found the sentence name.
	 *	@return ipick, the ordinal of the picked sentence within the group.
	 */
	int Pick(int isentenceg, SentenceIndexName& found);

	/**
	 *	@brief ignore lru. pick next sentence from sentence group.
	 *		Go in order until we hit the last sentence, then repeat list if freset is true.
	 *		If freset is false, then repeat last sentence.
	 *	@param ipick requested sentence ordinal
	 *	@return Next value for @p ipick, or -1 if there was an error.
	 */
	int PickSequential(int isentenceg, SentenceIndexName& found, int ipick, bool freset) const;

	const char* CheckForSentenceReplacement(CBaseEntity* entity, const char* sentenceName) const;

private:
	std::shared_ptr<spdlog::logger> m_Logger;

	std::vector<Sentence> m_Sentences;
	std::vector<SENTENCEG> m_SentenceGroups;
};

inline SentencesSystem g_Sentences;
}
