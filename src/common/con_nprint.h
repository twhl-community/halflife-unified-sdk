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

#pragma once
<<<<<<< HEAD:src / common / con_nprint.h
#endif

	====== =

	>>>>>> > 58c150d8f646b2dc93733f399e6c71bf793ee946:common / con_nprint.h
	typedef struct con_nprint_s
{
	int		index;			// Row #
	float	time_to_live;	// # of seconds before it dissappears
	float	color[3];		// RGB colors ( 0.0 -> 1.0 scale )
} con_nprint_t;

<<<<<< < HEAD:src / common / con_nprint.h
	void Con_NPrintf(int idx, char* fmt, ...);
void Con_NXPrintf(struct con_nprint_s* info, char* fmt, ...);

#endif
====== =
void Con_NPrintf(int idx, char* fmt, ...);
void Con_NXPrintf(struct con_nprint_s* info, char* fmt, ...);
>>>>>> > 58c150d8f646b2dc93733f399e6c71bf793ee946:common / con_nprint.h
