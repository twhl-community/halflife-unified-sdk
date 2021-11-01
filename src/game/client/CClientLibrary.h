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
*	@brief Handles core client actions
*/
class CClientLibrary final
{
public:
	CClientLibrary() = default;
	~CClientLibrary() = default;

	CClientLibrary(const CClientLibrary&) = delete;
	CClientLibrary& operator=(const CClientLibrary&) = delete;
	CClientLibrary(CClientLibrary&&) = delete;
	CClientLibrary& operator=(CClientLibrary&&) = delete;

	/**
	*	@brief Handles client-side initialization
	*	@return Whether initialization succeeded
	*/
	bool Initialize();

	/**
	*	@brief Handles client-side shutdown
	*	@details Called even if @see Initialize returned false
	*/
	void Shutdown();
};

inline CClientLibrary g_Client;
