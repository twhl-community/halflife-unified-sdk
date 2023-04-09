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

/**
 *	@brief Defines the typedef @c ThisClass.
 */
#define DECLARE_CLASS_NOBASE(thisClass) \
public:                                 \
	using ThisClass = thisClass

/**
 *	@brief Defines the typedefs @c ThisClass and @c BaseClass.
 */
#define DECLARE_CLASS(thisClass, baseClass) \
	DECLARE_CLASS_NOBASE(thisClass);        \
	using BaseClass = baseClass
