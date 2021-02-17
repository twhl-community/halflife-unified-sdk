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
#include "weapons.h"
#include "player.h"
#include "UserMessages.h"

#ifndef CLIENT_DLL
#include "CSpore.h"
#endif

#include "CSporeLauncher.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION	CSporeLauncher::m_SaveData[] =
{
	DEFINE_FIELD( CSporeLauncher, m_ReloadState, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSporeLauncher, CSporeLauncher::BaseClass );
#endif

LINK_ENTITY_TO_CLASS( weapon_sporelauncher, CSporeLauncher );

void CSporeLauncher::Precache()
{
	PRECACHE_MODEL( "models/w_spore_launcher.mdl" );
	PRECACHE_MODEL( "models/v_spore_launcher.mdl" );
	PRECACHE_MODEL( "models/p_spore_launcher.mdl" );

	PRECACHE_SOUND( "weapons/splauncher_fire.wav" );
	PRECACHE_SOUND( "weapons/splauncher_altfire.wav" );
	PRECACHE_SOUND( "weapons/splauncher_bounce.wav" );
	PRECACHE_SOUND( "weapons/splauncher_reload.wav" );
	PRECACHE_SOUND( "weapons/splauncher_pet.wav" );

	UTIL_PrecacheOther( "spore" );

	m_usFireSpore = PRECACHE_EVENT( 1, "events/spore.sc" );
}

void CSporeLauncher::Spawn()
{
	Precache();

	m_iId = WEAPON_SPORELAUNCHER;

	SET_MODEL( edict(), "models/w_spore_launcher.mdl" );

	m_iDefaultAmmo = SPORELAUNCHER_DEFAULT_GIVE;

	FallInit();

	pev->sequence = 0;

	pev->animtime = gpGlobals->time;

	pev->framerate = 1;
}

BOOL CSporeLauncher::AddToPlayer( CBasePlayer* pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->edict() );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return true;
	}
	return false;
}

BOOL CSporeLauncher::Deploy()
{
	return DefaultDeploy( "models/v_spore_launcher.mdl", "models/p_spore_launcher.mdl", SPLAUNCHER_DRAW1, "rpg" );
}

void CSporeLauncher::Holster( int skiplocal )
{
	m_ReloadState = ReloadState::NOT_RELOADING;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim( SPLAUNCHER_HOLSTER1 );
}

BOOL CSporeLauncher::ShouldWeaponIdle()
{
	return true;
}

void CSporeLauncher::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		if( m_iClip == 0 && m_ReloadState == ReloadState::NOT_RELOADING && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			Reload();
		}
		else if( m_ReloadState != ReloadState::NOT_RELOADING )
		{
			int maxClip = SPORELAUNCHER_MAX_CLIP;

			if (m_pPlayer->m_iItems & CTFItem::Backpack)
			{
				maxClip *= 2;
			}

			if( m_iClip != maxClip && m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( SPLAUNCHER_AIM );

				m_ReloadState = ReloadState::NOT_RELOADING;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.83;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if( flRand <= 0.75 )
			{
				iAnim = SPLAUNCHER_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
			}
			else if( flRand <= 0.95 )
			{
				iAnim = SPLAUNCHER_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4;
			}
			else
			{
				iAnim = SPLAUNCHER_FIDGET;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4;

				EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/splauncher_pet.wav", 0.7, ATTN_NORM );
			}

			SendWeaponAnim( iAnim );
		}
	}
}

void CSporeLauncher::PrimaryAttack()
{
	if( m_iClip )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		UTIL_MakeVectors( vecAngles );

		const Vector vecSrc = 
			m_pPlayer->GetGunPosition() + 
			gpGlobals->v_forward * 16 +
			gpGlobals->v_right * 8 + 
			gpGlobals->v_up * -8;

		vecAngles = vecAngles + m_pPlayer->GetAutoaimVectorFromPoint( vecSrc, AUTOAIM_10DEGREES );

		CSpore* pSpore = CSpore::CreateSpore( 
			vecSrc, vecAngles,
			m_pPlayer, 
			CSpore::SporeType::ROCKET, false, false );

		UTIL_MakeVectors( vecAngles );

		pSpore->pev->velocity = pSpore->pev->velocity + DotProduct( pSpore->pev->velocity, gpGlobals->v_forward ) * gpGlobals->v_forward;
#endif

		int flags;

#if defined( CLIENT_WEAPONS )
		flags = UTIL_DefaultPlaybackFlags();
#else
		flags = 0;
#endif

		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usFireSpore );

		--m_iClip;
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

	m_ReloadState = ReloadState::NOT_RELOADING;
}

void CSporeLauncher::SecondaryAttack()
{
	if( m_iClip )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		UTIL_MakeVectors( vecAngles );

		const Vector vecSrc =
			m_pPlayer->GetGunPosition() +
			gpGlobals->v_forward * 16 +
			gpGlobals->v_right * 8 +
			gpGlobals->v_up * -8;

		vecAngles = vecAngles + m_pPlayer->GetAutoaimVectorFromPoint( vecSrc, AUTOAIM_10DEGREES );

		CSpore* pSpore = CSpore::CreateSpore(
			vecSrc, vecAngles,
			m_pPlayer,
			CSpore::SporeType::GRENADE, false, false );

		UTIL_MakeVectors( vecAngles );

		pSpore->pev->velocity = m_pPlayer->pev->velocity + 800 * gpGlobals->v_forward;
#endif

		int flags;

#if defined( CLIENT_WEAPONS )
		flags = UTIL_DefaultPlaybackFlags();
#else
		flags = 0;
#endif

		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usFireSpore );

		--m_iClip;
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

	m_ReloadState = ReloadState::NOT_RELOADING;
}

void CSporeLauncher::Reload()
{
	int maxClip = SPORELAUNCHER_MAX_CLIP;

	if (m_pPlayer->m_iItems & CTFItem::Backpack)
	{
		maxClip *= 2;
	}

	if( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] <= 0 || m_iClip == maxClip)
		return;

	// don't reload until recoil is done
	if( m_flNextPrimaryAttack > UTIL_WeaponTimeBase() )
		return;

	// check to see if we're ready to reload
	if( m_ReloadState == ReloadState::NOT_RELOADING )
	{
		SendWeaponAnim( SPLAUNCHER_RELOAD_REACH );
		m_ReloadState = ReloadState::DO_RELOAD_EFFECTS;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.66;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.66;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.66;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.66;
		return;
	}
	else if( m_ReloadState == ReloadState::DO_RELOAD_EFFECTS )
	{
		if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;
		// was waiting for gun to move to side
		m_ReloadState = ReloadState::RELOAD_ONE;

		EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/splauncher_reload.wav", 0.7, ATTN_NORM );

		SendWeaponAnim( SPLAUNCHER_RELOAD );

		m_flNextReload = UTIL_WeaponTimeBase() + 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] -= 1;
		m_ReloadState = ReloadState::DO_RELOAD_EFFECTS;
	}
}

int CSporeLauncher::iItemSlot()
{
	return 4;
}

int CSporeLauncher::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = "spores";
	p->iMaxAmmo1 = SPORELAUNCHER_MAX_CARRY;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = SPORELAUNCHER_MAX_CLIP;
	p->iSlot = 6;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_SPORELAUNCHER;
	p->iFlags = 0;
	p->iWeight = SPORELAUNCHER_WEIGHT;

	return true;
}

void CSporeLauncher::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "spores", SPORELAUNCHER_MAX_CARRY))
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

void CSporeLauncher::GetWeaponData( weapon_data_t& data )
{
	BaseClass::GetWeaponData( data );

	//m_ReloadState is called m_fInSpecialReload in Op4. - Solokiller
	data.m_fInSpecialReload = static_cast<int>( m_ReloadState );
}

void CSporeLauncher::SetWeaponData( const weapon_data_t& data )
{
	BaseClass::SetWeaponData( data );

	m_ReloadState = static_cast<ReloadState>( data.m_fInSpecialReload );
}

#ifndef CLIENT_DLL
enum SporeAmmoAnim
{
	SPOREAMMO_IDLE = 0,
	SPOREAMMO_SPAWNUP,
	SPOREAMMO_SNATCHUP,
	SPOREAMMO_SPAWNDN,
	SPOREAMMO_SNATCHDN,
	SPOREAMMO_IDLE1,
	SPOREAMMO_IDLE2
};

enum SporeAmmoBody
{
	SPOREAMMOBODY_EMPTY = 0,
	SPOREAMMOBODY_FULL
};

class CSporeAmmo : public CBasePlayerAmmo
{
public:
	using BaseClass = CBasePlayerAmmo;

	void Precache() override
	{
		PRECACHE_MODEL( "models/spore_ammo.mdl" );
		PRECACHE_SOUND( "weapons/spore_ammo.wav" );
	}

	void Spawn() override
	{
		Precache();

		SET_MODEL( edict(), "models/spore_ammo.mdl" );

		pev->movetype = MOVETYPE_FLY;

		UTIL_SetSize( pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );

		pev->origin.z += 16;

		UTIL_SetOrigin( pev, pev->origin );

		pev->angles.x -= 90;

		pev->sequence = SPOREAMMO_SPAWNDN;

		pev->animtime = gpGlobals->time;

		pev->nextthink = gpGlobals->time + 4;

		pev->frame = 0;
		pev->framerate = 1;

		pev->health = 1;

		pev->body = SPOREAMMOBODY_FULL;

		pev->takedamage = DAMAGE_AIM;

		pev->solid = SOLID_BBOX;

		SetTouch( &CSporeAmmo::SporeTouch );
		SetThink( &CSporeAmmo::Idling );
	}

	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) override
	{
		if( pev->body == SPOREAMMOBODY_EMPTY )
		{
			return 0;
		}

		pev->body = SPOREAMMOBODY_EMPTY;

		pev->sequence = SPOREAMMO_SNATCHDN;

		pev->animtime = gpGlobals->time;
		pev->frame = 0;
		pev->nextthink = gpGlobals->time + 0.66;

		auto vecLaunchDir = pev->angles;

		vecLaunchDir.x -= 90;
		//Rotate it so spores that aren't rotated in Hammer point in the right direction. - Solokiller
		vecLaunchDir.y += 180;

		vecLaunchDir.x += RANDOM_FLOAT( -20, 20 );
		vecLaunchDir.y += RANDOM_FLOAT( -20, 20 );
		vecLaunchDir.z += RANDOM_FLOAT( -20, 20 );

		auto pSpore = CSpore::CreateSpore( pev->origin, vecLaunchDir, this, CSpore::SporeType::GRENADE, false, true );

		UTIL_MakeVectors( vecLaunchDir );

		pSpore->pev->velocity = gpGlobals->v_forward * 800;

		return 0;
	}

	BOOL AddAmmo( CBaseEntity* pOther ) override
	{
		if( pOther->GiveAmmo( AMMO_SPORE_GIVE, "spores", SPORELAUNCHER_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( edict(), CHAN_ITEM, "weapons/spore_ammo.wav", VOL_NORM, ATTN_NORM );

			return true;
		}

		return false;
	}

	void EXPORT Idling()
	{
		switch( pev->sequence )
		{
		case SPOREAMMO_SPAWNDN:
			{
				pev->sequence = SPOREAMMO_IDLE1;
				pev->animtime = gpGlobals->time;
				pev->frame = 0;
				break;
			}

		case SPOREAMMO_SNATCHDN:
			{
				pev->sequence = SPOREAMMO_IDLE;
				pev->animtime = gpGlobals->time;
				pev->frame = 0;
				pev->nextthink = gpGlobals->time + 10;
				break;
			}

		case SPOREAMMO_IDLE:
			{
				pev->body = SPOREAMMOBODY_FULL;
				pev->sequence = SPOREAMMO_SPAWNDN;
				pev->animtime = gpGlobals->time;
				pev->frame = 0;
				pev->nextthink = gpGlobals->time + 4;
				break;
			}

		default: break;
		}
	}

	void EXPORT SporeTouch( CBaseEntity* pOther )
	{
		if( !pOther->IsPlayer() || pev->body == SPOREAMMOBODY_EMPTY )
			return;

		if( AddAmmo( pOther ) )
		{
			pev->body = SPOREAMMOBODY_EMPTY;

			pev->sequence = SPOREAMMO_SNATCHDN;

			pev->animtime = gpGlobals->time;
			pev->frame = 0;
			pev->nextthink = gpGlobals->time + 0.66;
		}
	}
};

LINK_ENTITY_TO_CLASS( ammo_spore, CSporeAmmo );
#endif
