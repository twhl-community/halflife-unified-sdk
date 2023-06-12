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

#include "Platform.h"

#define INTERFACE_VERSION 140

#include "custom.h"
#include "cvardef.h"
#include "Sequence.h"
#include "netadr.h"

struct clientdata_t;
struct cvar_t;
struct delta_t;
struct entity_state_t;
struct entvars_t;
struct playermove_t;
struct usercmd_t;
struct weapon_data_t;

using CRC32_t = std::uint32_t;

//
// Defines entity interface between engine and DLLs.
// This header file included by engine files and DLL files.
//
// Before including this header, DLLs must:
//		include progdefs.h
// This is conveniently done for them in extdll.h
//

enum ALERT_TYPE
{
	at_notice,
	at_console,	  // same as at_notice, but forces a ConPrintf, not a message box
	at_aiconsole, // same as at_console, but only shown if developer level is 2!
	at_warning,
	at_error,
	at_logged // Server print to console ( only in multiplayer games ).
};

// 4-22-98  JOHN: added for use in pfnClientPrintf
enum PRINT_TYPE
{
	print_console,
	print_center,
	print_chat,
};

// For integrity checking of content on clients
enum FORCE_TYPE
{
	force_exactfile,					// File on client must exactly match server's file
	force_model_samebounds,				// For model files only, the geometry must fit in the same bbox
	force_model_specifybounds,			// For model files only, the geometry must fit in the specified bbox
	force_model_specifybounds_if_avail, // For Steam model files only, the geometry must fit in the specified bbox (if the file is available)
};

// Returned by TraceLine
struct TraceResult
{
	int fAllSolid;	 // if true, plane is not valid
	int fStartSolid; // if true, the initial point was in a solid area
	int fInOpen;
	int fInWater;
	float flFraction; // time completed, 1.0 = didn't hit anything
	Vector vecEndPos; // final position
	float flPlaneDist;
	Vector vecPlaneNormal; // surface normal at impact
	edict_t* pHit;		   // entity the surface is on
	int iHitgroup;		   // 0 == generic, non zero is specific body part
};

/**
 *	@brief Engine hands this to DLLs for functionality callbacks
 */
struct enginefuncs_t
{
	/**
	 *	@brief Precaches a model.
	 *	@details The game will return to the main menu if:
	 *		@li The model does not exist
	 *		@li Too many models (> 512) are precached
	 *		@li The model is precached after the game has started
	 *		@li @p s is a null pointer or has bad text data
	 *	@param s Name of a model to precache. The given string must continue to exist until the next map change or
	 *		until game shutdown.
	 *	@return The index of the model.
	 */
	int (*pfnPrecacheModel)(const char* s);

	/**
	 *	@brief Precaches a sound.
	 *	@details The game will return to the main menu if:
	 *		@li The sound does not exist
	 *		@li Too many sounds (> 512) are precached
	 *		@li The sound is precached after the game has started
	 *		@li @p s is a null pointer or has bad text data
	 *	@param s Name of a sound to precache. The given string must continue to exist until the next map change or
	 *		until game shutdown.
	 *	@return The index of the sound.
	 */
	int (*pfnPrecacheSound)(const char* s);

	/**
	 *	@brief Sets the given entity's model to the given model name.
	 *	@details The entity's bounds will be set to the model's bounds.
	 *		The game will return to the main menu if the model has not been precached.
	 *	@param e Entity whose model is to be set.
	 *	@param m Model to set. The empty string @c "" clears the model.
	 */
	void (*pfnSetModel)(edict_t* e, const char* m);

	/**
	 *	@brief Gets the index of the model with name @p m
	 *	@details The game will shut down if the model has not been precached.
	 */
	int (*pfnModelIndex)(const char* m);

	/**
	 *	@brief For sprite models, returns the number of frames in the sprite.
	 *	@return For studio models, returns the number of submodels in the entire model.
	 *		In all other cases, returns 1.
	 */
	int (*pfnModelFrames)(int modelIndex);

	/**
	 *	@brief Sets the given entity's bounds to the given bounds.
	 *	@details If any of the minimum bounds are &gt;= than the maximum bounds the game will return to the main menu.
	 *	@param e Entity whose bounds are to be set.
	 *	@param rgflMin Minimum bounds.
	 *	@param rgflMax Maximum bounds.
	 */
	void (*pfnSetSize)(edict_t* e, const float* rgflMin, const float* rgflMax);

	/**
	 *	@brief Queues up a level change to @p s1.
	 *	@details If @p s2 is specified the @c changelevel2 command will be used to
	 *		perform a persistent level change that saves entities and transitions them to the next map.
	 *		If @p s2 is null then the @c changelevel command will be used to perform a non-persistent level change.
	 *	@param s1 Name of the level to change to.
	 *	@param s2 If specified, the name of the landmark in the map to connect to.
	 */
	void (*pfnChangeLevel)(const char* s1, const char* s2);

	[[deprecated("Does not do anything")]] void (*pfnGetSpawnParms)(edict_t* ent);
	[[deprecated("Does not do anything")]] void (*pfnSaveSpawnParms)(edict_t* ent);
	[[deprecated("Use VectorToYaw instead")]] float (*pfnVecToYaw)(const float* rgflVector);
	[[deprecated("Use VectorAngles or UTIL_VecToAngles instead")]] void (*pfnVecToAngles)(const float* rgflVectorIn, float* rgflVectorOut);

	/**
	 *	@brief Moves the given entity to the given position, covering at most @p dist units.
	 *	@param ent Entity to move.
	 *	@param pflGoal 3D vector specifying the destination.
	 *	@param dist Maximum distance to cover.
	 *	@param iMoveType Whether to move in current facing direction or direction specified.
	 */
	void (*pfnMoveToOrigin)(edict_t* ent, const float* pflGoal, float dist, int iMoveType);

	[[deprecated("Implemented in CBaseMonster::ChangeYaw")]] void (*pfnChangeYaw)(edict_t* ent);
	[[deprecated("If needed, implement based on CBaseMonster::ChangeYaw")]] void (*pfnChangePitch)(edict_t* ent);

	/**
	 *	@brief Finds an entity by comparing the variable specified by @p pszField.
	 *	@details Only works with ::string_t variables in ::entvars_t.
	 *	@param pEdictStartSearchAfter If not null, start searching entities after this entity in the entity list.
	 *	@param pszField Name of the variable to compare.
	 *	@param pszValue Value to compare against.
	 *	@return If the field is unknown or the value is null, returns null.
	 *		If no entity could be found, returns worldspawn.
	 *		Otherwise, returns the next entity that matches the value.
	 */
	edict_t* (*pfnFindEntityByString)(edict_t* pEdictStartSearchAfter, const char* pszField, const char* pszValue);

	/**
	 *	@brief Returns the entity's illumination level.
	 *	@details Based on client-side calculations, does not work properly on dedicated servers.
	 *	@return Illumination level in the range [0, 255].
	 */
	int (*pfnGetEntityIllum)(edict_t* pEnt);

	/**
	 *	@brief Finds the next entity that is within @p rad units of @p org.
	 *	@param pEdictStartSearchAfter If not null, start searching entities after this entity in the entity list.
	 *	@param org Center of the sphere to search.
	 *	@param rad Radius of the sphere to search.
	 *	@return If no entity could be found, returns worldspawn.
	 *		Otherwise, returns the next entity that matches the value.
	 */
	edict_t* (*pfnFindEntityInSphere)(edict_t* pEdictStartSearchAfter, const float* org, float rad);

	/**
	 *	@brief Finds a client visible in the given entity's Potentially Visible Set.
	 *	@param pEdict Entity whose PVS to check.
	 *	@return If a client could be found in the PVS, returns that client. Otherwise, returns worldspawn.
	 */
	edict_t* (*pfnFindClientInPVS)(edict_t* pEdict);

	/**
	 *	@brief Finds all entities visible to the player using a PVS check.
	 *	The list of entities is stored in entvars_t::chain.
	 *	@return If at least one entity was found, the first entity in the list.
	 *		Otherwise returns worldspawn.
	 */
	edict_t* (*pfnEntitiesInPVS)(edict_t* pplayer);

	[[deprecated("Use UTIL_MakeVectors instead")]] void (*pfnMakeVectors)(const float* rgflVector);
	[[deprecated("Use AngleVectors instead")]] void (*pfnAngleVectors)(const float* rgflVector, float* forward, float* right, float* up);
	edict_t* (*pfnCreateEntity)();

	/**
	 *	@brief Removes the given entity, clearing the slot for use.
	 */
	void (*pfnRemoveEntity)(edict_t* e);

	[[deprecated("Use CBaseEntity::Create instead")]] edict_t* (*pfnCreateNamedEntity)(string_t_value className);
	[[deprecated("Not implemented in the engine, does not work")]] void (*pfnMakeStatic)(edict_t* ent);

	/**
	 *	@brief Returns whether the entity is on the floor.
	 */
	int (*pfnEntIsOnFloor)(edict_t* e);

	/**
	 *	@brief Tries to drop the entity to the floor.
	 *	@return
	 *		@li @c -1 If the entity is stuck in geometry
	 *		@li @c 0 If no floor could be found within 256 units below the entity
	 *		@li @c 1 If the entity was dropped to the floor
	 */
	int (*pfnDropToFloor)(edict_t* e);

	/**
	 *	@brief Tries to make the given entity walk in the given direction for the given distance.
	 *	@param ent Entity to move.
	 *	@param yaw Yaw angle, direction to move in.
	 *	@param dist Distance to move.
	 *	@param iMode Movement mode.
	 */
	int (*pfnWalkMove)(edict_t* ent, float yaw, float dist, int iMode);

	/**
	 *	@brief Sets the origin of the given entity to the given position.
	 *	@details Updates the entity's link information, used by physics.
	 */
	void (*pfnSetOrigin)(edict_t* e, const float* rgflOrigin);

	/**
	 *	@brief Plays a sound on the given entity's channel.
	 *	@param entity Entity to play the sound for.
	 *	@param channel Channel to play the sound on.
	 *	@param sample Name of the sound to play.
	 *	@param volume Sound volume in the range [0, 1].
	 *	@param attenuation Sound attenuation (range)
	 *	@param fFlags Sound flags.
	 *	@param pitch Sound pitch in the range [1, 255].
	 */
	void (*pfnEmitSound)(edict_t* entity, int channel, const char* sample, float volume, float attenuation, int fFlags, int pitch);

	/**
	 *	@brief Plays a sound on ::CHAN_STATIC for the given entity.
	 *	@see pfnEmitSound()
	 */
	void (*pfnEmitAmbientSound)(edict_t* entity, const float* pos, const char* samp, float vol, float attenuation, int fFlags, int pitch);

	/**
	 *	@brief Performs a trace between a starting and ending position.
	 *	@param v1 Start position.
	 *	@param v2 End position.
	 *	@param fNoMonsters Bit vector containing trace flags.
	 *	@param pentToSkip Entity to ignore during the trace. If set to worldspawn, nothing will be ignored.
	 *	@param ptr TraceResult instance that will contain the result of the trace.
	 */
	void (*pfnTraceLine)(const float* v1, const float* v2, int fNoMonsters, edict_t* pentToSkip, TraceResult* ptr);

	/**
	 *	@brief Traces a toss.
	 *	This simulates tossing the entity using its current origin, velocity, angular velocity, angles and gravity.
	 *	Note that this does not use the same code as MOVETYPE_TOSS, and may return different results.
	 *	@param pEntity Entity to toss.
	 *	@param pentToIgnore Entity to ignore during the trace. If set to worldspawn, nothing will be ignored.
	 *	@param ptr TraceResult instance that will contain the result of the trace.
	 */
	void (*pfnTraceToss)(edict_t* pent, edict_t* pentToIgnore, TraceResult* ptr);

	/**
	 *	@brief Performs a trace between a starting and ending position, using the given entity's mins and maxs.
	 *	This can be any entity, not just monsters.
	 *	@param pEntity Entity whose hull will be used.
	 *	@param v1 Start position.
	 *	@param v2 End position.
	 *	@param fNoMonsters Bit vector containing trace flags. @see TraceLineFlag
	 *	@param pentToSkip Entity to ignore during the trace. If set to worldspawn, nothing will be ignored.
	 *	@param ptr TraceResult instance that will contain the result of the trace.
	 *	@return true if the trace was entirely in a solid object, or if it hit something.
	 */
	int (*pfnTraceMonsterHull)(edict_t* pEdict, const float* v1, const float* v2, int fNoMonsters, edict_t* pentToSkip, TraceResult* ptr);

	/**
	 *	@brief Performs a trace between a starting and ending position, using the specified hull.
	 *	@param v1 Start position.
	 *	@param v2 End position.
	 *	@param fNoMonsters Bit vector containing trace flags. @see TraceLineFlag
	 *	@param hullNumber Hull to use.
	 *	@param pentToSkip Entity to ignore during the trace. If set to worldspawn, nothing will be ignored.
	 *	@param ptr TraceResult instance that will contain the result of the trace.
	 */
	void (*pfnTraceHull)(const float* v1, const float* v2, int fNoMonsters, int hullNumber, edict_t* pentToSkip, TraceResult* ptr);

	/**
	 *	@brief Performs a trace between a starting and ending position.
	 *	@details Similar to TraceHull, but will instead perform a trace in the given world hull using the given entity's model's hulls.
	 *	For studio models this will use the model's hitboxes.
	 *
	 *	If the given entity's model is a studio model, uses its hitboxes.
	 *	If it's a brush model, the brush model's hull for the given hull number is used (this may differ if custom brush hull sizes are in use).
	 *	Otherwise, the entity bounds are converted into a hull.
	 *
	 *	@param v1 Start position.
	 *	@param v2 End position.
	 *	@param hullNumber Hull to use.
	 *	@param pEntity Entity whose hull will be used.
	 *	@param ptr TraceResult instance that will contain the result of the trace.
	 */
	void (*pfnTraceModel)(const float* v1, const float* v2, int hullNumber, edict_t* pent, TraceResult* ptr);

	/**
	 *	@brief Used to get texture info.
	 *	@details The given entity must have a brush model set.
	 *	If the traceline intersects the model, the texture of the surface it intersected is returned.
	 *	Otherwise, returns null.
	 *	@param pTextureEntity Entity whose texture is to be retrieved.
	 *	@param v1 Start position.
	 *	@param v2 End position.
	 *	@return Name of the texture or null.
	 */
	const char* (*pfnTraceTexture)(edict_t* pTextureEntity, const float* v1, const float* v2);

	[[deprecated("Not implemented shuts the game down with a fatal error")]] void (*pfnTraceSphere)(const float* v1, const float* v2, int fNoMonsters, float radius, edict_t* pentToSkip, TraceResult* ptr);

	/**
	 *	@brief Get the aim vector for the given entity
	 *	Assumes MakeVectors was called with ent->v.angles beforehand.
	 *
	 *	@details The aim vector is the autoaim vector used when sv_aim is enabled.
	 *	It will snap to entities that are close to the entity's forward vector axis.
	 *	@param ent Entity to retrieve the aim vector for.
	 *	@param speed Not used.
	 *	@param rgflReturn The resulting aim vector.
	 */
	void (*pfnGetAimVector)(edict_t* ent, float speed, float* rgflReturn);

	/**
	 *	@brief Issues a command to the server.
	 *	@details The command must end with either a newline ('\n') or a semicolon (';') in order to be considered valid by the engine.
	 *	The command will be enqueued for execution at a later point.
	 *	@param str Command to execute.
	 */
	void (*pfnServerCommand)(const char* str);

	/**
	 *	@brief Executes all pending server commands.
	 *	@details Note: if a changelevel command is in the buffer, this can result in the caller being freed before this call returns.
	 */
	void (*pfnServerExecute)();

	/**
	 *	@brief Sends a client command to the given client.
	 *	@param pEdict Edict of the client that should execute the command.
	 *	@param szFmt Format string.
	 *	@param ... Format arguments.
	 */
	void (*pfnClientCommand)(edict_t* pEdict, const char* szFmt, ...);

	/**
	 *	@brief Creates a particle effect.
	 *	@param org Origin in the world.
	 *	@param dir Direction of the effect.
	 *	@param color Color of the effect.
	 *	@param count Number of particles to create.
	 */
	void (*pfnParticleEffect)(const float* org, const float* dir, float color, float count);

	/**
	 *	@brief Sets the given light style to the given value.
	 *	@param style Style index.
	 *	@param val Value to set. This string must live for at least as long as the map itself.
	 */
	void (*pfnLightStyle)(int style, const char* val);

	/**
	 *	@brief Gets the index of the given decal.
	 *	@param pszName Name of the decal.
	 *	@return Index of the decal, or -1 if the decal couldn't be found.
	 */
	int (*pfnDecalIndex)(const char* name);

	/**
	 *	@brief Gets the contents of the given location in the world.
	 *	@param rgflVector Location in the world.
	 *	@return Contents of the location in the world.
	 */
	int (*pfnPointContents)(const float* rgflVector);

	/**
	 *	@brief Begins a new network message.
	 *	@details If the message type is to one client, and no client is provided, triggers a sys error.
	 *	If the message type is to all clients, and a client is provided, triggers a sys error.
	 *	If another message had already been started and was not ended, triggers a sys error.
	 *	If an invalid message ID is provided, triggers a sys error.
	 *	@param msg_dest Message type.
	 *	@param msg_type Message ID.
	 *	@param pOrigin Optional. Origin to use for PVS and PAS checks.
	 *	@param ed Optional. If it's a message to one client, client to send the message to.
	 */
	void (*pfnMessageBegin)(int msg_dest, int msg_type, const float* pOrigin, edict_t* ed);

	/**
	 *	@brief Ends a network message.
	 *	@details If no message had been started, triggers a sys error.
	 *	If the buffer had overflowed, triggers a sys error.
	 *	If the message is a user message, and exceeds 192 bytes, triggers a host error.
	 *	If the message has a fixed size and the wrong size was written, triggers a sys error.
	 *	If the given client is invalid, triggers a host error.
	 */
	void (*pfnMessageEnd)();

	/**
	 *	@brief Writes a single unsigned byte.
	 *	@details If no message had been started, triggers a sys error.
	 */
	void (*pfnWriteByte)(int iValue);

	/**
	 *	@brief Writes a single character.
	 *	@details If no message had been started, triggers a sys error.
	 */
	void (*pfnWriteChar)(int iValue);

	/**
	 *	@brief Writes a single unsigned short.
	 *	@details If no message had been started, triggers a sys error.
	 */
	void (*pfnWriteShort)(int iValue);

	/**
	 *	@brief Writes a single unsigned int.
	 *	@details If no message had been started, triggers a sys error.
	 */
	void (*pfnWriteLong)(int iValue);

	/**
	 *	@brief Writes a single angle value.
	 *	@details If no message had been started, triggers a sys error.
	 */
	void (*pfnWriteAngle)(float flValue);

	/**
	 *	@brief Writes a single coordinate value.
	 *	@details If no message had been started, triggers a sys error.
	 */
	void (*pfnWriteCoord)(float flValue);

	/**
	 *	@brief Writes a single null terminated string.
	 *	@details If no message had been started, triggers a sys error.
	 */
	void (*pfnWriteString)(const char* sz);

	/**
	 *	@brief Writes a single character.
	 *	@details If no message had been started, triggers a sys error.
	 */
	void (*pfnWriteEntity)(int iValue);

	/**
	 *	@brief Registers a cvar.
	 *	@details Sets the flag FCVAR_EXTDLL on the cvar.
	 *	@param pCvar Cvar to register.
	 */
	void (*pfnCVarRegister)(cvar_t* pCvar);

	/**
	 *	@brief Gets the value of a cvar as a float.
	 *	@param szVarName Name of the cvar whose value is to be retrieved.
	 *	@return Value of the cvar, or 0 if the cvar doesn't exist.
	 */
	float (*pfnCVarGetFloat)(const char* szVarName);

	/**
	 *	@brief Gets the value of a cvar as a string.
	 *	@param szVarName Name of the cvar whose value is to be retrieved.
	 *	@return Value of the cvar, or an empty string if the cvar doesn't exist.
	 */
	const char* (*pfnCVarGetString)(const char* szVarName);

	/**
	 *	@brief Sets the value of a cvar as a float.
	 *	@param szVarName Name of the cvar whose value to set.
	 *	@param flValue Value to set.
	 */
	void (*pfnCVarSetFloat)(const char* szVarName, float flValue);

	/**
	 *	@brief Sets the value of a cvar as a string.
	 *	@param szVarName Name of the cvar whose value to set.
	 *	@param szValue Value to set. The string is copied.
	 */
	void (*pfnCVarSetString)(const char* szVarName, const char* szValue);

	/**
	 *	@brief Outputs a message to the server console.
	 *	@details If @p atype is at_logged and this is a multiplayer game, logs the message to the log file.
	 *	Otherwise, if the developer cvar is not 0, outputs the message to the console.
	 *	@param atype Type of message.
	 *	@param szFmt Format string.
	 *	@param ... Format arguments.
	 */
	void (*pfnAlertMessage)(ALERT_TYPE atype, const char* szFmt, ...);

	[[deprecated("Does not work, only prints an error message")]] void (*pfnEngineFprintf)(void* pfile, const char* szFmt, ...);
	[[deprecated("Local memory allocations are used instead")]] void* (*pfnPvAllocEntPrivateData)(edict_t* pEdict, int32 cb);
	[[deprecated("Local memory allocations are used instead")]] void* (*pfnPvEntPrivateData)(edict_t* pEdict);
	[[deprecated("Local memory allocations are used instead")]] void (*pfnFreeEntPrivateData)(edict_t* pEdict);
	[[deprecated("Use STRING() instead")]] const char* (*pfnSzFromIndex)(int iString);
	[[deprecated("Use ALLOC_STRING() instead")]] int (*pfnAllocString)(const char* szValue);
	[[deprecated("Use &pEdict->v instead")]] entvars_t* (*pfnGetVarsOfEnt)(edict_t* pEdict);

	/**
	 *	@brief Still called by UTIL_GetEntityList so it can't be marked deprecated, but don't use this anywhere else.
	 */
	edict_t* (*pfnPEntityOfEntOffset)(int iEntOffset);

	[[deprecated("Do not use entity offsets. Use entity indices or handles instead")]] int (*pfnEntOffsetOfPEntity)(const edict_t* pEdict);

	/**
	 *	@brief Gets the entity index of the edict.
	 *	@param pEdict Edict whose entity index is to be retrieved.
	 *	@return	If pEdict is null, returns 0.
	 *			If pEdict is not managed by the engine, triggers a sys error.
	 *			Otherwise, returns the entity index.
	 */
	int (*pfnIndexOfEdict)(const edict_t* pEdict);

	/**
	 *	@brief Gets the edict at the given entity index.
	 *	@details Has an off-by-one bug involving players.
	 *	@param iEntIndex Entity index.
	 *	@return	If the given index is not valid, returns null.
	 *			Otherwise, if the entity at the given index is not in use, returns null.
	 *			Otherwise, if the entity at the given index is not a player and does not have a CBaseEntity instance, returns null.
	 *			Otherwise, returns the entity.
	 *	@see pfnPEntityOfEntIndexAllEntities
	 */
	edict_t* (*pfnPEntityOfEntIndex)(int iEntIndex);

	/**
	 *	@brief Gets the edict of an entvars.
	 *	@details This will enumerate all entities, so this operation can be very expensive.
	 *	Use pvars->pContainingEntity if possible.
	 *	@param pvars Entvars.
	 *	@return Edict.
	 */
	edict_t* (*pfnFindEntityByVars)(entvars_t* pvars);

	/**
	 *	@brief Gets the model pointer of the given entity.
	 *	@param pEdict Entity.
	 *	@return	Pointer to the model, or null if the entity doesn't have one.
	 *			Triggers a sys error if the model wasn't loaded and couldn't be loaded.
	 */
	void* (*pfnGetModelPtr)(edict_t* pEdict);

	/**
	 *	@brief Registers a user message.
	 *	@param pszName Name of the message. Maximum length is 12, excluding null terminator. Can be a temporary string.
	 *	@param iSize Size of the message, in bytes. Maximum size is 192 bytes. Specify -1 for variable length messages.
	 *	@return Message ID, or 0 if the message could not be registered.
	 */
	int (*pfnRegUserMsg)(const char* pszName, int iSize);

	[[deprecated("Does nothing")]] void (*pfnAnimationAutomove)(const edict_t* pEdict, float flTime);

	/**
	 *	@brief Gets the bone position and angles for the given entity and bone.
	 *	@details If the given entity is invalid, or does not have a studio model, or the bone index is invalid, will cause invalid accesses to occur.
	 *	@param pEdict Entity whose model should be queried.
	 *	@param iBone Bone index.
	 *	@param[out] rgflOrigin Origin of the bone.
	 *	@param[out] rgflAngles Angles of the bone. Is not set by the engine.
	 */
	void (*pfnGetBonePosition)(const edict_t* pEdict, int iBone, float* rgflOrigin, float* rgflAngles);

	[[deprecated("Use DataMap_FindFunctionAddress instead")]] uint32 (*pfnFunctionFromName)(const char* pName);
	[[deprecated("Use DataMap_FindFunctionName instead")]] const char* (*pfnNameForFunction)(uint32 function);

	/**
	 *	@brief Sends a message to the client console.
	 *	@details print_chat outputs to the console, just as print_console.
	 *	@param pEdict Client to send the message to.
	 *	@param ptype Where to print the message. @see PRINT_TYPE
	 *	@param szMsg Message to send.
	 */
	void (*pfnClientPrintf)(edict_t* pEdict, PRINT_TYPE ptype, const char* szMsg);

	/**
	 *	@brief Sends a message to the server console.
	 *	@details The message is output regardless of the value of the developer cvar.
	 */
	void (*pfnServerPrint)(const char* szMsg);

	/**
	 *	@return String containing all of the command arguments, not including the command name.
	 */
	const char* (*pfnCmd_Args)(); // these 3 added

	/**
	 *	@brief Gets the command argument at the given index.
	 *	@details Argument 0 is the command name.
	 *	@param argc Argument index.
	 *	@return Command argument.
	 */
	const char* (*pfnCmd_Argv)(int argc); // so game DLL can easily

	/**
	 *	@return The number of command arguments. This includes the command name.
	 */
	int (*pfnCmd_Argc)(); // access client 'cmd' strings

	/**
	 *	@brief Gets the attachment origin and angles.
	 *	@details If the entity is null, or does not have a studio model, illegal access will occur.
	 *	@param pEdict Entity whose model will be queried for the attachment data.
	 *	@param iAttachment Attachment index.
	 *	@param[out] rgflOrigin Attachment origin.
	 *	@param[out] rgflAngles Attachment angles. Is not set by the engine.
	 */
	void (*pfnGetAttachment)(const edict_t* pEdict, int iAttachment, float* rgflOrigin, float* rgflAngles);

	/**
	 *	@brief Initializes the CRC instance.
	 */
	void (*pfnCRC32_Init)(CRC32_t* pulCRC);

	/**
	 *	@brief Processes a buffer and updates the CRC.
	 *	@param pulCRC CRC instance.
	 *	@param p Buffer to process.
	 *	@param len Number of bytes in the buffer to process.
	 */
	void (*pfnCRC32_ProcessBuffer)(CRC32_t* pulCRC, const void* p, int len);

	/**
	 *	@brief Processes a single byte.
	 *	@param pulCRC CRC instance.
	 *	@param ch Byte.
	 */
	void (*pfnCRC32_ProcessByte)(CRC32_t* pulCRC, unsigned char ch);

	/**
	 *	@brief Finalizes the CRC instance.
	 *	@param pulCRC CRC instance.
	 *	@return CRC value.
	 */
	CRC32_t (*pfnCRC32_Final)(CRC32_t pulCRC);

	/**
	 *	@brief Generates a random long number in the range [lLow, lHigh].
	 *	@param lLow Lower bound.
	 *	@param lHigh Higher bound.
	 *	@return Random number, or lLow if lHigh is smaller than or equal to lLow.
	 */
	int32 (*pfnRandomLong)(int32 lLow, int32 lHigh);

	/**
	 *	@brief Generates a random float number in the range [flLow, flHigh].
	 *	@param flLow Lower bound.
	 *	@param flHigh Higher bound.
	 *	@return Random number.
	 */
	float (*pfnRandomFloat)(float flLow, float flHigh);

	/**
	 *	@brief Sets the view of a client to the given entity.
	 *	@details If pClient is not a client, triggers a host error.
	 *	Set the view to the client itself to reset it.
	 *	@param pClient Client whose view is to be set.
	 *	@param pViewent Entity to use as the client's viewpoint.
	 */
	void (*pfnSetView)(const edict_t* pClient, const edict_t* pViewent);

	/**
	 *	@brief Used for delta operations that operate in real world time,
	 *	as opposed to game world time (which will advance frame by frame, and can be paused).
	 *	@return The time since the first call to Time.
	 */
	float (*pfnTime)();

	/**
	 *	@brief Sets the angles of the given player's crosshairs to the given settings.
	 *	Set both to 0 to disable.
	 *	@param pClient Client whose crosshair settings should be set.
	 *	@param pitch Pitch.
	 *	@param yaw Yaw.
	 */
	void (*pfnCrosshairAngle)(const edict_t* pClient, float pitch, float yaw);

	[[deprecated("Use FileSystem_LoadFileIntoBuffer instead")]] byte* (*pfnLoadFileForMe)(const char* filename, int* pLength);
	[[deprecated("Use FileSystem_LoadFileIntoBuffer instead")]] void (*pfnFreeFile)(void* buffer);

	/**
	 *	@brief Signals the engine that a section has ended.
	 *	Possible values:
	 *	@li _oem_end_training
	 *	@li _oem_end_logo
	 *	@li _oem_end_demo
	 *	@li null pointer
	 *
	 *	A disconnect command is sent by this call.
	 */
	void (*pfnEndSection)(const char* pszSectionName); // trigger_endsection

	/**
	 *	@brief Compares file times.
	 *	@param filename1 First file to compare.
	 *	@param filename2 Second file to compare.
	 *	@param[out] iCompare If both files are equal, 0.
	 *		If the first file is older, -1.
	 *		If the second file is older, 1.
	 *	@return true if both filenames are non-null, false otherwise.
	 */
	int (*pfnCompareFileTime)(const char* filename1, const char* filename2, int* iCompare);

	/**
	 *	@brief Gets the game directory name.
	 *	@details Prefer ::FileSystem_GetGameDirectory().
	 *	@param[out] szGetGameDir Buffer to store the game directory name in.
	 *		Must be at least ::MAX_PATH_LENGTH bytes large.
	 */
	void (*pfnGetGameDir)(char* szGetGameDir);

	/**
	 *	@brief Registers a Cvar.
	 *	@details Identical to CvarRegister, except it doesn't set the ::FCVAR_EXTDLL flag.
	 */
	void (*pfnCvar_RegisterVariable)(cvar_t* variable);

	[[deprecated("Does not work")]] void (*pfnFadeClientVolume)(const edict_t* pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds);

	/**
	 *	@brief Sets the client's maximum speed value.
	 *	@details Effectively sets pEdict->v.maxspeed.
	 *	@param pEdict Client to set.
	 *	@param fNewMaxspeed Maximum speed value.
	 */
	void (*pfnSetClientMaxspeed)(const edict_t* pEdict, float fNewMaxspeed);

	/**
	 *	@brief Creates a fake client (bot).
	 *	@param netname Name of the client to show.
	 *	@return The fake client, or null if it can't be created.
	 */
	edict_t* (*pfnCreateFakeClient)(const char* netname);

	/**
	 *	@brief Runs player movement for a fake client.
	 *	@param fakeclient client to move. Must be a fake client.
	 *	@param viewangles Client view angles.
	 *	@param forwardmove Velocity X value.
	 *	@param sidemove Velocity Y value.
	 *	@param upmove Velocity Z value.
	 *	@param buttons Buttons that are currently pressed in. Equivalent to player pev.button.
	 *	@param impulse Impulse commands to execute. Equivalent to player pev.impulse.
	 *	@param msec Time between now and previous RunPlayerMove call.
	 */
	void (*pfnRunPlayerMove)(edict_t* fakeclient, const float* viewangles, float forwardmove, float sidemove, float upmove,
		unsigned short buttons, byte impulse, byte msec);

	/**
	 *	@brief Computes the total number of entities currently in existence.
	 *	@details Note: this will calculate the number of entities in real-time. May be expensive if called many times.
	 *	@return Number of entities.
	 */
	int (*pfnNumberOfEntities)();

	/**
	 *	@brief Gets the given client's info key buffer.
	 *	@details Passing in the world gets the serverinfo.
	 *	Passing in null gets the localinfo. Localinfo is not used by the engine itself, only the game.
	 *	Note: this function checks the maxplayers value incorrectly and may crash if the wrong edict gets passed in.
	 *	@param e Client.
	 *	@return Info key buffer.
	 */
	char* (*pfnGetInfoKeyBuffer)(edict_t* e);

	/**
	 *	@brief Gets the value of the given key from the given buffer.
	 *	@param infobuffer Buffer to query.
	 *	@param key Key whose value to retrieve.
	 *	@return The requested value, or an empty string.
	 */
	char* (*pfnInfoKeyValue)(char* infobuffer, const char* key);

	/**
	 *	@brief Sets the value of the given key in the given buffer.
	 *	@details If the given buffer is not the localinfo or serverinfo buffer, triggers a sys error.
	 *	@param infobuffer Buffer to modify.
	 *	@param key Key whose value to set.
	 *	@param value Value to set.
	 */
	void (*pfnSetKeyValue)(char* infobuffer, const char* key, const char* value);

	/**
	 *	@brief Sets the value of the given key in the given buffer.
	 *	@details This only works for client buffers.
	 *	@param clientIndex Entity index of the client.
	 *	@param infobuffer Buffer to modify.
	 *	@param key Key whose value to set.
	 *	@param value Value to set.
	 */
	void (*pfnSetClientKeyValue)(int clientIndex, char* infobuffer, const char* key, const char* value);

	/**
	 *	@brief Checks if the given file exists.
	 *	@param filename Name of the map to check.
	 *	@return true if the map is valid, false otherwise.
	 */
	int (*pfnIsMapValid)(const char* filename);

	/**
	 *	@brief Projects a static decal in the world.
	 *	@param origin Origin in the world to project the decal at.
	 *	@param decalIndex Index of the decal to project.
	 *	@param entityIndex Index of the entity to project the decal onto.
	 *	@param modelIndex Index of the model to project the decal onto.
	 */
	void (*pfnStaticDecal)(const float* origin, int decalIndex, int entityIndex, int modelIndex);

	/**
	 *	@brief Precaches a file for downloading.
	 *	@details The game will return to the main menu if:
	 *		@li The file does not exist
	 *		@li Too many files (> 512) are precached
	 *		@li The file is precached after the game has started
	 *		@li @p s is a null pointer or has bad text data
	 *	@param s Name of a file to precache. The given string must continue to exist until the next map change or
	 *		until game shutdown.
	 *	@return The index of the file in the generic precache list.
	 */
	int (*pfnPrecacheGeneric)(const char* s);

	/**
	 *	@brief Returns the server assigned userid for this player.
	 *	@details Useful for logging frags, etc.
	 *	Returns -1 if the edict couldn't be found in the list of clients.
	 *	@param e Client.
	 *	@return User ID, or -1.
	 */
	int (*pfnGetPlayerUserId)(edict_t* e);

	[[deprecated("Use the new sound system")]] void (*pfnBuildSoundMsg)(edict_t* entity, int channel, const char* sample,
		float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type,
		const float* pOrigin, edict_t* ed);

	/**
	 *	@return Whether this is a dedicated server.
	 */
	int (*pfnIsDedicatedServer)();

	/**
	 *	@brief Gets the pointer to a cvar.
	 *	@param szVarName Name of the cvar to retrieve.
	 *	@return Cvar pointer, or null if the cvar doesn't exist.
	 */
	cvar_t* (*pfnCVarGetPointer)(const char* szVarName);

	[[deprecated("Always returns -1 in Steam HL")]] unsigned int (*pfnGetPlayerWONId)(edict_t* e);

	// YWB 8/1/99 TFF Physics additions
	/**
	 *	@brief Removes a key from the info buffer.
	 *	@param pszInfoBuffer Buffer to modify.
	 *	@param pszKey Key to remove.
	 */
	void (*pfnInfo_RemoveKey)(char* s, const char* key);

	/**
	 *	@brief Gets the given physics keyvalue from the given client's buffer.
	 *	@param pClient Client whose buffer will be queried.
	 *	@param key Key whose value will be retrieved.
	 *	@return The value, or an empty string if the key does not exist.
	 */
	const char* (*pfnGetPhysicsKeyValue)(const edict_t* pClient, const char* key);

	/**
	 *	@brief Sets the given physics keyvalue in the given client's buffer.
	 *	@param pClient Client whose buffer will be modified.
	 *	@param key Key whose value will be set.
	 *	@param value Value to set.
	 */
	void (*pfnSetPhysicsKeyValue)(const edict_t* pClient, const char* key, const char* value);

	/**
	 *	@brief Gets the physics info string for the given client.
	 *	@param pClient whose buffer will be retrieved.
	 *	@return Buffer, or an empty string if the client is invalid.
	 */
	const char* (*pfnGetPhysicsInfoString)(const edict_t* pClient);

	/**
	 *	@brief Precaches an event.
	 *	@details The client has to hook the event using cl_enginefunc_t::pfnHookEvent.
	 *	@param type Should always be 1.
	 *	@param psz Name of the event. Format should be events/\<name\>.sc, including the directory and extension.
	 *	@return Event index. Used with pfnPlaybackEvent
	 *	@see cl_enginefunc_t::pfnHookEvent
	 *	@see pfnPlaybackEvent
	 */
	unsigned short (*pfnPrecacheEvent)(int type, const char* psz);

	/**
	 *	@param flags Event flags.
	 *	@param pInvoker Client that triggered the event.
	 *	@param eventindex Event index. @see pfnPrecacheEvent
	 *	@param delay Delay before the event should be run.
	 *	@param origin If not ::g_vecZero, this is the origin parameter sent to the clients.
	 *	@param angles If not ::g_vecZero, this is the angles parameter sent to the clients.
	 *	@param fparam1 Float parameter 1.
	 *	@param fparam2 Float parameter 2.
	 *	@param iparam1 Integer parameter 1.
	 *	@param iparam2 Integer parameter 2.
	 *	@param bparam1 Boolean parameter 1.
	 *	@param bparam2 Boolean parameter 2.
	 */
	void (*pfnPlaybackEvent)(int flags, const edict_t* pInvoker, unsigned short eventindex, float delay,
		const float* origin, const float* angles,
		float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);

	/**
	 *	@brief Sets the Fat Potentially Visible Set buffer to contain data based on the given origin.
	 *	@return PVS data.
	 */
	unsigned char* (*pfnSetFatPVS)(const float* org);

	/**
	 *	@brief Sets the Fat Potentially Audible Set buffer to contain data based on the given origin.
	 *	@return PAS data.
	 */
	unsigned char* (*pfnSetFatPAS)(const float* org);

	/**
	 *	@brief Checks if the given entity is visible in the given visible set.
	 *	@param entity Entity to check.
	 *	@param pset Buffer detailing the current visible set.
	 *	@return Whether the given entity is visible in the given visible set.
	 */
	int (*pfnCheckVisibility)(const edict_t* entity, unsigned char* pset);

	/**
	 *	@brief Marks the given field in the given list as set.
	 *	@param pFields List of fields.
	 *	@param fieldname Field name.
	 */
	void (*pfnDeltaSetField)(delta_t* pFields, const char* fieldname);

	/**
	 *	@brief Marks the given field in the given list as not set.
	 *	@param pFields List of fields.
	 *	@param fieldname Field name.
	 */
	void (*pfnDeltaUnsetField)(delta_t* pFields, const char* fieldname);

	/**
	 *	@brief Adds a delta encoder.
	 *	@param name Name of the @c delta.lst entry to add the encoder for.
	 *	@param conditionalencode Encoder function.
	 */
	void (*pfnDeltaAddEncoder)(const char* name, void (*conditionalencode)(delta_t* pFields, const unsigned char* from, const unsigned char* to));

	/**
	 *	@return	The client index of the client that is currently being handled by an engine callback.
	 *			Returns -1 if no client is currently being handled.
	 */
	int (*pfnGetCurrentPlayer)();

	/**
	 *	@return true if the given client has cl_lw (weapon prediction) enabled.
	 */
	int (*pfnCanSkipPlayer)(const edict_t* player);

	/**
	 *	@brief Finds the index of a delta field.
	 *	@param pFields List of fields.
	 *	@param fieldname Name of the field to find.
	 *	@return Index of the delta field, or -1 if the field couldn't be found.
	 */
	int (*pfnDeltaFindField)(delta_t* pFields, const char* fieldname);

	/**
	 *	@brief Marks a delta field as set by index.
	 *	@details If the index is invalid, causes illegal access.
	 *	@param pFields List of fields.
	 *	@param fieldNumber Index of the field.
	 */
	void (*pfnDeltaSetFieldByIndex)(delta_t* pFields, int fieldNumber);

	/**
	 *	@brief Marks a delta field as not set by index.
	 *	@details If the index is invalid, causes illegal access.
	 *	@param pFields List of fields.
	 *	@param fieldNumber Index of the field.
	 */
	void (*pfnDeltaUnsetFieldByIndex)(delta_t* pFields, int fieldNumber);

	/**
	 *	@brief Used to filter contents checks.
	 *	@param mask Mask to check.
	 *	@param op Operation to perform during masking.
	 */
	void (*pfnSetGroupMask)(int mask, int op);

	/**
	 *	@brief Creates an instanced baseline. Used to define a baseline for a particular entity type.
	 *	@param classname Name of the entity class.
	 *	@param baseline Baseline to set.
	 */
	int (*pfnCreateInstancedBaseline)(int classname, entity_state_t* baseline);

	/**
	 *	@brief Directly sets a cvar value.
	 *	@param var Cvar.
	 *	@param value Value to set.
	 */
	void (*pfnCvar_DirectSet)(cvar_t* var, const char* value);

	/**
	 *	@brief Forces the client and server to have the same version of the specified file (e.g., a player model).
	 *	@details Calling this has no effect in single player.
	 *	@param type Force type.
	 *	@param mins If not null, the minimum bounds that a model can be.
	 *	@param maxs If not null, the maximum bounds that a model can be.
	 *	@param filename File to verify. This string must live for at least as long as the map itself.
	 */
	void (*pfnForceUnmodified)(FORCE_TYPE type, const float* mins, const float* maxs, const char* filename);

	/**
	 *	@brief Get player statistics.
	 *	@param pClient Client to query.
	 *	@param[out] ping Current ping.
	 *	@param[out] packet_loss Current packet loss, measured in percentage.
	 */
	void (*pfnGetPlayerStats)(const edict_t* pClient, int* ping, int* packet_loss);

	/**
	 *	@brief Adds a server command.
	 *	@param cmd_name Name of the command to add. This string must live for the rest of the server's lifetime.
	 *	@param function Function to invoke when the command is received.
	 */
	void (*pfnAddServerCommand)(const char* cmd_name, void (*function)());

	/**
	 *	@brief Gets whether the given receiver can hear the given sender.
	 *	@param iReceiver Receiver. This is an entity index.
	 *	@param iSender Sender. This is an entity index.
	 *	@return Whether the given receiver can hear the given sender.
	 */
	qboolean (*pfnVoice_GetClientListening)(int iReceiver, int iSender);

	/**
	 *	@brief Sets whether the given receiver can hear the given sender.
	 *	@param iReceiver Receiver. This is an entity index.
	 *	@param iSender Sender. This is an entity index.
	 *	@param bListen Whether the given receiver can hear the given sender.
	 *	@return Whether the setting was changed.
	 */
	qboolean (*pfnVoice_SetClientListening)(int iReceiver, int iSender, qboolean bListen);

	/**
	 *	@brief Gets the player's auth ID (Steam ID formatted as a Steam2 string).
	 *	@param e Client.
	 *	@return The player's auth ID, or an empty string. This points to a temporary buffer, copy the results.
	 */
	const char* (*pfnGetPlayerAuthId)(edict_t* e);

	// PSV: Added for CZ training map
	//	const char *(*pfnKeyNameForBinding)		( const char* pBinding );

	[[deprecated("Sequence files are not used in this SDK")]] sequenceEntry_s* (*pfnSequenceGet)(const char* fileName, const char* entryName);
	[[deprecated("Sequence files are not used in this SDK")]] sentenceEntry_s* (*pfnSequencePickSentence)(const char* groupName, int pickMethod, int* picked);
	[[deprecated("Use g_pFileSystem->Size() instead")]] int (*pfnGetFileSize)(const char* filename);

	/**
	 *	@brief Gets the average wave length in seconds.
	 *	@param filepath Name of the sound.
	 *	@return Length of the sound file, in seconds.
	 */
	unsigned int (*pfnGetApproxWavePlayLen)(const char* filepath);

	// MDC: Added for CZ career-mode
	/**
	 *	@return Whether this is a Condition Zero Career match.
	 */
	int (*pfnIsCareerMatch)();

	/**
	 *	@return Number of characters of the localized string referenced by using @p label.
	 *	@author BGC
	 */
	int (*pfnGetLocalizedStringLength)(const char* label);

	// BGC: added to facilitate persistent storage of tutor message decay values for
	// different career game profiles.  Also needs to persist regardless of mp.dll being
	// destroyed and recreated.
	[[deprecated("Intended for Condition Zero only")]] void (*pfnRegisterTutorMessageShown)(int mid);
	[[deprecated("Intended for Condition Zero only")]] int (*pfnGetTimesTutorMessageShown)(int mid);
	[[deprecated("Intended for Condition Zero only")]] void (*ProcessTutorMessageDecayBuffer)(int* buffer, int bufferLength);
	[[deprecated("Intended for Condition Zero only")]] void (*ConstructTutorMessageDecayBuffer)(int* buffer, int bufferLength);
	[[deprecated("Intended for Condition Zero only")]] void (*ResetTutorMessageDecayData)();

	/**
	 *	@brief Queries the given client for a cvar value.
	 *	@details The response is sent to NEW_DLL_FUNCTIONS::pfnCvarValue
	 *	@param pPlayer Player to query.
	 *	@param cvarName Cvar to query.
	 */
	void (*pfnQueryClientCvarValue)(const edict_t* player, const char* cvarName);

	/**
	 *	@brief Queries the given client for a cvar value.
	 *	@details The response is sent to NEW_DLL_FUNCTIONS::pfnCvarValue2
	 *	@param player Player to query.
	 *	@param cvarName Cvar to query.
	 *	@param requestID Request ID to pass to pfnCvarValue2
	 */
	void (*pfnQueryClientCvarValue2)(const edict_t* player, const char* cvarName, int requestID);

	/**
	 *	@brief Checks if a command line parameter was provided.
	 *	@param pchCmdLineToken Command key to look for.
	 *	@param[out] ppnext Optional. If the key was found, this is set to the value. Otherwise it is set to null.
	 *	@return Key index in the command line buffer, or 0 if it wasn't found.
	 */
	int (*pfnCheckParm)(const char* pchCmdLineToken, const char** ppnext);

	/**
	 *	@brief Gets the edict at the given entity index.
	 *	@param iEntIndex Entity index.
	 *	@return	If the given index is not valid, returns null.
	 *			Otherwise, if the entity at the given index is not in use, returns null.
	 *			Otherwise, if the entity at the given index is not a player and does not have a CBaseEntity instance, returns null.
	 *			Otherwise, returns the entity.
	 */
	edict_t* (*pfnPEntityOfEntIndexAllEntities)(int iEntIndex);
};


// ONLY ADD NEW FUNCTIONS TO THE END OF THIS STRUCT.  INTERFACE VERSION IS FROZEN AT 138

// Passed to pfnKeyValue
struct KeyValueData
{
	const char* szClassName; // in: entity classname
	const char* szKeyName;	 // in: name of key
	const char* szValue;	 // in: value of key
	int32 fHandled;			 // out: DLL sets to true if key-value pair was understood
};


struct LEVELLIST
{
	char mapName[32];
	char landmarkName[32];
	edict_t* pentLandmark;
	Vector vecLandmarkOrigin;
};
#define MAX_LEVEL_CONNECTIONS 16 // These are encoded in the lower 16bits of ENTITYTABLE->flags

struct ENTITYTABLE
{
	int id;		   // Ordinal ID of this entity (used for entity <--> pointer conversions)
	edict_t* pent; // Pointer to the in-game entity

	int location;		// Offset from the base data of this entity
	int size;			// Byte size of this entity's data
	int flags;			// This could be a short -- bit mask of transitions that this entity is in the PVS of
	string_t classname; // entity class name
};

#define FENTTABLE_PLAYER 0x80000000
#define FENTTABLE_REMOVED 0x40000000
#define FENTTABLE_MOVEABLE 0x20000000
#define FENTTABLE_GLOBAL 0x10000000

struct SAVERESTOREDATA
{
	char* pBaseData;							// Start of all entity save data
	char* pCurrentData;							// Current buffer pointer for sequential access
	int size;									// Current data size
	int bufferSize;								// Total space for data
	int tokenSize;								// Size of the linear list of tokens
	int tokenCount;								// Number of elements in the pTokens table
	char** pTokens;								// Hash table of entity strings (sparse)
	int currentIndex;							// Holds a global entity table ID
	int tableCount;								// Number of elements in the entity table
	int connectionCount;						// Number of elements in the levelList[]
	ENTITYTABLE* pTable;						// Array of ENTITYTABLE elements (1 for each entity)
	LEVELLIST levelList[MAX_LEVEL_CONNECTIONS]; // List of connections from this level

	// smooth transition
	int fUseLandmark;
	char szLandmarkName[20];  // landmark we'll spawn near in next level
	Vector vecLandmarkOffset; // for landmark transitions
	float time;
	char szCurrentMapName[32]; // To check global entities
};

/**
 *	@details The Steam version of the engine uses the following types:
 *	FIELD_FLOAT
 *	FIELD_STRING
 *	FIELD_EDICT
 *	FIELD_VECTOR
 *	FIELD_INTEGER
 *	FIELD_CHARACTER
 *	FIELD_TIME
 *	They must remain supported.
 */
enum class ENGINEFIELDTYPE
{
	FIELD_FLOAT = 0,	   // Any floating point value
	FIELD_STRING,		   // A string ID (return from ALLOC_STRING)
	FIELD_DEPRECATED1,	   // An entity offset (EOFFSET). Deprecated, do not use.
	FIELD_CLASSPTR,		   // CBaseEntity *
	FIELD_EHANDLE,		   // Entity handle
	FIELD_DEPRECATED2,	   // EVARS *. Deprecated, do not use.
	FIELD_EDICT,		   // edict_t*. Deprecated, only used by the engine and entvars_t. Do not use.
	FIELD_VECTOR,		   // Any vector
	FIELD_POSITION_VECTOR, // A world coordinate (these are fixed up across level transitions automagically)
	FIELD_POINTER,		   // Arbitrary data pointer... to be removed, use an array of FIELD_CHARACTER
	FIELD_INTEGER,		   // Any integer or enum
	FIELD_FUNCTION,		   // A class function pointer (Think, Use, etc)
	FIELD_BOOLEAN,		   // boolean, implemented as an int, I may use this as a hint for compression
	FIELD_SHORT,		   // 2 byte integer
	FIELD_CHARACTER,	   // a byte
	FIELD_TIME,			   // a floating point time (these are fixed up automatically too!)
	FIELD_MODELNAME,	   // Engine string that is a model name (needs precache)
	FIELD_SOUNDNAME,	   // Engine string that is a sound name (needs precache)

	FIELD_TYPECOUNT, // MUST BE LAST
};

#define FTYPEDESC_GLOBAL 0x0001 // This field is masked for global entity save/restore

struct TYPEDESCRIPTION
{
	ENGINEFIELDTYPE fieldType;
	const char* fieldName;
	int fieldOffset;
	short fieldSize;
	short flags;
};

struct DLL_FUNCTIONS
{
	// Initialize/shutdown the game (one-time call after loading of game .dll )
	void (*pfnGameInit)();
	int (*pfnSpawn)(edict_t* pent);
	void (*pfnThink)(edict_t* pent);
	void (*pfnUse)(edict_t* pentUsed, edict_t* pentOther);
	void (*pfnTouch)(edict_t* pentTouched, edict_t* pentOther);
	void (*pfnBlocked)(edict_t* pentBlocked, edict_t* pentOther);
	void (*pfnKeyValue)(edict_t* pentKeyvalue, KeyValueData* pkvd);
	void (*pfnSave)(edict_t* pent, SAVERESTOREDATA* pSaveData);
	int (*pfnRestore)(edict_t* pent, SAVERESTOREDATA* pSaveData, int globalEntity);
	void (*pfnSetAbsBox)(edict_t* pent);

	void (*pfnSaveWriteFields)(SAVERESTOREDATA*, const char*, void*, TYPEDESCRIPTION*, int);
	void (*pfnSaveReadFields)(SAVERESTOREDATA*, const char*, void*, TYPEDESCRIPTION*, int);

	void (*pfnSaveGlobalState)(SAVERESTOREDATA*);
	void (*pfnRestoreGlobalState)(SAVERESTOREDATA*);
	void (*pfnResetGlobalState)();

	qboolean (*pfnClientConnect)(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]);

	void (*pfnClientDisconnect)(edict_t* pEntity);
	void (*pfnClientKill)(edict_t* pEntity);
	void (*pfnClientPutInServer)(edict_t* pEntity);
	void (*pfnClientCommand)(edict_t* pEntity);
	void (*pfnClientUserInfoChanged)(edict_t* pEntity, char* infobuffer);

	void (*pfnServerActivate)(edict_t* pEdictList, int edictCount, int clientMax);
	void (*pfnServerDeactivate)();

	void (*pfnPlayerPreThink)(edict_t* pEntity);
	void (*pfnPlayerPostThink)(edict_t* pEntity);

	void (*pfnStartFrame)();
	void (*pfnParmsNewLevel)();
	void (*pfnParmsChangeLevel)();

	// Returns string describing current .dll.  E.g., TeamFotrress 2, Half-Life
	const char* (*pfnGetGameDescription)();

	// Notify dll about a player customization.
	void (*pfnPlayerCustomization)(edict_t* pEntity, customization_t* pCustom);

	// Spectator funcs
	void (*pfnSpectatorConnect)(edict_t* pEntity);
	void (*pfnSpectatorDisconnect)(edict_t* pEntity);
	void (*pfnSpectatorThink)(edict_t* pEntity);

	// Notify game .dll that engine is going to shut down.  Allows mod authors to set a breakpoint.
	void (*pfnSys_Error)(const char* error_string);

	void (*pfnPM_Move)(playermove_t* ppmove, qboolean server);
	void (*pfnPM_Init)(playermove_t* ppmove);
	char (*pfnPM_FindTextureType)(const char* name);
	void (*pfnSetupVisibility)(edict_t* pViewEntity, edict_t* pClient, unsigned char** pvs, unsigned char** pas);
	void (*pfnUpdateClientData)(const edict_t* ent, int sendweapons, clientdata_t* cd);
	int (*pfnAddToFullPack)(entity_state_t* state, int e, edict_t* ent, edict_t* host, int hostflags, int player, unsigned char* pSet);
	void (*pfnCreateBaseline)(int player, int eindex, entity_state_t* baseline, edict_t* entity, int playermodelindex, Vector* player_mins, Vector* player_maxs);
	void (*pfnRegisterEncoders)();
	int (*pfnGetWeaponData)(edict_t* player, weapon_data_t* info);

	void (*pfnCmdStart)(const edict_t* player, const usercmd_t* cmd, unsigned int random_seed);
	void (*pfnCmdEnd)(const edict_t* player);

	// Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
	//  size of the response_buffer, so you must zero it out if you choose not to respond.
	int (*pfnConnectionlessPacket)(const netadr_t* net_from, const char* args, char* response_buffer, int* response_buffer_size);

	// Enumerates player hulls.  Returns 0 if the hull number doesn't exist, 1 otherwise
	int (*pfnGetHullBounds)(int hullnumber, float* mins, float* maxs);

	// Create baselines for certain "unplaced" items.
	void (*pfnCreateInstancedBaselines)();

	// One of the pfnForceUnmodified files failed the consistency check for the specified player
	// Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
	int (*pfnInconsistentFile)(const edict_t* player, const char* filename, char* disconnect_message);

	// The game .dll should return 1 if lag compensation should be allowed ( could also just set
	//  the sv_unlag cvar.
	// Most games right now should return 0, until client-side weapon prediction code is written
	//  and tested for them.
	int (*pfnAllowLagCompensation)();
};

// Current version.
#define NEW_DLL_FUNCTIONS_VERSION 1

// Pointers will be null if the game DLL doesn't support this API.
struct NEW_DLL_FUNCTIONS
{
	// Called right before the object's memory is freed.
	// Calls its destructor.
	void (*pfnOnFreeEntPrivateData)(edict_t* pEnt);
	void (*pfnGameShutdown)();
	int (*pfnShouldCollide)(edict_t* pentTouched, edict_t* pentOther);
	void (*pfnCvarValue)(const edict_t* pEnt, const char* value);
	void (*pfnCvarValue2)(const edict_t* pEnt, int requestID, const char* cvarName, const char* value);
};
typedef int (*NEW_DLL_FUNCTIONS_FN)(NEW_DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);

typedef int (*APIFUNCTION)(DLL_FUNCTIONS* pFunctionTable, int interfaceVersion);
typedef int (*APIFUNCTION2)(DLL_FUNCTIONS* pFunctionTable, int* interfaceVersion);
