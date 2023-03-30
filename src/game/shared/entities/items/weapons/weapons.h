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

#pragma once

#include <memory>
#include <spdlog/logger.h>

#include "basemonster.h"
#include "CBaseAnimating.h"
#include "effects.h"
#include "WeaponDataSystem.h"
#include "weaponinfo.h"
#include "items/CBaseItem.h"

class CBasePlayer;
class CBasePlayerWeapon;

void W_Precache();

/**
*	@brief removes all satchels owned by the provided player. Should only be used upon death.
*	Made this global on purpose.
*/
void DeactivateSatchels(CBasePlayer* pOwner);

/**
*	@brief Contact Grenade / Timed grenade / Satchel Charge
*/
class CGrenade : public CBaseMonster
{
public:
	void OnCreate() override;
	void Precache() override;
	void Spawn() override;

	enum SATCHELCODE
	{
		SATCHEL_DETONATE = 0,
		SATCHEL_RELEASE
	};

	static CGrenade* ShootTimed(CBaseEntity* owner, Vector vecStart, Vector vecVelocity, float time);
	static CGrenade* ShootContact(CBaseEntity* owner, Vector vecStart, Vector vecVelocity);
	static CGrenade* ShootSatchelCharge(CBaseEntity* owner, Vector vecStart, Vector vecVelocity);
	static void UseSatchelCharges(CBaseEntity* owner, SATCHELCODE code);

	void Explode(Vector vecSrc, Vector vecAim);
	void Explode(TraceResult* pTrace, int bitsDamageType);
	void EXPORT Smoke();

	void EXPORT BounceTouch(CBaseEntity* pOther);
	void EXPORT SlideTouch(CBaseEntity* pOther);

	/**
	*	@brief Contact grenade, explode when it touches something
	*/
	void EXPORT ExplodeTouch(CBaseEntity* pOther);
	void EXPORT DangerSoundThink();
	void EXPORT PreDetonate();
	void EXPORT Detonate();
	void EXPORT DetonateUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT TumbleThink();

	virtual void BounceSound();
	int BloodColor() override { return DONT_BLEED; }
	void Killed(CBaseEntity* attacker, int iGib) override;

	bool m_fRegisteredSound; // whether or not this grenade has issued its DANGER sound to the world sound list yet.
};

#define MAX_NORMAL_BATTERY 100

// weapon weight factors (for auto-switching)   (-1 = noswitch)
#define CROWBAR_WEIGHT 0
#define GLOCK_WEIGHT 10
#define PYTHON_WEIGHT 15
#define MP5_WEIGHT 15
#define SHOTGUN_WEIGHT 15
#define CROSSBOW_WEIGHT 10
#define RPG_WEIGHT 20
#define GAUSS_WEIGHT 20
#define EGON_WEIGHT 20
#define HORNETGUN_WEIGHT 10
#define HANDGRENADE_WEIGHT 5
#define SNARK_WEIGHT 5
#define SATCHEL_WEIGHT -10
#define TRIPMINE_WEIGHT -10
#define EAGLE_WEIGHT 15
#define SHOCKRIFLE_WEIGHT 15
#define PIPEWRENCH_WEIGHT 2
#define M249_WEIGHT 20
#define DISPLACER_WEIGHT 10
#define SPORELAUNCHER_WEIGHT 20
#define SNIPERRIFLE_WEIGHT 10
#define PENGUIN_WEIGHT 5

// weapon clip/carry ammo capacities
#define URANIUM_MAX_CARRY 100
#define _9MM_MAX_CARRY 250
#define _357_MAX_CARRY 36
#define BUCKSHOT_MAX_CARRY 125
#define BOLT_MAX_CARRY 50
#define ROCKET_MAX_CARRY 5
#define HANDGRENADE_MAX_CARRY 10
#define SATCHEL_MAX_CARRY 5
#define TRIPMINE_MAX_CARRY 5
#define SNARK_MAX_CARRY 15
#define HORNET_MAX_CARRY 8
#define M203_GRENADE_MAX_CARRY 10
#define M249_MAX_CARRY 200
#define SPORELAUNCHER_MAX_CARRY 20
#define SNIPERRIFLE_MAX_CARRY 15
#define PENGUIN_MAX_CARRY 9

// the maximum amount of ammo each weapon's clip can hold

// #define CROWBAR_MAX_CLIP		WEAPON_NOCLIP
#define GLOCK_MAX_CLIP 17
#define PYTHON_MAX_CLIP 6
#define MP5_MAX_CLIP 50
#define MP5_DEFAULT_AMMO 25
#define SHOTGUN_MAX_CLIP 8
#define CROSSBOW_MAX_CLIP 5
#define RPG_MAX_CLIP 1
#define GAUSS_MAX_CLIP WEAPON_NOCLIP
#define EGON_MAX_CLIP WEAPON_NOCLIP
#define HORNETGUN_MAX_CLIP WEAPON_NOCLIP
#define HANDGRENADE_MAX_CLIP WEAPON_NOCLIP
#define SATCHEL_MAX_CLIP WEAPON_NOCLIP
#define TRIPMINE_MAX_CLIP WEAPON_NOCLIP
#define SNARK_MAX_CLIP WEAPON_NOCLIP
#define EAGLE_MAX_CLIP 7
#define M249_MAX_CLIP 50
#define SPORELAUNCHER_MAX_CLIP 5
#define SHOCKRIFLE_MAX_CLIP 10
#define SNIPERRIFLE_MAX_CLIP 5
#define PENGUIN_MAX_CLIP 3

// the default amount of ammo that comes with each gun when it spawns
#define GLOCK_DEFAULT_GIVE 17
#define PYTHON_DEFAULT_GIVE 6
#define DEAGLE_DEFAULT_GIVE 7
#define MP5_DEFAULT_GIVE 25
#define MP5_DEFAULT_AMMO 25
#define MP5_M203_DEFAULT_GIVE 0
#define SHOTGUN_DEFAULT_GIVE 12
#define CROSSBOW_DEFAULT_GIVE 5
#define RPG_DEFAULT_GIVE 1
#define GAUSS_DEFAULT_GIVE 20
#define EGON_DEFAULT_GIVE 20
#define HANDGRENADE_DEFAULT_GIVE 5
#define SATCHEL_DEFAULT_GIVE 1
#define TRIPMINE_DEFAULT_GIVE 1
#define SNARK_DEFAULT_GIVE 5
#define HIVEHAND_DEFAULT_GIVE 8
#define M249_DEFAULT_GIVE 50
#define DISPLACER_DEFAULT_GIVE 40
#define SPORELAUNCHER_DEFAULT_GIVE 5
#define SHOCKRIFLE_DEFAULT_GIVE 10
#define SNIPERRIFLE_DEFAULT_GIVE 5

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE 20
#define AMMO_GLOCKCLIP_GIVE GLOCK_MAX_CLIP
#define AMMO_357BOX_GIVE PYTHON_MAX_CLIP
#define AMMO_MP5CLIP_GIVE MP5_MAX_CLIP
#define AMMO_CHAINBOX_GIVE 200
#define AMMO_M203BOX_GIVE 2
#define AMMO_BUCKSHOTBOX_GIVE 12
#define AMMO_CROSSBOWCLIP_GIVE CROSSBOW_MAX_CLIP
#define AMMO_RPGCLIP_GIVE RPG_MAX_CLIP
#define AMMO_URANIUMBOX_GIVE 20
#define AMMO_SNARKBOX_GIVE 5
#define AMMO_M249_GIVE 50
#define AMMO_EAGLE_GIVE 7
#define AMMO_SPORE_GIVE 1
#define AMMO_SNIPERRIFLE_GIVE 5

// bullet types
enum Bullet
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM,		// glock
	BULLET_PLAYER_MP5,		// mp5
	BULLET_PLAYER_357,		// python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR,	// crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,

	BULLET_PLAYER_556,
	BULLET_PLAYER_762,
	BULLET_PLAYER_EAGLE,
};

#define ITEM_FLAG_SELECTONEMPTY 1
#define ITEM_FLAG_NOAUTORELOAD 2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY 4
#define ITEM_FLAG_LIMITINWORLD 8
#define ITEM_FLAG_EXHAUSTIBLE 16 // A player can totally exhaust their ammo supply and lose this weapon

enum class WeaponState
{
	Inactive = 0,
	Active,
	OnTarget
};

void Weapon_RegisterWeaponData();

constexpr int RefillAllAmmoAmount = -1;

/**
*	@brief Weapons that the player has in their inventory that they can use.
*/
class CBasePlayerWeapon : public CBaseItem
{
public:
	static inline std::shared_ptr<spdlog::logger> WeaponsLogger;

	void SetObjectCollisionBox() override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];

	ItemType GetType() const { return ItemType::InventoryItem; }

	void Accept(IItemVisitor& visitor) override
	{
		visitor.Visit(this);
	}

	void OnCreate() override;

	bool KeyValue(KeyValueData* pkvd) override;

	void Spawn() override;

	void LinkWeaponInfo();

	/**
	*	@brief return true if the item you want the item added to the player inventory
	*/
	virtual bool CanAddToPlayer(CBasePlayer* player) { return true; }

	virtual void AddToPlayer(CBasePlayer* pPlayer);

	/**
	*	@brief return true if you want your duplicate removed from world
	*	CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is @c original.
	*/
	virtual bool AddDuplicate(CBasePlayerWeapon* original);

	/**
	*	@brief called by the new item with the existing item as parameter
	*	@details If we call ExtractAmmo(), it's because the player is picking up this type of weapon for the first time.
	*	If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
	*	if this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo,
	*	which means only the ammo in the weapon clip comes along.
	*	@return true if you can add ammo to yourself when picked up
	*/
	virtual bool ExtractAmmo(CBasePlayerWeapon* weapon);

	/**
	*	@brief Called by the new item's class with the existing item as parameter
	*	@return true if you can add ammo to yourself when picked up
	*/
	virtual bool ExtractClipAmmo(CBasePlayerWeapon* weapon);

	// generic "shared" ammo handlers
	bool AddPrimaryAmmo(CBasePlayerWeapon* origin, int iCount, const char* szName, int iMaxClip);
	bool AddSecondaryAmmo(int iCount, const char* szName);

	void EXPORT DestroyItem();

	/**
	*	@brief returns false if struct not filled out
	*/
	virtual bool GetWeaponInfo(WeaponInfo& info) { return false; }
	
	/**
	*	@brief this function determines whether or not a weapon is useable by the player in its current state.
	*	(does it have ammo loaded? do I have any ammo for the weapon?, etc)
	*/
	virtual bool IsUseable();

	virtual bool CanDeploy();

	/**
	*	@brief returns is deploy was successful
	*/
	virtual bool Deploy()
	{
		return true;
	}

	/**
	*	@brief can this weapon be put away right now?
	*/
	virtual bool CanHolster() { return true; }

	/**
	 *	@brief Put away weapon
	 */
	virtual void Holster();

	/**
	*	@brief updates HUD state
	*/
	virtual void UpdateItemInfo() {}

	virtual void SendWeaponAnim(int iAnim, int body = 0);

	bool DefaultDeploy(const char* szViewModel, const char* szWeaponModel, int iAnim, const char* szAnimExt, int body = 0);
	bool DefaultReload(int iClipSize, int iAnim, float fDelay, int body = 0);

	/**
	*	@brief called each frame by player PreThink
	*/
	virtual void ItemPreFrame() {}

	/**
	*	@brief called each frame by player PostThink
	*/
	virtual void ItemPostFrame();

	// called by CBasePlayerWeapons ItemPostFrame()

	/**
	*	@brief do "+ATTACK"
	*/
	virtual void PrimaryAttack() {}

	/**
	*	@brief do "+ATTACK2"
	*/
	virtual void SecondaryAttack() {}

	/**
	*	@brief do "+RELOAD"
	*/
	virtual void Reload() {}

	virtual bool ShouldWeaponIdle() { return false; }

	/**
	*	@brief called when no buttons pressed
	*/
	virtual void WeaponIdle() {}

	/**
	 *	@brief Animate weapon model
	 */
	virtual bool PlayEmptySound();
	virtual void ResetEmptySound();

	virtual void Kill();
	virtual void AttachToPlayer(CBasePlayer* pPlayer);

	int PrimaryAmmoIndex() { return m_iPrimaryAmmoType; }
	int SecondaryAmmoIndex() { return m_iSecondaryAmmoType; }

	virtual void IncrementAmmo(CBasePlayer* pPlayer) {}

	/**
	*	@brief sends hud info to client dll, if things have changed
	*/
	virtual bool UpdateClientData(CBasePlayer* pPlayer);

	virtual void GetWeaponData(weapon_data_t& data) {}

	virtual void SetWeaponData(const weapon_data_t& data) {}

	virtual void DecrementTimers() {}

	/**
	*	@brief no more ammo for this gun, put it away.
	*/
	void RetireWeapon();

	// Can't use virtual functions as think functions so this wrapper is needed.
	void EXPORT CallDoRetireWeapon()
	{
		DoRetireWeapon();
	}

	virtual void DoRetireWeapon();

	/**
	 *	@brief Whether to use relative time values that are decremented every frame or absolute time values.
	 *	Relative time values are required for predicted weapons.
	 *	@see UTIL_WeaponTimeBase
	 */
	virtual bool UseDecrement() { return false; }

	/**
	*	@brief An accurate way of calculating the next attack time.
	*/
	float GetNextAttackDelay(float delay);

	void SetWeaponModels(const char* viewModel, const char* weaponModel);

	void PrintState();

	int iItemSlot() { return m_WeaponInfo->Slot; } // return 0 to MAX_ITEMS_SLOTS, used in hud
	int iItemPosition() { return m_WeaponInfo->Position; }
	const char* pszAmmo1() { return !m_WeaponInfo->AttackModeInfo[0].AmmoType.empty() ? m_WeaponInfo->AttackModeInfo[0].AmmoType.c_str() : nullptr; }
	const char* pszAmmo2() { return !m_WeaponInfo->AttackModeInfo[1].AmmoType.empty() ? m_WeaponInfo->AttackModeInfo[1].AmmoType.c_str() : nullptr; }
	const char* pszName() { return m_WeaponInfo->Name.c_str(); }
	int iMaxClip() { return m_WeaponInfo->AttackModeInfo[0].MagazineSize; }
	int MagazineSize2() { return m_WeaponInfo->AttackModeInfo[1].MagazineSize; }
	int iWeight() { return m_WeaponInfo->Weight; }
	int iFlags() { return m_WeaponInfo->Flags; }

protected:
	CBasePlayerWeapon* GetItemToRespawn(const Vector& respawnPoint) override;

	ItemAddResult Apply(CBasePlayer* player) override;

public:
	CBasePlayer* m_pPlayer;
	CBasePlayerWeapon* m_pNext;
	int m_iId; // WEAPON_???
	const WeaponInfo* m_WeaponInfo{};

	/**
	*	@brief Hack so deploy animations work when weapon prediction is enabled.
	*/
	bool m_ForceSendAnimations = false;

	bool m_iPlayEmptySound;

	/**
	*	@brief True when the gun is empty and the player is still holding down the attack key(s)
	*/
	bool m_fFireOnEmpty;

	float m_flPumpTime;

	int m_fInSpecialReload;				//!< Are we in the middle of a reload for the shotguns
	float m_flNextPrimaryAttack;		//!< soonest time ItemPostFrame will call PrimaryAttack
	float m_flNextSecondaryAttack;		//!< soonest time ItemPostFrame will call SecondaryAttack
	float m_flTimeWeaponIdle;			//!< soonest time ItemPostFrame will call WeaponIdle
	int m_iPrimaryAmmoType = -1;		//!< "primary" ammo index into players m_rgAmmo[]
	int m_iSecondaryAmmoType = -1;		//!< "secondary" ammo index into players m_rgAmmo[]
	int m_iClip;						//!< number of shots left in the primary weapon clip, -1 it not used
	int m_iClientClip;					//!< the last version of m_iClip sent to hud dll
	WeaponState m_ClientWeaponState;	//!< the last version of the weapon state sent to hud dll (is current weapon, is on target)
	bool m_fInReload;					//!< Are we in the middle of a reload

	int m_iDefaultAmmo; //!< how much ammo you get when you pick up this weapon as placed by a level designer.
	int m_iDefaultPrimaryAmmo; // Copy of m_iDefaultAmmo after KeyValue is handled for respawn.

	// hle time creep vars
	float m_flPrevPrimaryAttack;
	float m_flLastFireTime;

	string_t m_WorldModel;
	string_t m_ViewModel;
	string_t m_PlayerModel;
};

constexpr char DefaultItemPickupSound[] = "items/9mmclip1.wav";

/**
*	@brief Base class for ammo entities.
*	@details Inheriting classes must set m_AmmoAmount to the default give amount
*	and m_AmmoName to a valid ammo name in @c OnCreate.
*/
class CBasePlayerAmmo : public CBaseItem
{
public:
#ifndef CLIENT_DLL
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;

	static TYPEDESCRIPTION m_SaveData[];
#endif

	bool KeyValue(KeyValueData* pkvd) override;

	void Precache() override
	{
		CBaseItem::Precache();
		PrecacheSound(DefaultItemPickupSound);
	}

	void Spawn() override;

	ItemType GetType() const override { return ItemType::Consumable; }

	void Accept(IItemVisitor& visitor) override
	{
		visitor.Visit(this);
	}

protected:
	ItemAddResult Apply(CBasePlayer* player) override
	{
		return AddAmmo(player) ? ItemAddResult::Added : ItemAddResult::NotAdded;
	}

	void PlayPickupSound(const char* pickupSoundName);

	bool GiveAmmo(CBasePlayer* player, int amount, const char* ammoName, const char* pickupSoundName);

	bool DefaultGiveAmmo(CBasePlayer* player, int amount, const char* ammoName, bool playSound);

	virtual bool AddAmmo(CBasePlayer* player);

protected:
	// Set to -2 to detect missing initialization.
	int m_AmmoAmount = -2;
	string_t m_AmmoName;
};

inline short g_sModelIndexLaser; // holds the index for the laser beam
constexpr const char* g_pModelNameLaser = "sprites/laserbeam.spr";

inline short g_sModelIndexLaserDot;	  // holds the index for the laser beam dot
inline short g_sModelIndexFireball;	  // holds the index for the fireball
inline short g_sModelIndexSmoke;	  // holds the index for the smoke cloud
inline short g_sModelIndexWExplosion; // holds the index for the underwater explosion
inline short g_sModelIndexBubbles;	  // holds the index for the bubbles model
inline short g_sModelIndexBloodDrop;  // holds the sprite index for blood drops
inline short g_sModelIndexBloodSpray; // holds the sprite index for blood spray (bigger)

/**
*	@brief resets the global multi damage accumulator
*/
void ClearMultiDamage();

/**
*	@brief inflicts contents of global multi damage register on gMultiDamage.pEntity
*/
void ApplyMultiDamage(CBaseEntity* inflictor, CBaseEntity* attacker);
void AddMultiDamage(CBaseEntity* inflictor, CBaseEntity* pEntity, float flDamage, int bitsDamageType);

void DecalGunshot(TraceResult* pTrace, int iBulletType);
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
int DamageDecal(CBaseEntity* pEntity, int bitsDamageType);

/**
*	@brief this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
*	only damage ents that can clearly be seen by the explosion!
*/
void RadiusDamage(Vector vecSrc, CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType);

/**
*	@brief Collects multiple small damages into a single damage
*/
struct MULTIDAMAGE
{
	CBaseEntity* pEntity;
	float amount;
	int type;
};

inline MULTIDAMAGE gMultiDamage;

void FindHullIntersection(const Vector& vecSrc, TraceResult& tr, const Vector& mins, const Vector& maxs, CBaseEntity* pEntity);

#define LOUD_GUN_VOLUME 1000
#define NORMAL_GUN_VOLUME 600
#define QUIET_GUN_VOLUME 200

#define BRIGHT_GUN_FLASH 512
#define NORMAL_GUN_FLASH 256
#define DIM_GUN_FLASH 128

#define BIG_EXPLOSION_VOLUME 2048
#define NORMAL_EXPLOSION_VOLUME 1024
#define SMALL_EXPLOSION_VOLUME 512

#define WEAPON_ACTIVITY_VOLUME 64

#define VECTOR_CONE_1DEGREES Vector(0.00873, 0.00873, 0.00873)
#define VECTOR_CONE_2DEGREES Vector(0.01745, 0.01745, 0.01745)
#define VECTOR_CONE_3DEGREES Vector(0.02618, 0.02618, 0.02618)
#define VECTOR_CONE_4DEGREES Vector(0.03490, 0.03490, 0.03490)
#define VECTOR_CONE_5DEGREES Vector(0.04362, 0.04362, 0.04362)
#define VECTOR_CONE_6DEGREES Vector(0.05234, 0.05234, 0.05234)
#define VECTOR_CONE_7DEGREES Vector(0.06105, 0.06105, 0.06105)
#define VECTOR_CONE_8DEGREES Vector(0.06976, 0.06976, 0.06976)
#define VECTOR_CONE_9DEGREES Vector(0.07846, 0.07846, 0.07846)
#define VECTOR_CONE_10DEGREES Vector(0.08716, 0.08716, 0.08716)
#define VECTOR_CONE_15DEGREES Vector(0.13053, 0.13053, 0.13053)
#define VECTOR_CONE_20DEGREES Vector(0.17365, 0.17365, 0.17365)

#ifdef CLIENT_DLL
inline bool g_irunninggausspred = false;

void LoadVModel(const char* szViewModel, CBasePlayer* m_pPlayer);
#endif
