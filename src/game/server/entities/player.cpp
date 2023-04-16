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
 *	functions dealing with the player
 */

#include <limits>

#include <EASTL/fixed_string.h>

#include "cbase.h"
#include "CCorpse.h"
#include "AmmoTypeSystem.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "talkmonster.h"
#include "squadmonster.h"
#include "items/CWeaponBox.h"
#include "military/COFSquadTalkMonster.h"
#include "shake.h"
#include "spawnpoints.h"
#include "CHalfLifeCTFplay.h"
#include "ctf/CHUDIconTrigger.h"
#include "pm_shared.h"
#include "hltv.h"
#include "UserMessages.h"
#include "client.h"
#include "ServerLibrary.h"

#include "ctf/ctf_goals.h"
#include "rope/CRope.h"

// #define DUCKFIX

#define TRAIN_ACTIVE 0x80
#define TRAIN_NEW 0xc0
#define TRAIN_OFF 0x00
#define TRAIN_NEUTRAL 0x01
#define TRAIN_SLOW 0x02
#define TRAIN_MEDIUM 0x03
#define TRAIN_FAST 0x04
#define TRAIN_BACK 0x05

#define FLASH_DRAIN_TIME 1.2  // 100 units/3 minutes
#define FLASH_CHARGE_TIME 0.2 // 100 units/20 seconds  (seconds per unit)

BEGIN_DATAMAP(CBasePlayer)
DEFINE_FIELD(m_SuitLightType, FIELD_INTEGER),

	DEFINE_FIELD(m_flFlashLightTime, FIELD_TIME),
	DEFINE_FIELD(m_iFlashBattery, FIELD_INTEGER),

	DEFINE_FIELD(m_afButtonLast, FIELD_INTEGER),
	DEFINE_FIELD(m_afButtonPressed, FIELD_INTEGER),
	DEFINE_FIELD(m_afButtonReleased, FIELD_INTEGER),

	DEFINE_ARRAY(m_rgItems, FIELD_INTEGER, MAX_ITEMS),
	DEFINE_FIELD(m_afPhysicsFlags, FIELD_INTEGER),

	DEFINE_FIELD(m_flTimeStepSound, FIELD_TIME),
	DEFINE_FIELD(m_flTimeWeaponIdle, FIELD_TIME),
	DEFINE_FIELD(m_flSwimTime, FIELD_TIME),
	DEFINE_FIELD(m_flDuckTime, FIELD_TIME),
	DEFINE_FIELD(m_flWallJumpTime, FIELD_TIME),

	DEFINE_FIELD(m_flSuitUpdate, FIELD_TIME),
	DEFINE_ARRAY(m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST),
	DEFINE_FIELD(m_iSuitPlayNext, FIELD_INTEGER),
	DEFINE_ARRAY(m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT),
	DEFINE_ARRAY(m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT),
	DEFINE_FIELD(m_lastDamageAmount, FIELD_INTEGER),

	DEFINE_ARRAY(m_rgpPlayerWeapons, FIELD_CLASSPTR, MAX_WEAPON_SLOTS),
	DEFINE_FIELD(m_pActiveWeapon, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pLastWeapon, FIELD_CLASSPTR),
	DEFINE_FIELD(m_WeaponBits, FIELD_INT64),
	DEFINE_FIELD(m_HudFlags, FIELD_INTEGER),

	DEFINE_ARRAY(m_rgAmmo, FIELD_INTEGER, MAX_AMMO_TYPES),
	DEFINE_FIELD(m_idrowndmg, FIELD_INTEGER),
	DEFINE_FIELD(m_idrownrestored, FIELD_INTEGER),

	DEFINE_FIELD(m_iTrain, FIELD_INTEGER),
	DEFINE_FIELD(m_bitsHUDDamage, FIELD_INTEGER),
	DEFINE_FIELD(m_flFallVelocity, FIELD_FLOAT),
	DEFINE_FIELD(m_iTargetVolume, FIELD_INTEGER),
	DEFINE_FIELD(m_iWeaponVolume, FIELD_INTEGER),
	DEFINE_FIELD(m_iExtraSoundTypes, FIELD_INTEGER),
	DEFINE_FIELD(m_iWeaponFlash, FIELD_INTEGER),
	DEFINE_FIELD(m_fLongJump, FIELD_BOOLEAN),
	DEFINE_FIELD(m_fInitHUD, FIELD_BOOLEAN),
	DEFINE_FIELD(m_tbdPrev, FIELD_TIME),

	DEFINE_FIELD(m_pTank, FIELD_EHANDLE),
	DEFINE_FIELD(m_hViewEntity, FIELD_EHANDLE),
	DEFINE_FIELD(m_iHideHUD, FIELD_INTEGER),
	DEFINE_FIELD(m_iFOV, FIELD_INTEGER),

	DEFINE_FIELD(m_SndRoomtype, FIELD_INTEGER),
	// Don't save these. Let the game recalculate the closest env_sound, and continue to use the last room type like it always has.
	// DEFINE_FIELD(m_SndLast, FIELD_EHANDLE),
	// DEFINE_FIELD(m_flSndRange, FIELD_FLOAT),

	DEFINE_FIELD(m_pRope, FIELD_CLASSPTR),
	DEFINE_FIELD(m_flLastClimbTime, FIELD_TIME),
	DEFINE_FIELD(m_bIsClimbing, FIELD_BOOLEAN),

	// Vanilla Op4 doesn't restore this. Not a big deal but it can cause you to teleport to the wrong area after a restore
	DEFINE_FIELD(m_DisplacerReturn, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_DisplacerSndRoomtype, FIELD_INTEGER),
	DEFINE_FIELD(m_HudColor, FIELD_INTEGER),

	DEFINE_FIELD(m_bInfiniteAir, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bInfiniteArmor, FIELD_BOOLEAN),

	// Save this in the unlikely case where the player spawns and saves and loads
	// in the split second between spawning and the first update.
	DEFINE_FIELD(m_FireSpawnTarget, FIELD_BOOLEAN),

	DEFINE_FUNCTION(PlayerDeathThink),

	// DEFINE_FIELD(m_fDeadTime, FIELD_FLOAT), // only used in multiplayer games
	// DEFINE_FIELD(m_fGameHUDInitialized, FIELD_INTEGER), // only used in multiplayer games
	// DEFINE_FIELD(m_flStopExtraSoundTime, FIELD_TIME),
	// DEFINE_FIELD(m_iPlayerSound, FIELD_INTEGER),	// Don't restore, set in Precache()
	// DEFINE_FIELD(m_flgeigerRange, FIELD_FLOAT),	// Don't restore, reset in Precache()
	// DEFINE_FIELD(m_flgeigerDelay, FIELD_FLOAT),	// Don't restore, reset in Precache()
	// DEFINE_FIELD(m_igeigerRangePrev, FIELD_FLOAT),	// Don't restore, reset in Precache()
	// DEFINE_FIELD(m_iStepLeft, FIELD_INTEGER), // Don't need to restore
	// DEFINE_ARRAY(m_szTextureName, FIELD_CHARACTER, TextureNameMax), // Don't need to restore
	// DEFINE_FIELD(m_chTextureType, FIELD_CHARACTER), // Don't need to restore
	// DEFINE_FIELD(m_fNoPlayerSound, FIELD_BOOLEAN), // Don't need to restore, debug
	// DEFINE_FIELD(m_iUpdateTime, FIELD_INTEGER), // Don't need to restore
	// DEFINE_FIELD(m_iClientHealth, FIELD_INTEGER), // Don't restore, client needs reset
	// DEFINE_FIELD(m_iClientBattery, FIELD_INTEGER), // Don't restore, client needs reset
	// DEFINE_FIELD(m_iClientHideHUD, FIELD_INTEGER), // Don't restore, client needs reset
	// DEFINE_FIELD(m_fWeapon, FIELD_BOOLEAN),  // Don't restore, client needs reset
	// DEFINE_FIELD(m_nCustomSprayFrames, FIELD_INTEGER), // Don't restore, depends on server message after spawning and only matters in multiplayer
	// DEFINE_FIELD(m_vecAutoAim, FIELD_VECTOR), // Don't save/restore - this is recomputed
	// DEFINE_ARRAY(m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_TYPES), // Don't need to restore
	// DEFINE_FIELD(m_fOnTarget, FIELD_BOOLEAN), // Don't need to restore
	// DEFINE_FIELD(m_nCustomSprayFrames, FIELD_INTEGER), // Don't need to restore
	END_DATAMAP();

void CBasePlayer::Pain()
{
	float flRndSound; // sound randomizer

	flRndSound = RANDOM_FLOAT(0, 1);

	if (flRndSound <= 0.33)
		EmitSound(CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
	else if (flRndSound <= 0.66)
		EmitSound(CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
	else
		EmitSound(CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
}

Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer::DeathSound()
{
	// water death sounds
	/*
	if (pev->waterlevel == 3)
	{
		EmitSound(CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch (RANDOM_LONG(1, 5))
	{
	case 1:
		EmitSound(CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
		break;
	case 2:
		EmitSound(CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
		break;
	case 3:
		EmitSound(CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
		break;
	}

	// play one of the suit death alarms
	EMIT_GROUPNAME_SUIT(this, "HEV_DEAD");
}

bool CBasePlayer::GiveHealth(float flHealth, int bitsDamageType)
{
	return CBaseMonster::GiveHealth(flHealth, bitsDamageType);
}

Vector CBasePlayer::GetGunPosition()
{
	//	UTIL_MakeVectors(pev->v_angle);
	//	m_HackedGunPos = pev->view_ofs;
	Vector origin;

	origin = pev->origin + pev->view_ofs;

	return origin;
}

void CBasePlayer::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (0 != pev->takedamage)
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch (ptr->iHitgroup)
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= GetSkillFloat("player_head"sv);
			break;
		case HITGROUP_CHEST:
			flDamage *= GetSkillFloat("player_chest"sv);
			break;
		case HITGROUP_STOMACH:
			flDamage *= GetSkillFloat("player_stomach"sv);
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= GetSkillFloat("player_arm"sv);
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= GetSkillFloat("player_leg"sv);
			break;
		default:
			break;
		}

		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage); // a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		AddMultiDamage(attacker, this, flDamage, bitsDamageType);
	}
}

#define ARMOR_RATIO 0.2 // Armor Takes 80% of the damage
#define ARMOR_BONUS 0.5 // Each Point of Armor is work 1/x points of health

static const char* m_szSquadClasses[] =
	{
		"monster_human_grunt_ally",
		"monster_human_medic_ally",
		"monster_human_torch_ally"};

bool CBasePlayer::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	bool ffound = true;
	bool fmajor;
	bool fcritical;
	bool fTookDamage;
	bool ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev = pev->health;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if ((bitsDamageType & DMG_BLAST) != 0 && g_pGameRules->IsMultiplayer())
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if (!IsAlive())
		return false;
	// go take the damage first


	CBaseEntity* pAttacker = CBaseEntity::Instance(attacker);

	if (pAttacker && pAttacker->IsPlayer())
	{
		auto pAttackerPlayer = static_cast<CBasePlayer*>(pAttacker);

		// TODO: this is a pretty bad way to handle damage increase
		if ((pAttackerPlayer->m_iItems & CTFItem::Acceleration) != 0)
		{
			flDamage *= 1.6;

			EmitSound(CHAN_STATIC, "turret/tu_ping.wav", VOL_NORM, ATTN_NORM);
		}
		if (m_pFlag)
		{
			m_nLastShotBy = pAttackerPlayer->entindex();
			m_flLastShotTime = gpGlobals->time;
		}
	}

	if (!g_pGameRules->FPlayerCanTakeDamage(this, pAttacker))
	{
		// Refuse the damage
		return false;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	// Armor.
	if (0 != pev->armorvalue && (bitsDamageType & (DMG_FALL | DMG_DROWN)) == 0) // armor doesn't protect against fall or drown damage!
	{
		float flNew = flDamage * flRatio;

		float flArmor;

		flArmor = (flDamage - flNew) * flBonus;

		// Does this use more armor than we have?
		if (flArmor > pev->armorvalue)
		{
			flArmor = pev->armorvalue;
			flArmor *= (1 / flBonus);
			flNew = flDamage - flArmor;
			if (!m_bInfiniteArmor)
				pev->armorvalue = 0;
		}
		else if (!m_bInfiniteArmor)
			pev->armorvalue -= flArmor;

		flDamage = flNew;
	}

	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage(inflictor, attacker, (int)flDamage, bitsDamageType);

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if ((bitsDamageType & (DMG_PARALYZE << i)) != 0)
				m_rgbTimeBasedDamage[i] = 0;
	}

	// tell director about it
	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);						// command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);			// take damage event
	WRITE_SHORT(entindex());			// index number of primary entity
	WRITE_SHORT(inflictor->entindex()); // index number of secondary entity
	WRITE_LONG(5);						// eventflags (priority and flags)
	MESSAGE_END();


	// how bad is it, doc?

	ftrivial = (pev->health > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (pev->health < 30);

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

	// DMG_BURN
	// DMG_FREEZE
	// DMG_BLAST
	// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;			// make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || (bitsDamage & DMG_TIMEBASED) != 0) && ffound && 0 != bitsDamage)
	{
		ffound = false;

		if ((bitsDamage & DMG_CLUB) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC); // minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = true;
		}
		if ((bitsDamage & (DMG_FALL | DMG_CRUSH)) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", false, SUIT_NEXT_IN_30SEC); // major fracture
			else
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC); // minor fracture

			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = true;
		}

		if ((bitsDamage & DMG_BULLET) != 0)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", false, SUIT_NEXT_IN_30SEC); // blood loss detected
			// else
			//	SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC);	// minor laceration

			bitsDamage &= ~DMG_BULLET;
			ffound = true;
		}

		if ((bitsDamage & DMG_SLASH) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", false, SUIT_NEXT_IN_30SEC); // major laceration
			else
				SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC); // minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = true;
		}

		if ((bitsDamage & DMG_SONIC) != 0)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", false, SUIT_NEXT_IN_1MIN); // internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = true;
		}

		if ((bitsDamage & (DMG_POISON | DMG_PARALYZE)) != 0)
		{
			SetSuitUpdate("!HEV_DMG3", false, SUIT_NEXT_IN_1MIN); // blood toxins detected
			bitsDamage &= ~(DMG_POISON | DMG_PARALYZE);
			ffound = true;
		}

		if ((bitsDamage & DMG_ACID) != 0)
		{
			SetSuitUpdate("!HEV_DET1", false, SUIT_NEXT_IN_1MIN); // hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = true;
		}

		if ((bitsDamage & DMG_NERVEGAS) != 0)
		{
			SetSuitUpdate("!HEV_DET0", false, SUIT_NEXT_IN_1MIN); // biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = true;
		}

		if ((bitsDamage & DMG_RADIATION) != 0)
		{
			SetSuitUpdate("!HEV_DET2", false, SUIT_NEXT_IN_1MIN); // radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = true;
		}
		if ((bitsDamage & DMG_SHOCK) != 0)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = true;
		}
	}

	pev->punchangle.x = -2;

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75)
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", false, SUIT_NEXT_IN_30MIN); // automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", false, SUIT_NEXT_IN_30MIN); // morphine shot
	}

	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{

		// already took major damage, now it's critical...
		if (pev->health < 6)
			SetSuitUpdate("!HEV_HLTH3", false, SUIT_NEXT_IN_10MIN); // near death
		else if (pev->health < 20)
			SetSuitUpdate("!HEV_HLTH2", false, SUIT_NEXT_IN_10MIN); // health critical

		// give critical health warnings
		if (!RANDOM_LONG(0, 3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); // seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && (bitsDamageType & DMG_TIMEBASED) != 0 && flHealthPrev < 75)
	{
		if (flHealthPrev < 50)
		{
			if (!RANDOM_LONG(0, 3))
				SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN); // seek medical attention
		}
		else
			SetSuitUpdate("!HEV_HLTH1", false, SUIT_NEXT_IN_10MIN); // health dropping
	}

	// Make all grunts following me attack the NPC that attacked me
	if (pAttacker)
	{
		auto enemy = pAttacker->MyMonsterPointer();

		if (!enemy || enemy->IRelationship(this) == R_AL)
		{
			return fTookDamage;
		}

		for (std::size_t i = 0; i < std::size(m_szSquadClasses); ++i)
		{
			for (auto ally : UTIL_FindEntitiesByClassname<CBaseEntity>(m_szSquadClasses[i]))
			{
				auto squadAlly = ally->MySquadTalkMonsterPointer();

				if (squadAlly && squadAlly->m_hTargetEnt && squadAlly->m_hTargetEnt->IsPlayer())
				{
					squadAlly->SquadMakeEnemy(enemy);
				}
			}
		}
	}

	return fTookDamage;
}

void CBasePlayer::PackDeadPlayerItems()
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBasePlayerWeapon* rgpPackWeapons[MAX_WEAPONS];
	int iPackAmmo[MAX_AMMO_TYPES + 1];
	int iPW = 0; // index into packweapons array
	int iPA = 0; // index into packammo array

	memset(rgpPackWeapons, 0, sizeof(rgpPackWeapons));
	memset(iPackAmmo, -1, sizeof(iPackAmmo));

	// get the game rules
	iWeaponRules = g_pGameRules->DeadPlayerWeapons(this);
	iAmmoRules = g_pGameRules->DeadPlayerAmmo(this);

	if (iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO)
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems(true);
		return;
	}

	// go through all of the weapons and make a list of the ones to pack
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		if (m_rgpPlayerWeapons[i])
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerWeapon* weapon = m_rgpPlayerWeapons[i];

			while (weapon)
			{
				switch (iWeaponRules)
				{
				case GR_PLR_DROP_GUN_ACTIVE:
					if (m_pActiveWeapon && weapon == m_pActiveWeapon)
					{
						// this is the active item. Pack it.
						rgpPackWeapons[iPW++] = weapon;
					}
					break;

				case GR_PLR_DROP_GUN_ALL:
					rgpPackWeapons[iPW++] = weapon;
					break;

				default:
					break;
				}

				weapon = weapon->m_pNext;
			}
		}
	}

	// now go through ammo and make a list of which types to pack.
	if (iAmmoRules != GR_PLR_DROP_AMMO_NO)
	{
		for (i = 0; i < MAX_AMMO_TYPES; i++)
		{
			if (m_rgAmmo[i] > 0)
			{
				// player has some ammo of this type.
				switch (iAmmoRules)
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[iPA++] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					if (m_pActiveWeapon && i == m_pActiveWeapon->PrimaryAmmoIndex())
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					else if (m_pActiveWeapon && i == m_pActiveWeapon->SecondaryAmmoIndex())
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[iPA++] = i;
					}
					break;

				default:
					break;
				}
			}
		}
	}

	// create a box to pack the stuff into.
	CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin, pev->angles, this);

	pWeaponBox->pev->angles.x = 0; // don't let weaponbox tilt.
	pWeaponBox->pev->angles.z = 0;

	pWeaponBox->SetThink(&CWeaponBox::Kill);
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

	// back these two lists up to their first elements
	iPA = 0;
	iPW = 0;

	// pack the ammo
	while (iPackAmmo[iPA] != -1)
	{
		pWeaponBox->PackAmmo(MAKE_STRING(g_AmmoTypes.GetByIndex(iPackAmmo[iPA])->Name.c_str()), m_rgAmmo[iPackAmmo[iPA]]);
		iPA++;
	}

	// now pack all of the items in the lists
	while (rgpPackWeapons[iPW])
	{
		// weapon unhooked from the player. Pack it into der box.
		pWeaponBox->PackWeapon(rgpPackWeapons[iPW]);

		iPW++;
	}

	pWeaponBox->pev->velocity = pev->velocity * 1.2; // weaponbox has player's velocity, then some.

	RemoveAllItems(true); // now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems(bool removeSuit)
{
	if (m_pActiveWeapon)
	{
		ResetAutoaim();
		m_pActiveWeapon->Holster();
		m_pActiveWeapon = nullptr;
	}

	m_pLastWeapon = nullptr;

	if (m_pTank != nullptr)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = nullptr;
	}

	int i;
	CBasePlayerWeapon* pendingWeapon;
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		m_pActiveWeapon = m_rgpPlayerWeapons[i];
		while (m_pActiveWeapon)
		{
			pendingWeapon = m_pActiveWeapon->m_pNext;
			m_pActiveWeapon->Kill();
			m_pActiveWeapon = pendingWeapon;
		}
		m_rgpPlayerWeapons[i] = nullptr;
	}
	m_pActiveWeapon = nullptr;

	pev->viewmodel = string_t::Null;
	pev->weaponmodel = string_t::Null;

	m_WeaponBits = 0ULL;

	// Re-add suit bit if needed.
	SetHasSuit(!removeSuit);

	for (i = 0; i < MAX_AMMO_TYPES; i++)
		m_rgAmmo[i] = 0;

	UpdateClientData();
}

void CBasePlayer::Killed(CBaseEntity* attacker, int iGib)
{
	CSound* pSound;

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveWeapon)
		m_pActiveWeapon->Holster();

	g_pGameRules->PlayerKilled(this, attacker, g_pevLastInflictor);

	if (m_pTank != nullptr)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = nullptr;
	}

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
	{
		if (pSound)
		{
			pSound->Reset();
		}
	}

	SetAnimation(PLAYER_DIE);

	m_iRespawnFrames = 0;

	pev->deadflag = DEAD_DYING;
	pev->movetype = MOVETYPE_TOSS;
	ClearBits(pev->flags, FL_ONGROUND);
	if (pev->velocity.z < 10)
		pev->velocity.z += RANDOM_FLOAT(0, 300);

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(nullptr, false, 0);

	// send "health" update message to zero
	m_iClientHealth = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgHealth, nullptr, edict());
	WRITE_SHORT(m_iClientHealth);
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, nullptr, edict());
	WRITE_BYTE(0);
	WRITE_BYTE(0XFF);
	WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, nullptr, edict());
	WRITE_BYTE(0);
	MESSAGE_END();


	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	// UTIL_ScreenFade( edict(), Vector(128,0,0), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

	if ((pev->health < -40 && iGib != GIB_NEVER) || iGib == GIB_ALWAYS)
	{
		pev->solid = SOLID_NOT;
		GibMonster(); // This clears pev->model
		pev->effects |= EF_NODRAW;
		return;
	}

	DeathSound();

	pev->angles.x = 0;
	pev->angles.z = 0;

	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
	int animDesired;
	float speed;
	char szAnim[64];

	speed = pev->velocity.Length2D();

	if ((pev->flags & FL_FROZEN) != 0)
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch (playerAnim)
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;

	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;

	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		m_IdealActivity = GetDeathActivity();
		break;

	case PLAYER_ATTACK1:
		switch (m_Activity)
		{
		case ACT_HOVER:
		case ACT_SWIM:
		case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if (!FBitSet(pev->flags, FL_ONGROUND) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP)) // Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else if (pev->waterlevel > WaterLevel::Feet)
		{
			if (speed == 0)
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;

	case PLAYER_GRAPPLE:
	{
		if (FBitSet(pev->flags, FL_ONGROUND))
		{
			if (pev->waterlevel > WaterLevel::Feet)
			{
				if (speed == 0)
					m_IdealActivity = ACT_HOVER;
				else
					m_IdealActivity = ACT_SWIM;
			}
			else
			{
				m_IdealActivity = ACT_WALK;
			}
		}
		else if (speed == 0)
		{
			m_IdealActivity = ACT_HOVER;
		}
		else
		{
			m_IdealActivity = ACT_SWIM;
		}
	}
	break;
	}

	switch (m_IdealActivity)
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if (m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity(m_Activity);
		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		pev->gaitsequence = 0;
		pev->sequence = animDesired;
		pev->frame = 0;
		ResetSequenceInfo();
		return;

	case ACT_RANGE_ATTACK1:
		if (FBitSet(pev->flags, FL_DUCKING)) // crouching
			strcpy(szAnim, "crouch_shoot_");
		else
			strcpy(szAnim, "ref_shoot_");
		strcat(szAnim, m_szAnimExtention);
		animDesired = LookupSequence(szAnim);
		if (animDesired == -1)
			animDesired = 0;

		if (pev->sequence != animDesired || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		if (!m_fSequenceLoops)
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence = animDesired;
		ResetSequenceInfo();
		break;

	case ACT_WALK:
		if (m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished)
		{
			if (FBitSet(pev->flags, FL_DUCKING)) // crouching
				strcpy(szAnim, "crouch_aim_");
			else
				strcpy(szAnim, "ref_aim_");
			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);
			if (animDesired == -1)
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
			animDesired = pev->sequence;
		}
	}

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		if (speed == 0)
		{
			pev->gaitsequence = LookupActivity(ACT_CROUCHIDLE);
			// pev->gaitsequence	= LookupActivity( ACT_CROUCH );
		}
		else
		{
			pev->gaitsequence = LookupActivity(ACT_CROUCH);
		}
	}
	else if (speed > 220)
	{
		pev->gaitsequence = LookupActivity(ACT_RUN);
	}
	else if (speed > 0)
	{
		pev->gaitsequence = LookupActivity(ACT_WALK);
	}
	else
	{
		// pev->gaitsequence	= LookupActivity( ACT_WALK );
		pev->gaitsequence = LookupSequence("deep_idle");
	}


	// Already using the desired animation?
	if (pev->sequence == animDesired)
		return;

	// Logger->debug("Set animation to {}", animDesired);
	//  Reset to first frame of desired animation
	pev->sequence = animDesired;
	pev->frame = 0;
	ResetSequenceInfo();
}

#define AIRTIME 12 // lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP)
		return;

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != WaterLevel::Head)
	{
		// not underwater

		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EmitSound(CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EmitSound(CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{ // fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (!m_bInfiniteAir && pev->air_finished < gpGlobals->time) // drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;
				TakeDamage(World, World, pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += pev->dmg;
			}
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (WaterLevel::Dry == pev->waterlevel)
	{
		if (FBitSet(pev->flags, FL_INWATER))
		{
			ClearBits(pev->flags, FL_INWATER);
		}
		return;
	}

	// make bubbles

	if (pev->waterlevel == WaterLevel::Head)
	{
		air = (int)(pev->air_finished - gpGlobals->time);
		if (!RANDOM_LONG(0, 0x1f) && RANDOM_LONG(0, AIRTIME - 1) >= air)
		{
			switch (RANDOM_LONG(0, 3))
			{
			case 0:
				EmitSound(CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM);
				break;
			case 1:
				EmitSound(CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM);
				break;
			case 2:
				EmitSound(CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM);
				break;
			case 3:
				EmitSound(CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM);
				break;
			}
		}
	}

	if (pev->watertype == CONTENTS_LAVA) // do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(World, World, 10 * static_cast<int>(pev->waterlevel), DMG_BURN);
	}
	else if (pev->watertype == CONTENTS_SLIME) // do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(World, World, 4 * static_cast<int>(pev->waterlevel), DMG_ACID);
	}

	if (!FBitSet(pev->flags, FL_INWATER))
	{
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}
}

bool CBasePlayer::IsOnLadder()
{
	return (pev->movetype == MOVETYPE_FLY);
}

void CBasePlayer::PlayerDeathThink()
{
	float flForward;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		flForward = pev->velocity.Length() - 20;
		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	if (HasWeapons())
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}


	if (0 != pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
	{
		StudioFrameAdvance();

		m_iRespawnFrames++;			// Note, these aren't necessarily real "frames", so behavior is dependent on # of client movement commands
		if (m_iRespawnFrames < 120) // Animations should be no longer than this
			return;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if (pev->movetype != MOVETYPE_NONE && FBitSet(pev->flags, FL_ONGROUND))
		pev->movetype = MOVETYPE_NONE;

	if (pev->deadflag == DEAD_DYING)
		pev->deadflag = DEAD_DEAD;

	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0.0;

	bool fAnyButtonDown = (pev->button & ~IN_SCORE) != 0;

	// wait for all buttons released
	if (pev->deadflag == DEAD_DEAD)
	{
		if (fAnyButtonDown)
			return;

		if (g_pGameRules->FPlayerCanRespawn(this))
		{
			m_fDeadTime = gpGlobals->time;
			pev->deadflag = DEAD_RESPAWNABLE;
		}

		return;
	}

	// if the player has been dead for one second longer than allowed by forcerespawn,
	// forcerespawn isn't on. Send the player off to an intermission camera until they
	// choose to respawn.
	if (g_pGameRules->IsMultiplayer() && (gpGlobals->time > (m_fDeadTime + 6)) && (m_afPhysicsFlags & PFLAG_OBSERVER) == 0)
	{
		// go to dead camera.
		StartDeathCam();
	}

	if (0 != pev->iuser1) // player is in spectator mode
		return;

	// wait for any button down,  or mp_forcerespawn is set and the respawn time is up
	if (!fAnyButtonDown && !(g_pGameRules->IsMultiplayer() && forcerespawn.value > 0 && (gpGlobals->time > (m_fDeadTime + 5))))
		return;

	pev->button = 0;
	m_iRespawnFrames = 0;

	// Logger->debug("Respawn");

	respawn(this, (m_afPhysicsFlags & PFLAG_OBSERVER) == 0); // don't copy a corpse if we're in deathcam.
	pev->nextthink = -1;
}

void CBasePlayer::StartDeathCam()
{
	if (pev->view_ofs == g_vecZero)
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	auto pSpot = UTIL_FindEntityByClassname(nullptr, "info_intermission");

	if (!FNullEnt(pSpot))
	{
		// at least one intermission spot in the world.
		int iRand = RANDOM_LONG(0, 3);

		CBaseEntity* pNewSpot;

		while (iRand > 0)
		{
			pNewSpot = UTIL_FindEntityByClassname(pSpot, "info_intermission");

			if (pNewSpot)
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		CopyToBodyQue(this);

		SetOrigin(pSpot->pev->origin);
		pev->angles = pev->v_angle = pSpot->pev->v_angle;
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		CopyToBodyQue(this);
		UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, 128), ignore_monsters, edict(), &tr);

		SetOrigin(tr.vecEndPos);
		pev->angles = pev->v_angle = UTIL_VecToAngles(tr.vecEndPos - pev->origin);
	}

	// start death cam

	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->view_ofs = g_vecZero;
	pev->fixangle = FIXANGLE_ABSOLUTE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->modelindex = 0;
}

void CBasePlayer::StartObserver(Vector vecPosition, Vector vecViewAngle)
{
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_KILLPLAYERATTACHMENTS);
	WRITE_BYTE((byte)entindex());
	MESSAGE_END();

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveWeapon)
		m_pActiveWeapon->Holster();

	if (m_pTank != nullptr)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = nullptr;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(nullptr, false, 0);

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, nullptr, edict());
	WRITE_BYTE(0);
	WRITE_BYTE(0XFF);
	WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, nullptr, edict());
	WRITE_BYTE(0);
	MESSAGE_END();

	// Setup flags
	m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = FIXANGLE_ABSOLUTE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	ClearBits(m_afPhysicsFlags, PFLAG_DUCKING);
	ClearBits(pev->flags, FL_DUCKING);
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

	// Clear out the status bar
	m_fInitHUD = true;

	pev->team = string_t::Null;
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(entindex());
	WRITE_STRING("");
	MESSAGE_END();

	// Remove all the player's stuff
	RemoveAllItems(false);

	// Move them to the new position
	SetOrigin(vecPosition);

	// Find a player to watch
	m_flNextObserverInput = 0;
	Observer_SetMode(m_iObserverLastMode);
}

#define PLAYER_SEARCH_RADIUS (float)64

void CBasePlayer::PlayerUse()
{
	if (IsObserver())
		return;

	// Was use pressed or released?
	if (((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE) == 0)
		return;

	// Hit Use on a train?
	if ((m_afButtonPressed & IN_USE) != 0)
	{
		if (m_pTank != nullptr)
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = nullptr;
			return;
		}
		else
		{
			if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
			else
			{ // Start controlling the train!
				CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);

				if (pTrain && (pev->button & IN_JUMP) == 0 && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) != 0 && pTrain->OnControls(this))
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;
					EmitSound(CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
					return;
				}
			}
		}
	}

	CBaseEntity* pObject = nullptr;
	CBaseEntity* pClosest = nullptr;
	Vector vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors(pev->v_angle); // so we know which way we are facing

	while ((pObject = UTIL_FindEntityInSphere(pObject, pev->origin, PLAYER_SEARCH_RADIUS)) != nullptr)
	{
		// Special behavior for ropes: check if the player is close enough to the rope segment origin
		if (pObject->ClassnameIs("rope_segment"))
		{
			if ((pev->origin - pObject->pev->origin).Length() > PLAYER_SEARCH_RADIUS)
			{
				continue;
			}
		}

		if ((pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE)) != 0)
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin(pObject) - (pev->origin + pev->view_ofs));

			// This essentially moves the origin of the target to the corner nearest the player to test to see
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox(vecLOS, pObject->pev->size * 0.5);

			flDot = DotProduct(vecLOS, gpGlobals->v_forward);
			if (flDot > flMaxDot)
			{ // only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
				// Logger->debug("{} : {}", STRING(pObject->pev->classname), flDot);
			}
			// Logger->debug("{} : {}", STRING(pObject->pev->classname), flDot);
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject)
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls
		int caps = pObject->ObjectCaps();

		if ((m_afButtonPressed & IN_USE) != 0)
			EmitSound(CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

		if (((pev->button & IN_USE) != 0 && (caps & FCAP_CONTINUOUS_USE) != 0) ||
			((m_afButtonPressed & IN_USE) != 0 && (caps & (FCAP_IMPULSE_USE | FCAP_ONOFF_USE)) != 0))
		{
			if ((caps & FCAP_CONTINUOUS_USE) != 0)
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use(this, this, USE_SET, 1);
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ((m_afButtonReleased & IN_USE) != 0 && (pObject->ObjectCaps() & FCAP_ONOFF_USE) != 0) // BUGBUG This is an "off" use
		{
			pObject->Use(this, this, USE_SET, 0);
		}
	}
	else
	{
		if ((m_afButtonPressed & IN_USE) != 0)
			EmitSound(CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
	}
}

void CBasePlayer::Jump()
{
	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;

	if (pev->waterlevel >= WaterLevel::Waist)
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if (!FBitSet(m_afButtonPressed, IN_JUMP))
		return; // don't pogo stick

	if ((pev->flags & FL_ONGROUND) == 0 || !pev->groundentity)
	{
		return;
	}

	// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors(pev->angles);

	// ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk

	SetAnimation(PLAYER_JUMP);

	if (m_fLongJump &&
		(pev->button & IN_DUCK) != 0 &&
		(pev->flDuckTime > 0) &&
		pev->velocity.Length() > 50)
	{
		SetAnimation(PLAYER_SUPERJUMP);
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	CBaseEntity* ground = GetGroundEntity();
	if (ground && (ground->pev->flags & FL_CONVEYOR) != 0)
	{
		pev->velocity = pev->velocity + pev->basevelocity;
	}
}

// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck(CBaseEntity* pPlayer)
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for (int i = 0; i < 18; i++)
	{
		UTIL_TraceHull(pPlayer->pev->origin, pPlayer->pev->origin, dont_ignore_monsters, head_hull, pPlayer->edict(), &trace);
		if (0 != trace.fStartSolid)
			pPlayer->pev->origin.z++;
		else
			break;
	}
}

void CBasePlayer::Duck()
{
	if ((pev->button & IN_DUCK) != 0)
	{
		if (m_IdealActivity != ACT_LEAP)
		{
			SetAnimation(PLAYER_WALK);
		}
	}
}

int CBasePlayer::Classify()
{
	return CLASS_PLAYER;
}

void CBasePlayer::AddPoints(int score, bool bAllowNegativeScore)
{
	// Positive score always adds
	if (score < 0)
	{
		if (!bAllowNegativeScore)
		{
			if (pev->frags < 0) // Can't go more negative
				return;

			if (-score > pev->frags) // Will this go negative?
			{
				score = -pev->frags; // Sum will be 0
			}
		}
	}

	pev->frags += score;

	SendScoreInfoAll();
}

void CBasePlayer::AddPointsToTeam(int score, bool bAllowNegativeScore)
{
	int index = entindex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		auto pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer && i != index)
		{
			if (g_pGameRules->PlayerRelationship(this, pPlayer) == GR_TEAMMATE)
			{
				pPlayer->AddPoints(score, bAllowNegativeScore);
			}
		}
	}
}

void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0;
}

void CBasePlayer::UpdateStatusBar()
{
	int newSBarState[SBAR_END];
	char sbuf0[SBAR_STRING_SIZE];
	char sbuf1[SBAR_STRING_SIZE];

	memset(newSBarState, 0, sizeof(newSBarState));
	strcpy(sbuf0, m_SbarString0);
	strcpy(sbuf1, m_SbarString1);

	// Find an ID Target
	TraceResult tr;
	UTIL_MakeVectors(pev->v_angle + pev->punchangle);
	Vector vecSrc = EyePosition();
	Vector vecEnd = vecSrc + (gpGlobals->v_forward * MAX_ID_RANGE);
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);

	if (tr.flFraction != 1.0)
	{
		if (!FNullEnt(tr.pHit))
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

			if (pEntity->IsPlayer())
			{
				newSBarState[SBAR_ID_TARGETNAME] = pEntity->entindex();
				strcpy(sbuf1, "1 %p1\n2 Health: %i2%%\n3 Armor: %i3%%");

				// allies and medics get to see the targets health
				if (g_pGameRules->PlayerRelationship(this, pEntity) == GR_TEAMMATE)
				{
					newSBarState[SBAR_ID_TARGETHEALTH] = 100 * (pEntity->pev->health / pEntity->pev->max_health);
					newSBarState[SBAR_ID_TARGETARMOR] = pEntity->pev->armorvalue; // No need to get it % based since 100 it's the max.
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
			}
		}
		else if (m_flStatusBarDisappearDelay > gpGlobals->time)
		{
			// hold the values for a short amount of time after viewing the object
			newSBarState[SBAR_ID_TARGETNAME] = m_izSBarState[SBAR_ID_TARGETNAME];
			newSBarState[SBAR_ID_TARGETHEALTH] = m_izSBarState[SBAR_ID_TARGETHEALTH];
			newSBarState[SBAR_ID_TARGETARMOR] = m_izSBarState[SBAR_ID_TARGETARMOR];
		}
	}

	bool bForceResend = false;

	if (0 != strcmp(sbuf0, m_SbarString0))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, nullptr, edict());
		WRITE_BYTE(0);
		WRITE_STRING(sbuf0);
		MESSAGE_END();

		strcpy(m_SbarString0, sbuf0);

		// make sure everything's resent
		bForceResend = true;
	}

	if (0 != strcmp(sbuf1, m_SbarString1))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, nullptr, edict());
		WRITE_BYTE(1);
		WRITE_STRING(sbuf1);
		MESSAGE_END();

		strcpy(m_SbarString1, sbuf1);

		// make sure everything's resent
		bForceResend = true;
	}

	// Check values and send if they don't match
	for (int i = 1; i < SBAR_END; i++)
	{
		if (newSBarState[i] != m_izSBarState[i] || bForceResend)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusValue, nullptr, edict());
			WRITE_BYTE(i);
			WRITE_SHORT(newSBarState[i]);
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}

static void ClearEntityInfo(CBasePlayer* player)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgEntityInfo, nullptr, player->edict());
	WRITE_STRING("");
	MESSAGE_END();
}

void CBasePlayer::UpdateEntityInfo()
{
	const bool isEnabled = sv_entityinfo_enabled.value != 0;

	if (m_EntityInfoEnabled != isEnabled)
	{
		// Clear out any info that may be drawn.
		if (m_EntityInfoEnabled)
		{
			ClearEntityInfo(this);
		}

		m_EntityInfoEnabled = isEnabled;
	}

	if (!isEnabled)
	{
		return;
	}

	if (m_NextEntityInfoUpdateTime > gpGlobals->time)
	{
		return;
	}

	m_NextEntityInfoUpdateTime = gpGlobals->time + EntityInfoUpdateInterval;

	const auto entity = UTIL_FindEntityForward(this);

	if (entity)
	{
		// Pick a color based on entity type.
		RGB24 color{RGB_WHITE};

		// Not all NPCs set the FL_MONSTER flag (e.g. Barnacle).
		const auto monster = entity->MyMonsterPointer();

		if (monster)
		{
			const auto classification = monster->Classify();

			if (classification != CLASS_NONE && classification >= CLASS_FIRST && classification <= CLASS_LAST)
			{
				color = [](int relationship)
				{
					// Make sure these are colorblind-friendly.
					switch (relationship)
					{
					case R_AL: return RGB_BLUEISH;
					case R_DL: [[fallthrough]];
					case R_HT: [[fallthrough]];
					case R_NM: return RGB_REDISH;
					default: return RGB_WHITE;
					}
				}(monster->IRelationship(this));
			}
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgEntityInfo, nullptr, edict());
		WRITE_STRING(STRING(entity->pev->classname));
		WRITE_LONG(std::clamp(static_cast<int>(entity->pev->health), std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max()));
		WRITE_RGB24(color);
		MESSAGE_END();
	}
	// Clear out the previous entity's info if requested. Otherwise the client will clear it after the draw time has passed.
	else if (sv_entityinfo_eager.value != 0)
	{
		ClearEntityInfo(this);
	}
}

#define CLIMB_SHAKE_FREQUENCY 22 // how many frames in between screen shakes when climbing
#define MAX_CLIMB_SPEED 200		 // fastest vertical climbing speed possible
#define CLIMB_SPEED_DEC 15		 // climbing deceleration rate
#define CLIMB_PUNCH_X -7		 // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z 7			 // how far to 'punch' client Z axis when climbing

void CBasePlayer::PreThink()
{
	int buttonsChanged = (m_afButtonLast ^ pev->button); // These buttons have changed this frame

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed = buttonsChanged & pev->button;	  // The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button); // The ones not down are "released"

	g_pGameRules->PlayerThink(this);

	if (g_fGameOver)
		return; // intermission or finale

	UpdateShockEffect();

	UTIL_MakeVectors(pev->v_angle); // is this still used?

	ItemPreFrame();
	WaterMove();

	if (g_pGameRules && g_pGameRules->FAllowFlashlight())
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	if (m_bResetViewEntity)
	{
		m_bResetViewEntity = false;

		CBaseEntity* viewEntity = m_hViewEntity;

		if (viewEntity)
		{
			SET_VIEW(edict(), viewEntity->edict());
		}
	}

	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.

	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	if (0 != WorldGraph.m_fGraphPresent && 0 == WorldGraph.m_fGraphPointersSet)
	{
		if (!WorldGraph.FSetGraphPointers())
		{
			CGraph::Logger->debug("**Graph pointers were not set!");
		}
		else
		{
			CGraph::Logger->debug("**Graph Pointers Set!");
		}
	}

	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();

	CheckTimeBasedDamage();

	CheckSuitUpdate();

	// Observer Button Handling
	if (IsObserver())
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
		pev->impulse = 0;
		return;
	}

	if (pev->deadflag >= DEAD_DYING)
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
		pev->flags |= FL_ONTRAIN;
	else
		pev->flags &= ~FL_ONTRAIN;

	// We're on a rope.
	if ((m_afPhysicsFlags & PFLAG_ONROPE) != 0 && m_pRope)
	{
		pev->velocity = g_vecZero;

		const Vector vecAttachPos = m_pRope->GetAttachedObjectsPosition();

		pev->origin = vecAttachPos;

		Vector vecForce;

		if ((pev->button & IN_MOVERIGHT) != 0)
		{
			vecForce.x = gpGlobals->v_right.x;
			vecForce.y = gpGlobals->v_right.y;
			vecForce.z = 0;

			m_pRope->ApplyForceFromPlayer(vecForce);
		}

		if ((pev->button & IN_MOVELEFT) != 0)
		{
			vecForce.x = -gpGlobals->v_right.x;
			vecForce.y = -gpGlobals->v_right.y;
			vecForce.z = 0;
			m_pRope->ApplyForceFromPlayer(vecForce);
		}

		// Determine if any force should be applied to the rope, or if we should move around.
		if ((pev->button & (IN_BACK | IN_FORWARD)) != 0)
		{
			if ((gpGlobals->v_forward.x * gpGlobals->v_forward.x +
					gpGlobals->v_forward.y * gpGlobals->v_forward.y -
					gpGlobals->v_forward.z * gpGlobals->v_forward.z) <= 0)
			{
				if (m_bIsClimbing)
				{
					const float flDelta = gpGlobals->time - m_flLastClimbTime;
					m_flLastClimbTime = gpGlobals->time;

					if ((pev->button & IN_FORWARD) != 0)
					{
						if (gpGlobals->v_forward.z < 0.0)
						{
							if (!m_pRope->MoveDown(flDelta))
							{
								// Let go of the rope, detach.
								pev->movetype = MOVETYPE_WALK;
								pev->solid = SOLID_SLIDEBOX;

								m_afPhysicsFlags &= ~PFLAG_ONROPE;
								m_pRope->DetachObject();
								m_pRope = nullptr;
								m_bIsClimbing = false;
							}
						}
						else
						{
							m_pRope->MoveUp(flDelta);
						}
					}
					if ((pev->button & IN_BACK) != 0)
					{
						if (gpGlobals->v_forward.z < 0.0)
						{
							m_pRope->MoveUp(flDelta);
						}
						else if (!m_pRope->MoveDown(flDelta))
						{
							// Let go of the rope, detach.
							pev->movetype = MOVETYPE_WALK;
							pev->solid = SOLID_SLIDEBOX;
							m_afPhysicsFlags &= ~PFLAG_ONROPE;
							m_pRope->DetachObject();
							m_pRope = nullptr;
							m_bIsClimbing = false;
						}
					}
				}
				else
				{
					m_bIsClimbing = true;
					m_flLastClimbTime = gpGlobals->time;
				}
			}
			else
			{
				vecForce.x = gpGlobals->v_forward.x;
				vecForce.y = gpGlobals->v_forward.y;
				vecForce.z = 0.0;
				if ((pev->button & IN_BACK) != 0)
				{
					vecForce.x = -gpGlobals->v_forward.x;
					vecForce.y = -gpGlobals->v_forward.y;
					vecForce.z = 0;
				}
				m_pRope->ApplyForceFromPlayer(vecForce);
				m_bIsClimbing = false;
			}
		}
		else
		{
			m_bIsClimbing = false;
		}

		if ((m_afButtonPressed & IN_JUMP) != 0)
		{
			// We've jumped off the rope, give us some momentum
			pev->movetype = MOVETYPE_WALK;
			pev->solid = SOLID_SLIDEBOX;
			m_afPhysicsFlags &= ~PFLAG_ONROPE;

			Vector vecDir = gpGlobals->v_up * 165.0 + gpGlobals->v_forward * 150.0;

			Vector vecVelocity = m_pRope->GetAttachedObjectsVelocity() * 2;

			vecVelocity = vecVelocity.Normalize();

			vecVelocity = vecVelocity * 200;

			pev->velocity = vecVelocity + vecDir;

			m_pRope->DetachObject();
			m_pRope = nullptr;
			m_bIsClimbing = false;
		}

		return;
	}

	// Train speed control
	if ((m_afPhysicsFlags & PFLAG_ONTRAIN) != 0)
	{
		CBaseEntity* pTrain = CBaseEntity::Instance(pev->groundentity);
		float vel;

		if (!pTrain)
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -38), ignore_monsters, edict(), &trainTrace);

			// HACKHACK - Just look for the func_tracktrain classname
			if (trainTrace.flFraction != 1.0 && trainTrace.pHit)
				pTrain = CBaseEntity::Instance(trainTrace.pHit);


			if (!pTrain || (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) == 0 || !pTrain->OnControls(this))
			{
				// Logger->error("In train mode with no train!");
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
		}
		else if (!FBitSet(pev->flags, FL_ONGROUND) || FBitSet(pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL) || (pev->button & (IN_MOVELEFT | IN_MOVERIGHT)) != 0)
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;
		if ((m_afButtonPressed & IN_FORWARD) != 0)
		{
			vel = 1;
			pTrain->Use(this, this, USE_SET, vel);
		}
		else if ((m_afButtonPressed & IN_BACK) != 0)
		{
			vel = -1;
			pTrain->Use(this, this, USE_SET, vel);
		}

		if (0 != vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if ((m_iTrain & TRAIN_ACTIVE) != 0)
		m_iTrain = TRAIN_NEW; // turn off train

	if ((pev->button & IN_JUMP) != 0)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}


	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) != 0 || FBitSet(pev->flags, FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) != 0)
		Duck();

	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = nullptr;

	if ((m_afPhysicsFlags & PFLAG_ONBARNACLE) != 0)
	{
		pev->velocity = g_vecZero;
	}
}

// TODO: move this documentation out of the source code and make sure it's correct.

/* Time based Damage works as follows:
	1) There are several types of timebased damage

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.
*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

void CBasePlayer::CheckTimeBasedDamage()
{
	int i;
	byte bDuration = 0;

	if ((m_bitsDamageType & DMG_TIMEBASED) == 0)
		return;

	// only check for time based damage approx. every 2 seconds
	if (fabs(gpGlobals->time - m_tbdPrev) < 2.0)
		return;

	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if ((m_bitsDamageType & (DMG_PARALYZE << i)) != 0)
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				//				TakeDamage(this, this, NERVEGAS_DAMAGE, DMG_GENERIC);
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(this, this, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				//				TakeDamage(this, this, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = std::min(m_idrowndmg - m_idrownrestored, 10);

					GiveHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4; // get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				//				TakeDamage(this, this, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				//				TakeDamage(this, this, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				//				TakeDamage(this, this, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (0 != m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison) && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (0 != m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", false, SUIT_REPEAT_OK);
					}
				}


				// decrement damage duration, detect when done.
				if (0 == m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation.
Some functions are automatic, some require power.
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit
		will come on and the battery will drain while the player stays in the area.
		After the battery is dead, the player starts to take damage.
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge.

Notification (via the HUD)

x	Health
x	Ammo
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged.
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor.

Augmentation

	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA
		Used automatically after picked up and after player enters the water.
		Works for N seconds. Single use.

Things powered by the battery

	Armor
		Uses N watts for every M units of damage.
*/

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

void CBasePlayer::UpdateGeigerCounter()
{
	byte range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client

	range = (byte)(m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN(MSG_ONE, gmsgGeigerRange, nullptr, edict());
		WRITE_BYTE(range);
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0, 3))
		m_flgeigerRange = 1000;
}

#define SUITUPDATETIME 3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate()
{
	int i;
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	// Ignore suit updates if no suit
	if (!HasSuit())
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if (g_pGameRules->IsMultiplayer())
	{
		// don't bother updating HEV voice in multiplayer.
		return;
	}

	if (gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (i = 0; i < CSUITPLAYLIST; i++)
		{
			isentence = m_rgSuitPlayList[isearch];
			if (0 != isentence)
				break;

			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
		}

		if (0 != isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[sentences::CBSENTENCENAME_MAX + 1];
				strcpy(sentence, "!");
				strcat(sentence, sentences::g_Sentences.GetSentenceNameByIndex(isentence));
				EMIT_SOUND_SUIT(this, sentence);
			}
			else
			{
				// play sentence group
				EMIT_GROUPID_SUIT(this, -isentence);
			}
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check
			m_flSuitUpdate = 0;
	}
}

void CBasePlayer::SetSuitUpdate(const char* name, bool fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;


	// Ignore suit updates if no suit
	if (!HasSuit())
		return;

	if (g_pGameRules->IsMultiplayer())
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == nullptr, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = sentences::g_Sentences.LookupSentence(this, name, nullptr);
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -sentences::g_Sentences.GetGroupIndex(this, name);

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
		{
			// this sentence or group is already in
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if (0 == m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (0 != iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT - 1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot

	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
	}
}

void CBasePlayer::UpdatePlayerSound()
{
	int iBodyVolume;
	int iVolume;
	CSound* pSound;

	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	if (!pSound)
	{
		Logger->debug("Client lost reserved sound!");
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		iBodyVolume = pev->velocity.Length();

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast.
		if (iBodyVolume > 512)
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if ((pev->button & IN_JUMP) != 0)
	{
		iBodyVolume += 100;
	}

	// convert player move speed and actions into sound audible by monsters.
	if (m_iWeaponVolume > iBodyVolume)
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player.
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= 250 * gpGlobals->frametime;
	if (m_iWeaponVolume < 0)
	{
		iVolume = 0;
	}


	// if target volume is greater than the player sound's current volume, we paste the new volume in
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if (m_iTargetVolume > iVolume)
	{
		iVolume = m_iTargetVolume;
	}
	else if (iVolume > m_iTargetVolume)
	{
		iVolume -= 250 * gpGlobals->frametime;

		if (iVolume < m_iTargetVolume)
		{
			iVolume = 0;
		}
	}

	if (m_fNoPlayerSound)
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if (gpGlobals->time > m_flStopExtraSoundTime)
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if (pSound)
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= (bits_SOUND_PLAYER | m_iExtraSoundTypes);
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= 256 * gpGlobals->frametime;
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	// UTIL_MakeVectors ( pev->angles );
	// gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the
	// player is making.
	// UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	// Logger->debug("{}/{}", iVolume, m_iTargetVolume);
}

void CBasePlayer::PostThink()
{
	if (g_fGameOver)
		goto pt_end; // intermission or finale

	if (!IsAlive())
		goto pt_end;

	// Handle Tank controlling
	if (m_pTank != nullptr)
	{ // if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if (m_pTank->OnControls(this) && FStringNull(pev->weaponmodel))
		{
			m_pTank->Use(this, this, USE_SET, 2); // try fire the gun
		}
		else
		{ // they've moved off the platform
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = nullptr;
		}
	}

	// do weapon stuff
	ItemPostFrame();

	// check to see if player landed hard enough to make a sound
	// falling farther than half of the maximum safe distance, but not as far a max safe distance will
	// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
	// of maximum safe distance will make no sound. Falling farther than max safe distance will play a
	// fallpain sound, and damage will be inflicted based on how far the player fell

	if ((FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		// Logger->debug("{}", m_flFallVelocity);

		if (pev->watertype == CONTENTS_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when
			// BUG - water has current force
			// if (auto groundEntity = GetGroundEntity(); !groundEntity || groundEntity->pev->velocity.z == 0)
			// EmitSound(CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{ // after this point, we start doing damage

			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			if (flFallDamage > pev->health)
			{ // splat
				// note: play on item channel because we play footstep landing on body channel
				EmitSound(CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}

			if (flFallDamage > 0)
			{
				TakeDamage(World, World, flFallDamage, DMG_FALL);
				pev->punchangle.x = 0;
			}
		}

		if (IsAlive())
		{
			SetAnimation(PLAYER_WALK);
		}
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
		{
			CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2);
			// Logger->debug("fall {}", m_flFallVelocity);
		}
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character
	if (IsAlive())
	{
		if (0 == pev->velocity.x && 0 == pev->velocity.y)
			SetAnimation(PLAYER_IDLE);
		else if ((0 != pev->velocity.x || 0 != pev->velocity.y) && (FBitSet(pev->flags, FL_ONGROUND)))
			SetAnimation(PLAYER_WALK);
		else if (pev->waterlevel > WaterLevel::Feet)
			SetAnimation(PLAYER_WALK);
	}

	StudioFrameAdvance();

	UpdatePlayerSound();

pt_end:
#if defined(CLIENT_WEAPONS)
	// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		if (m_rgpPlayerWeapons[i])
		{
			CBasePlayerWeapon* gun = m_rgpPlayerWeapons[i];

			while (gun)
			{
				if (gun->UseDecrement())
				{
					gun->m_flNextPrimaryAttack = std::max(gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.1f);
					gun->m_flNextSecondaryAttack = std::max(gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001f);

					if (gun->m_flTimeWeaponIdle != 1000)
					{
						gun->m_flTimeWeaponIdle = std::max(gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001f);
					}

					if (gun->pev->fuser1 != 1000)
					{
						gun->pev->fuser1 = std::max(gun->pev->fuser1 - gpGlobals->frametime, -0.001f);
					}

					gun->DecrementTimers();

					// Only decrement if not flagged as NO_DECREMENT
					// if (gun->m_flPumpTime != 1000)
					// {
					//	gun->m_flPumpTime = std::max(gun->m_flPumpTime - gpGlobals->frametime, -0.001f);
					// }
				}

				gun = gun->m_pNext;
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if (m_flNextAttack < -0.001)
		m_flNextAttack = -0.001;

	if (m_flNextAmmoBurn != 1000)
	{
		m_flNextAmmoBurn -= gpGlobals->frametime;

		if (m_flNextAmmoBurn < -0.001)
			m_flNextAmmoBurn = -0.001;
	}

	if (m_flAmmoStartCharge != 1000)
	{
		m_flAmmoStartCharge -= gpGlobals->frametime;

		if (m_flAmmoStartCharge < -0.001)
			m_flAmmoStartCharge = -0.001;
	}
#endif

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;
}

void CBasePlayer::Spawn()
{
	m_bIsSpawning = true;

	// Make sure this gets reset even if somebody adds an early return or throws an exception.
	const CallOnDestroy resetIsSpawning{[this]()
		{
			// Done spawning; reset.
			m_bIsSpawning = false;
		}};

	pev->health = 100;
	pev->armorvalue = 0;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_WALK;
	pev->max_health = pev->health;
	pev->flags &= FL_PROXY | FL_FAKECLIENT; // keep proxy and fakeclient flags set by engine
	pev->flags |= FL_CLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2; // initial water damage
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0;
	pev->gravity = 1.0;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;
	SetHasLongJump(false); // no longjump module.

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "jpj", "0");

	m_iFOV = 0;		   // init field of view.
	m_iClientFOV = -1; // make sure fov reset is sent
	m_ClientSndRoomtype = -1;

	m_flNextDecalTime = 0; // let this player decal as soon as he spawns.

	m_DisplacerReturn = g_vecZero;
	m_flgeigerDelay = gpGlobals->time + 2.0; // wait a few seconds until user-defined message registrations
											 // are recieved by all clients

	m_flTimeStepSound = 0;
	m_iStepLeft = 0;
	m_flFieldOfView = 0.5; // some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor = BLOOD_COLOR_RED;
	m_flNextAttack = UTIL_WeaponTimeBase();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	if (!g_pGameRules->IsCTF())
		g_pGameRules->SetDefaultPlayerTeam(this);

	if (g_pGameRules->IsCTF() && m_iTeamNum == CTFTeam::None)
	{
		pev->iuser1 = OBS_ROAMING;
	}

	g_pGameRules->GetPlayerSpawnSpot(this);

	SetModel("models/player.mdl");
	pev->sequence = LookupActivity(ACT_IDLE);

	if (FBitSet(pev->flags, FL_DUCKING))
		SetSize(VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		SetSize(VEC_HULL_MIN, VEC_HULL_MAX);

	pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos = Vector(0, 32, 0);

	if (m_iPlayerSound == SOUNDLIST_EMPTY)
	{
		Logger->debug("Couldn't alloc player sound slot!");
	}

	m_fNoPlayerSound = false; // normal sound behavior.

	m_pLastWeapon = nullptr;
	m_fInitHUD = true;
	m_iClientHideHUD = -1; // force this to be recalculated
	m_fWeapon = false;
	m_pClientActiveWeapon = nullptr;
	m_iClientBattery = -1;

	// reset all ammo values to 0
	for (int i = 0; i < MAX_AMMO_TYPES; i++)
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0; // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;
	m_iLastDamage = 0;
	m_bIsClimbing = false;
	m_flLastDamageTime = 0;

	m_flNextChatTime = gpGlobals->time;

	g_pGameRules->PlayerSpawn(this);

	if (g_pGameRules->IsCTF() && m_iTeamNum == CTFTeam::None)
	{
		pev->effects |= EF_NODRAW;
		pev->iuser1 = OBS_ROAMING;
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NOCLIP;
		pev->takedamage = DAMAGE_NO;
		m_iHideHUD = HIDEHUD_WEAPONS | HIDEHUD_HEALTH;
		m_afPhysicsFlags |= PFLAG_OBSERVER;
		pev->flags |= FL_SPECTATOR;

		MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
		WRITE_BYTE(entindex());
		WRITE_BYTE(1);
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_ONE, gmsgTeamFull, nullptr, edict());
		WRITE_BYTE(0);
		MESSAGE_END();

		m_pGoalEnt = nullptr;

		m_iCurrentMenu = m_iNewTeamNum > CTFTeam::None ? MENU_DEFAULT : MENU_CLASS;

		if (g_pGameRules->IsCTF())
			Player_Menu();
	}
}

void CBasePlayer::Precache()
{
	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;

	m_iTrain |= TRAIN_NEW;

	m_iUpdateTime = 5; // won't update for 1/2 a second

	if (gInitHUD)
		m_fInitHUD = true;
}

void CBasePlayer::RenewItems()
{
}

void CBasePlayer::PostRestore()
{
	BaseClass::PostRestore();

	SAVERESTOREDATA* pSaveData = (SAVERESTOREDATA*)gpGlobals->pSaveData;
	// landmark isn't present.
	if (0 == pSaveData->fUseLandmark)
	{
		Logger->debug("No Landmark:{}", pSaveData->szLandmarkName);

		// default to normal spawn
		CBaseEntity* pSpawnSpot = EntSelectSpawnPoint(this);
		pev->origin = pSpawnSpot->pev->origin + Vector(0, 0, 1);
		pev->angles = pSpawnSpot->pev->angles;
	}
	pev->v_angle.z = 0; // Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = FIXANGLE_ABSOLUTE; // turn this way immediately

	m_iClientFOV = -1; // Make sure the client gets the right FOV value.
	m_ClientSndRoomtype = -1;

	// Reset room type on level change.
	if (!FStrEq(pSaveData->szCurrentMapName, STRING(gpGlobals->mapname)))
	{
		m_SndRoomtype = 0;
	}

	// Copied from spawn() for now
	m_bloodColor = BLOOD_COLOR_RED;

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		// Use the crouch HACK
		// FixPlayerCrouchStuck(this);
		// Don't need to do this with new player prediction code.
		SetSize(VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		SetSize(VEC_HULL_MIN, VEC_HULL_MAX);
	}

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	SetHasLongJump(m_fLongJump);

	RenewItems();

#if defined(CLIENT_WEAPONS)
	// HACK:	This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//			as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//			Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase();
#endif

	// If we have an active item and it has valid model strings, reset the models now.
	// Otherwise the weapon will do this itself.
	if (m_pActiveWeapon)
	{
		if (!FStringNull(m_pActiveWeapon->m_ViewModel) && !FStringNull(m_pActiveWeapon->m_PlayerModel))
		{
			m_pActiveWeapon->SetWeaponModels(STRING(m_pActiveWeapon->m_ViewModel), STRING(m_pActiveWeapon->m_PlayerModel));
		}
	}

	m_bResetViewEntity = true;

	m_bRestored = true;
}

void CBasePlayer::SelectNextItem(int iItem)
{
	CBasePlayerWeapon* weapon = m_rgpPlayerWeapons[iItem];

	if (!weapon)
		return;

	if (weapon == m_pActiveWeapon)
	{
		// select the next one in the chain
		weapon = m_pActiveWeapon->m_pNext;
		if (!weapon)
		{
			return;
		}

		CBasePlayerWeapon* pLast;
		pLast = weapon;
		while (pLast->m_pNext)
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveWeapon;
		m_pActiveWeapon->m_pNext = nullptr;
		m_rgpPlayerWeapons[iItem] = weapon;
	}

	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->Holster();
	}

	m_pActiveWeapon = weapon;

	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->Deploy();
		m_pActiveWeapon->UpdateItemInfo();
	}
}

bool CBasePlayer::HasWeapons()
{
	int i;

	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		if (m_rgpPlayerWeapons[i])
		{
			return true;
		}
	}

	return false;
}

void CBasePlayer::SelectPrevItem(int iItem)
{
}

const char* CBasePlayer::TeamID()
{
	if (pev == nullptr) // Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}

/**
 *	@brief !!!UNDONE:ultra temporary SprayCan entity to apply decal frame at a time. For PreAlpha CD
 */
class CSprayCan : public CBaseEntity
{
public:
	void Spawn(CBaseEntity* owner);
	void Think() override;

	int ObjectCaps() override { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS(spraycan, CSprayCan);

void CSprayCan::Spawn(CBaseEntity* owner)
{
	pev->origin = owner->pev->origin + Vector(0, 0, 32);
	pev->angles = owner->pev->v_angle;
	pev->owner = owner->edict();
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EmitSound(CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM);
}

void CSprayCan::Think()
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer* pPlayer;

	pPlayer = (CBasePlayer*)GET_PRIVATE(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);

	// Logger->debug("Spray by player {}, {} of {}", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace(&tr, DECAL_LAMBDA6);
		UTIL_Remove(this);
	}
	else
	{
		UTIL_PlayerDecalTrace(&tr, playernum, pev->frame, true);
		// Just painted last custom frame.
		if (pev->frame++ >= (nFrames - 1))
			UTIL_Remove(this);
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class CBloodSplat : public CBaseEntity
{
public:
	void Spawn(CBaseEntity* owner);
	void Spray();
};

LINK_ENTITY_TO_CLASS(blood_splat, CBloodSplat);

void CBloodSplat::Spawn(CBaseEntity* owner)
{
	pev->origin = owner->pev->origin + Vector(0, 0, 32);
	pev->angles = owner->pev->v_angle;
	pev->owner = owner->edict();

	SetThink(&CBloodSplat::Spray);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray()
{
	TraceResult tr;

	if (g_Language != LANGUAGE_GERMAN)
	{
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
	}
	SetThink(&CBloodSplat::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

CBaseItem* CBasePlayer::GiveNamedItem(std::string_view className, std::optional<int> defaultAmmo)
{
	// Only give items to player.
	auto entity = g_ItemDictionary->Create(className);

	if (FNullEnt(entity))
	{
		CBaseEntity::Logger->debug("nullptr Ent in GiveNamedItem!");
		return nullptr;
	}

	entity->pev->origin = pev->origin;
	entity->m_RespawnDelay = ITEM_NEVER_RESPAWN_DELAY;

	DispatchSpawn(entity->edict());

	if (defaultAmmo)
	{
		if (auto weapon = dynamic_cast<CBasePlayerWeapon*>(entity); weapon)
		{
			weapon->m_iDefaultAmmo = *defaultAmmo;
		}
	}

	if (entity->AddToPlayer(this) != ItemAddResult::Added)
	{
		g_engfuncs.pfnRemoveEntity(entity->edict());
		return nullptr;
	}

	return entity;
}

int CBasePlayer::GetFlashlightFlag() const
{
	switch (m_SuitLightType)
	{
	default:
	case SuitLightType::Flashlight:
		return EF_DIMLIGHT;

	case SuitLightType::Nightvision:
		return EF_BRIGHTLIGHT;
	}
}

bool CBasePlayer::FlashlightIsOn()
{
	return FBitSet(pev->effects, GetFlashlightFlag());
}

static void UpdateFlashlight(CBasePlayer* player, bool isOn)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, nullptr, player->edict());
	WRITE_BYTE(static_cast<int>(player->m_SuitLightType));
	WRITE_BYTE(isOn ? 1 : 0);
	WRITE_BYTE(player->m_iFlashBattery);
	MESSAGE_END();
}

void CBasePlayer::FlashlightTurnOn()
{
	if (!g_pGameRules->FAllowFlashlight())
	{
		return;
	}

	if (HasSuit())
	{
		auto onSound = [this]()
		{
			switch (m_SuitLightType)
			{
			default:
			case SuitLightType::Flashlight:
				return SOUND_FLASHLIGHT_ON;
			case SuitLightType::Nightvision:
				return SOUND_NIGHTVISION_ON;
			}
		}();

		EmitSound(CHAN_WEAPON, onSound, 1.0, ATTN_NORM);

		SetBits(pev->effects, GetFlashlightFlag());
		UpdateFlashlight(this, true);

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
	}
}

void CBasePlayer::FlashlightTurnOff()
{
	auto offSound = [this]()
	{
		switch (m_SuitLightType)
		{
		default:
		case SuitLightType::Flashlight:
			return SOUND_FLASHLIGHT_OFF;
		case SuitLightType::Nightvision:
			return SOUND_NIGHTVISION_OFF;
		}
	}();

	EmitSound(CHAN_WEAPON, offSound, 1.0, ATTN_NORM);

	ClearBits(pev->effects, GetFlashlightFlag());
	UpdateFlashlight(this, false);

	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
}

void CBasePlayer::SetSuitLightType(SuitLightType type)
{
	if (m_SuitLightType == type)
	{
		return;
	}

	const bool isOn = FlashlightIsOn();

	ClearBits(pev->effects, GetFlashlightFlag());

	m_SuitLightType = type;

	if (isOn)
	{
		SetBits(pev->effects, GetFlashlightFlag());
	}

	UpdateFlashlight(this, isOn);
}

void CBasePlayer::ForceClientDllUpdate()
{
	m_iClientHealth = -1;
	m_iClientBattery = -1;
	m_iClientHideHUD = -1;
	m_iClientFOV = -1;
	m_ClientWeaponBits = 0;
	m_ClientSndRoomtype = -1;

	for (int i = 0; i < MAX_AMMO_TYPES; ++i)
	{
		m_rgAmmoLast[i] = 0;
	}

	m_iTrain |= TRAIN_NEW; // Force new train message.
	m_fWeapon = false;	   // Force weapon send
	m_fInitHUD = true;	   // Force HUD gmsgResetHUD message

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

void CBasePlayer::ImpulseCommands()
{
	TraceResult tr; // UNDONE: kill me! This is temporary for PreAlpha CDs

	int iImpulse = pev->impulse;
	switch (iImpulse)
	{
	case 99:
	{

		bool iOn;

		if (0 == gmsgLogo)
		{
			iOn = true;
			gmsgLogo = REG_USER_MSG("Logo", 1);
		}
		else
		{
			iOn = false;
		}

		ASSERT(gmsgLogo > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgLogo, nullptr, edict());
		WRITE_BYTE(static_cast<int>(iOn));
		MESSAGE_END();

		if (!iOn)
			gmsgLogo = 0;
		break;
	}
	case 100:
		// temporary flashlight for level designers
		if (FlashlightIsOn())
		{
			FlashlightTurnOff();
		}
		else
		{
			FlashlightTurnOn();
		}
		break;

	case 201: // paint decal

		if (gpGlobals->time < m_flNextDecalTime)
		{
			// too early!
			break;
		}

		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, edict(), &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
			CSprayCan* pCan = g_EntityDictionary->Create<CSprayCan>("spraycan");
			pCan->Spawn(this);
		}

		break;

	case 205:
	{
		DropPlayerCTFPowerup(this);
		break;
	}

	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands(iImpulse);
		break;
	}

	pev->impulse = 0;
}

void CBasePlayer::CheatImpulseCommands(int iImpulse)
{
	if (0 == g_psv_cheats->value)
	{
		return;
	}

	CBaseEntity* pEntity;
	TraceResult tr;

	switch (iImpulse)
	{
	case 76:
	{
		if (!giPrecacheGrunt)
		{
			giPrecacheGrunt = true;
			Con_DPrintf("You must now restart to use Grunt-o-matic.\n");
		}
		else
		{
			UTIL_MakeVectors(Vector(0, pev->v_angle.y, 0));
			Create("monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles);
		}
		break;
	}


	case 101:
	{
		const bool hasActiveWeapon = m_pActiveWeapon != nullptr;

		SetHasSuit(true);

		pev->armorvalue = MAX_NORMAL_BATTERY;

		for (auto weapon : g_WeaponDictionary->GetClassNames())
		{
			GiveNamedItem(weapon);
		}

		for (int i = 0; i < g_AmmoTypes.GetCount(); ++i)
		{
			auto ammoType = g_AmmoTypes.GetByIndex(i + 1);
			SetAmmoCount(ammoType->Name.c_str(), ammoType->MaximumCapacity);
		}

		// Default to the MP5.
		if (!hasActiveWeapon)
		{
			SelectItem("weapon_9mmar");
		}

		break;
	}

	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs(this, 1, true);
		break;

	case 103:
		// What the hell are you doing?
		pEntity = UTIL_FindEntityForward(this);
		if (pEntity)
		{
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if (pMonster)
				pMonster->ReportAIState();
		}
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case 105: // player makes no sound for monsters to hear.
	{
		if (m_fNoPlayerSound)
		{
			Con_DPrintf("Player is audible\n");
			m_fNoPlayerSound = false;
		}
		else
		{
			Con_DPrintf("Player is silent\n");
			m_fNoPlayerSound = true;
		}
		break;
	}

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = UTIL_FindEntityForward(this);
		if (pEntity)
		{
			Con_DPrintf("Classname: %s", pEntity->GetClassname());

			if (!FStringNull(pEntity->pev->targetname))
			{
				Con_DPrintf(" - Targetname: %s\n", pEntity->GetTargetname());
			}
			else
			{
				Con_DPrintf(" - TargetName: No Targetname\n");
			}

			Con_DPrintf("Model: %s\n", pEntity->GetModelName());
			if (!FStringNull(pEntity->pev->globalname))
				Con_DPrintf("Globalname: %s\n", pEntity->GetGlobalname());
		}
		break;

	case 107:
	{
		TraceResult tr;

		edict_t* pWorld = CBaseEntity::World->edict();

		Vector start = pev->origin + pev->view_ofs;
		Vector end = start + gpGlobals->v_forward * 1024;
		UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);
		if (tr.pHit)
			pWorld = tr.pHit;
		const char* pTextureName = TRACE_TEXTURE(pWorld, start, end);
		if (pTextureName)
			Con_DPrintf("Texture: %s (%s)\n", pTextureName, g_MaterialSystem.FindTextureType(pTextureName));
	}
	break;
	case 195: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_fly", pev->origin, pev->angles);
	}
	break;
	case 196: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_large", pev->origin, pev->angles);
	}
	break;
	case 197: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_human", pev->origin, pev->angles);
	}
	break;
	case 199: // show nearest node and all connections
	{
		Con_DPrintf("%d\n", WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
		WorldGraph.ShowNodeConnections(WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
	}
	break;
	case 202: // Random blood splatter
		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, edict(), &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			CBloodSplat* pBlood = g_EntityDictionary->Create<CBloodSplat>("blood_splat");
			pBlood->Spawn(this);
		}
		break;
	case 203: // remove creature.
		pEntity = UTIL_FindEntityForward(this);
		if (pEntity)
		{
			if (0 != pEntity->pev->takedamage)
				pEntity->SetThink(&CBaseEntity::SUB_Remove);
		}
		break;
	}
}

ItemAddResult CBasePlayer::AddPlayerWeapon(CBasePlayerWeapon* weapon)
{
	CBasePlayerWeapon* pInsert = m_rgpPlayerWeapons[weapon->iItemSlot()];

	while (pInsert)
	{
		if (pInsert->ClassnameIs(STRING(weapon->pev->classname)))
		{
			if (weapon->AddDuplicate(pInsert))
			{
				// ugly hack to update clip w/o an update clip message
				pInsert->UpdateItemInfo();
				if (m_pActiveWeapon)
					m_pActiveWeapon->UpdateItemInfo();

				weapon->Kill();
				return ItemAddResult::AddedAsDuplicate;
			}

			return ItemAddResult::NotAdded;
		}
		pInsert = pInsert->m_pNext;
	}

	if (weapon->CanAddToPlayer(this))
	{
		weapon->AddToPlayer(this);

		// Add to the list before extracting ammo so weapon ownership checks work properly.
		weapon->m_pNext = m_rgpPlayerWeapons[weapon->iItemSlot()];
		m_rgpPlayerWeapons[weapon->iItemSlot()] = weapon;

		weapon->ExtractAmmo(weapon);

		// Immediately update the ammo HUD so weapon pickup isn't sometimes red because the HUD doesn't know about regenerating/free ammo yet.
		if (-1 != weapon->m_iPrimaryAmmoType)
		{
			SendSingleAmmoUpdate(CBasePlayer::GetAmmoIndex(weapon->pszAmmo1()));
		}

		if (-1 != weapon->m_iSecondaryAmmoType)
		{
			SendSingleAmmoUpdate(CBasePlayer::GetAmmoIndex(weapon->pszAmmo2()));
		}

		// Don't show weapon pickup if we're spawning or if it's an exhaustible weapon (will show ammo pickup instead).
		if (!m_bIsSpawning && (weapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE) == 0)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, edict());
			WRITE_BYTE(weapon->m_iId);
			MESSAGE_END();
		}

		// should we switch to this item?
		if (g_pGameRules->FShouldSwitchWeapon(this, weapon))
		{
			SwitchWeapon(weapon);
		}

		return ItemAddResult::Added;
	}

	return ItemAddResult::NotAdded;
}

bool CBasePlayer::RemovePlayerWeapon(CBasePlayerWeapon* weapon)
{
	if (m_pActiveWeapon == weapon)
	{
		ResetAutoaim();
		weapon->Holster();
		weapon->pev->nextthink = 0; // crowbar may be trying to swing again, etc.
		weapon->SetThink(nullptr);
		m_pActiveWeapon = nullptr;
		pev->viewmodel = string_t::Null;
		pev->weaponmodel = string_t::Null;
	}
	if (m_pLastWeapon == weapon)
		m_pLastWeapon = nullptr;

	CBasePlayerWeapon* pPrev = m_rgpPlayerWeapons[weapon->iItemSlot()];

	if (pPrev == weapon)
	{
		m_rgpPlayerWeapons[weapon->iItemSlot()] = weapon->m_pNext;
		return true;
	}
	else
	{
		while (pPrev && pPrev->m_pNext != weapon)
		{
			pPrev = pPrev->m_pNext;
		}
		if (pPrev)
		{
			pPrev->m_pNext = weapon->m_pNext;
			return true;
		}
	}
	return false;
}

int CBasePlayer::GiveAmmo(int iCount, const char* szName)
{
	if (!szName)
	{
		// no ammo.
		return -1;
	}

	const auto type = g_AmmoTypes.GetByName(szName);

	if (!type)
	{
		// Ammo type not registered.
		return -1;
	}

	if (iCount == RefillAllAmmoAmount)
	{
		iCount = type->MaximumCapacity;
	}

	if (!g_pGameRules->CanHaveAmmo(this, szName))
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int iAdd = std::min(iCount, type->MaximumCapacity - m_rgAmmo[type->Id]);

	// If we couldn't give any more ammo then just bow out. (Should already be handled by gamerules above).
	if (iAdd <= 0)
		return -1;

	// If this is an exhaustible weapon make sure the player has it.
	if (!type->WeaponName.empty())
	{
		if (!HasNamedPlayerWeapon(type->WeaponName.c_str()))
		{
			GiveNamedItem(type->WeaponName.c_str(), 0);
		}
	}

	m_rgAmmo[type->Id] += iAdd;


	if (0 != gmsgAmmoPickup) // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoPickup, nullptr, edict());
		WRITE_BYTE(GetAmmoIndex(szName)); // ammo ID
		WRITE_BYTE(iAdd);				  // amount
		MESSAGE_END();
	}

	return type->Id;
}

int CBasePlayer::GiveMagazine(CBasePlayerWeapon* weapon, int attackMode)
{
	if (!weapon)
	{
		return -1;
	}

	assert(attackMode >= 0 && attackMode < MAX_WEAPON_ATTACK_MODES);

	const auto& attackModeInfo = weapon->m_WeaponInfo->AttackModeInfo[attackMode];

	if (attackModeInfo.AmmoType.empty())
	{
		return -1;
	}

	int magazineSize = attackModeInfo.MagazineSize;

	// If the weapon does not use magazines, just give one unit of ammo instead.
	if (magazineSize == WEAPON_NOCLIP)
	{
		magazineSize = 1;
	}

	return GiveAmmo(magazineSize, attackModeInfo.AmmoType.c_str());
}

void CBasePlayer::ItemPreFrame()
{
	if (m_flNextAttack > UTIL_WeaponTimeBase())
	{
		return;
	}

	if (!m_pActiveWeapon)
		return;

	m_pActiveWeapon->ItemPreFrame();
}

void CBasePlayer::ItemPostFrame()
{
	// check if the player is using a tank
	if (m_pTank != nullptr)
		return;

	const bool canUseItem = m_flNextAttack <= UTIL_WeaponTimeBase();

	if (canUseItem || GetSkillFloat("allow_use_while_busy"))
	{
		// Handle use events
		PlayerUse();
	}

	if (!canUseItem)
	{
		return;
	}

	ImpulseCommands();

	// check again if the player is using a tank if they started using it in PlayerUse
	if (m_pTank != nullptr)
		return;

	if (!m_pActiveWeapon)
		return;

	m_pActiveWeapon->ItemPostFrame();
}

void CBasePlayer::SendAmmoUpdate()
{
	for (int i = 0; i < MAX_AMMO_TYPES; i++)
	{
		InternalSendSingleAmmoUpdate(i);
	}
}

bool CBasePlayer::HasLongJump() const
{
	return m_fLongJump;
}

void CBasePlayer::SetHasLongJump(bool hasLongJump)
{
	m_fLongJump = hasLongJump;
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", hasLongJump ? "1" : "0");
	// TODO: CTF long jump integration
}

void CBasePlayer::SendSingleAmmoUpdate(int ammoIndex)
{
	if (ammoIndex < 0 || ammoIndex >= MAX_AMMO_TYPES)
	{
		return;
	}

	InternalSendSingleAmmoUpdate(ammoIndex);
}

void CBasePlayer::InternalSendSingleAmmoUpdate(int ammoIndex)
{
	if (m_rgAmmo[ammoIndex] != m_rgAmmoLast[ammoIndex])
	{
		m_rgAmmoLast[ammoIndex] = m_rgAmmo[ammoIndex];

		ASSERT(m_rgAmmo[ammoIndex] >= 0);
		ASSERT(m_rgAmmo[ammoIndex] < 255);

		// send "Ammo" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoX, NULL, edict());
		WRITE_BYTE(ammoIndex);
		WRITE_BYTE(std::clamp(m_rgAmmo[ammoIndex], 0, 254)); // clamp the value to one byte
		MESSAGE_END();
	}
}

void CBasePlayer::UpdateClientData()
{
	const bool fullHUDInitRequired = m_fInitHUD != false;

	if (fullHUDInitRequired || m_bRestored)
	{
		g_Server.PlayerActivating(this);
		FireTargets("game_playeractivate", this, this, USE_TOGGLE, 0);
	}

	if (m_fInitHUD)
	{
		m_fInitHUD = false;
		gInitHUD = false;

		MESSAGE_BEGIN(MSG_ONE, gmsgResetHUD, nullptr, edict());
		WRITE_BYTE(0);
		MESSAGE_END();

		if (!m_fGameHUDInitialized)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInitHUD, nullptr, edict());
			MESSAGE_END();

			g_pGameRules->InitHUD(this);
			m_fGameHUDInitialized = true;

			m_iObserverLastMode = OBS_ROAMING;

			if (g_pGameRules->IsMultiplayer())
			{
				FireTargets("game_playerjoin", this, this, USE_TOGGLE, 0);
			}
		}

		if (g_pGameRules->IsMultiplayer())
		{
			RefreshCustomHUD(this);
		}

		InitStatusBar();
	}

	if (m_FireSpawnTarget)
	{
		m_FireSpawnTarget = false;

		// In case a game_player_equip is triggered, this ensures the player equips the last weapon given.
		const int savedAutoWepSwitch = m_iAutoWepSwitch;
		m_iAutoWepSwitch = 1;

		// This counts as spawning, it suppresses weapon pickup notifications.
		m_bIsSpawning = true;
		FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);
		m_bIsSpawning = false;

		m_iAutoWepSwitch = savedAutoWepSwitch;
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, nullptr, edict());
		WRITE_BYTE(m_iHideHUD);
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if (m_iFOV != m_iClientFOV)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, nullptr, edict());
		WRITE_BYTE(m_iFOV);
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	// TODO: will not work properly in multiplayer
	if (!g_DisplayTitleName.empty())
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgShowGameTitle, nullptr, edict());
		WRITE_STRING(g_DisplayTitleName.c_str());
		MESSAGE_END();
		g_DisplayTitleName.clear();
	}

	if (pev->health != m_iClientHealth)
	{
		// make sure that no negative health values are sent
		int iHealth = std::clamp(pev->health, 0.f, static_cast<float>(std::numeric_limits<short>::max()));
		if (pev->health > 0.0f && pev->health <= 1.0f)
			iHealth = 1;

		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgHealth, nullptr, edict());
		WRITE_SHORT(iHealth);
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}


	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = pev->armorvalue;

		ASSERT(gmsgBattery > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgBattery, nullptr, edict());
		WRITE_SHORT((int)pev->armorvalue);
		MESSAGE_END();
	}

	if (m_WeaponBits != m_ClientWeaponBits)
	{
		m_ClientWeaponBits = m_WeaponBits;

		const int lowerBits = m_WeaponBits & 0xFFFFFFFF;
		const int upperBits = (m_WeaponBits >> 32) & 0xFFFFFFFF;

		MESSAGE_BEGIN(MSG_ONE, gmsgWeapons, nullptr, edict());
		WRITE_LONG(lowerBits);
		WRITE_LONG(upperBits);
		MESSAGE_END();
	}

	if (0 != pev->dmg_take || 0 != pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t* other = pev->dmg_inflictor;
		if (other)
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(other);
			if (pEntity)
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN(MSG_ONE, gmsgDamage, nullptr, edict());
		WRITE_BYTE(pev->dmg_save);
		WRITE_BYTE(pev->dmg_take);
		WRITE_LONG(visibleDamageBits);
		WRITE_COORD(damageOrigin.x);
		WRITE_COORD(damageOrigin.y);
		WRITE_COORD(damageOrigin.z);
		MESSAGE_END();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	if (fullHUDInitRequired || m_bRestored)
	{
		// Always tell client about battery state
		MESSAGE_BEGIN(MSG_ONE, gmsgFlashBattery, nullptr, edict());
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		// Sync up client flashlight state.
		UpdateFlashlight(this, FlashlightIsOn());
	}

	if (m_bRestored)
	{
		// Reinitialize hud color to saved off value.
		SetHudColor(RGB24::FromInteger(m_HudColor));
	}

	// Update Flashlight
	if ((0 != m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->time))
	{
		if (FlashlightIsOn())
		{
			if (0 != m_iFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
				m_iFlashBattery--;

				if (0 == m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgFlashBattery, nullptr, edict());
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();
	}


	if ((m_iTrain & TRAIN_NEW) != 0)
	{
		ASSERT(gmsgTrain > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgTrain, nullptr, edict());
		WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	SendAmmoUpdate();

	// Update all the items
	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		if (m_rgpPlayerWeapons[i]) // each item updates it's successors
			m_rgpPlayerWeapons[i]->UpdateClientData(this);
	}

	// Active item is becoming null, or we're sending all HUD state to client
	// Only if we're not in Observer mode, which uses the target player's weapon
	if (pev->iuser1 == OBS_NONE && !m_pActiveWeapon && ((m_pClientActiveWeapon != m_pActiveWeapon) || fullHUDInitRequired))
	{
		// Tell ammo hud that we have no weapon selected
		MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, nullptr, edict());
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		MESSAGE_END();
	}

	// Cache and client weapon change
	m_pClientActiveWeapon = m_pActiveWeapon;
	m_iClientFOV = m_iFOV;
	UpdateCTFHud();

	// Update Status Bar
	if (m_flNextSBarUpdateTime < gpGlobals->time)
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}

	UpdateEntityInfo();

	// Send new room type to client.
	if (m_ClientSndRoomtype != m_SndRoomtype)
	{
		m_ClientSndRoomtype = m_SndRoomtype;

		MESSAGE_BEGIN(MSG_ONE, SVC_ROOMTYPE, nullptr, edict());
		WRITE_SHORT((short)m_SndRoomtype); // sequence number
		MESSAGE_END();
	}

	if (fullHUDInitRequired || m_bRestored)
	{
		g_Skill.SendAllNetworkedSkillVars(this);
	}

	// Handled anything that needs resetting
	m_bRestored = false;
}

void CBasePlayer::UpdateCTFHud()
{
	if (0 != gmsgPlayerIcon && 0 != gmsgStatusIcon)
	{
		if (m_iItems != m_iClientItems)
		{
			const unsigned int itemsChanged = m_iItems ^ m_iClientItems;

			const unsigned int removedItems = m_iClientItems & itemsChanged;
			const unsigned int addedItems = m_iItems & itemsChanged;

			m_iClientItems = m_iItems;

			// Update flag item info
			for (int id = CTFItem::BlackMesaFlag; id <= CTFItem::OpposingForceFlag; id <<= 1)
			{
				bool state;

				if ((addedItems & id) != 0)
				{
					state = true;
				}
				else if ((removedItems & id) != 0)
				{
					state = false;
				}
				else
				{
					// Unchanged
					continue;
				}

				g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgPlayerIcon, nullptr, nullptr);
				g_engfuncs.pfnWriteByte(entindex());
				g_engfuncs.pfnWriteByte(static_cast<int>(state));
				g_engfuncs.pfnWriteByte(1);
				g_engfuncs.pfnWriteByte(id);
				g_engfuncs.pfnMessageEnd();
			}

			// Ugly hack
			struct ItemData
			{
				const char* ClassName;
				int R, G, B;
			};

			static const ItemData CTFItemData[] =
				{
					{"item_ctfljump", 255, 160, 0},
					{"item_ctfphev", 128, 160, 255},
					{"item_ctfbpack", 255, 255, 0},
					{"item_ctfaccel", 255, 0, 0},
					{"Unknown", 0, 0, 0}, // Not actually used, but needed to match the index
					{"item_ctfregen", 0, 255, 0},
				};

			for (int id = CTFItem::LongJump, i = 0; id <= CTFItem::Regeneration; id <<= 1, ++i)
			{
				bool state;

				if ((addedItems & id) != 0)
				{
					state = true;
				}
				else if ((removedItems & id) != 0)
				{
					state = false;
				}
				else
				{
					// Unchanged
					continue;
				}

				const auto& itemData{CTFItemData[i]};

				g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgStatusIcon, nullptr, edict());
				g_engfuncs.pfnWriteByte(static_cast<int>(state));
				g_engfuncs.pfnWriteString(itemData.ClassName);

				if (state)
				{
					g_engfuncs.pfnWriteByte(itemData.R);
					g_engfuncs.pfnWriteByte(itemData.G);
					g_engfuncs.pfnWriteByte(itemData.B);
				}

				g_engfuncs.pfnMessageEnd();

				g_engfuncs.pfnMessageBegin(MSG_ALL, gmsgPlayerIcon, nullptr, nullptr);
				g_engfuncs.pfnWriteByte(entindex());
				g_engfuncs.pfnWriteByte(static_cast<int>(state));
				g_engfuncs.pfnWriteByte(0);
				g_engfuncs.pfnWriteByte(id);
				g_engfuncs.pfnMessageEnd();
			}
		}
	}
}

bool CBasePlayer::FBecomeProne()
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return true;
}

void CBasePlayer::BarnacleVictimBitten(CBaseEntity* pevBarnacle)
{
	TakeDamage(pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB);
}

void CBasePlayer::BarnacleVictimReleased()
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}

int CBasePlayer::Illumination()
{
	int iIllum = CBaseEntity::Illumination();

	iIllum += m_iWeaponFlash;
	if (iIllum > 255)
		return 255;
	return iIllum;
}

void CBasePlayer::EnableControl(bool fControl)
{
	if (!fControl)
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}

// TODO: move
#define DOT_1DEGREE 0.9998476951564
#define DOT_2DEGREE 0.9993908270191
#define DOT_3DEGREE 0.9986295347546
#define DOT_4DEGREE 0.9975640502598
#define DOT_5DEGREE 0.9961946980917
#define DOT_6DEGREE 0.9945218953683
#define DOT_7DEGREE 0.9925461516413
#define DOT_8DEGREE 0.9902680687416
#define DOT_9DEGREE 0.9876883405951
#define DOT_10DEGREE 0.9848077530122
#define DOT_15DEGREE 0.9659258262891
#define DOT_20DEGREE 0.9396926207859
#define DOT_25DEGREE 0.9063077870367

Vector CBasePlayer::GetAutoaimVector(float flDelta)
{
	return GetAutoaimVectorFromPoint(GetGunPosition(), flDelta);
}

Vector CBasePlayer::GetAutoaimVectorFromPoint(const Vector& vecSrc, float flDelta)
{
	if (g_Skill.GetSkillLevel() == SkillLevel::Hard)
	{
		UTIL_MakeVectors(pev->v_angle + pev->punchangle);
		return gpGlobals->v_forward;
	}

	float flDist = 8192;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (true || g_Skill.GetSkillLevel() == SkillLevel::Medium)
	{
		m_vecAutoAim = Vector(0, 0, 0);
		// flDelta *= 0.5;
	}

	bool m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta);

	// update ontarget if changed
	if (!g_pGameRules->AllowAutoTargetCrosshair())
		m_fOnTarget = false;
	else if (m_fOldTargeting != m_fOnTarget)
	{
		m_pActiveWeapon->UpdateItemInfo();
	}

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;


	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (false || g_Skill.GetSkillLevel() == SkillLevel::Easy)
	{
		m_vecAutoAim = m_vecAutoAim * 0.67 + angles * 0.33;
	}
	else
	{
		m_vecAutoAim = angles * 0.9;
	}

	// m_vecAutoAim = m_vecAutoAim * 0.99;

	// Don't send across network if sv_aim is 0
	if (g_psv_aim->value != 0)
	{
		if (m_vecAutoAim.x != m_lastx ||
			m_vecAutoAim.y != m_lasty)
		{
			SET_CROSSHAIRANGLE(edict(), -m_vecAutoAim.x, m_vecAutoAim.y);

			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}
	else
	{
		ResetAutoaim();
	}

	// Logger->debug("{} {}", angles.x, angles.y);

	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);
	return gpGlobals->v_forward;
}

Vector CBasePlayer::AutoaimDeflection(const Vector& vecSrc, float flDist, float flDelta)
{
	edict_t* pEdict = UTIL_GetEntityList() + 1;
	CBaseEntity* pEntity;
	float bestdot;
	Vector bestdir;
	edict_t* bestent;
	TraceResult tr;

	if (g_psv_aim->value == 0)
	{
		m_fOnTarget = false;
		return g_vecZero;
	}

	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);

	// try all possible entities
	bestdir = gpGlobals->v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = nullptr;

	m_fOnTarget = false;

	UTIL_TraceLine(vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, edict(), &tr);


	if (tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO)
	{
		// don't look through water
		if (!((pev->waterlevel != WaterLevel::Head && tr.pHit->v.waterlevel == WaterLevel::Head) || (pev->waterlevel == WaterLevel::Head && tr.pHit->v.waterlevel == WaterLevel::Dry)))
		{
			if (tr.pHit->v.takedamage == DAMAGE_AIM)
				m_fOnTarget = true;

			return m_vecAutoAim;
		}
	}

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		Vector center;
		Vector dir;
		float dot;

		if (0 != pEdict->free) // Not in use
			continue;

		if (pEdict->v.takedamage != DAMAGE_AIM)
			continue;
		if (pEdict == edict())
			continue;

		pEntity = Instance(pEdict);

		if (pEntity == nullptr)
			continue;

		//		if (pev->team > 0 && pEdict->v.team == pev->team)
		//			continue;	// don't aim at teammate
		if (!g_pGameRules->ShouldAutoAim(this, pEntity))
			continue;

		if (!pEntity->IsAlive())
			continue;

		// don't look through water
		if ((pev->waterlevel != WaterLevel::Head && pEntity->pev->waterlevel == WaterLevel::Head) || (pev->waterlevel == WaterLevel::Head && pEntity->pev->waterlevel == WaterLevel::Dry))
			continue;

		center = pEntity->BodyTarget(vecSrc);

		dir = (center - vecSrc).Normalize();

		// make sure it's in front of the player
		if (DotProduct(dir, gpGlobals->v_forward) < 0)
			continue;

		dot = fabs(DotProduct(dir, gpGlobals->v_right)) + fabs(DotProduct(dir, gpGlobals->v_up)) * 0.5;

		// tweek for distance
		dot *= 1.0 + 0.2 * ((center - vecSrc).Length() / flDist);

		if (dot > bestdot)
			continue; // to far to turn

		UTIL_TraceLine(vecSrc, center, dont_ignore_monsters, edict(), &tr);
		if (tr.flFraction != 1.0 && tr.pHit != pEdict)
		{
			// Logger->debug("hit {}, can't see {}", STRING(tr.pHit->v.classname), STRING(pEdict->v.classname));
			continue;
		}

		// don't shoot at friends
		if (IRelationship(pEntity) < 0)
		{
			if (!pEntity->IsPlayer() && !g_pGameRules->IsMultiplayer())
				// Logger->debug("friend");
				continue;
		}

		// can shoot at this one
		bestdot = dot;
		bestent = pEdict;
		bestdir = dir;
	}

	if (bestent)
	{
		bestdir = UTIL_VecToAngles(bestdir);
		bestdir.x = -bestdir.x;
		bestdir = bestdir - pev->v_angle - pev->punchangle;

		if (bestent->v.takedamage == DAMAGE_AIM)
			m_fOnTarget = true;

		return bestdir;
	}

	return Vector(0, 0, 0);
}

void CBasePlayer::ResetAutoaim()
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = Vector(0, 0, 0);
		SET_CROSSHAIRANGLE(edict(), 0, 0);
	}
	m_fOnTarget = false;
}

void CBasePlayer::SetCustomDecalFrames(int nFrames)
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

int CBasePlayer::GetCustomDecalFrames()
{
	return m_nCustomSprayFrames;
}

void CBasePlayer::DropPlayerWeapon(const char* pszItemName)
{
	if (!g_pGameRules->IsMultiplayer() || (weaponstay.value > 0))
	{
		// no dropping in single player.
		return;
	}

	if (0 == strlen(pszItemName))
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = nullptr;
	}

	CBasePlayerWeapon* weapon;
	int i;

	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		weapon = m_rgpPlayerWeapons[i];

		while (weapon)
		{
			if (pszItemName)
			{
				// try to match by name.
				if (0 == strcmp(pszItemName, STRING(weapon->pev->classname)))
				{
					// match!
					break;
				}
			}
			else
			{
				// trying to drop active item
				if (weapon == m_pActiveWeapon)
				{
					// active item!
					break;
				}
			}

			weapon = weapon->m_pNext;
		}


		// if we land here with a valid pWeapon pointer, that's because we found the
		// item we want to drop and hit a BREAK;  pWeapon is the item.
		if (weapon)
		{
			if (!g_pGameRules->GetNextBestWeapon(this, weapon))
				return; // can't drop the item they asked for, may be our last item or something we can't holster

			UTIL_MakeVectors(pev->angles);

			ClearWeaponBit(weapon->m_iId); // take item off hud

			CWeaponBox* pWeaponBox = (CWeaponBox*)CBaseEntity::Create("weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, this);
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->PackWeapon(weapon);
			pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;

			// drop half of the ammo for this weapon.
			int iAmmoIndex;

			iAmmoIndex = GetAmmoIndex(weapon->pszAmmo1()); // ???

			if (iAmmoIndex != -1)
			{
				// this weapon weapon uses ammo, so pack an appropriate amount.
				if ((weapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE) != 0)
				{
					// pack up all the ammo, this weapon is its own ammo type
					pWeaponBox->PackAmmo(MAKE_STRING(weapon->pszAmmo1()), m_rgAmmo[iAmmoIndex]);
					m_rgAmmo[iAmmoIndex] = 0;
				}
				else
				{
					// pack half of the ammo
					pWeaponBox->PackAmmo(MAKE_STRING(weapon->pszAmmo1()), m_rgAmmo[iAmmoIndex] / 2);
					m_rgAmmo[iAmmoIndex] /= 2;
				}
			}

			return; // we're done, so stop searching with the FOR loop.
		}
	}
}

bool CBasePlayer::HasPlayerWeapon(CBasePlayerWeapon* checkWeapon)
{
	CBasePlayerWeapon* pItem = m_rgpPlayerWeapons[checkWeapon->iItemSlot()];

	while (pItem)
	{
		if (pItem->ClassnameIs(STRING(checkWeapon->pev->classname)))
		{
			return true;
		}
		pItem = pItem->m_pNext;
	}

	return false;
}

bool CBasePlayer::HasNamedPlayerWeapon(const char* pszItemName)
{
	CBasePlayerWeapon* weapon;
	int i;

	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		weapon = m_rgpPlayerWeapons[i];

		while (weapon)
		{
			if (0 == strcmp(pszItemName, STRING(weapon->pev->classname)))
			{
				return true;
			}
			weapon = weapon->m_pNext;
		}
	}

	return false;
}

bool CBasePlayer::SwitchWeapon(CBasePlayerWeapon* weapon)
{
	if (weapon && !weapon->CanDeploy())
	{
		return false;
	}

	ResetAutoaim();

	if (m_pActiveWeapon)
	{
		m_pActiveWeapon->Holster();
	}

	m_pActiveWeapon = weapon;

	if (weapon)
	{
		weapon->m_ForceSendAnimations = true;
		weapon->Deploy();
		weapon->m_ForceSendAnimations = false;
	}

	return true;
}

void CBasePlayer::Player_Menu()
{
	if (g_pGameRules->IsCTF())
	{
		if (m_iCurrentMenu == MENU_TEAM)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgAllowSpec, nullptr, edict());
			WRITE_BYTE(1);
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_ONE, gmsgTeamNames, nullptr, edict());
			g_engfuncs.pfnWriteByte(2);
			WRITE_STRING("#CTFTeam_BM");
			WRITE_STRING("#CTFTeam_OF");
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_ONE, gmsgVGUIMenu, nullptr, edict());
			WRITE_BYTE(MENU_TEAM);
			MESSAGE_END();
		}
		else if (m_iCurrentMenu == MENU_CLASS)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgSetMenuTeam, nullptr, edict());
			WRITE_BYTE(static_cast<int>(m_iNewTeamNum == CTFTeam::None ? m_iTeamNum : m_iNewTeamNum));
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_ONE, gmsgVGUIMenu, nullptr, edict());
			WRITE_BYTE(MENU_CLASS);
			MESSAGE_END();
		}
	}
}

void CBasePlayer::ResetMenu()
{
	if (0 != gmsgShowMenu)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, nullptr, edict());
		WRITE_SHORT(0);
		WRITE_CHAR(0);
		WRITE_BYTE(0);
		WRITE_STRING("");
		MESSAGE_END();
	}

	switch (m_iCurrentMenu)
	{
	case MENU_DEFAULT:
		m_iCurrentMenu = MENU_TEAM;
		break;

	case MENU_TEAM:
		m_iCurrentMenu = MENU_CLASS;
		break;

	default:
		m_iCurrentMenu = MENU_NONE;
		return;
	}

	if (g_pGameRules->IsCTF())
		Player_Menu();
}

bool CBasePlayer::Menu_Team_Input(int inp)
{
	m_iNewTeamNum = CTFTeam::None;

	if (inp == -1)
	{
		g_pGameRules->ChangePlayerTeam(this, nullptr, false, false);

		MESSAGE_BEGIN(MSG_ONE, gmsgTeamFull, nullptr, edict());
		WRITE_BYTE(0);
		MESSAGE_END();
		return true;
	}
	else if (inp == 3)
	{
		if (g_pGameRules->TeamsBalanced())
		{
			if (m_iTeamNum == CTFTeam::None)
			{
				m_iNewTeamNum = static_cast<CTFTeam>(RANDOM_LONG(0, 1) + 1);
				if (m_iNewTeamNum <= CTFTeam::None)
				{
					m_iNewTeamNum = CTFTeam::BlackMesa;
				}
				else if (m_iNewTeamNum > CTFTeam::OpposingForce)
				{
					m_iNewTeamNum = CTFTeam::OpposingForce;
				}
			}
			else
			{
				m_iNewTeamNum = m_iTeamNum;
			}
		}
		else
		{
			m_iNewTeamNum = static_cast<CTFTeam>(g_pGameRules->GetTeamIndex(g_pGameRules->TeamWithFewestPlayers()) + 1);
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgTeamFull, nullptr, edict());
		WRITE_BYTE(0);
		MESSAGE_END();

		ResetMenu();

		pev->impulse = 0;

		return true;
	}
	else if (inp <= g_pGameRules->GetNumTeams() && inp > 0)
	{
		auto unbalanced = false;

		if (ctf_autoteam.value == 1)
		{
			if (m_iTeamNum != static_cast<CTFTeam>(inp))
			{
				if (!g_pGameRules->TeamsBalanced() && inp != g_pGameRules->GetTeamIndex(g_pGameRules->TeamWithFewestPlayers()) + 1)
				{
					ClientPrint(this, HUD_PRINTCONSOLE, "Team balancing enabled.\nThis team is full.\n");
					MESSAGE_BEGIN(MSG_ONE, gmsgTeamFull, nullptr, edict());
					WRITE_BYTE(1);
					MESSAGE_END();

					unbalanced = true;
				}
				else
				{
					m_iNewTeamNum = static_cast<CTFTeam>(inp);
				}
			}
			else
			{
				m_iNewTeamNum = m_iTeamNum;
			}
		}
		else
		{
			m_iNewTeamNum = static_cast<CTFTeam>(inp);
		}

		if (!unbalanced)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTeamFull, nullptr, edict());
			WRITE_BYTE(0);
			MESSAGE_END();

			ResetMenu();

			pev->impulse = 0;

			return true;
		}
	}
	else if (inp == 6 && m_iTeamNum > CTFTeam::None)
	{
		m_iCurrentMenu = MENU_NONE;
		return true;
	}

	m_iCurrentMenu = MENU_TEAM;

	if (g_pGameRules->IsCTF())
		Player_Menu();

	return false;
}

bool CBasePlayer::Menu_Char_Input(int inp)
{
	if (m_iNewTeamNum == CTFTeam::None)
	{
		ClientPrint(this, HUD_PRINTCONSOLE, "Can't change character; not in a team.\n");
		return false;
	}

	const char* pszCharacterType;

	if (inp == 7)
	{
		pszCharacterType = g_pGameRules->GetCharacterType(static_cast<int>(m_iNewTeamNum) - 1, RANDOM_LONG(0, 5));
	}
	else
	{
		const auto characterIndex = inp - 1;

		if (characterIndex < 0 || characterIndex > 5)
			return false;

		pszCharacterType = g_pGameRules->GetCharacterType(static_cast<int>(m_iNewTeamNum) - 1, characterIndex);
	}

	// Kill and gib the player if they're changing teams
	const auto killAndGib = 0 == pev->iuser1 && m_iTeamNum != m_iNewTeamNum;

	g_pGameRules->ChangePlayerTeam(this, pszCharacterType, killAndGib, killAndGib);

	ResetMenu();

	pev->impulse = 0;

	if (0 != pev->iuser1)
	{
		pev->effects &= ~EF_NODRAW;
		pev->flags &= FL_FAKECLIENT;
		pev->flags |= FL_CLIENT;
		pev->takedamage = DAMAGE_YES;
		m_iHideHUD &= ~(HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
		m_afPhysicsFlags &= PFLAG_OBSERVER;
		pev->flags &= ~FL_SPECTATOR;

		MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
		WRITE_BYTE(entindex());
		WRITE_BYTE(0);
		MESSAGE_END();

		pev->iuser1 = 0;
		pev->deadflag = 0;
		pev->solid = SOLID_SLIDEBOX;
		pev->movetype = MOVETYPE_WALK;

		g_pGameRules->GetPlayerSpawnSpot(this);
		g_pGameRules->PlayerSpawn(this);
	}

	return true;
}

void CBasePlayer::EquipWeapon()
{
	if (m_pActiveWeapon)
	{
		if ((!FStringNull(pev->viewmodel) || !FStringNull(pev->weaponmodel)))
		{
			// Already have a weapon equipped and deployed.
			return;
		}

		// Have a weapon equipped, but not deployed.
		if (m_pActiveWeapon->CanDeploy() && m_pActiveWeapon->Deploy())
		{
			return;
		}
	}

	// No weapon equipped or couldn't deploy it, find a suitable alternative.
	g_pGameRules->GetNextBestWeapon(this, m_pActiveWeapon, true);
}

void CBasePlayer::SetPrefsFromUserinfo(char* infobuffer)
{
	const char* value = g_engfuncs.pfnInfoKeyValue(infobuffer, "cl_autowepswitch");

	if ('\0' != *value)
	{
		m_iAutoWepSwitch = atoi(value);
	}
	else
	{
		m_iAutoWepSwitch = 1;
	}
}

void CBasePlayer::SetHudColor(RGB24 color)
{
	m_HudColor = color.ToInteger();

	g_engfuncs.pfnMessageBegin(MSG_ONE, gmsgHudColor, nullptr, edict());
	g_engfuncs.pfnWriteByte(color.Red);
	g_engfuncs.pfnWriteByte(color.Green);
	g_engfuncs.pfnWriteByte(color.Blue);
	g_engfuncs.pfnMessageEnd();
}

static void SendScoreInfoMessage(CBasePlayer* owner)
{
	WRITE_BYTE(owner->entindex());
	WRITE_SHORT(owner->pev->frags);
	WRITE_SHORT(owner->m_iDeaths);

	// To properly emulate Opposing Force's behavior we need to write -1 for these 2 in CTF.
	if (g_pGameRules->IsCTF())
	{
		WRITE_SHORT(-1);
		WRITE_SHORT(-1);
	}
	else
	{
		WRITE_SHORT(0);
		WRITE_SHORT(g_pGameRules->GetTeamIndex(owner->m_szTeamName) + 1);
	}

	MESSAGE_END();
}

void CBasePlayer::SendScoreInfo(CBasePlayer* destination)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, nullptr, destination->edict());
	SendScoreInfoMessage(this);
}

void CBasePlayer::SendScoreInfoAll()
{
	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	SendScoreInfoMessage(this);
}

static edict_t* SV_TestEntityPosition(CBaseEntity* ent)
{
	// Need to use this trace function so the engine checks the hull during collision tests.
	TraceResult tr;
	TRACE_MONSTER_HULL(ent->edict(), ent->pev->origin, ent->pev->origin, dont_ignore_monsters, nullptr, &tr);

	if (tr.fStartSolid != 0)
	{
		return tr.pHit;
	}

	return nullptr;
}

static bool FindPassableSpace(CBaseEntity* ent, const Vector& direction, float step)
{
	for (int i = 100; i > 0; --i)
	{
		ent->pev->origin = ent->pev->origin + step * direction;

		if (!SV_TestEntityPosition(ent))
		{
			ent->pev->oldorigin = ent->pev->origin;
			return true;
		}
	}

	return false;
}

static void Noclip_Toggle(CBasePlayer* player)
{
	if (player->pev->movetype == MOVETYPE_NOCLIP)
	{
		player->pev->movetype = MOVETYPE_WALK;

		player->pev->oldorigin = player->pev->origin;

		if (SV_TestEntityPosition(player))
		{
			Vector forward, right, up;
			AngleVectors(player->pev->v_angle, forward, right, up);

			// Try to find a valid position near the player.
			if (!FindPassableSpace(player, forward, 1) &&
				!FindPassableSpace(player, right, 1) &&
				!FindPassableSpace(player, right, -1) &&
				!FindPassableSpace(player, up, 1) &&
				!FindPassableSpace(player, up, -1) &&
				!FindPassableSpace(player, forward, -1))
			{
				Con_DPrintf("Can't find the world\n");
			}

			player->pev->origin = player->pev->oldorigin;
		}
	}
	else
	{
		player->pev->movetype = MOVETYPE_NOCLIP;
	}
}

void CBasePlayer::ToggleCheat(Cheat cheat)
{
	switch (cheat)
	{
	case Cheat::Godmode:
		pev->flags ^= FL_GODMODE;
		UTIL_ConsolePrint(edict(), "godmode {}\n", (pev->flags & FL_GODMODE) != 0 ? "ON" : "OFF");
		break;
	case Cheat::Notarget:
		pev->flags ^= FL_NOTARGET;
		UTIL_ConsolePrint(edict(), "notarget {}\n", (pev->flags & FL_NOTARGET) != 0 ? "ON" : "OFF");
		break;
	case Cheat::Noclip:
		Noclip_Toggle(this);
		UTIL_ConsolePrint(edict(), "noclip {}\n", pev->movetype == MOVETYPE_NOCLIP ? "ON" : "OFF");
		break;
	case Cheat::InfiniteAir:
		m_bInfiniteAir = !m_bInfiniteAir;
		UTIL_ConsolePrint(edict(), "Infinite air: {}\n", m_bInfiniteAir ? "ON" : "OFF");
		break;
	case Cheat::InfiniteArmor:
		m_bInfiniteArmor = !m_bInfiniteArmor;
		UTIL_ConsolePrint(edict(), "Infinite armor: {}\n", m_bInfiniteArmor ? "ON" : "OFF");
		break;
	default:
		UTIL_ConsolePrint(edict(), "Bogus cheat value!\n");
	}
}

class CDeadHEV : public CBaseMonster
{
public:
	void OnCreate() override;
	void Spawn() override;
	int Classify() override { return CLASS_HUMAN_MILITARY; }

	bool KeyValue(KeyValueData* pkvd) override;

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	static const char* m_szPoses[4];
};

const char* CDeadHEV::m_szPoses[] = {"deadback", "deadsitting", "deadstomach", "deadtable"};

void CDeadHEV::OnCreate()
{
	CBaseMonster::OnCreate();

	// Corpses have less health
	pev->health = 8;
	pev->model = MAKE_STRING("models/deadhaz.mdl");
}

bool CDeadHEV::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_hevsuit_dead, CDeadHEV);

void CDeadHEV::Spawn()
{
	PrecacheModel(STRING(pev->model));
	SetModel(STRING(pev->model));

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	pev->body = 1;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		Logger->debug("Dead hevsuit with bad pose");
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	MonsterInitDead();
}

/**
 *	@brief Multiplayer intermission spots.
 */
class CInfoIntermission : public CPointEntity
{
	void Spawn() override;
	void Think() override;
};

void CInfoIntermission::Spawn()
{
	SetOrigin(pev->origin);
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2; // let targets spawn!
}

void CInfoIntermission::Think()
{
	// find my target
	auto pTarget = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));

	if (!FNullEnt(pTarget))
	{
		pev->v_angle = UTIL_VecToAngles((pTarget->pev->origin - pev->origin).Normalize());
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);
