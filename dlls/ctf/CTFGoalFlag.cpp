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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "basemonster.h"
#include "player.h"
#include "ctf/CTFGoal.h"
#include "ctf/CTFGoalBase.h"
#include "ctf/CTFGoalFlag.h"

#include "gamerules.h"
#include "ctf/ctfplay_gamerules.h"
#include "UserMessages.h"

void DumpCTFFlagInfo(CBasePlayer* pPlayer)
{
	if (g_pGameRules->IsCTF())
	{
		for (auto entity : UTIL_FindEntitiesByClassname<CTFGoalFlag>("item_ctfflag"))
		{
			entity->DisplayFlagStatus(pPlayer);
		}
	}
}

LINK_ENTITY_TO_CLASS(item_ctfflag, CTFGoalFlag);

void CTFGoalFlag::Precache()
{
	if (pev->model)
	{
		PRECACHE_MODEL((char*)STRING(pev->model));
	}

	g_engfuncs.pfnPrecacheSound("ctf/bm_flagtaken.wav");
	g_engfuncs.pfnPrecacheSound("ctf/soldier_flagtaken.wav");
	g_engfuncs.pfnPrecacheSound("ctf/marine_flag_capture.wav");
	g_engfuncs.pfnPrecacheSound("ctf/civ_flag_capture.wav");
}

void CTFGoalFlag::PlaceItem()
{
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->oldorigin = pev->origin;
}

void CTFGoalFlag::ReturnFlag()
{
	if (pev->owner)
	{
		auto player = GET_PRIVATE<CBasePlayer>(pev->owner);
		player->m_pFlag = nullptr;
		player->m_iItems = static_cast<CTFItem::CTFItem>(player->m_iItems & ~(CTFItem::BlackMesaFlag | CTFItem::OpposingForceFlag));
	}

	SetTouch(nullptr);
	SetThink(nullptr);

	pev->nextthink = gpGlobals->time;

	m_iGoalState = 1;
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->aiment = nullptr;

	pev->angles = m_OriginalAngles;
	pev->effects |= EF_NODRAW;
	pev->origin = pev->oldorigin;

	UTIL_SetOrigin(pev, pev->origin);

	if (pev->model)
	{
		pev->sequence = LookupSequence("flag_positioned");
		if (pev->sequence != -1)
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}

	SetThink(&CTFGoalFlag::ReturnFlagThink);

	pev->nextthink = gpGlobals->time + 0.25;

	for (auto entity : UTIL_FindEntitiesByClassname<CTFGoalBase>("item_ctfbase"))
	{
		if (entity->m_iGoalNum == m_iGoalNum)
		{
			entity->pev->skin = 0;
			break;
		}
	}
}

void CTFGoalFlag::FlagCarryThink()
{
	Vector vecLightPos, vecLightAng;
	GetAttachment(0, vecLightPos, vecLightAng);

	g_engfuncs.pfnMessageBegin(MSG_ALL, SVC_TEMPENTITY, 0, 0);
	g_engfuncs.pfnWriteByte(TE_ELIGHT);
	g_engfuncs.pfnWriteShort(entindex() + 0x1000);
	g_engfuncs.pfnWriteCoord(vecLightPos.x);
	g_engfuncs.pfnWriteCoord(vecLightPos.y);
	g_engfuncs.pfnWriteCoord(vecLightPos.z);
	g_engfuncs.pfnWriteCoord(128);

	if (m_iGoalNum == 1)
	{
		g_engfuncs.pfnWriteByte(255);
		g_engfuncs.pfnWriteByte(128);
		g_engfuncs.pfnWriteByte(128);
	}
	else
	{
		g_engfuncs.pfnWriteByte(128);
		g_engfuncs.pfnWriteByte(255);
		g_engfuncs.pfnWriteByte(128);
	}
	g_engfuncs.pfnWriteByte(255);
	g_engfuncs.pfnWriteCoord(0);
	g_engfuncs.pfnMessageEnd();

	auto owner = CBaseEntity::Instance(pev->owner);

	bool hasFlag = false;

	if (owner && owner->IsPlayer())
	{
		auto player = static_cast<CBasePlayer*>(owner);

		if (player->m_iItems & (CTFItem::BlackMesaFlag | CTFItem::OpposingForceFlag))
		{
			hasFlag = true;

			bool flagFound = false;

			for (auto entity : UTIL_FindEntitiesByClassname<CTFGoalFlag>("item_ctfflag"))
			{
				if (static_cast<CTFTeam>(entity->m_iGoalNum) == player->m_iTeamNum)
				{
					flagFound = entity->m_iGoalState == 1;
					break;
				}
			}

			if (flagFound)
			{
				ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFPickUpFlagP");
			}
			else
			{
				ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFPickUpFlagG");
			}

			pev->nextthink = gpGlobals->time + 20.0;
		}
		else
		{
			player->m_iClientItems = CTFItem::BlackMesaFlag | CTFItem::OpposingForceFlag;
		}
	}

	if (!hasFlag)
	{
		ReturnFlag();
	}
}

void CTFGoalFlag::goal_item_dropthink()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->aiment = 0;

	int contents = UTIL_PointContents(pev->origin);

	if (contents == CONTENTS_SOLID
		|| contents == CONTENTS_SLIME
		|| contents == CONTENTS_LAVA
		|| contents == CONTENTS_SKY)
	{
		ReturnFlag();
	}
	else
	{
		SetThink(&CTFGoalFlag::ReturnFlag);

		pev->nextthink = gpGlobals->time + V_max(0, g_flFlagReturnTime - 5.0);
	}
}

void CTFGoalFlag::Spawn()
{
	Precache();

	if (m_iGoalNum)
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_TRIGGER;
		UTIL_SetOrigin(pev, pev->origin);

		Vector vecMax, vecMin;
		vecMax.x = 16;
		vecMax.y = 16;
		vecMax.z = 72;
		vecMin.x = -16;
		vecMin.y = -16;
		vecMin.z = 0;
		UTIL_SetSize(pev, vecMin, vecMax);

		if (!g_engfuncs.pfnDropToFloor(edict()))
		{
			ALERT(
				at_error,
				"Item %s fell out of level at %f,%f,%f",
				STRING(pev->classname),
				pev->origin.x,
				pev->origin.y,
				pev->origin.z);

			UTIL_Remove(this);
		}
		else
		{
			if (pev->model)
			{
				g_engfuncs.pfnSetModel(edict(), STRING(pev->model));

				pev->sequence = LookupSequence("flag_positioned");

				if (pev->sequence != -1)
				{
					ResetSequenceInfo();
					pev->frame = 0;
				}
			}

			m_iGoalState = 1;
			pev->solid = SOLID_TRIGGER;

			if (!pev->netname)
			{
				pev->netname = MAKE_STRING("goalflag");
			}

			UTIL_SetOrigin(pev, pev->origin);

			m_OriginalAngles = pev->angles;

			SetTouch(&CTFGoalFlag::goal_item_touch);
			SetThink(&CTFGoalFlag::PlaceItem);

			pev->nextthink = gpGlobals->time + 0.2;
		}
	}
	else
	{
		ALERT(at_error, "Invalid goal_no set for CTF flag\n");
	}
}

void CTFGoalFlag::ReturnFlagThink()
{
	pev->effects &= ~EF_NODRAW;

	SetThink(nullptr);
	SetTouch(&CTFGoalFlag::goal_item_touch);

	const char* name = "";

	switch (m_iGoalNum)
	{
	case 1: name = "ReturnedBlackMesaFlag";
	case 2: name = "ReturnedOpposingForceFlag";
	}

	UTIL_LogPrintf("World triggered \"%s\"\n", name);
	DisplayTeamFlags(nullptr);
}

void CTFGoalFlag::StartItem()
{
	SetThink(&CTFGoalFlag::PlaceItem);
	pev->nextthink = gpGlobals->time + 0.2;
}

void CTFGoalFlag::ScoreFlagTouch(CBasePlayer* pPlayer)
{
	if (pPlayer && (m_iGoalNum == 1 || m_iGoalNum == 2))
	{
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto player = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i));

			if (!player)
			{
				continue;
			}

			if (player->pev->flags & FL_SPECTATOR)
			{
				continue;
			}

			if (player->m_iTeamNum == pPlayer->m_iTeamNum)
			{
				if (static_cast<int>(player->m_iTeamNum) == m_iGoalNum)
				{
					if (pPlayer == player)
						ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFCaptureFlagP", 0, 0, 0, 0);
					else
						ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFCaptureFlagT", 0, 0, 0, 0);
				}
				else if (pPlayer == player)
				{
					bool foundFlag = false;

					for (auto entity : UTIL_FindEntitiesByClassname<CTFGoalFlag>("item_ctfflag"))
					{
						if (entity->m_iGoalNum == static_cast<int>(pPlayer->m_iTeamNum))
						{
							foundFlag = entity->m_iGoalState == 1;
							break;
						}
					}

					if (foundFlag)
					{
						ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFPickUpFlagP");
					}
					else
					{
						ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFPickUpFlagG");
					}
				}
				else
				{
					ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFPickUpFlagT");
				}
			}
			else if (static_cast<int>(pPlayer->m_iTeamNum) == m_iGoalNum)
			{
				ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFFlagCaptured");
			}
			else
			{
				ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFLoseFlag");
			}
		}

		if (m_iGoalNum == 1)
		{
			if (pPlayer->m_iTeamNum == CTFTeam::BlackMesa)
			{
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pPlayer->pev->netname));
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagScorerOF");
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "\n");

				UTIL_LogPrintf("\"%s<%i><%u><%s>\" triggered \"CapturedFlag\"\n",
					STRING(pPlayer->pev->netname),
					g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
					g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
					GetTeamName(pPlayer->edict()));

				++teamscores[static_cast<int>(pPlayer->m_iTeamNum) - 1];

				++pPlayer->m_iFlagCaptures;
				pPlayer->m_iCTFScore += 10;
				pPlayer->m_iOffense += 10;

				g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgCTFScore, 0, 0);
				g_engfuncs.pfnWriteByte(pPlayer->entindex());
				g_engfuncs.pfnWriteByte(pPlayer->m_iCTFScore);
				g_engfuncs.pfnMessageEnd();

				ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#CTFScorePoints");
				g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgScoreInfo, 0, 0);
				g_engfuncs.pfnWriteByte(pPlayer->entindex());
				g_engfuncs.pfnWriteShort(pPlayer->pev->frags);
				g_engfuncs.pfnWriteShort(pPlayer->m_iDeaths);
				g_engfuncs.pfnMessageEnd();

				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFTeamBM");
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs(": %d\n", teamscores[0]));
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFTeamOF");
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs(": %d\n", teamscores[1]));

				if (m_nReturnPlayer > 0 && g_flCaptureAssistTime > gpGlobals->time - m_flReturnTime)
				{
					auto returnPlayer = UTIL_PlayerByIndex(m_nReturnPlayer);
					++pPlayer->m_iCTFScore;
					++pPlayer->m_iOffense;
					g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgCTFScore, 0, 0);
					g_engfuncs.pfnWriteByte(pPlayer->entindex());
					g_engfuncs.pfnWriteByte(pPlayer->m_iCTFScore);
					g_engfuncs.pfnMessageEnd();

					ClientPrint(returnPlayer->pev, HUD_PRINTTALK, "#CTFScorePoint");
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s", STRING(returnPlayer->pev->netname)));
					UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagAssistBM");
				}
			}
			else
			{
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pPlayer->pev->netname));
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagGetBM");
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "\n");

				UTIL_LogPrintf("\"%s<%i><%u><%s>\" triggered \"TookFlag\"\n",
					STRING(pPlayer->pev->netname),
					g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
					g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
					GetTeamName(pPlayer->edict()));
			}
		}
		else
		{
			if (static_cast<int>(pPlayer->m_iTeamNum) != m_iGoalNum)
			{
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pPlayer->pev->netname));
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagGetOF");
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "\n");

				UTIL_LogPrintf("\"%s<%i><%u><%s>\" triggered \"TookFlag\"\n",
					STRING(pPlayer->pev->netname),
					g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
					g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
					GetTeamName(pPlayer->edict()));
			}

			UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pPlayer->pev->netname));
			UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagScorerBM");
			UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "\n");

			UTIL_LogPrintf("\"%s<%i><%u><%s>\" triggered \"CapturedFlag\"\n",
				STRING(pPlayer->pev->netname),
				g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
				g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
				GetTeamName(pPlayer->edict()));

			++teamscores[static_cast<int>(pPlayer->m_iTeamNum) - 1];

			++pPlayer->m_iFlagCaptures;
			pPlayer->m_iCTFScore += 10;
			pPlayer->m_iOffense += 10;

			g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgCTFScore, 0, 0);
			g_engfuncs.pfnWriteByte(pPlayer->entindex());
			g_engfuncs.pfnWriteByte(pPlayer->m_iCTFScore);
			g_engfuncs.pfnMessageEnd();

			ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#CTFScorePoints");

			g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgScoreInfo, 0, 0);
			g_engfuncs.pfnWriteByte(pPlayer->entindex());
			g_engfuncs.pfnWriteShort(pPlayer->pev->frags);
			g_engfuncs.pfnWriteShort(pPlayer->m_iDeaths);
			g_engfuncs.pfnMessageEnd();

			UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFTeamBM");
			UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs(": %d\n", teamscores[0]));
			UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFTeamOF");
			UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs(": %d\n", teamscores[1]));

			if (m_nReturnPlayer > 0 && g_flCaptureAssistTime > gpGlobals->time - m_flReturnTime)
			{
				auto returnPlayer = UTIL_PlayerByIndex(m_nReturnPlayer);

				++pPlayer->m_iCTFScore;
				++pPlayer->m_iOffense;

				g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgCTFScore, 0, 0);
				g_engfuncs.pfnWriteByte(pPlayer->entindex());
				g_engfuncs.pfnWriteByte(pPlayer->m_iCTFScore);
				g_engfuncs.pfnMessageEnd();

				ClientPrint(returnPlayer->pev, HUD_PRINTTALK, "#CTFScorePoint");
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, UTIL_VarArgs("%s", STRING(returnPlayer->pev->netname)));
				UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagAssistOF");
			}
		}
	}

	DisplayTeamFlags(nullptr);
}

void CTFGoalFlag::TurnOnLight(CBasePlayer* pPlayer)
{
	Vector vecLightPos, vecLightAng;
	GetAttachment(0, vecLightPos, vecLightAng);

	g_engfuncs.pfnMessageBegin(MSG_ONE, SVC_TEMPENTITY, 0, pPlayer->edict());
	g_engfuncs.pfnWriteByte(TE_ELIGHT);
	g_engfuncs.pfnWriteShort(entindex() + 0x1000);
	g_engfuncs.pfnWriteCoord(vecLightPos.x);
	g_engfuncs.pfnWriteCoord(vecLightPos.y);
	g_engfuncs.pfnWriteCoord(vecLightPos.z);
	g_engfuncs.pfnWriteCoord(128.0);

	if (m_iGoalNum == 1)
	{
		g_engfuncs.pfnWriteByte(255);
		g_engfuncs.pfnWriteByte(128);
		g_engfuncs.pfnWriteByte(128);
	}
	else
	{
		g_engfuncs.pfnWriteByte(128);
		g_engfuncs.pfnWriteByte(255);
		g_engfuncs.pfnWriteByte(128);
	}

	g_engfuncs.pfnWriteByte(255);
	g_engfuncs.pfnWriteCoord(0);
	g_engfuncs.pfnMessageEnd();
}

void CTFGoalFlag::GiveFlagToPlayer(CBasePlayer* pPlayer)
{
	pPlayer->m_pFlag = this;

	if (m_iGoalNum == 1)
	{
		pPlayer->m_iItems = static_cast<CTFItem::CTFItem>(pPlayer->m_iItems | CTFItem::BlackMesaFlag);
	}
	else if (m_iGoalNum == 2)
	{
		pPlayer->m_iItems = static_cast<CTFItem::CTFItem>(pPlayer->m_iItems | CTFItem::OpposingForceFlag);
	}

	SetTouch(nullptr);
	SetThink(nullptr);

	pev->nextthink = gpGlobals->time;
	pev->owner = pPlayer->edict();
	pev->movetype = MOVETYPE_FOLLOW;
	pev->aiment = pPlayer->edict();

	if (pev->model)
	{
		pev->sequence = LookupSequence("carried");

		if (pev->sequence != -1)
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}
	pev->effects &= ~EF_NODRAW;

	pev->solid = SOLID_NOT;

	m_iGoalState = 2;

	if (pPlayer->m_iTeamNum == CTFTeam::BlackMesa)
	{
		EMIT_SOUND_DYN(edict(), CHAN_STATIC, "ctf/bm_flagtaken.wav", VOL_NORM, ATTN_NONE, 0, PITCH_NORM);
	}
	else if (pPlayer->m_iTeamNum == CTFTeam::OpposingForce)
	{
		EMIT_SOUND_DYN(edict(), CHAN_STATIC, "ctf/soldier_flagtaken.wav", VOL_NORM, ATTN_NONE, 0, PITCH_NORM);
	}

	ScoreFlagTouch(pPlayer);

	SetThink(&CTFGoalFlag::FlagCarryThink);
	pev->nextthink = gpGlobals->time + 0.1;

	++pPlayer->m_iCTFScore;
	++pPlayer->m_iOffense;

	g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgCTFScore, 0, 0);
	g_engfuncs.pfnWriteByte(pPlayer->entindex());
	g_engfuncs.pfnWriteByte(pPlayer->m_iCTFScore);
	g_engfuncs.pfnMessageEnd();

	ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#CTFScorePoint");

	g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgScoreInfo, 0, 0);
	g_engfuncs.pfnWriteByte(pPlayer->entindex());
	g_engfuncs.pfnWriteShort(pPlayer->pev->frags);
	g_engfuncs.pfnWriteShort(pPlayer->m_iDeaths);
	g_engfuncs.pfnMessageEnd();

	DisplayTeamFlags(nullptr);

	for (auto entity : UTIL_FindEntitiesByClassname<CTFGoalBase>("item_ctfbase"))
	{
		if (entity->m_iGoalNum == m_iGoalNum)
		{
			entity->pev->skin = 1;
			break;
		}
	}
}

void CTFGoalFlag::goal_item_touch(CBaseEntity* pOther)
{
	if (pOther->Classify() != CLASS_PLAYER)
		return;

	if (pOther->pev->health <= 0)
		return;

	auto pPlayer = static_cast<CBasePlayer*>(pOther);

	if (static_cast<int>(pPlayer->m_iTeamNum) != m_iGoalNum)
	{
		GiveFlagToPlayer(pPlayer);
		return;
	}

	if (m_iGoalState == 1)
	{
		if (!(pPlayer->m_iItems & (CTFItem::BlackMesaFlag | CTFItem::OpposingForceFlag)))
			return;

		auto otherFlag = static_cast<CTFGoalFlag*>(static_cast<CBaseEntity*>(pPlayer->m_pFlag));

		if (otherFlag)
			otherFlag->ReturnFlag();

		if (pPlayer->m_iTeamNum == CTFTeam::BlackMesa)
		{
			EMIT_SOUND_DYN(edict(), CHAN_STATIC, "ctf/civ_flag_capture.wav", VOL_NORM, ATTN_NONE, 0, PITCH_NORM);
		}
		else if (pPlayer->m_iTeamNum == CTFTeam::OpposingForce)
		{
			EMIT_SOUND_DYN(edict(), CHAN_STATIC, "ctf/marine_flag_capture.wav", VOL_NORM, ATTN_NONE, 0, PITCH_NORM);
		}

		ScoreFlagTouch(pPlayer);
		return;
	}

	if (pPlayer->m_iTeamNum == CTFTeam::BlackMesa)
	{
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pPlayer->pev->netname));
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagReturnBM");

		UTIL_LogPrintf(
			"\"%s<%i><%u><%s>\" triggered \"ReturnedFlag\"\n",
			STRING(pPlayer->pev->netname),
			g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
			g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
			GetTeamName(pPlayer->edict()));
	}
	else if (pPlayer->m_iTeamNum == CTFTeam::OpposingForce)
	{
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(pPlayer->pev->netname));
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#CTFFlagReturnOF");

		UTIL_LogPrintf(
			"\"%s<%i><%u><%s>\" triggered \"ReturnedFlag\"\n",
			STRING(pPlayer->pev->netname),
			g_engfuncs.pfnGetPlayerUserId(pPlayer->edict()),
			g_engfuncs.pfnGetPlayerWONId(pPlayer->edict()),
			GetTeamName(pPlayer->edict()));
	}

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto player = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i));

		if (!player || player->pev->flags & FL_SPECTATOR)
		{
			continue;
		}

		if (static_cast<int>(player->m_iTeamNum) == m_iGoalNum)
		{
			ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFFlagReturnedT");
		}
		else
		{
			ClientPrint(player->pev, HUD_PRINTCENTER, "#CTFFlagReturnedE");
		}
	}

	++pPlayer->m_iCTFScore;
	++pPlayer->m_iDefense;

	g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgCTFScore, 0, 0);
	g_engfuncs.pfnWriteByte(pPlayer->entindex());
	g_engfuncs.pfnWriteByte(pPlayer->m_iCTFScore);
	g_engfuncs.pfnMessageEnd();

	ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#CTFScorePoint");

	g_engfuncs.pfnMessageBegin(2, gmsgScoreInfo, 0, 0);
	g_engfuncs.pfnWriteByte(pPlayer->entindex());
	g_engfuncs.pfnWriteShort(pPlayer->pev->frags);
	g_engfuncs.pfnWriteShort(pPlayer->m_iDeaths);
	g_engfuncs.pfnMessageEnd();

	ReturnFlag();

	m_nReturnPlayer = pPlayer->entindex();
	m_flReturnTime = gpGlobals->time;
}

void CTFGoalFlag::SetDropTouch()
{
	SetTouch(&CTFGoalFlag::goal_item_touch);
	SetThink(&CTFGoalFlag::goal_item_dropthink);
	pev->nextthink = gpGlobals->time + 4.5;
}

void CTFGoalFlag::DoDrop(const Vector& vecOrigin)
{
	pev->flags &= ~FL_ONGROUND;
	pev->origin = vecOrigin;
	UTIL_SetOrigin(pev, pev->origin);

	pev->angles = g_vecZero;

	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	pev->movetype = MOVETYPE_TOSS;

	pev->velocity = g_vecZero;
	pev->velocity.z = -100;

	SetTouch(&CTFGoalFlag::goal_item_touch);
	SetThink(&CTFGoalFlag::goal_item_dropthink);
	pev->nextthink = gpGlobals->time + 5.0;
}

void CTFGoalFlag::DropFlag(CBasePlayer* pPlayer)
{
	pPlayer->m_pFlag = nullptr;
	pPlayer->m_iItems = static_cast<CTFItem::CTFItem>(pPlayer->m_iItems & ~(CTFItem::BlackMesaFlag | CTFItem::OpposingForceFlag));

	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->aiment = nullptr;
	pev->owner = nullptr;

	if (pev->model)
	{
		pev->sequence = LookupSequence("not_carried");

		if (pev->sequence != -1)
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}

	const float height = (pev->flags & FL_DUCKING) ? 34 : 16;

	pev->origin = pPlayer->pev->origin + Vector(0, 0, height);

	DoDrop(pev->origin);

	m_iGoalState = 3;

	DisplayTeamFlags(nullptr);
}

void CTFGoalFlag::DisplayFlagStatus(CBasePlayer* pPlayer)
{
	switch (m_iGoalState)
	{
	case 1:
		if (m_iGoalNum == 1)
			ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "#CTFInfoFlagAtBaseBM");
		else
			ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "#CTFInfoFlagAtBaseOF");
		break;

	case 2:
	{
		auto owner = CBaseEntity::Instance(pev->owner);

		if (owner)
		{
			if (m_iGoalNum == 1)
			{
				ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "%s", STRING(owner->pev->netname));
				ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "#CTFInfoFlagCarriedBM");
			}
			else
			{
				ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "%s", STRING(owner->pev->netname));
				ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "#CTFInfoFlagCarriedOF");
			}
		}
		else if (m_iGoalNum == 1)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "#CTFInfoFlagCarryBM");
		}
		else
		{
			ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "#CTFInfoFlagCarryOF");
		}
	}
	break;

	case 3:
		if (m_iGoalNum == 1)
			ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "#CTFInfoFlagDroppedBM");
		else
			ClientPrint(pPlayer->pev, HUD_PRINTNOTIFY, "#CTFInfoFlagDroppedOF");
		break;
	}
}

void CTFGoalFlag::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 72);
}
