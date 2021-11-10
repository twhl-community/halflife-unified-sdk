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

//This is a header included in a shared header file, so avoid including headers here if possible and put them in CLogSystem.h instead

#include <memory>

#include <spdlog/logger.h>

class ILogSystem
{
public:
	virtual ~ILogSystem() {}

	virtual bool Initialize() = 0;
	virtual bool PostInitialize() = 0;
	virtual void Shutdown() = 0;

	virtual std::shared_ptr<spdlog::logger> CreateLogger(const std::string& name) = 0;

	virtual void RemoveLogger(const std::shared_ptr<spdlog::logger>& logger) = 0;

	virtual std::shared_ptr<spdlog::logger> GetGlobalLogger() = 0;
};