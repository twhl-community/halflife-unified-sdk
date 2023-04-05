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
/**
*	@file
*	precaches and defs for entities and other data that must always be available.
*/

#include "cbase.h"
#include "CCorpse.h"
#include "nodes.h"
#include "client.h"
#include "CHalfLifeCTFplay.h"
#include "spawnpoints.h"
#include "world.h"
#include "ServerLibrary.h"
#include "ctf/ctf_items.h"

/**
*	@details This must match the list in util.h
*/
DLL_DECALLIST gDecals[] = {
	{"{shot1", 0},		 // DECAL_GUNSHOT1
	{"{shot2", 0},		 // DECAL_GUNSHOT2
	{"{shot3", 0},		 // DECAL_GUNSHOT3
	{"{shot4", 0},		 // DECAL_GUNSHOT4
	{"{shot5", 0},		 // DECAL_GUNSHOT5
	{"{lambda01", 0},	 // DECAL_LAMBDA1
	{"{lambda02", 0},	 // DECAL_LAMBDA2
	{"{lambda03", 0},	 // DECAL_LAMBDA3
	{"{lambda04", 0},	 // DECAL_LAMBDA4
	{"{lambda05", 0},	 // DECAL_LAMBDA5
	{"{lambda06", 0},	 // DECAL_LAMBDA6
	{"{scorch1", 0},	 // DECAL_SCORCH1
	{"{scorch2", 0},	 // DECAL_SCORCH2
	{"{blood1", 0},		 // DECAL_BLOOD1
	{"{blood2", 0},		 // DECAL_BLOOD2
	{"{blood3", 0},		 // DECAL_BLOOD3
	{"{blood4", 0},		 // DECAL_BLOOD4
	{"{blood5", 0},		 // DECAL_BLOOD5
	{"{blood6", 0},		 // DECAL_BLOOD6
	{"{yblood1", 0},	 // DECAL_YBLOOD1
	{"{yblood2", 0},	 // DECAL_YBLOOD2
	{"{yblood3", 0},	 // DECAL_YBLOOD3
	{"{yblood4", 0},	 // DECAL_YBLOOD4
	{"{yblood5", 0},	 // DECAL_YBLOOD5
	{"{yblood6", 0},	 // DECAL_YBLOOD6
	{"{break1", 0},		 // DECAL_GLASSBREAK1
	{"{break2", 0},		 // DECAL_GLASSBREAK2
	{"{break3", 0},		 // DECAL_GLASSBREAK3
	{"{bigshot1", 0},	 // DECAL_BIGSHOT1
	{"{bigshot2", 0},	 // DECAL_BIGSHOT2
	{"{bigshot3", 0},	 // DECAL_BIGSHOT3
	{"{bigshot4", 0},	 // DECAL_BIGSHOT4
	{"{bigshot5", 0},	 // DECAL_BIGSHOT5
	{"{spit1", 0},		 // DECAL_SPIT1
	{"{spit2", 0},		 // DECAL_SPIT2
	{"{bproof1", 0},	 // DECAL_BPROOF1
	{"{gargstomp", 0},	 // DECAL_GARGSTOMP1,	// Gargantua stomp crack
	{"{smscorch1", 0},	 // DECAL_SMALLSCORCH1,	// Small scorch mark
	{"{smscorch2", 0},	 // DECAL_SMALLSCORCH2,	// Small scorch mark
	{"{smscorch3", 0},	 // DECAL_SMALLSCORCH3,	// Small scorch mark
	{"{mommablob", 0},	 // DECAL_MOMMABIRTH		// BM Birth spray
	{"{mommablob", 0},	 // DECAL_MOMMASPLAT		// BM Mortar spray?? need decal
	{"{spr_splt1", 0},	 // DECAL_SPR_SPLT1
	{"{spr_splt2", 0},	 // DECAL_SPR_SPLT2
	{"{spr_splt3", 0},	 // DECAL_SPR_SPLT3
	{"{ofscorch1", 0},	 // DECAL_OFSCORCH1
	{"{ofscorch2", 0},	 // DECAL_OFSCORCH2
	{"{ofscorch3", 0},	 // DECAL_OFSCORCH3
	{"{ofsmscorch1", 0}, // DECAL_OFSMSCORCH1
	{"{ofsmscorch2", 0}, // DECAL_OFSMSCORCH2
	{"{ofsmscorch3", 0}, // DECAL_OFSMSCORCH3
};

#define SF_DECAL_NOTINDEATHMATCH 2048

class CDecal : public CBaseEntity
{
	DECLARE_CLASS(CDecal, CBaseEntity);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void StaticDecal();
	void TriggerDecal(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
};

BEGIN_DATAMAP(CDecal)
DEFINE_FUNCTION(StaticDecal),
	DEFINE_FUNCTION(TriggerDecal),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(infodecal, CDecal);

// UNDONE:  These won't get sent to joining players in multi-player
void CDecal::Spawn()
{
	if (pev->skin < 0 || (0 != gpGlobals->deathmatch && FBitSet(pev->spawnflags, SF_DECAL_NOTINDEATHMATCH)))
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	if (FStringNull(pev->targetname))
	{
		SetThink(&CDecal::StaticDecal);
		// if there's no targetname, the decal will spray itself on as soon as the world is done spawning.
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		SetThink(nullptr);
		SetUse(&CDecal::TriggerDecal);
	}
}

void CDecal::TriggerDecal(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// this is set up as a USE function for infodecals that have targetnames, so that the
	// decal doesn't get applied until it is fired. (usually by a scripted sequence)
	TraceResult trace;

	UTIL_TraceLine(pev->origin - Vector(5, 5, 5), pev->origin + Vector(5, 5, 5), ignore_monsters, ENT(pev), &trace);

	auto hit = CBaseEntity::Instance(trace.pHit);

	const int entityIndex = (short)hit->entindex();

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BSPDECAL);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT(pev->skin);
	WRITE_SHORT(entityIndex);
	if (0 != entityIndex)
		WRITE_SHORT(hit->pev->modelindex);
	MESSAGE_END();

	SetThink(&CDecal::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CDecal::StaticDecal()
{
	TraceResult trace;

	UTIL_TraceLine(pev->origin - Vector(5, 5, 5), pev->origin + Vector(5, 5, 5), ignore_monsters, ENT(pev), &trace);

	auto hit = CBaseEntity::Instance(trace.pHit);

	const int entityIndex = (short)hit->entindex();
	const int modelIndex = 0 != entityIndex ? hit->pev->modelindex : 0;

	g_engfuncs.pfnStaticDecal(pev->origin, pev->skin, entityIndex, modelIndex);

	SUB_Remove();
}

bool CDecal::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->skin = DECAL_INDEX(pkvd->szValue);

		if (pev->skin < 0)
		{
			Logger->debug("Can't find decal {}", pkvd->szValue);
		}

		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(worldspawn, CWorld);

CWorld::CWorld()
{
	if (World)
	{
		Logger->error("Do not create multiple instances of worldspawn");
		return;
	}

	g_GameLogger->trace("worldspawn created");

	World = this;

	// Clear previous map's title if it wasn't cleared already.
	g_DisplayTitleName.clear();
}

CWorld::~CWorld()
{
	if (World != this)
	{
		return;
	}

	g_Server.MapIsEnding();

	World = nullptr;

	g_GameLogger->trace("worldspawn destroyed");
}

void CWorld::Spawn()
{
	g_fGameOver = false;
	Precache();
	CItemCTF::m_pLastSpawn = nullptr;

	if (g_pGameRules->IsCTF())
	{
		ResetTeamScores();
	}
}

void CWorld::Precache()
{
	// Flag this entity for removal if it's not the actual world entity.
	if (World != this)
	{
		UTIL_Remove(this);
		return;
	}

	g_GameLogger->trace("Initializing world");

	g_pLastSpawn = nullptr;

#if 1
	CVAR_SET_STRING("sv_gravity", "800"); // 67ft/sec
	CVAR_SET_STRING("sv_stepsize", "18");
#else
	CVAR_SET_STRING("sv_gravity", "384"); // 32ft/sec
	CVAR_SET_STRING("sv_stepsize", "24");
#endif

	CVAR_SET_STRING("room_type", "0"); // clear DSP

	// If we're loading a save game then this must be singleplayer, and gamerules has been installed by the ServerLibrary already.
	if (!g_Server.IsCurrentMapLoadedFromSaveGame())
	{
		// Set up game rules
		delete g_pGameRules;

		g_pGameRules = InstallGameRules(this);
	}

	//!!!UNDONE why is there so much Spawn code in the Precache function? I'll just keep it here

	///!!!LATER - do we want a sound ent in deathmatch? (sjb)
	pSoundEnt = g_EntityDictionary->Create<CSoundEnt>("soundent");
	
	if (pSoundEnt)
	{
		pSoundEnt->Spawn();
	}
	else
	{
		Logger->debug("**COULD NOT CREATE SOUNDENT**");
	}

	InitBodyQue();

	// the area based ambient sounds MUST be the first precache_sounds

	// player precaches
	W_Precache(); // get weapon precaches

	ClientPrecache();

	g_GameLogger->trace("Precaching common assets");

	// sounds used from C physics code
	PrecacheSound("common/null.wav"); // clears sound channels

	PrecacheSound("items/suitchargeok1.wav"); //!!! temporary sound for respawning weapons.
	PrecacheSound("items/gunpickup2.wav");	  // player picks up a gun.

	PrecacheSound("common/bodydrop3.wav"); // dead bodies hitting the ground (animation events)
	PrecacheSound("common/bodydrop4.wav");

	g_Language = (int)CVAR_GET_FLOAT("sv_language");
	if (g_Language == LANGUAGE_GERMAN)
	{
		PrecacheModel("models/germangibs.mdl");
	}
	else
	{
		PrecacheModel("models/hgibs.mdl");
		PrecacheModel("models/agibs.mdl");
	}

	PrecacheSound("weapons/ric1.wav");
	PrecacheSound("weapons/ric2.wav");
	PrecacheSound("weapons/ric3.wav");
	PrecacheSound("weapons/ric4.wav");
	PrecacheSound("weapons/ric5.wav");
	//
	// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
	//

	// 0 normal
	LIGHT_STYLE(0, "m");

	// 1 FLICKER (first variety)
	LIGHT_STYLE(1, "mmnmmommommnonmmonqnmmo");

	// 2 SLOW STRONG PULSE
	LIGHT_STYLE(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	// 3 CANDLE (first variety)
	LIGHT_STYLE(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	// 4 FAST STROBE
	LIGHT_STYLE(4, "mamamamamama");

	// 5 GENTLE PULSE 1
	LIGHT_STYLE(5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	// 6 FLICKER (second variety)
	LIGHT_STYLE(6, "nmonqnmomnmomomno");

	// 7 CANDLE (second variety)
	LIGHT_STYLE(7, "mmmaaaabcdefgmmmmaaaammmaamm");

	// 8 CANDLE (third variety)
	LIGHT_STYLE(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	// 9 SLOW STROBE (fourth variety)
	LIGHT_STYLE(9, "aaaaaaaazzzzzzzz");

	// 10 FLUORESCENT FLICKER
	LIGHT_STYLE(10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	LIGHT_STYLE(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

	// 12 UNDERWATER LIGHT MUTATION
	// this light only distorts the lightmap - no contribution
	// is made to the brightness of affected surfaces
	LIGHT_STYLE(12, "mmnnmmnnnmmnn");

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	LIGHT_STYLE(63, "a");

	for (std::size_t i = 0; i < std::size(gDecals); i++)
		gDecals[i].index = DECAL_INDEX(gDecals[i].name);

	g_GameLogger->trace("Setting up node graph");

	// init the WorldGraph.
	WorldGraph.InitGraph();

	// make sure the .NOD file is newer than the .BSP file.
	if (!WorldGraph.CheckNODFile(STRING(gpGlobals->mapname)))
	{ // NOD file is not present, or is older than the BSP file.
		WorldGraph.AllocNodes();
	}
	else
	{ // Load the node graph for this level
		if (!WorldGraph.FLoadGraph(STRING(gpGlobals->mapname)))
		{ // couldn't load, so alloc and prepare to build a graph.
			CGraph::Logger->debug("*Error opening .NOD file");
			WorldGraph.AllocNodes();
		}
		else
		{
			CGraph::Logger->debug("\n*Graph Loaded!");
		}
	}

	if (pev->speed > 0)
		CVAR_SET_FLOAT("sv_zmax", pev->speed);
	else
		CVAR_SET_FLOAT("sv_zmax", 4096);

	if (!FStringNull(pev->netname))
	{
		Logger->debug("Chapter title: {}", GetNetname());
		CBaseEntity* pEntity = CBaseEntity::Create("env_message", g_vecZero, g_vecZero, nullptr);
		if (pEntity)
		{
			pEntity->SetThink(&CBaseEntity::SUB_CallUseToggle);
			pEntity->pev->message = pev->netname;
			pev->netname = string_t::Null;
			pEntity->pev->nextthink = gpGlobals->time + 0.3;
			pEntity->pev->spawnflags = SF_MESSAGE_ONCE;
		}
	}

	if ((pev->spawnflags & SF_WORLD_DARK) != 0)
		CVAR_SET_FLOAT("v_dark", 1.0);
	else
		CVAR_SET_FLOAT("v_dark", 0.0);

	if ((pev->spawnflags & SF_WORLD_FORCETEAM) != 0)
	{
		CVAR_SET_FLOAT("mp_defaultteam", 1);
	}
	else
	{
		CVAR_SET_FLOAT("mp_defaultteam", 0);
	}

	if ((pev->spawnflags & SF_WORLD_COOP) != 0)
	{
		CVAR_SET_FLOAT("mp_defaultcoop", 1);
	}
	else
	{
		CVAR_SET_FLOAT("mp_defaultcoop", 0);
	}

	CVAR_SET_FLOAT("sv_wateramp", pev->scale);
}

bool CWorld::KeyValue(KeyValueData* pkvd)
{
	//ignore the "wad" field.
	if (FStrEq(pkvd->szKeyName, "skyname"))
	{
		// Sent over net now.
		// Reset to default in ServerLibrary::NewMapStarted.
		CVAR_SET_STRING("sv_skyname", pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "WaveHeight"))
	{
		// Sent over net now.
		pev->scale = atof(pkvd->szValue) * (1.0 / 8.0);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "MaxRange"))
	{
		pev->speed = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "chaptertitle"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "startdark"))
	{
		// UNDONE: This is a gross hack!!! The CVAR is NOT sent over the client/sever link
		// but it will work for single player
		int flag = atoi(pkvd->szValue);
		if (0 != flag)
			pev->spawnflags |= SF_WORLD_DARK;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "newunit"))
	{
		// Single player only.  Clear save directory if set
		if (0 != atoi(pkvd->szValue))
			CVAR_SET_FLOAT("sv_newunit", 1);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gametitle"))
	{
		// No need to save, only displayed on startup. Bugged in vanilla, loading a save game shows it again.
		g_DisplayTitleName = pkvd->szValue;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "mapteams"))
	{
		pev->team = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "defaultteam"))
	{
		if (0 != atoi(pkvd->szValue))
		{
			pev->spawnflags |= SF_WORLD_FORCETEAM;
		}
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "defaultctf"))
	{
		if (0 != atoi(pkvd->szValue))
		{
			pev->spawnflags |= SF_WORLD_CTF;
		}
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}
