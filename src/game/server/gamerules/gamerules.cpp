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
//=========================================================
// GameRules.cpp
//=========================================================

#include "cbase.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "ctfplay_gamerules.h"
#include "coopplay_gamerules.h"
#include "world.h"
#include "UserMessages.h"

extern edict_t* EntSelectSpawnPoint(CBasePlayer* pPlayer);

CBasePlayerItem* CGameRules::FindNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pCurrentWeapon)
{
	if (pCurrentWeapon != nullptr && !pCurrentWeapon->CanHolster())
	{
		// can't put this gun away right now, so can't switch.
		return nullptr;
	}

	const int currentWeight = pCurrentWeapon != nullptr ? pCurrentWeapon->iWeight() : -1;

	CBasePlayerItem* pBest = nullptr; // this will be used in the event that we don't find a weapon in the same category.

	int iBestWeight = -1; // no weapon lower than -1 can be autoswitched to

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		for (auto pCheck = pPlayer->m_rgpPlayerItems[i]; pCheck; pCheck = pCheck->m_pNext)
		{
			// don't reselect the weapon we're trying to get rid of
			if (pCheck == pCurrentWeapon)
			{
				continue;
			}

			if (pCheck->iWeight() > -1 && pCheck->iWeight() == currentWeight)
			{
				// this weapon is from the same category.
				if (pCheck->CanDeploy())
				{
					if (pPlayer->SwitchWeapon(pCheck))
					{
						return pCheck;
					}
				}
			}
			else if (pCheck->iWeight() > iBestWeight)
			{
				//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted
				// weapon.
				if (pCheck->CanDeploy())
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}
		}
	}

	// if we make it here, we've checked all the weapons and found no useable
	// weapon in the same catagory as the current weapon.

	// if pBest is nullptr, we didn't find ANYTHING. Shouldn't be possible- should always
	// at least get the crowbar, but ya never know.

	return pBest;
}

bool CGameRules::GetNextBestWeapon(CBasePlayer* pPlayer, CBasePlayerItem* pCurrentWeapon, bool alwaysSearch)
{
	if (auto pBest = FindNextBestWeapon(pPlayer, pCurrentWeapon); pBest != nullptr)
	{
		pPlayer->SwitchWeapon(pBest);
		return true;
	}

	return false;
}

//=========================================================
//=========================================================
bool CGameRules::CanHaveAmmo(CBasePlayer* pPlayer, const char* pszAmmoName, int iMaxCarry)
{
	int iAmmoIndex;

	if (pszAmmoName)
	{
		iAmmoIndex = pPlayer->GetAmmoIndex(pszAmmoName);

		if (iAmmoIndex > -1)
		{
			if (pPlayer->AmmoInventory(iAmmoIndex) < iMaxCarry)
			{
				// player has room for more of this type of ammo
				return true;
			}
		}
	}

	return false;
}

//=========================================================
//=========================================================
edict_t* CGameRules::GetPlayerSpawnSpot(CBasePlayer* pPlayer)
{
	edict_t* pentSpawnSpot = EntSelectSpawnPoint(pPlayer);

	pPlayer->pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS(pentSpawnSpot)->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = 1;

	return pentSpawnSpot;
}

//=========================================================
//=========================================================
bool CGameRules::CanHavePlayerItem(CBasePlayer* pPlayer, CBasePlayerItem* pWeapon)
{
	// only living players can have items
	if (pPlayer->pev->deadflag != DEAD_NO)
		return false;

	if (pWeapon->pszAmmo1())
	{
		if (!CanHaveAmmo(pPlayer, pWeapon->pszAmmo1(), pWeapon->iMaxAmmo1()))
		{
			// we can't carry anymore ammo for this gun. We can only
			// have the gun if we aren't already carrying one of this type
			if (pPlayer->HasPlayerItem(pWeapon))
			{
				return false;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if (pPlayer->HasPlayerItem(pWeapon))
		{
			return false;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return true;
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CGameRules::RefreshSkillData()
{
	int iSkill = (int)CVAR_GET_FLOAT("skill");

	iSkill = std::clamp(iSkill, static_cast<int>(SkillLevel::Easy), static_cast<int>(SkillLevel::Hard));

	g_Skill.SetSkillLevel(iSkill);

	//Agrunt
	gSkillData.agruntHealth = g_Skill.GetValue("sk_agrunt_health");
	gSkillData.agruntDmgPunch = g_Skill.GetValue("sk_agrunt_dmg_punch");

	// Apache
	gSkillData.apacheHealth = g_Skill.GetValue("sk_apache_health");

	// Barney
	gSkillData.barneyHealth = g_Skill.GetValue("sk_barney_health");

	// Otis
	gSkillData.otisHealth = g_Skill.GetValue("sk_otis_health");

	// Big Momma
	gSkillData.bigmommaHealthFactor = g_Skill.GetValue("sk_bigmomma_health_factor");
	gSkillData.bigmommaDmgSlash = g_Skill.GetValue("sk_bigmomma_dmg_slash");
	gSkillData.bigmommaDmgBlast = g_Skill.GetValue("sk_bigmomma_dmg_blast");
	gSkillData.bigmommaRadiusBlast = g_Skill.GetValue("sk_bigmomma_radius_blast");

	// Bullsquid
	gSkillData.bullsquidHealth = g_Skill.GetValue("sk_bullsquid_health");
	gSkillData.bullsquidDmgBite = g_Skill.GetValue("sk_bullsquid_dmg_bite");
	gSkillData.bullsquidDmgWhip = g_Skill.GetValue("sk_bullsquid_dmg_whip");
	gSkillData.bullsquidDmgSpit = g_Skill.GetValue("sk_bullsquid_dmg_spit");

	// Pit Drone
	gSkillData.pitdroneHealth = g_Skill.GetValue("sk_pitdrone_health");
	gSkillData.pitdroneDmgBite = g_Skill.GetValue("sk_pitdrone_dmg_bite");
	gSkillData.pitdroneDmgWhip = g_Skill.GetValue("sk_pitdrone_dmg_whip");
	gSkillData.pitdroneDmgSpit = g_Skill.GetValue("sk_pitdrone_dmg_spit");

	// Gargantua
	gSkillData.gargantuaHealth = g_Skill.GetValue("sk_gargantua_health");
	gSkillData.gargantuaDmgSlash = g_Skill.GetValue("sk_gargantua_dmg_slash");
	gSkillData.gargantuaDmgFire = g_Skill.GetValue("sk_gargantua_dmg_fire");
	gSkillData.gargantuaDmgStomp = g_Skill.GetValue("sk_gargantua_dmg_stomp");

	// Hassassin
	gSkillData.hassassinHealth = g_Skill.GetValue("sk_hassassin_health");

	// Headcrab
	gSkillData.headcrabHealth = g_Skill.GetValue("sk_headcrab_health");
	gSkillData.headcrabDmgBite = g_Skill.GetValue("sk_headcrab_dmg_bite");

	// Shock Roach
	gSkillData.shockroachHealth = g_Skill.GetValue("sk_shockroach_health");
	gSkillData.shockroachDmgBite = g_Skill.GetValue("sk_shockroach_dmg_bite");
	gSkillData.shockroachLifespan = g_Skill.GetValue("sk_shockroach_lifespan");

	// Hgrunt
	gSkillData.hgruntHealth = g_Skill.GetValue("sk_hgrunt_health");
	gSkillData.hgruntDmgKick = g_Skill.GetValue("sk_hgrunt_kick");
	gSkillData.hgruntShotgunPellets = g_Skill.GetValue("sk_hgrunt_pellets");
	gSkillData.hgruntGrenadeSpeed = g_Skill.GetValue("sk_hgrunt_gspeed");

	// Hgrunt Ally
	gSkillData.hgruntAllyHealth = g_Skill.GetValue("sk_hgrunt_ally_health");
	gSkillData.hgruntAllyDmgKick = g_Skill.GetValue("sk_hgrunt_ally_kick");
	gSkillData.hgruntAllyShotgunPellets = g_Skill.GetValue("sk_hgrunt_ally_pellets");
	gSkillData.hgruntAllyGrenadeSpeed = g_Skill.GetValue("sk_hgrunt_ally_gspeed");

	// Hgrunt Medic
	gSkillData.medicAllyHealth = g_Skill.GetValue("sk_medic_ally_health");
	gSkillData.medicAllyDmgKick = g_Skill.GetValue("sk_medic_ally_kick");
	gSkillData.medicAllyGrenadeSpeed = g_Skill.GetValue("sk_medic_ally_gspeed");
	gSkillData.medicAllyHeal = g_Skill.GetValue("sk_medic_ally_heal");

	// Hgrunt Torch
	gSkillData.torchAllyHealth = g_Skill.GetValue("sk_torch_ally_health");
	gSkillData.torchAllyDmgKick = g_Skill.GetValue("sk_torch_ally_kick");
	gSkillData.torchAllyGrenadeSpeed = g_Skill.GetValue("sk_torch_ally_gspeed");

	// Male Assassin
	gSkillData.massassinHealth = g_Skill.GetValue("sk_massassin_health");
	gSkillData.massassinDmgKick = g_Skill.GetValue("sk_massassin_kick");
	gSkillData.massassinGrenadeSpeed = g_Skill.GetValue("sk_massassin_gspeed");

	// Shock Trooper
	gSkillData.shocktrooperHealth = g_Skill.GetValue("sk_shocktrooper_health");
	gSkillData.shocktrooperDmgKick = g_Skill.GetValue("sk_shocktrooper_kick");
	gSkillData.shocktrooperGrenadeSpeed = g_Skill.GetValue("sk_shocktrooper_gspeed");
	gSkillData.shocktrooperMaxCharge = g_Skill.GetValue("sk_shocktrooper_maxcharge");
	gSkillData.shocktrooperRechargeSpeed = g_Skill.GetValue("sk_shocktrooper_rchgspeed");

	// Houndeye
	gSkillData.houndeyeHealth = g_Skill.GetValue("sk_houndeye_health");
	gSkillData.houndeyeDmgBlast = g_Skill.GetValue("sk_houndeye_dmg_blast");

	// ISlave
	gSkillData.slaveHealth = g_Skill.GetValue("sk_islave_health");
	gSkillData.slaveDmgClaw = g_Skill.GetValue("sk_islave_dmg_claw");
	gSkillData.slaveDmgClawrake = g_Skill.GetValue("sk_islave_dmg_clawrake");
	gSkillData.slaveDmgZap = g_Skill.GetValue("sk_islave_dmg_zap");

	// Icthyosaur
	gSkillData.ichthyosaurHealth = g_Skill.GetValue("sk_ichthyosaur_health");
	gSkillData.ichthyosaurDmgShake = g_Skill.GetValue("sk_ichthyosaur_shake");

	// Leech
	gSkillData.leechHealth = g_Skill.GetValue("sk_leech_health");

	gSkillData.leechDmgBite = g_Skill.GetValue("sk_leech_dmg_bite");

	// Controller
	gSkillData.controllerHealth = g_Skill.GetValue("sk_controller_health");
	gSkillData.controllerDmgZap = g_Skill.GetValue("sk_controller_dmgzap");
	gSkillData.controllerSpeedBall = g_Skill.GetValue("sk_controller_speedball");
	gSkillData.controllerDmgBall = g_Skill.GetValue("sk_controller_dmgball");

	// Nihilanth
	gSkillData.nihilanthHealth = g_Skill.GetValue("sk_nihilanth_health");
	gSkillData.nihilanthZap = g_Skill.GetValue("sk_nihilanth_zap");

	// Scientist
	gSkillData.scientistHealth = g_Skill.GetValue("sk_scientist_health");

	// Cleansuit Scientist
	gSkillData.cleansuitScientistHealth = g_Skill.GetValue("sk_cleansuit_scientist_health");

	// Snark
	gSkillData.snarkHealth = g_Skill.GetValue("sk_snark_health");
	gSkillData.snarkDmgBite = g_Skill.GetValue("sk_snark_dmg_bite");
	gSkillData.snarkDmgPop = g_Skill.GetValue("sk_snark_dmg_pop");

	// Voltigore
	gSkillData.voltigoreHealth = g_Skill.GetValue("sk_voltigore_health");
	gSkillData.voltigoreDmgPunch = g_Skill.GetValue("sk_voltigore_dmg_punch");
	gSkillData.voltigoreDmgBeam = g_Skill.GetValue("sk_voltigore_dmg_beam");

	// Baby Voltigore
	gSkillData.babyvoltigoreHealth = g_Skill.GetValue("sk_babyvoltigore_health");
	gSkillData.babyvoltigoreDmgPunch = g_Skill.GetValue("sk_babyvoltigore_dmg_punch");

	// Pit Worm
	gSkillData.pitWormHealth = g_Skill.GetValue("sk_pitworm_health");
	gSkillData.pitWormDmgSwipe = g_Skill.GetValue("sk_pitworm_dmg_swipe");
	gSkillData.pitWormDmgBeam = g_Skill.GetValue("sk_pitworm_dmg_beam");

	// Gene Worm
	gSkillData.geneWormHealth = g_Skill.GetValue("sk_geneworm_health");
	gSkillData.geneWormDmgSpit = g_Skill.GetValue("sk_geneworm_dmg_spit");
	gSkillData.geneWormDmgHit = g_Skill.GetValue("sk_geneworm_dmg_hit");

	// Zombie
	gSkillData.zombieHealth = g_Skill.GetValue("sk_zombie_health");
	gSkillData.zombieDmgOneSlash = g_Skill.GetValue("sk_zombie_dmg_one_slash");
	gSkillData.zombieDmgBothSlash = g_Skill.GetValue("sk_zombie_dmg_both_slash");

	// Zombie Barney
	gSkillData.zombieBarneyHealth = g_Skill.GetValue("sk_zombie_barney_health");
	gSkillData.zombieBarneyDmgOneSlash = g_Skill.GetValue("sk_zombie_barney_dmg_one_slash");
	gSkillData.zombieBarneyDmgBothSlash = g_Skill.GetValue("sk_zombie_barney_dmg_both_slash");

	// Zombie Soldier
	gSkillData.zombieSoldierHealth = g_Skill.GetValue("sk_zombie_soldier_health");
	gSkillData.zombieSoldierDmgOneSlash = g_Skill.GetValue("sk_zombie_soldier_dmg_one_slash");
	gSkillData.zombieSoldierDmgBothSlash = g_Skill.GetValue("sk_zombie_soldier_dmg_both_slash");

	// Gonome
	gSkillData.gonomeDmgGuts = g_Skill.GetValue("sk_gonome_dmg_guts");
	gSkillData.gonomeHealth = g_Skill.GetValue("sk_gonome_health");
	gSkillData.gonomeDmgOneSlash = g_Skill.GetValue("sk_gonome_dmg_one_slash");
	gSkillData.gonomeDmgOneBite = g_Skill.GetValue("sk_gonome_dmg_one_bite");

	//Turret
	gSkillData.turretHealth = g_Skill.GetValue("sk_turret_health");

	// MiniTurret
	gSkillData.miniturretHealth = g_Skill.GetValue("sk_miniturret_health");

	// Sentry Turret
	gSkillData.sentryHealth = g_Skill.GetValue("sk_sentry_health");

	// PLAYER WEAPONS

	// Crowbar whack
	gSkillData.plrDmgCrowbar = g_Skill.GetValue("sk_plr_crowbar");

	// Glock Round
	gSkillData.plrDmg9MM = g_Skill.GetValue("sk_plr_9mm_bullet");

	// 357 Round
	gSkillData.plrDmg357 = g_Skill.GetValue("sk_plr_357_bullet");

	// MP5 Round
	gSkillData.plrDmgMP5 = g_Skill.GetValue("sk_plr_9mmAR_bullet");

	// M203 grenade
	gSkillData.plrDmgM203Grenade = g_Skill.GetValue("sk_plr_9mmAR_grenade");

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = g_Skill.GetValue("sk_plr_buckshot");

	// Crossbow
	gSkillData.plrDmgCrossbowClient = g_Skill.GetValue("sk_plr_xbow_bolt_client");
	gSkillData.plrDmgCrossbowMonster = g_Skill.GetValue("sk_plr_xbow_bolt_monster");

	// RPG
	gSkillData.plrDmgRPG = g_Skill.GetValue("sk_plr_rpg");

	// Gauss gun
	gSkillData.plrDmgGauss = g_Skill.GetValue("sk_plr_gauss");

	// Egon Gun
	gSkillData.plrDmgEgonNarrow = g_Skill.GetValue("sk_plr_egon_narrow");
	gSkillData.plrDmgEgonWide = g_Skill.GetValue("sk_plr_egon_wide");

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = g_Skill.GetValue("sk_plr_hand_grenade");

	// Satchel Charge
	gSkillData.plrDmgSatchel = g_Skill.GetValue("sk_plr_satchel");

	// Tripmine
	gSkillData.plrDmgTripmine = g_Skill.GetValue("sk_plr_tripmine");

	// Pipe Wrench
	gSkillData.plrDmgPipewrench = g_Skill.GetValue("sk_plr_pipewrench");

	// Knife
	gSkillData.plrDmgKnife = g_Skill.GetValue("sk_plr_knife");

	// Grapple
	gSkillData.plrDmgGrapple = g_Skill.GetValue("sk_plr_grapple");

	// Desert Eagle
	gSkillData.plrDmgEagle = g_Skill.GetValue("sk_plr_eagle");

	// Sniper Rifle
	gSkillData.plrDmg762 = g_Skill.GetValue("sk_plr_762_bullet");

	// M249
	gSkillData.plrDmg556 = g_Skill.GetValue("sk_plr_556_bullet");

	// Displacer
	gSkillData.plrDmgDisplacerSelf = g_Skill.GetValue("sk_plr_displacer_self");
	gSkillData.plrDmgDisplacerOther = g_Skill.GetValue("sk_plr_displacer_other");
	gSkillData.plrRadiusDisplacer = g_Skill.GetValue("sk_plr_displacer_radius");

	// Shock Roach
	gSkillData.plrDmgShockRoachS = g_Skill.GetValue("sk_plr_shockroachs");
	gSkillData.plrDmgShockRoachM = g_Skill.GetValue("sk_plr_shockroachm");

	// Spore Launcher
	gSkillData.plrDmgSpore = g_Skill.GetValue("sk_plr_spore");

	// MONSTER WEAPONS
	gSkillData.monDmg12MM = g_Skill.GetValue("sk_12mm_bullet");
	gSkillData.monDmgMP5 = g_Skill.GetValue("sk_9mmAR_bullet");
	gSkillData.monDmg9MM = g_Skill.GetValue("sk_9mm_bullet");

	// MONSTER HORNET
	gSkillData.monDmgHornet = g_Skill.GetValue("sk_hornet_dmg");

	// PLAYER HORNET
	gSkillData.plrDmgHornet = g_Skill.GetValue("sk_plr_hornet_dmg");


	// HEALTH/CHARGE
	gSkillData.suitchargerCapacity = g_Skill.GetValue("sk_suitcharger");
	gSkillData.batteryCapacity = g_Skill.GetValue("sk_battery");
	gSkillData.healthchargerCapacity = g_Skill.GetValue("sk_healthcharger");
	gSkillData.healthkitCapacity = g_Skill.GetValue("sk_healthkit");
	gSkillData.scientistHeal = g_Skill.GetValue("sk_scientist_heal");
	gSkillData.cleansuitScientistHeal = g_Skill.GetValue("sk_cleansuit_scientist_heal");

	// monster damage adj
	gSkillData.monHead = g_Skill.GetValue("sk_monster_head");
	gSkillData.monChest = g_Skill.GetValue("sk_monster_chest");
	gSkillData.monStomach = g_Skill.GetValue("sk_monster_stomach");
	gSkillData.monLeg = g_Skill.GetValue("sk_monster_leg");
	gSkillData.monArm = g_Skill.GetValue("sk_monster_arm");

	// player damage adj
	gSkillData.plrHead = g_Skill.GetValue("sk_player_head");
	gSkillData.plrChest = g_Skill.GetValue("sk_player_chest");
	gSkillData.plrStomach = g_Skill.GetValue("sk_player_stomach");
	gSkillData.plrLeg = g_Skill.GetValue("sk_player_leg");
	gSkillData.plrArm = g_Skill.GetValue("sk_player_arm");
}

//=========================================================
// instantiate the proper game rules object
//=========================================================

CGameRules* InstallGameRules(CBaseEntity* pWorld)
{
	SERVER_COMMAND("exec game.cfg\n");
	SERVER_EXECUTE();

	if (0 == gpGlobals->deathmatch)
	{
		// generic half-life
		g_teamplay = false;
		return new CHalfLifeRules;
	}

	if (coopplay.value > 0)
	{
		return new CHalfLifeCoopplay();
	}
	else
	{
		if ((pWorld->pev->spawnflags & SF_WORLD_CTF) != 0)
		{
			return new CHalfLifeCTFplay();
		}

		if (teamplay.value > 0)
		{
			// teamplay

			g_teamplay = true;
			return new CHalfLifeTeamplay;
		}
		if ((int)gpGlobals->deathmatch == 1)
		{
			// vanilla deathmatch
			g_teamplay = false;
			return new CHalfLifeMultiplay;
		}
		else
		{
			// vanilla deathmatch??
			g_teamplay = false;
			return new CHalfLifeMultiplay;
		}
	}
}
