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

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/logger.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>

#include "filesystem_utils.h"
#include "json_fwd.h"

class CCommandArgs;

const char* JSONTypeToString(json::value_t type);

class JSONSystem final
{
private:
	//Schemas are created on-demand to reduce memory usage
	struct SchemaData
	{
		std::string Name;
		std::function<json()> Callback;
	};

public:
	JSONSystem() = default;
	~JSONSystem() = default;
	JSONSystem(const JSONSystem&) = delete;
	JSONSystem& operator=(const JSONSystem&) = delete;

	bool Initialize();
	bool PostInitialize();
	void Shutdown();

	/**
	*	@brief Used to check if debug functionality is enabled.
	*	@details This allows sensitive commands and performance-lowering systems to be disabled.
	*/
	bool IsDebugEnabled() const
	{
		return !!m_JsonDebug->value;
	}

	/**
	*	@brief Registers a schema with a function to get the schema.
	*/
	void RegisterSchema(std::string&& name, std::function<json()>&& getSchemaFunction);

	/**
	*	@brief Loads a file using the engine's filesystem, parses it as JSON, and validates it using the given validator.
	*/
	std::optional<json> LoadJSONFile(const char* fileName, const json_validator& validator, const char* pathID = nullptr);

	/**
	*	@brief Loads a file using the engine's filesystem and parses it as JSON.
	*/
	std::optional<json> LoadJSONFile(const char* fileName, const char* pathID = nullptr);

	/**
	*	@brief Helper function to parse JSON.
	*	Pass in a callable object that will parse JSON into a movable or copyable object.
	*	If any JSON exceptions are thrown they will be logged.
	*	@return An optional value that contains the result object if no errors occurred, empty otherwise
	*/
	template <typename Callable>
	auto ParseJSON(Callable callable, const json& input) -> std::optional<decltype(callable(input))>;

	/**
	*	@brief Helper function to parse JSON Schemas.
	*/
	std::optional<json> ParseJSONSchema(std::string_view schema);

	/**
	*	@brief Loads a JSON file and parses it into an object.
	*	@see LoadJSONFile(const char*, const json_validator& validator const char*)
	*	@see ParseJSON(Callable, const json&)
	*/
	template <typename Callable>
	auto ParseJSONFile(const char* fileName, const json_validator& validator, Callable callable, const char* pathID = nullptr)
		-> std::optional<decltype(callable(json{}))>;

	/**
	*	@brief Loads a JSON file and parses it into an object.
	*	@see LoadJSONFile(const char*, const char*)
	*	@see ParseJSON(Callable, const json&)
	*/
	template <typename Callable>
	auto ParseJSONFile(const char* fileName, Callable callable, const char* pathID = nullptr) -> std::optional<decltype(callable(json{}))>;

private:
	std::optional<json> LoadJSONFile(const char* fileName, const json_validator* validator, const char* pathID);

	void ListSchemas(const CCommandArgs& args);

	void GenerateSchema(const CCommandArgs& args);

	void GenerateAllSchemas(const CCommandArgs& args);

	void WriteSchemaToFile(std::string_view schemaName, const json& schema);

private:
	cvar_t* m_JsonDebug = nullptr;
	cvar_t* m_JsonSchemaValidation = nullptr;
	std::shared_ptr<spdlog::logger> m_Logger;

	std::vector<SchemaData> m_Schemas;
};

template <typename Callable>
inline auto JSONSystem::ParseJSON(Callable callable, const json& input) -> std::optional<decltype(callable(input))>
{
	if (!m_Logger)
	{
		//Can't parse JSON when we're not initialized
		return {};
	}

	try
	{
		return callable(input);
	}
	catch (const json::exception& e)
	{
		m_Logger->error("Error \"{}\" parsing JSON: \"{}\"", e.id, e.what());
		return {};
	}
}

inline std::optional<json> JSONSystem::ParseJSONSchema(std::string_view schema)
{
	if (!m_Logger)
	{
		//Can't parse JSON when we're not initialized
		return {};
	}

	try
	{
		return json::parse(schema.begin(), schema.end());
	}
	catch (const json::exception& e)
	{
		m_Logger->error("Error \"{}\" parsing JSON schema: \"{}\"", e.id, e.what());
		return {};
	}
}

template <typename Callable>
inline auto JSONSystem::ParseJSONFile(const char* fileName, const json_validator& validator, Callable callable, const char* pathID)
	-> std::optional<decltype(callable(json{}))>
{
	if (auto data = LoadJSONFile(fileName, validator, pathID); data.has_value())
	{
		return ParseJSON(callable, data.value());
	}

	return {};
}

template <typename Callable>
inline auto JSONSystem::ParseJSONFile(const char* fileName, Callable callable, const char* pathID) -> std::optional<decltype(callable(json{}))>
{
	if (auto data = LoadJSONFile(fileName, pathID); data.has_value())
	{
		return ParseJSON(callable, data.value());
	}

	return {};
}

inline JSONSystem g_JSON;
