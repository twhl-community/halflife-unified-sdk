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

#include "GameSystem.h"

#include "filesystem_utils.h"
#include "json_fwd.h"

class CommandArgs;

const char* JSONTypeToString(json::value_t type);

struct JSONLoadParameters final
{
	/**
	 *	@brief If not empty, use the validator for this schema to validate the JSON before parsing it.
	 */
	const std::string_view SchemaName;

	/**
	 * @brief If provided, use this validator. If this is not null then JSONLoadParameters::SchemaName must be empty.
	 */
	const json_validator* Validator = nullptr;

	/**
	 *	@brief If not null, only files in this search path.
	 */
	const char* PathID = nullptr;
};

class JSONSystem final : public IGameSystem
{
private:
	// Schemas are created on-demand to reduce memory usage
	struct SchemaData
	{
		std::string Name;
		std::function<std::string()> Callback;
		std::optional<json_validator> Validator;
	};

public:
	JSONSystem() = default;
	~JSONSystem() = default;
	JSONSystem(const JSONSystem&) = delete;
	JSONSystem& operator=(const JSONSystem&) = delete;

	const char* GetName() const override { return "JSON"; }

	bool Initialize() override;
	void PostInitialize() override;
	void Shutdown() override;

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
	void RegisterSchema(std::string&& name, std::function<std::string()>&& getSchemaFunction);

	/**
	 *	@copydoc RegisterSchema(std::string&&, std::function<json()>&&)
	 */
	void RegisterSchema(std::string_view name, std::function<std::string()>&& getSchemaFunction)
	{
		RegisterSchema(std::string{name}, std::move(getSchemaFunction));
	}

	/**
	 *	@brief Gets the validator for the given schema.
	 *	@return If no schema by that name exists or the validator has not been created yet, returns null.
	 */
	const json_validator* GetValidator(std::string_view schemaName) const;

	/**
	 *	@brief Gets or creates the validator for the given schema.
	 *	If the schema exists and the validator has not been created yet, creates the validator.
	 *	@return If no schema by that name exists or the schema is invalid, returns null.
	 */
	const json_validator* GetOrCreateValidator(std::string_view schemaName);

	/**
	 *	@brief Loads a file using the engine's filesystem and parses it as JSON.
	 */
	std::optional<json> LoadJSONFile(const char* fileName, const JSONLoadParameters& parameters = {});

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
	 *	@see LoadJSONFile(const char*, const JSONLoadParameters& parameters)
	 *	@see ParseJSON(Callable, const json&)
	 */
	template <typename Callable>
	auto ParseJSONFile(const char* fileName, const JSONLoadParameters& parameters, Callable callable)
		-> std::optional<decltype(callable(json{}))>;

private:
	void ListSchemas(const CommandArgs& args);

	void GenerateSchema(const CommandArgs& args);

	void GenerateAllSchemas(const CommandArgs& args);

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
		// Can't parse JSON when we're not initialized
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
		// Can't parse JSON when we're not initialized
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
inline auto JSONSystem::ParseJSONFile(const char* fileName, const JSONLoadParameters& parameters, Callable callable)
	-> std::optional<decltype(callable(json{}))>
{
	if (auto data = LoadJSONFile(fileName, parameters); data.has_value())
	{
		return ParseJSON(callable, data.value());
	}

	return {};
}

inline JSONSystem g_JSON;
