#pragma once

#include "Platform.h"
#include "netadr.h"
#include "Sequence.h"
#include "common_types.h"
#include "enums.h"

#define MAX_ALIAS_NAME 32

typedef struct cmdalias_s
{
	struct cmdalias_s* next;
	char name[MAX_ALIAS_NAME];
	char* value;
} cmdalias_t;

/**
*	@brief Functions exported by the client .dll
*/
typedef struct
{
	int (*pInitFunc)(struct cl_enginefuncs_s*, int);
	void (*pHudInitFunc)(void);
	int (*pHudVidInitFunc)(void);
	int (*pHudRedrawFunc)(float, int);
	int (*pHudUpdateClientDataFunc)(struct client_data_s*, float);
	void (*pHudResetFunc)(void);
	void (*pClientMove)(struct playermove_s* ppmove, qboolean server);
	void (*pClientMoveInit)(struct playermove_s* ppmove);
	char (*pClientTextureType)(char* name);
	void (*pIN_ActivateMouse)(void);
	void (*pIN_DeactivateMouse)(void);
	void (*pIN_MouseEvent)(int mstate);
	void (*pIN_ClearStates)(void);
	void (*pIN_Accumulate)(void);
	void (*pCL_CreateMove)(float frametime, struct usercmd_s* cmd, int active);
	int (*pCL_IsThirdPerson)(void);
	void (*pCL_GetCameraOffsets)(float* ofs);
	struct kbutton_s* (*pFindKey)(const char* name);
	void (*pCamThink)(void);
	void (*pCalcRefdef)(struct ref_params_s* pparams);
	int (*pAddEntity)(int type, struct cl_entity_s* ent, const char* modelname);
	void (*pCreateEntities)(void);
	void (*pDrawNormalTriangles)(void);
	void (*pDrawTransparentTriangles)(void);
	void (*pStudioEvent)(const struct mstudioevent_s* event, const struct cl_entity_s* entity);
	void (*pPostRunCmd)(struct local_state_s* from, struct local_state_s* to, struct usercmd_s* cmd, int runfuncs, double time, unsigned int random_seed);
	void (*pShutdown)(void);
	void (*pTxferLocalOverrides)(struct entity_state_s* state, const struct clientdata_s* client);
	void (*pProcessPlayerState)(struct entity_state_s* dst, const struct entity_state_s* src);
	void (*pTxferPredictionData)(struct entity_state_s* ps, const struct entity_state_s* pps, struct clientdata_s* pcd, const struct clientdata_s* ppcd, struct weapon_data_s* wd, const struct weapon_data_s* pwd);
	void (*pReadDemoBuffer)(int size, unsigned char* buffer);
	int (*pConnectionlessPacket)(const netadr_t* net_from, const char* args, char* response_buffer, int* response_buffer_size);
	int (*pGetHullBounds)(int hullnumber, float* mins, float* maxs);
	void (*pHudFrame)(double);
	int (*pKeyEvent)(int eventcode, int keynum, const char* pszCurrentBinding);
	void (*pTempEntUpdate)(double frametime, double client_time, double cl_gravity, struct tempent_s** ppTempEntFree, struct tempent_s** ppTempEntActive, int (*Callback_AddVisibleEntity)(struct cl_entity_s* pEntity), void (*Callback_TempEntPlaySound)(struct tempent_s* pTemp, float damp));
	struct cl_entity_s* (*pGetUserEntity)(int index);

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
	int (*pStudioInterface)(int version, struct r_studio_interface_s** ppinterface, struct engine_studio_api_s* pstudio);

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
} cldll_func_t;

/**
*	@brief Functions exported by the engine
*/
typedef struct cl_enginefuncs_s
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
	void (*pfnSPR_DisableScissor)(void);
	struct client_sprite_s* (*pfnSPR_GetList)(const char* psz, int* piCount);
	void (*pfnFillRGBA)(int x, int y, int width, int height, int r, int g, int b, int a);
	int (*pfnGetScreenInfo)(struct SCREENINFO_s* pscrinfo);
	void (*pfnSetCrosshair)(HSPRITE hspr, Rect rc, int r, int g, int b);
	struct cvar_s* (*pfnRegisterVariable)(const char* szName, const char* szValue, int flags);
	float (*pfnGetCvarFloat)(const char* szName);
	const char* (*pfnGetCvarString)(const char* szName);
	int (*pfnAddCommand)(const char* cmd_name, void (*pfnEngSrc_function)(void));
	int (*pfnHookUserMsg)(const char* szMsgName, pfnUserMsgHook pfn);
	int (*pfnServerCmd)(const char* szCmdString);
	int (*pfnClientCmd)(const char* szCmdString);
	void (*pfnGetPlayerInfo)(int ent_num, struct hud_player_info_s* pinfo);
	void (*pfnPlaySoundByName)(const char* szSound, float volume);
	void (*pfnPlaySoundByIndex)(int iSound, float volume);
	void (*pfnAngleVectors)(const float* vecAngles, float* forward, float* right, float* up);
	struct client_textmessage_s* (*pfnTextMessageGet)(const char* pName);
	int (*pfnDrawCharacter)(int x, int y, int number, int r, int g, int b);
	int (*pfnDrawConsoleString)(int x, int y, const char* string);
	void (*pfnDrawSetTextColor)(float r, float g, float b);
	void (*pfnDrawConsoleStringLen)(const char* string, int* length, int* height);
	void (*pfnConsolePrint)(const char* string);
	void (*pfnCenterPrint)(const char* string);
	int (*GetWindowCenterX)(void);
	int (*GetWindowCenterY)(void);
	void (*GetViewAngles)(float*);
	void (*SetViewAngles)(float*);
	int (*GetMaxClients)(void);
	void (*Cvar_SetValue)(const char* cvar, float value);
	int (*Cmd_Argc)(void);
	const char* (*Cmd_Argv)(int arg);
	void (*Con_Printf)(const char* fmt, ...);
	void (*Con_DPrintf)(const char* fmt, ...);
	void (*Con_NPrintf)(int pos, const char* fmt, ...);
	void (*Con_NXPrintf)(struct con_nprint_s* info, const char* fmt, ...);
	const char* (*PhysInfo_ValueForKey)(const char* key);
	const char* (*ServerInfo_ValueForKey)(const char* key);
	float (*GetClientMaxspeed)(void);
	int (*CheckParm)(const char* parm, const char** ppnext);
	void (*Key_Event)(int key, int down);
	void (*GetMousePosition)(int* mx, int* my);
	int (*IsNoClipping)(void);
	struct cl_entity_s* (*GetLocalPlayer)(void);
	struct cl_entity_s* (*GetViewModel)(void);
	struct cl_entity_s* (*GetEntityByIndex)(int idx);
	float (*GetClientTime)(void);
	void (*V_CalcShake)(void);
	void (*V_ApplyShake)(float* origin, float* angles, float factor);
	int (*PM_PointContents)(float* point, int* truecontents);
	int (*PM_WaterEntity)(float* p);
	struct pmtrace_s* (*PM_TraceLine)(const float* start, const float* end, int flags, int usehull, int ignore_pe);
	struct model_s* (*CL_LoadModel)(const char* modelname, int* index);
	int (*CL_CreateVisibleEntity)(int type, struct cl_entity_s* ent);
	const struct model_s* (*GetSpritePointer)(HSPRITE hSprite);
	void (*pfnPlaySoundByNameAtLocation)(const char* szSound, float volume, const float* origin);
	unsigned short (*pfnPrecacheEvent)(int type, const char* psz);
	void (*pfnPlaybackEvent)(int flags, const struct edict_s* pInvoker, unsigned short eventindex, float delay, const float* origin, const float* angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2);
	void (*pfnWeaponAnim)(int iAnim, int body);
	float (*pfnRandomFloat)(float flLow, float flHigh);
	int32 (*pfnRandomLong)(int32 lLow, int32 lHigh);
	void (*pfnHookEvent)(const char* name, void (*pfnEvent)(struct event_args_s* args));
	int (*Con_IsVisible)();
	const char* (*pfnGetGameDirectory)(void);
	struct cvar_s* (*pfnGetCvarPointer)(const char* szName);
	const char* (*Key_LookupBinding)(const char* pBinding);
	const char* (*pfnGetLevelName)(void);
	void (*pfnGetScreenFade)(struct screenfade_s* fade);
	void (*pfnSetScreenFade)(struct screenfade_s* fade);
	void* (*VGui_GetPanel)();
	void (*VGui_ViewportPaintBackground)(int extents[4]);
	byte* (*COM_LoadFile)(const char* path, int usehunk, int* pLength);
	char* (*COM_ParseFile)(const char* data, char* token);
	void (*COM_FreeFile)(void* buffer);
	struct triangleapi_s* pTriAPI;
	struct efx_api_s* pEfxAPI;
	struct event_api_s* pEventAPI;
	struct demo_api_s* pDemoAPI;
	struct net_api_s* pNetAPI;
	struct IVoiceTweak_s* pVoiceTweak;
	int (*IsSpectateOnly)(void);
	struct model_s* (*LoadMapSprite)(const char* filename);
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
	struct cvar_s* (*GetFirstCvarPtr)();
	unsigned int (*GetFirstCmdFunctionHandle)();
	unsigned int (*GetNextCmdFunctionHandle)(unsigned int cmdhandle);
	const char* (*GetCmdFunctionName)(unsigned int cmdhandle);
	float (*hudGetClientOldTime)();
	float (*hudGetServerGravityValue)();
	struct model_s* (*hudGetModelByIndex)(int index);
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
	double (*GetAbsoluteTime)(void);
	void (*pfnProcessTutorMessageDecayBuffer)(int* buffer, int bufferLength);
	void (*pfnConstructTutorMessageDecayBuffer)(int* buffer, int bufferLength);
	void (*pfnResetTutorMessageDecayData)();
	void (*pfnPlaySoundByNameAtPitch)(const char* szSound, float volume, int pitch);
	void (*pfnFillRGBABlend)(int x, int y, int width, int height, int r, int g, int b, int a);
	int (*pfnGetAppID)(void);
	cmdalias_t* (*pfnGetAliasList)(void);
	void (*pfnVguiWrap2_GetMouseDelta)(int* x, int* y);
	int (*pfnFilteredClientCmd)(const char* szCmdString);
} cl_enginefunc_t;
