/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#include <cassert>

#include <AL/al.h>
#define AL_ALEXT_PROTOTYPES
#include <AL/alext.h>

#include <AL/efx-presets.h>

#include "cbase.h"
#include "HalfLifeSoundEffects.h"

namespace sound
{
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_GENERIC = EFX_REVERB_PRESET_GENERIC;
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_ROOM = EFX_REVERB_PRESET_ROOM;
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_BATHROOM = EFX_REVERB_PRESET_BATHROOM;
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_STONEROOM = EFX_REVERB_PRESET_STONEROOM;
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_CONCERTHALL = EFX_REVERB_PRESET_CONCERTHALL;
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_ARENA = EFX_REVERB_PRESET_ARENA;
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_STONECORRIDOR = EFX_REVERB_PRESET_STONECORRIDOR;
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_SEWERPIPE = EFX_REVERB_PRESET_SEWERPIPE;
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_UNDERWATER = EFX_REVERB_PRESET_UNDERWATER;
constexpr EFXEAXREVERBPROPERTIES EAXREVERB_PRESET_DIZZY = EFX_REVERB_PRESET_DIZZY;

struct RoomEffect
{
	const EFXEAXREVERBPROPERTIES* Properties;
	float Volume;
	float DecayTime;
	float Damping;
};

constexpr RoomEffect RoomEffects[RoomEffectCount] =
	{
		// Note: the decay time and damping for generic are 0 in the engine. The legal minimum is 0.1.
		{&EAXREVERB_PRESET_GENERIC, 0.0, 0.1, 0.1},
		{&EAXREVERB_PRESET_ROOM, 0.417, 0.40000001, 0.66600001},
		{&EAXREVERB_PRESET_BATHROOM, 0.30000001, 1.499, 0.16599999},
		{&EAXREVERB_PRESET_BATHROOM, 0.40000001, 1.499, 0.16599999},
		{&EAXREVERB_PRESET_BATHROOM, 0.60000002, 1.499, 0.16599999},
		{&EAXREVERB_PRESET_SEWERPIPE, 0.40000001, 2.8859999, 0.25},
		{&EAXREVERB_PRESET_SEWERPIPE, 0.60000002, 2.8859999, 0.25},
		{&EAXREVERB_PRESET_SEWERPIPE, 0.80000001, 2.8859999, 0.25},
		{&EAXREVERB_PRESET_STONEROOM, 0.5, 2.309, 0.88800001},
		{&EAXREVERB_PRESET_STONEROOM, 0.64999998, 2.309, 0.88800001},
		{&EAXREVERB_PRESET_STONEROOM, 0.80000001, 2.309, 0.88800001},
		{&EAXREVERB_PRESET_STONECORRIDOR, 0.30000001, 2.697, 0.63800001},
		{&EAXREVERB_PRESET_STONECORRIDOR, 0.5, 2.697, 0.63800001},
		{&EAXREVERB_PRESET_STONECORRIDOR, 0.64999998, 2.697, 0.63800001},
		// Note: the damping for underwater types is 0 in the engine.
		{&EAXREVERB_PRESET_UNDERWATER, 1.0, 1.499, 0.1},
		{&EAXREVERB_PRESET_UNDERWATER, 1.0, 2.4990001, 0.1},
		{&EAXREVERB_PRESET_UNDERWATER, 1.0, 3.4990001, 0.1},
		{&EAXREVERB_PRESET_GENERIC, 0.64999998, 1.493, 0.5},
		{&EAXREVERB_PRESET_GENERIC, 0.85000002, 1.493, 0.5},
		{&EAXREVERB_PRESET_GENERIC, 1.0, 1.493, 0.5},
		{&EAXREVERB_PRESET_ARENA, 0.40000001, 7.2839999, 0.33199999},
		{&EAXREVERB_PRESET_ARENA, 0.55000001, 7.2839999, 0.33199999},
		{&EAXREVERB_PRESET_ARENA, 0.69999999, 7.2839999, 0.33199999},
		{&EAXREVERB_PRESET_CONCERTHALL, 0.5, 3.961, 0.5},
		{&EAXREVERB_PRESET_CONCERTHALL, 0.69999999, 3.961, 0.5},
		{&EAXREVERB_PRESET_CONCERTHALL, 1.0, 3.961, 0.5},
		{&EAXREVERB_PRESET_DIZZY, 0.2, 17.233999, 0.66600001},
		{&EAXREVERB_PRESET_DIZZY, 0.30000001, 17.233999, 0.66600001},
		{&EAXREVERB_PRESET_DIZZY, 0.40000001, 17.233999, 0.66600001}};

void SetupEffect(ALuint effectId, unsigned int roomType)
{
	if (roomType >= RoomEffectCount)
	{
		assert(!"Invalid room type");
		roomType = 0;
	}

	// Based on http://openal.org/pipermail/openal/2014-March/000083.html

	const auto& effectData = RoomEffects[roomType];

	auto reverb = *effectData.Properties;

	reverb.flDecayTime = effectData.DecayTime;
	reverb.flDecayHFRatio = effectData.Damping;
	// According to the above mail this should be assigned to flLateReverbGain but the original effects work better with this.
	reverb.flGain = effectData.Volume;

	alEffecti(effectId, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

	alEffectf(effectId, AL_EAXREVERB_DENSITY, reverb.flDensity);
	alEffectf(effectId, AL_EAXREVERB_DIFFUSION, reverb.flDiffusion);

	alEffectf(effectId, AL_EAXREVERB_GAIN, reverb.flGain);
	alEffectf(effectId, AL_EAXREVERB_GAINHF, reverb.flGainHF);
	alEffectf(effectId, AL_EAXREVERB_GAINLF, reverb.flGainLF);

	alEffectf(effectId, AL_EAXREVERB_DECAY_TIME, reverb.flDecayTime);
	alEffectf(effectId, AL_EAXREVERB_DECAY_HFRATIO, reverb.flDecayHFRatio);
	alEffectf(effectId, AL_EAXREVERB_DECAY_LFRATIO, reverb.flDecayLFRatio);

	alEffectf(effectId, AL_EAXREVERB_REFLECTIONS_GAIN, reverb.flReflectionsGain);
	alEffectf(effectId, AL_EAXREVERB_REFLECTIONS_DELAY, reverb.flReflectionsDelay);
	alEffectfv(effectId, AL_EAXREVERB_REFLECTIONS_PAN, reverb.flReflectionsPan);

	alEffectf(effectId, AL_EAXREVERB_LATE_REVERB_GAIN, reverb.flLateReverbGain);
	alEffectf(effectId, AL_EAXREVERB_LATE_REVERB_DELAY, reverb.flLateReverbDelay);
	alEffectfv(effectId, AL_EAXREVERB_LATE_REVERB_PAN, reverb.flLateReverbPan);

	alEffectf(effectId, AL_EAXREVERB_ECHO_TIME, reverb.flEchoTime);
	alEffectf(effectId, AL_EAXREVERB_ECHO_DEPTH, reverb.flEchoDepth);

	alEffectf(effectId, AL_EAXREVERB_MODULATION_TIME, reverb.flModulationTime);
	alEffectf(effectId, AL_EAXREVERB_MODULATION_DEPTH, reverb.flModulationDepth);

	alEffectf(effectId, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, reverb.flAirAbsorptionGainHF);

	alEffectf(effectId, AL_EAXREVERB_HFREFERENCE, reverb.flHFReference);
	alEffectf(effectId, AL_EAXREVERB_LFREFERENCE, reverb.flLFReference);

	alEffectf(effectId, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, reverb.flRoomRolloffFactor);

	alEffecti(effectId, AL_EAXREVERB_DECAY_HFLIMIT, reverb.iDecayHFLimit);
}
}
