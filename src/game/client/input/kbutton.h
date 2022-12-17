//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#pragma once

struct kbutton_t
{
	int down[2]; // key nums holding it down
	int state;	 // low bit is down state
};
