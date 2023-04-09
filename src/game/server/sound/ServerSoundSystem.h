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

class CBaseEntity;
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

	/**
	 *	@details Use this to set the pitch of a sound.
	 *	Pitch of 100 is no pitch shift.
	 *	Pitch > 100 up to 255 is a higher pitch, pitch < 100 down to 1 is a lower pitch.
	 *	150 to 70 is the realistic range.
	 *	EmitSound with pitch != 100 should be used sparingly,
	 *	as it's not quite as fast as with normal pitch (the pitchshift mixer is not native coded).
	 *	TODO: is this still true?
	 */
	void EmitSound(CBaseEntity* entity, int channel, const char* sample, float volume, float attenuation, int flags, int pitch);

	void EmitAmbientSound(CBaseEntity* entity, const Vector& vecOrigin, const char* samp, float vol, float attenuation, int fFlags, int pitch);

	const char* CheckForSoundReplacement(const char* soundName) const;

private:
	const char* CheckForSoundReplacement(CBaseEntity* entity, const char* soundName) const;

	void EmitSoundCore(CBaseEntity* entity, int channel, const char* sample, float volume, float attenuation,
		int flags, int pitch, const Vector& origin, bool alwaysBroadcast);

	void EmitSoundSentence(CBaseEntity* entity, int channel, const char* sample, float volume, float attenuation,
		int flags, int pitch);

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	cvar_t* m_UseOpenAl{};
};

inline ServerSoundSystem g_ServerSound;
}
