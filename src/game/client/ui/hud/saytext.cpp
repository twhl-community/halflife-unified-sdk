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
//
// saytext.cpp
//
// implementation of CHudSayText class
//

#include "hud.h"

#include "vgui_TeamFortressViewport.h"

#include "sound/ISoundSystem.h"

Vector* GetClientColor(int clientIndex);

// allow 20 pixels on either side of the text
int CHudSayText::GetMaxLineWidth()
{
	return ScreenWidth - 40;
}

bool CHudSayText::Init()
{
	gHUD.AddHudElem(this);

	g_ClientUserMessages.RegisterHandler("SayText", &CHudSayText::MsgFunc_SayText, this);

	InitHUDData();

	m_HUD_saytext = gEngfuncs.pfnRegisterVariable("hud_saytext", "1", 0);
	m_HUD_saytext_time = gEngfuncs.pfnRegisterVariable("hud_saytext_time", "5", 0);
	m_con_color = gEngfuncs.pfnGetCvarPointer("con_color");

	m_iFlags |= HUD_INTERMISSION; // is always drawn during an intermission

	return true;
}


void CHudSayText::InitHUDData()
{
	memset(m_LineBuffer, 0, sizeof m_LineBuffer);
	memset(m_NameColors, 0, sizeof m_NameColors);
	memset(m_NameLengths, 0, sizeof m_NameLengths);
}

bool CHudSayText::VidInit()
{
	return true;
}


int CHudSayText::ScrollTextUp()
{
	m_LineBuffer[MAX_LINES][0] = 0;
	memmove(m_LineBuffer[0], m_LineBuffer[1], sizeof(m_LineBuffer) - sizeof(m_LineBuffer[0])); // overwrite the first line
	memmove(&m_NameColors[0], &m_NameColors[1], sizeof(m_NameColors) - sizeof(m_NameColors[0]));
	memmove(&m_NameLengths[0], &m_NameLengths[1], sizeof(m_NameLengths) - sizeof(m_NameLengths[0]));
	m_LineBuffer[MAX_LINES - 1][0] = 0;

	if (m_LineBuffer[0][0] == ' ') // also scroll up following lines
	{
		m_LineBuffer[0][0] = 2;
		return 1 + ScrollTextUp();
	}

	return 1;
}

bool CHudSayText::Draw(float flTime)
{
	int y = m_YStart;

	if ((gViewPort && !gViewPort->AllowedToPrintText()) || 0 == m_HUD_saytext->value)
		return true;

	// make sure the scrolltime is within reasonable bounds, to guard against the clock being reset
	m_ScrollTime = std::min(m_ScrollTime, flTime + m_HUD_saytext_time->value);

	if (m_ScrollTime <= flTime)
	{
		if ('\0' != *m_LineBuffer[0])
		{
			m_ScrollTime = flTime + m_HUD_saytext_time->value;
			// push the console up
			ScrollTextUp();
		}
		else
		{ // buffer is empty,  just disable drawing of this section
			m_iFlags &= ~HUD_ACTIVE;
		}
	}

	// Set text color to con_color cvar value before drawing to ensure consistent color.
	// The engine resets this color to that value after drawing a single string.
	if (int r, g, b; sscanf(m_con_color->string, "%i %i %i", &r, &g, &b) == 3)
	{
		gEngfuncs.pfnDrawSetTextColor(r / 255.0f, g / 255.0f, b / 255.0f);
	}

	char line[MAX_CHARS_PER_LINE]{};

	for (int i = 0; i < MAX_LINES; i++)
	{
		if ('\0' != *m_LineBuffer[i])
		{
			if (*m_LineBuffer[i] == 2 && m_NameColors[i])
			{
				// it's a saytext string

				// Make a copy we can freely modify
				strncpy(line, m_LineBuffer[i], sizeof(line) - 1);
				line[sizeof(line) - 1] = '\0';

				// draw the first x characters in the player color
				const std::size_t playerNameEndIndex = std::min(m_NameLengths[i], MAX_PLAYER_NAME_LENGTH + 31);

				// Cut off the actual text so we can print player name
				line[playerNameEndIndex] = '\0';

				gEngfuncs.pfnDrawSetTextColor(m_NameColors[i]->x, m_NameColors[i]->y, m_NameColors[i]->z);
				const int x = DrawConsoleString(LINE_START, y, line + 1); // don't draw the control code at the start

				// Reset last character
				line[playerNameEndIndex] = m_LineBuffer[i][playerNameEndIndex];

				// color is reset after each string draw
				// Print the text without player name
				DrawConsoleString(x, y, line + m_NameLengths[i]);
			}
			else
			{
				// normal draw
				DrawConsoleString(LINE_START, y, m_LineBuffer[i]);
			}
		}

		y += m_LineHeight;
	}

	return true;
}

void CHudSayText::MsgFunc_SayText(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int client_index = READ_BYTE(); // the client who spoke the message
	SayTextPrint(READ_STRING(), iSize - 1, client_index);
}

void CHudSayText::SayTextPrint(const char* pszBuf, int iBufSize, int clientIndex)
{
	// Print it straight to the console
	ConsolePrint(pszBuf);

	if (gViewPort && gViewPort->AllowedToPrintText() == false)
	{
		return;
	}

	int i;
	// find an empty string slot
	for (i = 0; i < MAX_LINES; i++)
	{
		if ('\0' == *m_LineBuffer[i])
			break;
	}
	if (i == MAX_LINES)
	{
		// force scroll buffer up
		ScrollTextUp();
		i = MAX_LINES - 1;
	}

	m_NameLengths[i] = 0;
	m_NameColors[i] = nullptr;

	// if it's a say message, search for the players name in the string
	if (*pszBuf == 2 && clientIndex > 0)
	{
		gEngfuncs.pfnGetPlayerInfo(clientIndex, &g_PlayerInfoList[clientIndex]);
		const char* pName = g_PlayerInfoList[clientIndex].name;

		if (pName)
		{
			const char* nameInString = strstr(pszBuf, pName);

			if (nameInString)
			{
				m_NameLengths[i] = strlen(pName) + (nameInString - pszBuf);
				m_NameColors[i] = GetClientColor(clientIndex);
			}
		}
	}

	strncpy(m_LineBuffer[i], pszBuf, MAX_CHARS_PER_LINE);

	// make sure the text fits in one line
	EnsureTextFitsInOneLineAndWrapIfHaveTo(i);

	// Set scroll time
	if (i == 0)
	{
		m_ScrollTime = gHUD.m_flTime + m_HUD_saytext_time->value;
	}

	m_iFlags |= HUD_ACTIVE;
	PlaySound("misc/talk.wav", 1);

	m_YStart = ScreenHeight - 60 - (m_LineHeight * (MAX_LINES + 2));
}

void CHudSayText::EnsureTextFitsInOneLineAndWrapIfHaveTo(int line)
{
	int line_width = 0;
	GetConsoleStringSize(m_LineBuffer[line], &line_width, &m_LineHeight);

	if ((line_width + LINE_START) > GetMaxLineWidth())
	{ // string is too long to fit on line
		// scan the string until we find what word is too long,  and wrap the end of the sentence after the word
		int length = LINE_START;
		int tmp_len = 0;
		char* last_break = nullptr;
		for (char* x = m_LineBuffer[line]; *x != 0; x++)
		{
			// check for a color change, if so skip past it
			if (x[0] == '/' && x[1] == '(')
			{
				x += 2;
				// skip forward until past mode specifier
				while (*x != 0 && *x != ')')
					x++;

				if (*x != 0)
					x++;

				if (*x == 0)
					break;
			}

			char buf[2];
			buf[1] = 0;

			if (*x == ' ' && x != m_LineBuffer[line]) // store each line break,  except for the very first character
				last_break = x;

			buf[0] = *x; // get the length of the current character
			GetConsoleStringSize(buf, &tmp_len, &m_LineHeight);
			length += tmp_len;

			if (length > GetMaxLineWidth())
			{ // needs to be broken up
				if (!last_break)
					last_break = x - 1;

				x = last_break;

				// find an empty string slot
				int j;
				do
				{
					for (j = 0; j < MAX_LINES; j++)
					{
						if ('\0' == *m_LineBuffer[j])
							break;
					}
					if (j == MAX_LINES)
					{
						// need to make more room to display text, scroll stuff up then fix the pointers
						int linesmoved = ScrollTextUp();
						line -= linesmoved;
						last_break = last_break - (sizeof(m_LineBuffer[0]) * linesmoved);
					}
				} while (j == MAX_LINES);

				// copy remaining string into next buffer,  making sure it starts with a space character
				if ((char)*last_break == (char)' ')
				{
					int linelen = strlen(m_LineBuffer[j]);
					int remaininglen = strlen(last_break);

					if ((linelen - remaininglen) <= MAX_CHARS_PER_LINE)
						strcat(m_LineBuffer[j], last_break);
				}
				else
				{
					if ((strlen(m_LineBuffer[j]) - strlen(last_break) - 2) < MAX_CHARS_PER_LINE)
					{
						strcat(m_LineBuffer[j], " ");
						strcat(m_LineBuffer[j], last_break);
					}
				}

				*last_break = 0; // cut off the last string

				EnsureTextFitsInOneLineAndWrapIfHaveTo(j);
				break;
			}
		}
	}
}
