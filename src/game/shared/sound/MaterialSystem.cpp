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

#include <cassert>

#include "cbase.h"
#include "MaterialSystem.h"

// Texture names
static bool bTextureTypeInit = false;

static int gcTextures = 0;
static char grgszTextureName[CTEXTURESMAX][CBTEXTURENAMEMAX];
static char grgchTextureType[CTEXTURESMAX];

static char* memfgets(const std::byte* pMemFile, std::size_t fileSize, std::size_t& filePos, char* pBuffer, std::size_t bufferSize)
{
	// Bullet-proofing
	if (!pMemFile || !pBuffer)
		return nullptr;

	if (filePos >= fileSize)
		return nullptr;

	std::size_t i = filePos;
	std::size_t last = fileSize;

	// fgets always nullptr terminates, so only read bufferSize-1 characters
	if (last - filePos > (bufferSize - 1))
		last = filePos + (bufferSize - 1);

	bool stop = false;

	const auto text = reinterpret_cast<const char*>(pMemFile);

	// Stop at the next newline (inclusive) or end of buffer
	while (i < last && !stop)
	{
		if (text[i] == '\n')
			stop = true;
		i++;
	}


	// If we actually advanced the pointer, copy it over
	if (i != filePos)
	{
		// We read in size bytes
		std::size_t size = i - filePos;
		// copy it out
		memcpy(pBuffer, text + filePos, sizeof(byte) * size);

		// If the buffer isn't full, terminate (this is always true)
		if (size < bufferSize)
			pBuffer[size] = 0;

		// Update file pointer
		filePos = i;
		return pBuffer;
	}

	// No data read, bail
	return nullptr;
}

void PM_SwapTextures(int i, int j)
{
	char chTemp;
	char szTemp[CBTEXTURENAMEMAX];

	strcpy(szTemp, grgszTextureName[i]);
	chTemp = grgchTextureType[i];

	strcpy(grgszTextureName[i], grgszTextureName[j]);
	grgchTextureType[i] = grgchTextureType[j];

	strcpy(grgszTextureName[j], szTemp);
	grgchTextureType[j] = chTemp;
}

void PM_SortTextures()
{
	// Bubble sort, yuck, but this only occurs at startup and it's only 512 elements...
	//
	int i, j;

	for (i = 0; i < gcTextures; i++)
	{
		for (j = i + 1; j < gcTextures; j++)
		{
			if (stricmp(grgszTextureName[i], grgszTextureName[j]) > 0)
			{
				// Swap
				//
				PM_SwapTextures(i, j);
			}
		}
	}
}

void PM_InitTextureTypes()
{
	char buffer[512];
	int i, j;

	if (bTextureTypeInit)
		return;

	memset(&(grgszTextureName[0][0]), 0, CTEXTURESMAX * CBTEXTURENAMEMAX);
	memset(grgchTextureType, 0, CTEXTURESMAX);

	gcTextures = 0;
	memset(buffer, 0, 512);

	const auto fileContents = FileSystem_LoadFileIntoBuffer("sound/materials.txt", FileContentFormat::Text, "GAMECONFIG");

	if (fileContents.empty())
		return;

	std::size_t filePos = 0;
	// for each line in the file...
	while (memfgets(fileContents.data(), fileContents.size(), filePos, buffer, std::size(buffer) - 1) != nullptr && (gcTextures < CTEXTURESMAX))
	{
		// skip whitespace
		i = 0;
		while ('\0' != buffer[i] && 0 != isspace(buffer[i]))
			i++;

		if ('\0' == buffer[i])
			continue;

		// skip comment lines
		if (buffer[i] == '/' || 0 == isalpha(buffer[i]))
			continue;

		// get texture type
		grgchTextureType[gcTextures] = toupper(buffer[i++]);

		// skip whitespace
		while ('\0' != buffer[i] && 0 != isspace(buffer[i]))
			i++;

		if ('\0' == buffer[i])
			continue;

		// get sentence name
		j = i;
		while ('\0' != buffer[j] && 0 == isspace(buffer[j]))
			j++;

		if ('\0' == buffer[j])
			continue;

		// null-terminate name and save in sentences array
		j = V_min(j, CBTEXTURENAMEMAX - 1 + i);
		buffer[j] = 0;
		strcpy(&(grgszTextureName[gcTextures++][0]), &(buffer[i]));
	}

	PM_SortTextures();

	bTextureTypeInit = true;
}

char PM_FindTextureType(char* name)
{
	int left, right, pivot;
	int val;

	assert(bTextureTypeInit);

	left = 0;
	right = gcTextures - 1;

	while (left <= right)
	{
		pivot = (left + right) / 2;

		val = strnicmp(name, grgszTextureName[pivot], CBTEXTURENAMEMAX - 1);
		if (val == 0)
		{
			return grgchTextureType[pivot];
		}
		else if (val > 0)
		{
			left = pivot + 1;
		}
		else if (val < 0)
		{
			right = pivot - 1;
		}
	}

	return CHAR_TEX_CONCRETE;
}
