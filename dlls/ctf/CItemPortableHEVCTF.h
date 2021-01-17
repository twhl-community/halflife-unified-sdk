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
#ifndef CITEMPORTABLEHEVCTF_H
#define CITEMPORTABLEHEVCTF_H

#include "CItemCTF.h"

class CItemPortableHEVCTF : public CItemCTF
{
public:
	void Precache() override;

	void RemoveEffect(CBasePlayer* pPlayer) override;

	bool MyTouch(CBasePlayer* pPlayer) override;

	void CItemPortableHEVCTF::Spawn() override;

	int Classify() override;
};

#endif
