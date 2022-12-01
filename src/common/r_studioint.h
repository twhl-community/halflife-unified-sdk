#pragma once

struct alight_t;
struct cache_user_t;
struct cl_entity_t;
struct cvar_t;
struct entity_state_t;
struct model_t;
struct player_info_t;

#define STUDIO_INTERFACE_VERSION 1

struct engine_studio_api_t
{
	// Allocate number*size bytes and zero it
	void* (*Mem_Calloc)(int number, size_t size);
	// Check to see if pointer is in the cache
	void* (*Cache_Check)(cache_user_t* c);
	// Load file into cache ( can be swapped out on demand )
	void (*LoadCacheFile)(char* path, cache_user_t* cu);
	// Retrieve model pointer for the named model
	model_t* (*Mod_ForName)(const char* name, int crash_if_missing);
	// Retrieve pointer to studio model data block from a model
	void* (*Mod_Extradata)(model_t* mod);
	// Retrieve indexed model from client side model precache list
	model_t* (*GetModelByIndex)(int index);
	// Get entity that is set for rendering
	cl_entity_t* (*GetCurrentEntity)();
	// Get referenced player_info_t
	player_info_t* (*PlayerInfo)(int index);
	// Get most recently received player state data from network system
	entity_state_t* (*GetPlayerState)(int index);
	// Get viewentity
	cl_entity_t* (*GetViewEntity)();
	// Get current frame count, and last two timestampes on client
	void (*GetTimes)(int* framecount, double* current, double* old);
	// Get a pointer to a cvar by name
	cvar_t* (*GetCvar)(const char* name);
	// Get current render origin and view vectors ( up, right and vpn )
	void (*GetViewInfo)(float* origin, float* upv, float* rightv, float* vpnv);
	// Get sprite model used for applying chrome effect
	model_t* (*GetChromeSprite)();
	// Get model counters so we can incement instrumentation
	void (*GetModelCounters)(int** s, int** a);
	// Get software scaling coefficients
	void (*GetAliasScale)(float* x, float* y);

	// Get bone, light, alias, and rotation matrices
	float**** (*StudioGetBoneTransform)();
	float**** (*StudioGetLightTransform)();
	float*** (*StudioGetAliasTransform)();
	float*** (*StudioGetRotationMatrix)();

	// Set up body part, and get submodel pointers
	void (*StudioSetupModel)(int bodypart, void** ppbodypart, void** ppsubmodel);
	// Check if entity's bbox is in the view frustum
	int (*StudioCheckBBox)();
	// Apply lighting effects to model
	void (*StudioDynamicLight)(cl_entity_t* ent, alight_t* plight);
	void (*StudioEntityLight)(alight_t* plight);
	void (*StudioSetupLighting)(alight_t* plighting);

	// Draw mesh vertices
	void (*StudioDrawPoints)();

	// Draw hulls around bones
	void (*StudioDrawHulls)();
	// Draw bbox around studio models
	void (*StudioDrawAbsBBox)();
	// Draws bones
	void (*StudioDrawBones)();
	// Loads in appropriate texture for model
	void (*StudioSetupSkin)(void* ptexturehdr, int index);
	// Sets up for remapped colors
	void (*StudioSetRemapColors)(int top, int bottom);
	// Set's player model and returns model pointer
	model_t* (*SetupPlayerModel)(int index);
	// Fires any events embedded in animation
	void (*StudioClientEvents)();
	// Retrieve/set forced render effects flags
	int (*GetForceFaceFlags)();
	void (*SetForceFaceFlags)(int flags);
	// Tell engine the value of the studio model header
	void (*StudioSetHeader)(void* header);
	// Tell engine which model_t * is being renderered
	void (*SetRenderModel)(model_t* model);

	// Final state setup and restore for rendering
	void (*SetupRenderer)(int rendermode);
	void (*RestoreRenderer)();

	// Set render origin for applying chrome effect
	void (*SetChromeOrigin)();

	// True if using D3D/OpenGL
	int (*IsHardware)();

	// Only called by hardware interface
	void (*GL_StudioDrawShadow)();
	void (*GL_SetRenderMode)(int mode);

	void (*StudioSetRenderamt)(int iRenderamt); //!!!CZERO added for rendering glass on viewmodels
	void (*StudioSetCullState)(int iCull);
	void (*StudioRenderShadow)(int iSprite, float* p1, float* p2, float* p3, float* p4);
};

struct server_studio_api_t
{
	// Allocate number*size bytes and zero it
	void* (*Mem_Calloc)(int number, size_t size);
	// Check to see if pointer is in the cache
	void* (*Cache_Check)(cache_user_t* c);
	// Load file into cache ( can be swapped out on demand )
	void (*LoadCacheFile)(char* path, cache_user_t* cu);
	// Retrieve pointer to studio model data block from a model
	void* (*Mod_Extradata)(model_t* mod);
};


// client blending
struct r_studio_interface_t
{
	int version;
	int (*StudioDrawModel)(int flags);
	int (*StudioDrawPlayer)(int flags, entity_state_t* pplayer);
};

// server blending
#define SV_BLENDING_INTERFACE_VERSION 1

struct sv_blending_interface_t
{
	int version;

	void (*SV_StudioSetupBones)(model_t* pModel,
		float frame,
		int sequence,
		const Vector angles,
		const Vector origin,
		const byte* pcontroller,
		const byte* pblending,
		int iBone,
		const edict_t* pEdict);
};
