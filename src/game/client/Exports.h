#pragma once

#include "Platform.h"
#include "netadr.h"

extern "C" {
// From hl_weapons
void DLLEXPORT HUD_PostRunCmd(local_state_t* from, local_state_t* to, usercmd_t* cmd, int runfuncs, double time, unsigned int random_seed);

// From cdll_int
int DLLEXPORT Initialize(cl_enginefunc_t* pEnginefuncs, int iVersion);
int DLLEXPORT HUD_VidInit(void);
void DLLEXPORT HUD_Init(void);
int DLLEXPORT HUD_Redraw(float flTime, int intermission);
int DLLEXPORT HUD_UpdateClientData(client_data_t* cdata, float flTime);
void DLLEXPORT HUD_Reset(void);
void DLLEXPORT HUD_PlayerMove(playermove_t* ppmove, int server);
void DLLEXPORT HUD_PlayerMoveInit(playermove_t* ppmove);
char DLLEXPORT HUD_PlayerMoveTexture(char* name);
int DLLEXPORT HUD_ConnectionlessPacket(const netadr_t* net_from, const char* args, char* response_buffer, int* response_buffer_size);
int DLLEXPORT HUD_GetHullBounds(int hullnumber, float* mins, float* maxs);
void DLLEXPORT HUD_Frame(double time);
void DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking);
void DLLEXPORT HUD_DirectorMessage(int iSize, void* pbuf);
void DLLEXPORT HUD_ChatInputPosition(int* x, int* y);

// From demo
void DLLEXPORT Demo_ReadBuffer(int size, unsigned char* buffer);

// From entity
int DLLEXPORT HUD_AddEntity(int type, cl_entity_t* ent, const char* modelname);
void DLLEXPORT HUD_CreateEntities(void);
void DLLEXPORT HUD_StudioEvent(const mstudioevent_t* event, const cl_entity_t* entity);
void DLLEXPORT HUD_TxferLocalOverrides(entity_state_t* state, const clientdata_t* client);
void DLLEXPORT HUD_ProcessPlayerState(entity_state_t* dst, const entity_state_t* src);
void DLLEXPORT HUD_TxferPredictionData(entity_state_t* ps, const entity_state_t* pps, clientdata_t* pcd, const clientdata_t* ppcd, weapon_data_t* wd, const weapon_data_t* pwd);
void DLLEXPORT HUD_TempEntUpdate(double frametime, double client_time, double cl_gravity, TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, int (*Callback_AddVisibleEntity)(cl_entity_t* pEntity), void (*Callback_TempEntPlaySound)(TEMPENTITY* pTemp, float damp));
cl_entity_t DLLEXPORT* HUD_GetUserEntity(int index);

// From in_camera
void DLLEXPORT CAM_Think(void);
int DLLEXPORT CL_IsThirdPerson(void);
void DLLEXPORT CL_CameraOffset(float* ofs);

// From input
kbutton_t DLLEXPORT* KB_Find(const char* name);
void DLLEXPORT CL_CreateMove(float frametime, usercmd_t* cmd, int active);
void DLLEXPORT HUD_Shutdown(void);
int DLLEXPORT HUD_Key_Event(int eventcode, int keynum, const char* pszCurrentBinding);

// From inputw32
void DLLEXPORT IN_ActivateMouse(void);
void DLLEXPORT IN_DeactivateMouse(void);
void DLLEXPORT IN_MouseEvent(int mstate);
void DLLEXPORT IN_Accumulate(void);
void DLLEXPORT IN_ClearStates(void);

// From tri
void DLLEXPORT HUD_DrawNormalTriangles(void);
void DLLEXPORT HUD_DrawTransparentTriangles(void);

// From view
void DLLEXPORT V_CalcRefdef(ref_params_t* pparams);

// From GameStudioModelRenderer
int DLLEXPORT HUD_GetStudioModelInterface(int version, r_studio_interface_t** ppinterface, engine_studio_api_t* pstudio);
}
