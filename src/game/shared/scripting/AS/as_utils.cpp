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

#include "cbase.h"

#include "as_utils.h"

using namespace std::literals;

namespace as
{
std::string_view ReturnCodeToString(int code)
{
	switch (code)
	{
	case asSUCCESS:
		return "Success"sv;
	case asERROR:
		return "General error (see log)"sv;
	case asCONTEXT_ACTIVE:
		return "Context active"sv;
	case asCONTEXT_NOT_FINISHED:
		return "Context not finished"sv;
	case asCONTEXT_NOT_PREPARED:
		return "Context not prepared"sv;
	case asINVALID_ARG:
		return "Invalid argument"sv;
	case asNO_FUNCTION:
		return "No function"sv;
	case asNOT_SUPPORTED:
		return "Not supported"sv;
	case asINVALID_NAME:
		return "Invalid name"sv;
	case asNAME_TAKEN:
		return "Name taken"sv;
	case asINVALID_DECLARATION:
		return "Invalid declaration"sv;
	case asINVALID_OBJECT:
		return "Invalid object"sv;
	case asINVALID_TYPE:
		return "Invalid type"sv;
	case asALREADY_REGISTERED:
		return "Already registered"sv;
	case asMULTIPLE_FUNCTIONS:
		return "Multiple functions"sv;
	case asNO_MODULE:
		return "No module"sv;
	case asNO_GLOBAL_VAR:
		return "No global variable"sv;
	case asINVALID_CONFIGURATION:
		return "Invalid configuration"sv;
	case asINVALID_INTERFACE:
		return "Invalid interface"sv;
	case asCANT_BIND_ALL_FUNCTIONS:
		return "Can't bind all functions"sv;
	case asLOWER_ARRAY_DIMENSION_NOT_REGISTERED:
		return "Lower array dimension not registered"sv;
	case asWRONG_CONFIG_GROUP:
		return "Wrong configuration group"sv;
	case asCONFIG_GROUP_IS_IN_USE:
		return "Configuration group is in use"sv;
	case asILLEGAL_BEHAVIOUR_FOR_TYPE:
		return "Illegal behavior for type"sv;
	case asWRONG_CALLING_CONV:
		return "Wrong calling convention"sv;
	case asBUILD_IN_PROGRESS:
		return "Build in progress"sv;
	case asINIT_GLOBAL_VARS_FAILED:
		return "Initialization of global variables failed"sv;
	case asOUT_OF_MEMORY:
		return "Out of memory"sv;
	case asMODULE_IS_IN_USE:
		return "Module is in use"sv;

		// No default case so new constants added in the future will show more easily
	}

	return "Unknown return code"sv;
}

std::string_view ContextStateToString(int code)
{
	switch (code)
	{
	case asEXECUTION_FINISHED:
		return "Finished"sv;
	case asEXECUTION_SUSPENDED:
		return "Suspended"sv;
	case asEXECUTION_ABORTED:
		return "Aborted"sv;
	case asEXECUTION_EXCEPTION:
		return "Exception"sv;
	case asEXECUTION_PREPARED:
		return "Prepared"sv;
	case asEXECUTION_UNINITIALIZED:
		return "Uninitialized"sv;
	case asEXECUTION_ACTIVE:
		return "Active"sv;
	case asEXECUTION_ERROR:
		return "Error"sv;
	}

	return "Unknown context state"sv;
}
}
