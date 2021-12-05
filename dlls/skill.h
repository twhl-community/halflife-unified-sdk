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
// skill.h - skill level concerns
//=========================================================

#pragma once

struct skilldata_t
{

	int iSkillLevel; // game skill level

	// Monster Health & Damage
	float agruntHealth;
	float agruntDmgPunch;

	float apacheHealth;

	float barneyHealth;

	float otisHealth;

	float bigmommaHealthFactor; // Multiply each node's health by this
	float bigmommaDmgSlash;		// melee attack damage
	float bigmommaDmgBlast;		// mortar attack damage
	float bigmommaRadiusBlast;	// mortar attack radius

	float bullsquidHealth;
	float bullsquidDmgBite;
	float bullsquidDmgWhip;
	float bullsquidDmgSpit;

	float pitdroneHealth;
	float pitdroneDmgBite;
	float pitdroneDmgWhip;
	float pitdroneDmgSpit;

	float gargantuaHealth;
	float gargantuaDmgSlash;
	float gargantuaDmgFire;
	float gargantuaDmgStomp;

	float hassassinHealth;

	float headcrabHealth;
	float headcrabDmgBite;

	float shockroachHealth;
	float shockroachDmgBite;
	float shockroachLifespan;

	float hgruntHealth;
	float hgruntDmgKick;
	float hgruntShotgunPellets;
	float hgruntGrenadeSpeed;

	float hgruntAllyHealth;
	float hgruntAllyDmgKick;
	float hgruntAllyShotgunPellets;
	float hgruntAllyGrenadeSpeed;

	float medicAllyHealth;
	float medicAllyDmgKick;
	float medicAllyGrenadeSpeed;
	float medicAllyHeal;

	float torchAllyHealth;
	float torchAllyDmgKick;
	float torchAllyGrenadeSpeed;

	float massassinHealth;
	float massassinDmgKick;
	float massassinGrenadeSpeed;

	float shocktrooperHealth;
	float shocktrooperDmgKick;
	float shocktrooperGrenadeSpeed;
	float shocktrooperMaxCharge;
	float shocktrooperRechargeSpeed;

	float houndeyeHealth;
	float houndeyeDmgBlast;

	float slaveHealth;
	float slaveDmgClaw;
	float slaveDmgClawrake;
	float slaveDmgZap;

	float ichthyosaurHealth;
	float ichthyosaurDmgShake;

	float leechHealth;
	float leechDmgBite;

	float controllerHealth;
	float controllerDmgZap;
	float controllerSpeedBall;
	float controllerDmgBall;

	float nihilanthHealth;
	float nihilanthZap;

	float scientistHealth;

	float cleansuitScientistHealth;

	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;

	float voltigoreHealth;
	float voltigoreDmgBeam;
	float voltigoreDmgPunch;

	float babyvoltigoreHealth;
	float babyvoltigoreDmgPunch;

	float pitWormHealth;
	float pitWormDmgSwipe;
	float pitWormDmgBeam;

	float geneWormHealth;
	float geneWormDmgSpit;
	float geneWormDmgHit;

	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;

	float zombieBarneyHealth;
	float zombieBarneyDmgOneSlash;
	float zombieBarneyDmgBothSlash;

	float zombieSoldierHealth;
	float zombieSoldierDmgOneSlash;
	float zombieSoldierDmgBothSlash;

	float gonomeDmgGuts;
	float gonomeHealth;
	float gonomeDmgOneSlash;
	float gonomeDmgOneBite;

	float turretHealth;
	float miniturretHealth;
	float sentryHealth;


	// Player Weapons
	float plrDmgCrowbar;
	float plrDmg9MM;
	float plrDmg357;
	float plrDmgMP5;
	float plrDmgM203Grenade;
	float plrDmgBuckshot;
	float plrDmgCrossbowClient;
	float plrDmgCrossbowMonster;
	float plrDmgRPG;
	float plrDmgGauss;
	float plrDmgEgonNarrow;
	float plrDmgEgonWide;
	float plrDmgHornet;
	float plrDmgHandGrenade;
	float plrDmgSatchel;
	float plrDmgTripmine;

	float plrDmgPipewrench;
	float plrDmgKnife;
	float plrDmgGrapple;
	float plrDmgEagle;
	float plrDmg762;
	float plrDmg556;
	float plrDmgDisplacerSelf;
	float plrDmgDisplacerOther;
	float plrRadiusDisplacer;
	float plrDmgShockRoachS;
	float plrDmgShockRoachM;
	float plrDmgSpore;

	// weapons shared by monsters
	float monDmg9MM;
	float monDmgMP5;
	float monDmg12MM;
	float monDmgHornet;

	// health/suit charge
	float suitchargerCapacity;
	float batteryCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
	float scientistHeal;
	float cleansuitScientistHeal;

	// monster damage adj
	float monHead;
	float monChest;
	float monStomach;
	float monLeg;
	float monArm;

	// player damage adj
	float plrHead;
	float plrChest;
	float plrStomach;
	float plrLeg;
	float plrArm;
};

inline DLL_GLOBAL skilldata_t gSkillData;
float GetSkillCvar(const char* pName);

inline DLL_GLOBAL int g_iSkillLevel;

#define SKILL_EASY 1
#define SKILL_MEDIUM 2
#define SKILL_HARD 3
