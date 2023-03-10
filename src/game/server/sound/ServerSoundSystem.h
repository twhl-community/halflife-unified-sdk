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

#include "GameSystem.h"
#include "networking/NetworkDataSystem.h"

struct cvar_t;

namespace sound
{
class ServerSoundSystem final : public IGameSystem, public INetworkDataBlockHandler
{
public:
	const char* GetName() const override { return "ServerSound"; }

	bool Initialize() override;
	void PostInitialize() override;
	void Shutdown() override;

	void HandleNetworkDataBlock(NetworkDataBlock& block) override;

	void EmitSound(edict_t* entity, int channel, const char* sample, float volume, float attenuation, int flags, int pitch);

	void EmitAmbientSound(edict_t* entity, const Vector& vecOrigin, const char* samp, float vol, float attenuation, int fFlags, int pitch);

	const char* CheckForSoundReplacement(const char* soundName) const;

private:
	void EmitSoundCore(edict_t* entity, int channel, const char* sample, float volume, float attenuation,
		int flags, int pitch, const Vector& origin, bool alwaysBroadcast);

	void EmitSoundSentence(edict_t* entity, int channel, const char* sample, float volume, float attenuation,
		int flags, int pitch);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	cvar_t* m_UseOpenAl{};
};

inline ServerSoundSystem g_ServerSound;
}
