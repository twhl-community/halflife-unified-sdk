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

#include <optional>

#include "cdll_dll.h"
#include "palette.h"
#include "utils/ReplacementMaps.h"

/**
 *	@brief Contains per-map state
 *	@details This class should default construct into a valid state and should be move constructible and assignable
 */
class MapState final
{
public:
	MapState() = default;
	~MapState() = default;

	MapState(const MapState&) = delete;
	MapState& operator=(const MapState&) = delete;

	MapState(MapState&&) = default;
	MapState& operator=(MapState&&) = default;

	std::optional<RGB24> m_HudColor;
	std::optional<SuitLightType> m_LightType;

	const ReplacementMap* m_GlobalModelReplacement{&ReplacementMap::Empty};
	const ReplacementMap* m_GlobalSentenceReplacement{&ReplacementMap::Empty};

	Filename m_GlobalSoundReplacementFileName;
	const ReplacementMap* m_GlobalSoundReplacement{&ReplacementMap::Empty};
};
