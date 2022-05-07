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

#include <string_view>

#include <spdlog/fmt/fmt.h>

#include "cbase.h"
#include "command_utils.h"

#include "json_utils.h"

using namespace std::literals;
using namespace nlohmann::json_schema;

constexpr std::string_view SchemasDirectory{"schemas"sv};
constexpr int SchemaIndentLevel = 1;
constexpr char SchemaIndentCharacter = '\t';
constexpr std::size_t MaxValidationContentsLogLength = 100;

const char* JSONTypeToString(json::value_t type)
{
	switch (type)
	{
	case json::value_t::null:
		return "null";
	case json::value_t::object:
		return "object";
	case json::value_t::array:
		return "array";
	case json::value_t::string:
		return "string";
	case json::value_t::boolean:
		return "boolean";
	case json::value_t::number_integer:
		return "signed integer";
	case json::value_t::number_unsigned:
		return "unsigned integer";
	case json::value_t::number_float:
		return "floating point number";
	case json::value_t::binary:
		return "binary data";
	case json::value_t::discarded:
		return "discarded";
	default:
		return "unknown";
	}
}

class JSONValidatorErrorHandler : public basic_error_handler
{
public:
	JSONValidatorErrorHandler(spdlog::logger& logger)
		: m_Logger(logger)
	{
	}

	void error(const nlohmann::json_pointer<nlohmann::basic_json<>>& pointer, const json& instance,
		const std::string& message) override
	{
		basic_error_handler::error(pointer, instance, message);

		const auto formatContents = [](std::string&& contents)
		{
			//Trim contents to a reasonable size.
			if (contents.length() > MaxValidationContentsLogLength)
			{
				contents.resize(MaxValidationContentsLogLength);
				contents += "...";
			}

			return contents;
		};

		m_Logger.error("Error validating JSON \"{}\" with value \"{}\": {}",
			formatContents(pointer.to_string()), formatContents(instance.dump()), message);
	}

private:
	spdlog::logger& m_Logger;
};

bool JSONSystem::Initialize()
{
	m_JsonDebug = g_ConCommands.CreateCVar("json_debug", "0", FCVAR_PROTECTED);
	m_JsonSchemaValidation = g_ConCommands.CreateCVar("json_schema_validation", "0", FCVAR_PROTECTED);

	g_ConCommands.CreateCommand("json_listschemas", [this](const auto& args) { ListSchemas(args); });
	g_ConCommands.CreateCommand("json_generateschema", [this](const auto& args) { GenerateSchema(args); });
	g_ConCommands.CreateCommand("json_generateallschemas", [this](const auto& args) { GenerateAllSchemas(args); });

	//Use the global logger during startup
	m_Logger = g_Logging.GetGlobalLogger();

	return true;
}

void JSONSystem::PostInitialize()
{
	//Create the logger using user-provided configuration
	m_Logger = g_Logging.CreateLogger("json");
}

void JSONSystem::Shutdown()
{
	m_Schemas.clear();
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
	m_JsonSchemaValidation = nullptr;
	m_JsonDebug = nullptr;
}

void JSONSystem::RegisterSchema(std::string&& name, std::function<json()>&& getSchemaFunction)
{
	if (name.empty() || !getSchemaFunction)
	{
		return;
	}

	if (std::find_if(m_Schemas.begin(), m_Schemas.end(), [&](const auto& data) {
			return data.Name == name;
		}) != m_Schemas.end())
	{
		return;
	}

	m_Schemas.push_back({std::move(name), std::move(getSchemaFunction)});
}

const json_validator* JSONSystem::GetValidator(std::string_view schemaName) const
{
	if (auto it = std::find_if(m_Schemas.begin(), m_Schemas.end(), [&](const auto& schema)
			{ return schema.Name == schemaName; });
		it != m_Schemas.end() && it->Validator.has_value())
	{
		return &it->Validator.value();
	}

	return nullptr;
}

const json_validator* JSONSystem::GetOrCreateValidator(std::string_view schemaName)
{
	if (auto it = std::find_if(m_Schemas.begin(), m_Schemas.end(), [&](const auto& schema)
		{ return schema.Name == schemaName;
		}); it != m_Schemas.end())
	{
		//Create validator on demand.
		if (!it->Validator.has_value())
		{
			m_Logger->trace("Creating validator for schema \"{}\"", schemaName);

			try
			{
				auto schema = it->Callback();

				if (schema.is_null())
				{
					m_Logger->error("Error creating validator for schema \"{}\": schema is invalid", schemaName);
					return nullptr;
				}

				json_validator validator;

				validator.set_root_schema(std::move(schema));

				it->Validator = std::move(validator);
			}
			catch(const std::invalid_argument& e)
			{
				m_Logger->error("Error creating validator for schema \"{}\": {}", schemaName, e.what());
				return nullptr;
			}
		}

		return &it->Validator.value();
	}

	m_Logger->error("Error creating validator for schema \"{}\": schema does not exist", schemaName);

	return nullptr;
}

std::optional<json> JSONSystem::LoadJSONFile(const char* fileName, const JSONLoadParameters& parameters)
{
	if (!m_Logger)
	{
		//Can't parse JSON when we're not initialized
		return {};
	}

	if (!fileName || '\0' == fileName[0])
	{
		return {};
	}

	auto validator = [&]() -> const json_validator*
	{
		//Only validate if enabled.
		if (0 == m_JsonSchemaValidation->value)
		{
			return nullptr;
		}

		if (parameters.Validator)
		{
			assert(parameters.SchemaName.empty());
			return parameters.Validator;
		}

		return GetOrCreateValidator(parameters.SchemaName);
	}();

	m_Logger->trace("Loading JSON file \"{}\"", fileName);

	if (const auto file = FileSystem_LoadFileIntoBuffer(fileName, FileContentFormat::Text, parameters.PathID); !file.empty())
	{
		try
		{
			auto text = reinterpret_cast<const char*>(file.data());

			auto data = json::parse(text, text + file.size(), nullptr, true, true);

			m_Logger->trace("Successfully loaded JSON file");

			if (validator)
			{
				m_Logger->trace("Validating JSON file");

				JSONValidatorErrorHandler errorHandler{*m_Logger};

				validator->validate(data, errorHandler);

				if (errorHandler)
				{
					m_Logger->trace("JSON file failed validation");
				}
				else
				{
					m_Logger->trace("JSON file passed validation");
				}
			}

			return data;
		}
		catch (const json::exception& e)
		{
			m_Logger->error("Error \"{}\" parsing JSON: \"{}\"", e.id, e.what());
		}
	}
	else
	{
		m_Logger->error("Couldn't open file \"{}\"", fileName);
	}

	return {};
}

void JSONSystem::ListSchemas(const CCommandArgs& args)
{
	if (!IsDebugEnabled())
	{
		return;
	}

	Con_Printf("%zu schemas:\n", m_Schemas.size());

	for (const auto& schema : m_Schemas)
	{
		Con_Printf("%s\n", schema.Name.c_str());
	}
}

void JSONSystem::GenerateSchema(const CCommandArgs& args)
{
	if (!IsDebugEnabled())
	{
		return;
	}

	if (2 == args.Count())
	{
		const auto schemaName = args.Argument(1);

		if (auto it = std::find_if(m_Schemas.begin(), m_Schemas.end(), [&](const auto& data) {
				return data.Name == schemaName;
			});
			it != m_Schemas.end())
		{
			WriteSchemaToFile(it->Name.c_str(), it->Callback());
		}
		else
		{
			Con_Printf("Schema \"%s\" not found\n", schemaName);
		}
	}
	else
	{
		Con_Printf("Usage: json_generateschema schema_name\n");
	}
}

void JSONSystem::GenerateAllSchemas(const CCommandArgs& args)
{
	if (!IsDebugEnabled())
	{
		return;
	}

	if (args.Count() != 1)
	{
		Con_Printf("Usage: json_generateallschemas\n");
		return;
	}

	for (const auto& data : m_Schemas)
	{
		WriteSchemaToFile(data.Name, data.Callback());
	}
}

void JSONSystem::WriteSchemaToFile(std::string_view schemaName, const json& schema)
{
	if (schema.is_null())
	{
		Con_Printf("Couldn't get schema to write to file\n");
		return;
	}

	const auto directory = fmt::format("{}/{}", SchemasDirectory, GetLongLibraryPrefix());

	g_pFileSystem->CreateDirHierarchy(directory.c_str(), "GAMECONFIG");

	const std::string fileName{fmt::format("{}/{}.json", directory, schemaName)};

	if (FileSystem_WriteTextToFile(fileName.c_str(), schema.dump(SchemaIndentLevel, SchemaIndentCharacter).c_str(), "GAMECONFIG"))
	{
		Con_Printf("Wrote schema to \"%s\"\n", fileName.c_str());
	}
}
