/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#include <cassert>
#include <limits>

#include "Platform.h"

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#else
#include "hud.h"
#include "cl_util.h"
#endif

#include "interface.h"

#include "filesystem_shared.h"

static CSysModule* g_pFileSystemModule = nullptr;

std::filesystem::path FileSystem_GetGameDirectory()
{
#ifndef CLIENT_DLL
	char gameDir[MAX_PATH]{};

	g_engfuncs.pfnGetGameDir(gameDir);

	return gameDir;
#else
	return gEngfuncs.pfnGetGameDirectory();
#endif
}

bool FileSystem_LoadFileSystem()
{
	// Determine which filesystem to use.
#if defined ( WIN32 )
	const char* szFsModule = "filesystem_stdio.dll";
#elif defined(OSX)
	const char* szFsModule = "filesystem_stdio.dylib";
#elif defined(LINUX)
	const char* szFsModule = "filesystem_stdio.so";
#else
#error
#endif

	char szFSDir[MAX_PATH]{};

#ifndef CLIENT_DLL
	//Just use the filename for the server. No COM_ExpandFilename here.
	strncpy(szFSDir, szFsModule, sizeof(szFSDir) - 1);
	szFSDir[sizeof(szFSDir) - 1] = '\0';
#else
	if (gEngfuncs.COM_ExpandFilename(szFsModule, szFSDir, sizeof(szFSDir)) == false)
	{
		return false;
	}
#endif

	// Get filesystem interface.
	g_pFileSystemModule = Sys_LoadModule(szFSDir);

	assert(g_pFileSystemModule);

	if (!g_pFileSystemModule)
	{
		return false;
	}

	CreateInterfaceFn fileSystemFactory = Sys_GetFactory(g_pFileSystemModule);

	if (!fileSystemFactory)
	{
		return false;
	}

	g_pFileSystem = (IFileSystem*)fileSystemFactory(FILESYSTEM_INTERFACE_VERSION, nullptr);

	assert(g_pFileSystem);

	if (!g_pFileSystem)
	{
		return false;
	}

	return true;
}

void FileSystem_FreeFileSystem()
{
	if (g_pFileSystem)
	{
		g_pFileSystem = nullptr;
	}

	if (g_pFileSystemModule)
	{
		Sys_UnloadModule(g_pFileSystemModule);
		g_pFileSystemModule = nullptr;
	}
}

std::vector<std::byte> FileSystem_LoadFileIntoBuffer(const char* filename, const char* pathID)
{
	assert(g_pFileSystem);

	auto fileHandle = g_pFileSystem->Open(filename, "rb", pathID);

	if (fileHandle == FILESYSTEM_INVALID_HANDLE)
	{
		return {};
	}

	const auto size = g_pFileSystem->Size(fileHandle);

	//Null terminate it in case it's actually text
	std::vector<std::byte> buffer;

	buffer.resize(size + 1);

	g_pFileSystem->Read(buffer.data(), size, fileHandle);

	buffer[size] = std::byte{'\0'};

	g_pFileSystem->Close(fileHandle);

	return buffer;
}

bool FileSystem_WriteTextToFile(const char* filename, const char* text, const char* pathID)
{
	if (!filename || !text)
	{
		return false;
	}

	const std::size_t length = std::strlen(text);

	if (length > static_cast<std::size_t>(std::numeric_limits<int>::max()))
	{
		Con_Printf("FileSystem_WriteTextToFile: text too long\n");
		return false;
	}

	if (FSFile file{filename, "w", pathID}; file)
	{
		file.Write(text, length);

		return true;
	}

	Con_Printf("FileSystem_WriteTextToFile: couldn't open file \"%s\" for writing\n", filename);

	return false;
}
