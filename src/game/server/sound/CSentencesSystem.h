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

#include <spdlog/logger.h>

#include "utils/GameSystem.h"

namespace sentences
{
class CSentencesSystem final : public IGameSystem
{
public:
	const char* GetName() const override { return "Sentences"; }

	bool Initialize() override;
	void PostInitialize() override;
	void Shutdown() override;

private:
	void LoadSentences();

private:
	std::shared_ptr<spdlog::logger> m_Logger;
};

inline CSentencesSystem g_Sentences;
}
