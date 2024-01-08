/***
 *
 *	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include <SDL2/SDL_messagebox.h>

#include "hud.h"
#include "utils/shared_utils.h"
#include "netadr.h"
#include "interface.h"
#include "com_weapons.h"
#include "ClientLibrary.h"
// #include "vgui_schememanager.h"

#include "pm_shared.h"

#include "vgui_int.h"

#include "Platform.h"
#include "Exports.h"

#include "tri.h"
#include "vgui_TeamFortressViewport.h"

#include "particleman.h"

extern bool g_ResetMousePosition;

void CL_LoadParticleMan();
void CL_UnloadParticleMan();

void InitInput();
void EV_HookEvents();
void IN_Commands();

/**
 *	@brief Engine calls this to enumerate player collision hulls, for prediction.
 *	@return 0 if the hullnumber doesn't exist, 1 otherwise.
 */
int DLLEXPORT HUD_GetHullBounds(int hullnumber, float* mins, float* maxs)
{
	return static_cast<int>(PM_GetHullBounds(hullnumber, mins, maxs));
}

/**
 *	@brief Return 1 if the packet is valid.
 *	@param net_from Address of the sender.
 *	@param args Argument string.
 *	@param response_buffer Buffer to write a response message to.
 *	@param[in,out] response_buffer_size Initially holds the maximum size of @p response_buffer.
 *		Set to size of response message or 0 if sending no response.
 */
int DLLEXPORT HUD_ConnectionlessPacket(const netadr_t* net_from, const char* args, char* response_buffer, int* response_buffer_size)
{
	// Parse stuff from args
	// int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

void DLLEXPORT HUD_PlayerMoveInit(playermove_t* ppmove)
{
	PM_Init(ppmove);
}

char DLLEXPORT HUD_PlayerMoveTexture(char* name)
{
	return g_MaterialSystem.FindTextureType(name);
}

void DLLEXPORT HUD_PlayerMove(playermove_t* ppmove, int server)
{
	PM_Move(ppmove, server);
}

static bool CL_InitClient()
{
	HUD_SetupServerEngineInterface();

	EV_HookEvents();
	CL_LoadParticleMan();

	if (!FileSystem_LoadFileSystem())
	{
		return false;
	}

	if (UTIL_IsValveGameDirectory())
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error",
			"This mod has detected that it is being run from a Valve game directory which is not supported\n"
			"Run this mod from its intended location\n\nThe game will now shut down", nullptr);
		return false;
	}

	if (!g_Client.Initialize())
	{
		return false;
	}

	// get tracker interface, if any
	return true;
}

int DLLEXPORT Initialize(cl_enginefunc_t* pEnginefuncs, int iVersion)
{
	gEngfuncs = *pEnginefuncs;

	if (iVersion != CLDLL_INTERFACE_VERSION)
		return 0;

	memcpy(&gEngfuncs, pEnginefuncs, sizeof(cl_enginefunc_t));

	if (!CL_InitClient())
	{
		gEngfuncs.Con_DPrintf("Error initializing client\n");
		gEngfuncs.pfnClientCmd("quit\n");
		return 0;
	}

	return 1;
}

/**
 *	@brief Called whenever the client connects to a server. Reinitializes all the hud variables.
 *	@details The original documentation for this and HUD_Init() were swapped.
 */
int DLLEXPORT HUD_VidInit()
{
	g_Client.VidInit();

	VGui_Startup();

	return 1;
}

/**
 *	@brief Called when the game initializes and whenever the vid_mode is changed so the HUD can reinitialize itself.
 *	@details The original documentation for this and HUD_VidInit() were swapped.
 */
void DLLEXPORT HUD_Init()
{
	g_Client.HudInit();
	InitInput();
	gHUD.Init();
	Scheme_Init();
}

/**
 *	@brief called every screen frame to redraw the HUD.
 */
int DLLEXPORT HUD_Redraw(float time, int intermission)
{
	gHUD.Redraw(time, 0 != intermission);

	return 1;
}

/**
 *	@brief called every time shared client dll/engine data gets changed,
 *	and gives the cdll a chance to modify the data.
 *	@return 1 if anything has been changed, 0 otherwise.
 */

int DLLEXPORT HUD_UpdateClientData(client_data_t* pcldata, float flTime)
{
	IN_Commands();

	return static_cast<int>(gHUD.UpdateClientData(pcldata, flTime));
}

/**
 *	@brief Called at start and end of demos to restore to "non"HUD state.
 *	@details Never called in Steam Half-Life.
 */
void DLLEXPORT HUD_Reset()
{
	gHUD.VidInit();
}

/**
 *	@brief Called by engine every frame that client .dll is loaded
 */
void DLLEXPORT HUD_Frame(double time)
{
	GetClientVoiceMgr()->Frame(time);

	g_Client.RunFrame();
}

/**
 *	@brief Called when a player starts or stops talking.
 */
void DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	GetClientVoiceMgr()->UpdateSpeakerStatus(entindex, 0 != bTalking);
}

/**
 *	@brief Called when a director event message was received
 */
void DLLEXPORT HUD_DirectorMessage(int iSize, void* pbuf)
{
	BufferReader reader{{reinterpret_cast<std::byte*>(pbuf), static_cast<std::size_t>(iSize)}};
	gHUD.m_Spectator.DirectorMessage(reader);
}

void CL_UnloadParticleMan()
{
	g_pParticleMan = nullptr;
}

void CL_LoadParticleMan()
{
	// Now implemented in the client library.
	auto particleManFactory = Sys_GetFactoryThis();

	g_pParticleMan = (IParticleMan*)particleManFactory(PARTICLEMAN_INTERFACE, nullptr);

	if (g_pParticleMan)
	{
		g_pParticleMan->SetUp(&gEngfuncs);
	}
}

/**
 *	@brief Used by Steam Half-Life to load the client dll interface.
 *	If not present, the client dll will be unloaded, global destructors will run, the client dll will be loaded again
 *	and functions will be retrieved using <tt>GetProcAddress/dlsym</tt>.
 */
extern "C" void DLLEXPORT F(void* pv)
{
	cldll_func_t* pcldll_func = (cldll_func_t*)pv;

	cldll_func_t cldll_func =
		{
			Initialize,
			HUD_Init,
			HUD_VidInit,
			HUD_Redraw,
			HUD_UpdateClientData,
			HUD_Reset,
			HUD_PlayerMove,
			HUD_PlayerMoveInit,
			HUD_PlayerMoveTexture,
			IN_ActivateMouse,
			IN_DeactivateMouse,
			IN_MouseEvent,
			IN_ClearStates,
			IN_Accumulate,
			CL_CreateMove,
			CL_IsThirdPerson,
			CL_CameraOffset,
			KB_Find,
			CAM_Think,
			V_CalcRefdef,
			HUD_AddEntity,
			HUD_CreateEntities,
			HUD_DrawNormalTriangles,
			HUD_DrawTransparentTriangles,
			HUD_StudioEvent,
			HUD_PostRunCmd,
			HUD_Shutdown,
			HUD_TxferLocalOverrides,
			HUD_ProcessPlayerState,
			HUD_TxferPredictionData,
			Demo_ReadBuffer,
			HUD_ConnectionlessPacket,
			HUD_GetHullBounds,
			HUD_Frame,
			HUD_Key_Event,
			HUD_TempEntUpdate,
			HUD_GetUserEntity,
			HUD_VoiceStatus,
			HUD_DirectorMessage,
			HUD_GetStudioModelInterface,
			HUD_ChatInputPosition,
		};

	*pcldll_func = cldll_func;
}

#include "cl_dll/IGameClientExports.h"

/**
 *	@brief Exports functions that are used by the gameUI for UI dialogs
 */
class CClientExports : public IGameClientExports
{
public:
	/**
	 *	@brief returns the name of the server the user is connected to, if any
	 *	@details Does not appear to be called in Steam Half-Life
	 */
	const char* GetServerHostName() override
	{
		/*if (gViewPortInterface)
		{
			return gViewPortInterface->GetServerName();
		}*/
		return "";
	}

	// ingame voice manipulation
	bool IsPlayerGameVoiceMuted(int playerIndex) override
	{
		if (GetClientVoiceMgr())
			return GetClientVoiceMgr()->IsPlayerBlocked(playerIndex);
		return false;
	}

	void MutePlayerGameVoice(int playerIndex) override
	{
		if (GetClientVoiceMgr())
		{
			GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, true);
		}
	}

	void UnmutePlayerGameVoice(int playerIndex) override
	{
		if (GetClientVoiceMgr())
		{
			GetClientVoiceMgr()->SetPlayerBlockedState(playerIndex, false);
		}
	}
};

EXPOSE_SINGLE_INTERFACE(CClientExports, IGameClientExports, GAMECLIENTEXPORTS_INTERFACE_VERSION);
