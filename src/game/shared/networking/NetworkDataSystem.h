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

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/logger.h>

#include "Platform.h"

#include "utils/GameSystem.h"
#include "utils/JSONSystem.h"

/**
*	@brief Version of the network data protocol.
*	Used to verify that the server and client are using the same version.
*	Increment this whenever the contents of the network data file change.
*	A protocol version of 0 is treated as invalid/missing.
*/
constexpr std::uint32_t NetworkDataProtocolVersion = 1;

constexpr std::uint32_t NetworkDataInvalidProtocolVersion = 0;

enum class NetworkDataMode
{
	Serialize,
	Deserialize
};

struct NetworkDataBlock
{
	const std::string_view Name;

	/**
	*	@brief Whether the block is being serialized or deserialized.
	*/
	const NetworkDataMode Mode;

	json Data;

	/**
	 *	@brief If an error occurs during the handling of a network data block,
	 *	set this to an error message to indicate that an error occurred and which message to show.
	 */
	std::string ErrorMessage;
};

/**
*	@brief Handles network data serialization and deserialization.
*/
class INetworkDataBlockHandler
{
public:
	virtual ~INetworkDataBlockHandler() = default;

	/**
	*	@brief Called when a network data set is about to be processed.
	*/
	virtual void OnBeginNetworkDataProcessing() {}

	/**
	*	@brief Called when a network data set has been processed.
	*/
	virtual void OnEndNetworkDataProcessing() {}

	virtual void HandleNetworkDataBlock(NetworkDataBlock& block) = 0;
};

/**
 *	@brief Handles the generation, transfer and deserialization of network data sent using a file.
 */
class NetworkDataSystem final : public IGameSystem
{
private:
	struct HandlerData
	{
		std::string Name;
		INetworkDataBlockHandler* Handler{};
	};

public:
	NetworkDataSystem() = default;

	NetworkDataSystem(const NetworkDataSystem&) = delete;
	NetworkDataSystem& operator=(const NetworkDataSystem&) = delete;

	const char* GetName() const override { return "NetworkData"; }

	bool Initialize() override;
	void PostInitialize() override;
	void Shutdown() override;

	spdlog::logger* GetLogger() const { return m_Logger.get(); }

	void RegisterHandler(std::string&& name, INetworkDataBlockHandler* handler);

	static void RemoveNetworkDataFiles(const char* pathID);

#ifndef CLIENT_DLL
	bool GenerateNetworkDataFile();

private:
	std::optional<json> TryGenerateNetworkData();
	bool TryWriteNetworkDataFile(const std::string& fileName, const json& output);
#else
	bool TryLoadNetworkDataFile();

private:
	std::optional<std::vector<std::uint8_t>> TryLoadDataFromFile(const std::string& fileName);
	bool TryParseNetworkData(const std::vector<std::uint8_t>& fileData);
#endif

private:
	std::shared_ptr<spdlog::logger> m_Logger;
	std::vector<HandlerData> m_Handlers;
};

inline NetworkDataSystem g_NetworkData;
