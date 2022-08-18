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
class CMapState final
{
public:
	CMapState() = default;
	~CMapState() = default;

	CMapState(const CMapState&) = delete;
	CMapState& operator=(const CMapState&) = delete;

	CMapState(CMapState&&) = default;
	CMapState& operator=(CMapState&&) = default;

	std::optional<RGB24> m_HudColor;
	std::optional<SuitLightType> m_LightType;

	ReplacementMap m_GlobalModelReplacement;
	ReplacementMap m_GlobalSentenceReplacement;
};
