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

#include <cassert>
#include <memory>
#include <string_view>
#include <vector>

#include <spdlog/logger.h>

class PrecacheList final
{
public:
	using ValidationFunction = const char* (*)(const char* str, PrecacheList* list);
	using EnginePrecacheFunction = int (*)(const char* str);

	explicit PrecacheList(std::string_view type,
		std::shared_ptr<spdlog::logger> logger,
		ValidationFunction validationFunction = nullptr,
		EnginePrecacheFunction enginePrecacheFunction = nullptr)
		: m_Type(type),
		  m_Logger(logger),
		  m_ValidationFunction(validationFunction),
		  m_EnginePrecacheFunction(enginePrecacheFunction)
	{
		assert(!type.empty());
		assert(logger);

		Clear();
	}

	std::string_view GetType() const { return m_Type; }

	spdlog::logger* GetLogger() { return m_Logger.get(); }

	std::size_t GetCount() const { return m_Precaches.size(); }

	const char* GetString(std::size_t index) const { return m_Precaches[index]; }

	int IndexOf(const char* str) const;

	int Add(const char* str);

	/**
	 *	@brief Adds a string directly without validation.
	 *	Only to be used for adding files precached by the engine.
	 */
	void AddUnchecked(const char* str);

	void Clear();

private:
	void LogString(spdlog::level::level_enum level, const char* state, const char* str, int index);

private:
	const std::string_view m_Type;
	const std::shared_ptr<spdlog::logger> m_Logger;
	const ValidationFunction m_ValidationFunction;
	const EnginePrecacheFunction m_EnginePrecacheFunction;
	std::vector<const char*> m_Precaches;
};
