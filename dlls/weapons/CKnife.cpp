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
#include "skill.h"

#include "gamerules.h"

#include "CKnife.h"

#define	KNIFE_BODYHIT_VOLUME 128
#define	KNIFE_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_knife, CKnife );

void CKnife::Precache()
{
	PRECACHE_MODEL( "models/v_knife.mdl" );
	PRECACHE_MODEL( "models/w_knife.mdl" );
	PRECACHE_MODEL( "models/p_knife.mdl" );

	PRECACHE_SOUND( "weapons/knife1.wav" );
	PRECACHE_SOUND( "weapons/knife2.wav" );
	PRECACHE_SOUND( "weapons/knife3.wav" );
	PRECACHE_SOUND( "weapons/knife_hit_flesh1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit_flesh2.wav" );
	PRECACHE_SOUND( "weapons/knife_hit_wall1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit_wall2.wav" );

	m_usKnife = PRECACHE_EVENT( 1, "events/knife.sc" );
}

void CKnife::Spawn()
{
	Precache();

	m_iId = WEAPON_KNIFE;

	SET_MODEL( edict(), "models/w_knife.mdl" );

	m_iClip = WEAPON_NOCLIP;

	FallInit();
}

BOOL CKnife::Deploy()
{
	return DefaultDeploy( 
		"models/v_knife.mdl", "models/p_knife.mdl", 
		KNIFE_DRAW, "crowbar" );
}

void CKnife::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim( KNIFE_HOLSTER );
}

void CKnife::PrimaryAttack()
{
	if( !Swing( true ) && !UTIL_UseOldWeapons() )
	{
#ifndef CLIENT_DLL
		SetThink( &CKnife::SwingAgain );
		pev->nextthink =gpGlobals->time + 0.1;
#endif
	}
}

bool CKnife::Swing( const bool bFirst )
{
	bool bDidHit = false;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	if( bFirst )
	{
		PLAYBACK_EVENT_FULL(UTIL_DefaultPlaybackFlags(), m_pPlayer->edict(), m_usKnife,
							 0.0, g_vecZero, g_vecZero, 0, 0, 0,
							 0.0, 0, 0.0 );
	}


	if( tr.flFraction >= 1.0 )
	{
		if( bFirst )
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ( ( m_iSwing++ ) % 2 ) + 1 )
		{
		case 0:
			SendWeaponAnim( KNIFE_ATTACK1 ); break;
		case 1:
			SendWeaponAnim( KNIFE_ATTACK2HIT ); break;
		case 2:
			SendWeaponAnim( KNIFE_ATTACK3HIT ); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL

		// hit
		bDidHit = true;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if( pEntity )
		{
			ClearMultiDamage();

			if( ( m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
			{
				// first swing does full damage
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgKnife, gpGlobals->v_forward, &tr, DMG_CLUB );
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgKnife / 2, gpGlobals->v_forward, &tr, DMG_CLUB );
			}
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
		}

#endif

		m_flNextPrimaryAttack = GetNextAttackDelay(0.25);

#ifndef CLIENT_DLL

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		bool bHitWorld = true;

		if( pEntity )
		{
			if( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG( 0, 1 ) )
				{
				case 0:
					EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/knife_hit_flesh1.wav", 1, ATTN_NORM ); break;
				case 1:
					EMIT_SOUND( m_pPlayer->edict(), CHAN_ITEM, "weapons/knife_hit_flesh2.wav", 1, ATTN_NORM ); break;
				}
				m_pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;
				if( !pEntity->IsAlive() )
					return true;
				else
					flVol = 0.1;

				bHitWorld = false;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if( bHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2, BULLET_PLAYER_CROWBAR );

			if( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch( RANDOM_LONG( 0, 1 ) )
			{
			case 0:
				EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, "weapons/knife_hit_wall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			case 1:
				EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, "weapons/knife_hit_wall2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;

		SetThink( &CKnife::Smack );
		pev->nextthink = gpGlobals->time + 0.2;
#endif
	}
	return bDidHit;
}

void CKnife::SwingAgain()
{
	Swing( false );
}

void CKnife::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

int CKnife::iItemSlot()
{
	return 1;
}

int CKnife::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = nullptr;
	p->iMaxAmmo1 = WEAPON_NOCLIP;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_KNIFE;
	p->iWeight = 0;
	return true;
}
