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

struct clientdata_t;

inline float g_LastPlayerJoinTime;

void respawn(entvars_t* pev, bool fCopyCorpse);
qboolean ClientConnect(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128]);
void ClientDisconnect(edict_t* pEntity);
void ClientKill(edict_t* pEntity);
void ClientPutInServer(edict_t* pEntity);
void SV_CreateClientCommands();
void ExecuteClientCommand(edict_t* pEntity);
void ClientUserInfoChanged(edict_t* pEntity, char* infobuffer);
void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax);
void ServerDeactivate();
void StartFrame();
void PlayerPostThink(edict_t* pEntity);
void PlayerPreThink(edict_t* pEntity);
void ParmsNewLevel();
void ParmsChangeLevel();

void ClientPrecache();

const char* GetGameDescription();
void PlayerCustomization(edict_t* pEntity, customization_t* pCust);

void Sys_Error(const char* error_string);

void SetupVisibility(edict_t* pViewEntity, edict_t* pClient, unsigned char** pvs, unsigned char** pas);
void UpdateClientData(const edict_t* ent, int sendweapons, clientdata_t* cd);
int AddToFullPack(entity_state_t* state, int e, edict_t* ent, edict_t* host, int hostflags, int player, unsigned char* pSet);
void CreateBaseline(int player, int eindex, entity_state_t* baseline, edict_t* entity, int playermodelindex, Vector* player_mins, Vector* player_maxs);
void RegisterEncoders();

int GetWeaponData(edict_t* player, weapon_data_t* info);

void CmdStart(const edict_t* player, const usercmd_t* cmd, unsigned int random_seed);
void CmdEnd(const edict_t* player);

int ConnectionlessPacket(const netadr_t* net_from, const char* args, char* response_buffer, int* response_buffer_size);

int GetHullBounds(int hullnumber, float* mins, float* maxs);

void CreateInstancedBaselines();

int InconsistentFile(const edict_t* player, const char* filename, char* disconnect_message);

int AllowLagCompensation();
