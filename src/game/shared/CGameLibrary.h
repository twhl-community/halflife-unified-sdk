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

/**
*	@brief Handles core game actions
*/
class CGameLibrary
{
protected:
	CGameLibrary() = default;

public:
	~CGameLibrary() = default;

	CGameLibrary(const CGameLibrary&) = delete;
	CGameLibrary& operator=(const CGameLibrary&) = delete;
	CGameLibrary(CGameLibrary&&) = delete;
	CGameLibrary& operator=(CGameLibrary&&) = delete;

protected:
	/**
	*	@brief Handles client-side initialization
	*	@return Whether initialization succeeded
	*/
	bool InitializeCommon();

	/**
	*	@brief Handles client-side shutdown
	*	@details Called even if @see Initialize returned false
	*/
	void ShutdownCommon();
};
