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
class GameLibrary
{
protected:
	GameLibrary() = default;

public:
	virtual ~GameLibrary() = default;

	GameLibrary(const GameLibrary&) = delete;
	GameLibrary& operator=(const GameLibrary&) = delete;
	GameLibrary(GameLibrary&&) = delete;
	GameLibrary& operator=(GameLibrary&&) = delete;

protected:
	virtual void AddGameSystems();

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
