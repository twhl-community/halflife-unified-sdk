#pragma once

#include "Platform.h"
#include "netadr.h"
#include "Sequence.h"
#include "common_types.h"

struct cl_enginefunc_t;
struct cl_entity_t;
struct client_data_t;
struct client_sprite_t;
struct client_textmessage_t;
struct clientdata_t;
struct con_nprint_t;
struct cvar_t;
struct demo_api_t;
struct efx_api_t;
struct engine_studio_api_t;
struct entity_state_t;
struct event_api_t;
struct event_args_t;
struct hud_player_info_t;
struct IVoiceTweak;
struct kbutton_t;
struct local_state_t;
struct model_t;
struct mstudioevent_t;
struct net_api_t;
struct playermove_t;
struct pmtrace_t;
struct r_studio_interface_t;
struct ref_params_t;
struct screenfade_t;
struct SCREENINFO;
struct TEMPENTITY;
struct triangleapi_t;
struct usercmd_t;
struct weapondata_t;

#define MAX_ALIAS_NAME 32

struct cmdalias_t
{
	cmdalias_t* next;
	char name[MAX_ALIAS_NAME];
	char* value;
};

/**
 *	@brief Functions exported by the client .dll
 */
struct cldll_func_t
{
	int (*pInitFunc)(cl_enginefunc_t*, int);
	void (*pHudInitFunc)();
	int (*pHudVidInitFunc)();
	int (*pHudRedrawFunc)(float, int);
	int (*pHudUpdateClientDataFunc)(client_data_t*, float);
	void (*pHudResetFunc)();
	void (*pClientMove)(playermove_t* ppmove, qboolean server);
	void (*pClientMoveInit)(playermove_t* ppmove);
	char (*pClientTextureType)(char* name);
	void (*pIN_ActivateMouse)();
	void (*pIN_DeactivateMouse)();
	void (*pIN_MouseEvent)(int mstate);
	void (*pIN_ClearStates)();
	void (*pIN_Accumulate)();
	void (*pCL_CreateMove)(float frametime, usercmd_t* cmd, int active);
	int (*pCL_IsThirdPerson)();
	void (*pCL_GetCameraOffsets)(float* ofs);
	kbutton_t* (*pFindKey)(const char* name);
	void (*pCamThink)();
	void (*pCalcRefdef)(ref_params_t* pparams);
	int (*pAddEntity)(int type, cl_entity_t* ent, const char* modelname);
	void (*pCreateEntities)();
	void (*pDrawNormalTriangles)();
	void (*pDrawTransparentTriangles)();
	void (*pStudioEvent)(const mstudioevent_t* event, const cl_entity_t* entity);
	void (*pPostRunCmd)(local_state_t* from, local_state_t* to, usercmd_t* cmd, int runfuncs, double time, unsigned int random_seed);
	void (*pShutdown)();
	void (*pTxferLocalOverrides)(entity_state_t* state, const clientdata_t* client);
	void (*pProcessPlayerState)(entity_state_t* dst, const entity_state_t* src);
	void (*pTxferPredictionData)(entity_state_t* ps, const entity_state_t* pps, clientdata_t* pcd, const clientdata_t* ppcd, weapon_data_t* wd, const weapon_data_t* pwd);
	void (*pReadDemoBuffer)(int size, unsigned char* buffer);
	int (*pConnectionlessPacket)(const netadr_t* net_from, const char* args, char* response_buffer, int* response_buffer_size);
	int (*pGetHullBounds)(int hullnumber, float* mins, float* maxs);
	void (*pHudFrame)(double);
	int (*pKeyEvent)(int eventcode, int keynum, const char* pszCurrentBinding);
	void (*pTempEntUpdate)(double frametime, double client_time, double cl_gravity, TEMPENTITY** ppTempEntFree, TEMPENTITY** ppTempEntActive, int (*Callback_AddVisibleEntity)(cl_entity_t* pEntity), void (*Callback_TempEntPlaySound)(TEMPENTITY* pTemp, float damp));
	cl_entity_t* (*pGetUserEntity)(int index);

	/**
	 *	@details Possibly null on old client dlls.
	 */
	void (*pVoiceStatus)(int entindex, qboolean bTalking);

	/**
	 *	@details Possibly null on old client dlls.
	 */
	void (*pDirectorMessage)(int iSize, void* pbuf);

	/**
	 *	@brief Not used by all clients
	 */
	int (*pStudioInterface)(int version, r_studio_interface_t** ppinterface, engine_studio_api_t* pstudio);

	/**
	 *	@brief Not used by all clients
	 */
	void (*pChatInputPosition)(int* x, int* y);

	/**
	 *	@brief Not used by all clients
	 */
	int (*pGetPlayerTeam)(int iplayer);

	/**
	 *	@brief this should be CreateInterfaceFn but that means including interface.h
	 *	which is a C++ file and some of the client files a C only...
	 *	so we return a void * which we then do a typecast on later.
	 */
	void* (*pClientFactory)();
};

/**
 *	@brief Functions exported by the engine
 */
struct cl_enginefunc_t
{
	HSPRITE (*pfnSPR_Load)(const char* szPicName);
	int (*pfnSPR_Frames)(HSPRITE hPic);
	int (*pfnSPR_Height)(HSPRITE hPic, int frame);
	int (*pfnSPR_Width)(HSPRITE hPic, int frame);
	void (*pfnSPR_Set)(HSPRITE hPic, int r, int g, int b);
	void (*pfnSPR_Draw)(int frame, int x, int y, const Rect* prc);
	void (*pfnSPR_DrawHoles)(int frame, int x, int y, const Rect* prc);
	void (*pfnSPR_DrawAdditive)(int frame, int x, int y, const Rect* prc);
	void (*pfnSPR_EnableScissor)(int x, int y, int width, int height);
	void (*pfnSPR_DisableScissor)();
	[[deprecated("Use g_HudSpriteConfig instead")]] client_sprite_t* (*pfnSPR_GetList)(const char* psz, int* piCount);
	void (*pfnFillRGBA)(int x, int y, int width, int height, int r, int g, int b, int a);
	int (*pfnGetScreenInfo)(SCREENINFO* pscrinfo);
	void (*pfnSetCrosshair)(HSPRITE hspr, Rect rc, int r, int g, int b);
	cvar_t* (*pfnRegisterVariable)(const char* szName, const char* szValue, int flags);
	float (*pfnGetCvarFloat)(const char* szName);
	const char* (*pfnGetCvarString)(const char* szName);
	int (*pfnAddCommand)(const char* cmd_name, void (*pfnEngSrc_function)());
	int (*pfnHookUserMsg)(const char* szMsgName, pfnUserMsgHook pfn);
	int (*pfnServerCmd)(const char* szCmdString);
	int (*pfnClientCmd)(const char* szCmdString);
	void (*pfnGetPlayerInfo)(int ent_num, hud_player_info_t* pinfo);
	void (*pfnPlaySoundByName)(const char* szSound, float volume);
	void (*pfnPlaySoundByIndex)(int iSound, float volume);
	void (*pfnAngleVectors)(const float* vecAngles, float* forward, float* right, float* up);
	client_textmessage_t* (*pfnTextMessageGet)(const char* pName);
	int (*pfnDrawCharacter)(int x, int y, int number, int r, int g, int b);
	int (*pfnDrawConsoleString)(int x, int y, const char* string);
	void (*pfnDrawSetTextColor)(float r, float g, float b);
	void (*pfnDrawConsoleStringLen)(const char* string, int* length, int* height);
	void (*pfnConsolePrint)(const char* string);
	void (*pfnCenterPrint)(const char* string);
	int (*GetWindowCenterX)();
	int (*GetWindowCenterY)();
	void (*GetViewAngles)(float*);
	void (*SetViewAngles)(float*);
	int (*GetMaxClients)();
	void (*Cvar_SetValue)(const char* cvar, float value);
	int (*Cmd_Argc)();
	const char* (*Cmd_Argv)(int arg);
	void (*Con_Printf)(const char* fmt, ...);
	void (*Con_DPrintf)(const char* fmt, ...);
	void (*Con_NPrintf)(int pos, const char* fmt, ...);
	void (*Con_NXPrintf)(con_nprint_t* info, const char* fmt, ...);
	const char* (*PhysInfo_ValueForKey)(const char* key);
	const char* (*ServerInfo_ValueForKey)(const char* key);
	float (*GetClientMaxspeed)();
	int (*CheckParm)(const char* parm, const char** ppnext);
	void (*Key_Event)(int key, int down);
	void (*GetMousePosition)(int* mx, int* my);
	int (*IsNoClipping)();
	cl_entity_t* (*GetLocalPlayer)();
	cl_entity_t* (*GetViewModel)();
	cl_entity_t* (*GetEntityByIndex)(int idx);
	float (*GetClientTime)();
	void (*V_CalcShake)();
	void (*V_ApplyShake)(float* origin, float* angles, float factor);
	int (*PM_PointContents)(float* point, int* truecontents);
	int (*PM_WaterEntity)(float* p);
	pmtrace_t* (*PM_TraceLine)(const float* start, const float* end, int flags, int usehull, int ignore_pe);
	model_t* (*CL_LoadModel)(const char* modelname, int* index);
	int (*CL_CreateVisibleEntity)(int type, cl_entity_t* ent);
	const model_t* (*GetSpritePointer)(HSPRITE hSprite);
	void (*pfnPlaySoundByNameAtLocation)(const char* szSound, float volume, const float* origin);
	unsigned short (*pfnPrecacheEvent)(int type, const char* psz);
	void (*pfnPlaybackEvent)(int flags, const edict_t* pInvoker, unsigned short eventindex, float delay, const float* origin, const float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
	void (*pfnWeaponAnim)(int iAnim, int body);
	float (*pfnRandomFloat)(float flLow, float flHigh);
	int32 (*pfnRandomLong)(int32 lLow, int32 lHigh);
	void (*pfnHookEvent)(const char* name, void (*pfnEvent)(event_args_t* args));
	int (*Con_IsVisible)();
	const char* (*pfnGetGameDirectory)();
	cvar_t* (*pfnGetCvarPointer)(const char* szName);
	const char* (*Key_LookupBinding)(const char* pBinding);
	const char* (*pfnGetLevelName)();
	void (*pfnGetScreenFade)(screenfade_t* fade);
	void (*pfnSetScreenFade)(screenfade_t* fade);
	void* (*VGui_GetPanel)();
	void (*VGui_ViewportPaintBackground)(int extents[4]);
	byte* (*COM_LoadFile)(const char* path, int usehunk, int* pLength);
	char* (*COM_ParseFile)(const char* data, char* token);
	void (*COM_FreeFile)(void* buffer);
	triangleapi_t* pTriAPI;
	efx_api_t* pEfxAPI;
	event_api_t* pEventAPI;
	demo_api_t* pDemoAPI;
	net_api_t* pNetAPI;
	IVoiceTweak* pVoiceTweak;
	int (*IsSpectateOnly)();
	model_t* (*LoadMapSprite)(const char* filename);
	void (*COM_AddAppDirectoryToSearchPath)(const char* pszBaseDir, const char* appName);
	int (*COM_ExpandFilename)(const char* fileName, char* nameOutBuffer, int nameOutBufferSize);
	const char* (*PlayerInfo_ValueForKey)(int playerNum, const char* key);
	void (*PlayerInfo_SetValueForKey)(const char* key, const char* value);
	qboolean (*GetPlayerUniqueID)(int iPlayer, char playerID[16]);
	int (*GetTrackerIDForPlayer)(int playerSlot);
	int (*GetPlayerForTrackerID)(int trackerID);
	int (*pfnServerCmdUnreliable)(char* szCmdString);
	void (*pfnGetMousePos)(Point* ppt);
	void (*pfnSetMousePos)(int x, int y);
	void (*pfnSetMouseEnable)(qboolean fEnable);
	cvar_t* (*GetFirstCvarPtr)();
	unsigned int (*GetFirstCmdFunctionHandle)();
	unsigned int (*GetNextCmdFunctionHandle)(unsigned int cmdhandle);
	const char* (*GetCmdFunctionName)(unsigned int cmdhandle);
	float (*hudGetClientOldTime)();
	float (*hudGetServerGravityValue)();
	model_t* (*hudGetModelByIndex)(int index);
	void (*pfnSetFilterMode)(int mode);
	void (*pfnSetFilterColor)(float r, float g, float b);
	void (*pfnSetFilterBrightness)(float brightness);
	sequenceEntry_s* (*pfnSequenceGet)(const char* fileName, const char* entryName);
	void (*pfnSPR_DrawGeneric)(int frame, int x, int y, const Rect* prc, int src, int dest, int w, int h);
	sentenceEntry_s* (*pfnSequencePickSentence)(const char* sentenceName, int pickMethod, int* entryPicked);
	// draw a complete string
	int (*pfnDrawString)(int x, int y, const char* str, int r, int g, int b);
	int (*pfnDrawStringReverse)(int x, int y, const char* str, int r, int g, int b);
	const char* (*LocalPlayerInfo_ValueForKey)(const char* key);
	int (*pfnVGUI2DrawCharacter)(int x, int y, int ch, unsigned int font);
	int (*pfnVGUI2DrawCharacterAdd)(int x, int y, int ch, int r, int g, int b, unsigned int font);
	unsigned int (*COM_GetApproxWavePlayLength)(const char* filename);
	void* (*pfnGetCareerUI)();
	void (*Cvar_Set)(const char* cvar, const char* value);
	int (*pfnIsCareerMatch)();
	void (*pfnPlaySoundVoiceByName)(const char* szSound, float volume, int pitch);
	void (*pfnPrimeMusicStream)(const char* szFilename, int looping);
	double (*GetAbsoluteTime)();
	void (*pfnProcessTutorMessageDecayBuffer)(int* buffer, int bufferLength);
	void (*pfnConstructTutorMessageDecayBuffer)(int* buffer, int bufferLength);
	void (*pfnResetTutorMessageDecayData)();
	void (*pfnPlaySoundByNameAtPitch)(const char* szSound, float volume, int pitch);
	void (*pfnFillRGBABlend)(int x, int y, int width, int height, int r, int g, int b, int a);
	int (*pfnGetAppID)();
	cmdalias_t* (*pfnGetAliasList)();
	void (*pfnVguiWrap2_GetMouseDelta)(int* x, int* y);
	int (*pfnFilteredClientCmd)(const char* szCmdString);
};
