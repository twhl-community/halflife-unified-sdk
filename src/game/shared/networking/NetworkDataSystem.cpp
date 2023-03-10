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

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <regex>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "cbase.h"
#include "ProjectInfo.h"

#include "networking/NetworkDataSystem.h"

/**
 *	@brief The minimum size that the generated file must be.
 *	See https://github.com/ValveSoftware/halflife/issues/3326 for why this is necessary.
 */
constexpr std::size_t MinimumFileDataSize = MAX_QPATH + "uncompressed"sv.size() + 1 + sizeof(int);

namespace
{
constexpr std::string_view NetworkDataDirectory{"networkdata"sv};
const std::string NetworkDataFileName{fmt::format("{}/data.json", NetworkDataDirectory)};
}

bool NetworkDataSystem::Initialize()
{
	m_Logger = g_Logging.CreateLogger("net_data");
	return true;
}

void NetworkDataSystem::PostInitialize()
{
}

void NetworkDataSystem::Shutdown()
{
	m_Handlers.clear();
	g_Logging.RemoveLogger(m_Logger);
	m_Logger.reset();
}

void NetworkDataSystem::RegisterHandler(std::string&& name, INetworkDataBlockHandler* handler)
{
	if (std::find_if_not(name.begin(), name.end(), [](auto c)
			{ return 0 != std::isalpha(c); }) != name.end())
	{
		assert(!"Producer name is invalid");
		return;
	}

	assert(handler);

	if (std::find_if(m_Handlers.begin(), m_Handlers.end(), [&](const auto& candidate)
			{ return candidate.Name == name; }) != m_Handlers.end())
	{
		assert(!"Tried to add multiple producers with the same name");
		return;
	}

	m_Handlers.emplace_back(std::move(name), handler);
}

void NetworkDataSystem::RemoveNetworkDataFiles(const char* pathID)
{
	g_pFileSystem->RemoveFile(NetworkDataFileName.c_str(), pathID);
}

#ifndef CLIENT_DLL
bool NetworkDataSystem::GenerateNetworkDataFile()
{
	const auto output = TryGenerateNetworkData();

	if (!output)
	{
		return false;
	}

	// Remove any existing files first to prevent problems if it isn't purged by the filesystem on open.
	RemoveNetworkDataFiles("GAMECONFIG");

	g_pFileSystem->CreateDirHierarchy(NetworkDataDirectory.data(), "GAMECONFIG");

	if (!TryWriteNetworkDataFile(NetworkDataFileName, *output))
	{
		return false;
	}

	// Precache the file so clients download it.
	UTIL_PrecacheGenericDirect(STRING(ALLOC_STRING(NetworkDataFileName.c_str())));

	return true;
}

std::optional<json> NetworkDataSystem::TryGenerateNetworkData()
{
	json output = json::object();

	{
		json metaData = json::object();

		metaData.emplace("ProtocolVersion", NetworkDataProtocolVersion);
		metaData.emplace("Version", fmt::format("{}.{}.{}", UnifiedSDKVersionMajor, UnifiedSDKVersionMinor, UnifiedSDKVersionPatch));

		output.emplace("Meta", std::move(metaData));
	}

	for (const auto& producer : m_Handlers)
	{
		producer.Handler->OnBeginNetworkDataProcessing();
	}

	bool success = true;

	json producerData = json::object();

	for (const auto& producer : m_Handlers)
	{
		NetworkDataBlock block{.Name = producer.Name, .Mode = NetworkDataMode::Serialize};

		producer.Handler->HandleNetworkDataBlock(block);

		json producerOutput;

		if (!block.ErrorMessage.empty())
		{
			m_Logger->critical(R"(Error saving network data: Network data producer \"{}\" failed to serialize data
	Reason: {})",
				producer.Name, block.ErrorMessage);
			success = false;
			break;
		}

		producerData.emplace(producer.Name, std::move(block.Data));
	}

	for (const auto& producer : m_Handlers)
	{
		producer.Handler->OnEndNetworkDataProcessing();
	}

	if (!success)
	{
		return {};
	}

	output.emplace("Data", std::move(producerData));

	return output;
}

bool NetworkDataSystem::TryWriteNetworkDataFile(const std::string& fileName, const json& output)
{
	FSFile file{fileName.c_str(), "wb", "GAMECONFIG"};

	if (!file.IsOpen())
	{
		m_Logger->critical(
			R"(Error saving network data: Could not open network data file for writing
	Make sure the {} directory is writable)",
			NetworkDataDirectory);
		return false;
	}

	try
	{
		// The engine generally accepts invalid UTF8 text in things like filenames so ignore invalid UTF8.
		auto fileData = output.dump(-1, ' ', false, nlohmann::detail::error_handler_t::ignore);

		// Pad the file with whitespace to reach the minium valid size.
		if (fileData.size() < MinimumFileDataSize)
		{
			fileData.append(MinimumFileDataSize - fileData.size(), ' ');
		}

		m_Logger->debug("Network data saved: {} bytes", fileData.size());

		file.Write(fileData.data(), fileData.size());

		return true;
	}
	catch (const std::exception& e)
	{
		m_Logger->critical("Error converting JSON to UTF8 text: {}", e.what());
		return false;
	}
}
#else
const std::regex VersionRegex{R"(^(\d+)\.(\d+)\.(\d+)$)"};

bool NetworkDataSystem::TryLoadNetworkDataFile()
{
	const auto fileData = TryLoadDataFromFile(NetworkDataFileName);

	if (!fileData)
	{
		return false;
	}

	return TryParseNetworkData(*fileData);
}

std::optional<std::vector<std::uint8_t>> NetworkDataSystem::TryLoadDataFromFile(const std::string& fileName)
{
	// The file can be in either of these, so check both.
	// Do NOT use a null path ID, since that allows overriding the file using alternate directories!
	// Check downloads first since we clear it no matter what.
	// If this is a listen server we'll fall back to the file saved by the server.
	FSFile file{fileName.c_str(), "rb", "GAMEDOWNLOAD"};

	if (!file.IsOpen())
	{
		file.Open(fileName.c_str(), "rb", "GAMECONFIG");
	}

	if (!file.IsOpen())
	{
		m_Logger->error("Error loading network data: file does not exist");
		return {};
	}

	std::size_t sizeInBytes = file.Size();

	std::vector<std::uint8_t> fileData;
	fileData.resize(sizeInBytes);

	if (file.Read(fileData.data(), fileData.size()) != fileData.size())
	{
		m_Logger->error("Error loading network data: read error");
		return {};
	}

	m_Logger->debug("Network data loaded: {} bytes", fileData.size());

	return fileData;
}

bool NetworkDataSystem::TryParseNetworkData(const std::vector<std::uint8_t>& fileData)
{
	json input = json::parse(fileData);

	if (!input.is_object())
	{
		m_Logger->error("Error loading network data: Network data is invalid");
		return false;
	}

	auto metaIt = input.find("Meta");

	if (metaIt == input.end())
	{
		m_Logger->error("Error loading network data: Network data is invalid (missing \"Meta\" key)");
		return false;
	}

	if (!metaIt->is_object())
	{
		m_Logger->error("Error loading network data: Network data is invalid (\"Meta\" is not an object)");
		return false;
	}

	auto dataIt = input.find("Data");

	if (dataIt == input.end())
	{
		m_Logger->error("Error loading network data: Network data is invalid (missing \"Data\" key)");
		return false;
	}

	if (!dataIt->is_object())
	{
		m_Logger->error("Error loading network data: Network data is invalid (\"Data\" is not an object)");
		return false;
	}

	try
	{
		const std::uint32_t protocolVersion = metaIt->value("ProtocolVersion", NetworkDataInvalidProtocolVersion);

		if (protocolVersion != NetworkDataProtocolVersion)
		{
			if (protocolVersion == NetworkDataInvalidProtocolVersion)
			{
				m_Logger->error("Error loading network data: Invalid or missing protocol version");
			}
			else if (NetworkDataProtocolVersion < protocolVersion)
			{
				m_Logger->error("Error loading network data: Client network protocol is out of date (client: {}, server: {})",
					NetworkDataProtocolVersion, protocolVersion);
			}
			else
			{
				m_Logger->error("Error loading network data: Client network protocol is newer than server (client: {}, server: {})",
					NetworkDataProtocolVersion, protocolVersion);
			}

			return false;
		}

		const std::string versionString = metaIt->value("Version", "");

		std::match_results<std::string::const_iterator> matches;

		if (!std::regex_match(versionString, matches, VersionRegex))
		{
			m_Logger->error("Error loading network data: Invalid server version string");
			return false;
		}

		const int major = UTIL_StringToInteger({matches[1].first, matches[1].second});
		const int minor = UTIL_StringToInteger({matches[2].first, matches[2].second});
		const int patch = UTIL_StringToInteger({matches[3].first, matches[3].second});

		if (major != UnifiedSDKVersionMajor || minor != UnifiedSDKVersionMinor || patch != UnifiedSDKVersionPatch)
		{
			m_Logger->error("Error loading network data: Server is running a different mod version (expected: {}.{}.{}, actual: {})",
				UnifiedSDKVersionMajor, UnifiedSDKVersionMinor, UnifiedSDKVersionPatch, versionString);
			return false;
		}
	}
	catch (const std::exception& e)
	{
		m_Logger->critical(R"(Error loading network data: Exception during deserialization
	Reason: {})",
			e.what());
		return false;
	}

	for (const auto& consumer : m_Handlers)
	{
		consumer.Handler->OnBeginNetworkDataProcessing();
	}

	bool success = true;

	try
	{
		for (auto& [key, value] : dataIt->items())
		{
			auto it = std::find_if(m_Handlers.begin(), m_Handlers.end(), [&](const auto& candidate)
				{ return candidate.Name == key; });

			if (it == m_Handlers.end())
			{
				m_Logger->error("Error loading network data: No consumer for network data \"{}\"", key);
				success = false;
				break;
			}

			NetworkDataBlock block{.Name = it->Name, .Mode = NetworkDataMode::Deserialize, .Data = std::move(value)};

			it->Handler->HandleNetworkDataBlock(block);

			if (!block.ErrorMessage.empty())
			{
				m_Logger->error(R"(Error loading network data: Network data consumer \"{}\" failed to deserialize data
	Reason: {})",
					it->Name, block.ErrorMessage);
				success = false;
				break;
			}
		}
	}
	catch (const std::exception& e)
	{
		m_Logger->critical(R"(Error loading network data: Exception during deserialization
	Reason: {})",
			e.what());
		success = false;
	}

	for (const auto& consumer : m_Handlers)
	{
		consumer.Handler->OnEndNetworkDataProcessing();
	}

	return success;
}

#endif
