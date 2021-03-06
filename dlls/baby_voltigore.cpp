/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Voltigore - Tank like alien
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"hornet.h"
#include "decals.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_BABYVOLTIGORE_THREAT_DISPLAY = LAST_COMMON_SCHEDULE + 1,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_BABYVOLTIGORE_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BABYVOLTIGORE_AE_LEFT_FOOT	 ( 10 )
#define		BABYVOLTIGORE_AE_RIGHT_FOOT ( 11 )
#define		BABYVOLTIGORE_AE_LEFT_PUNCH ( 12 )
#define		BABYVOLTIGORE_AE_RIGHT_PUNCH ( 13 )
#define		BABYVOLTIGORE_AE_RUN			14

#define		BABYVOLTIGORE_MELEE_DIST	128

const int BABY_VOLTIGORE_MAX_BEAMS = 8;

int iBabyVoltigoreMuzzleFlash;

class COFBabyVoltigore : public CSquadMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed () override;
	int  Classify () override;
	int  ISoundMask () override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void SetObjectCollisionBox() override
	{
		pev->absmin = pev->origin + Vector( -16, -16, 0 );
		pev->absmax = pev->origin + Vector( 16, 16, 32 );
	}

	Schedule_t* GetSchedule () override;
	Schedule_t* GetScheduleOfType ( int Type ) override;
	BOOL FCanCheckAttacks () override;
	BOOL CheckMeleeAttack1 ( float flDot, float flDist ) override;
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) override;
	void StartTask ( Task_t *pTask ) override;
	void RunTask( Task_t* pTask ) override;
	void AlertSound() override;
	void AttackSound();
	void PainSound () override;
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) override;
	int IRelationship( CBaseEntity *pTarget ) override;
	void StopTalking ();
	BOOL ShouldSpeak();

	void ClearBeams();

	void Killed( entvars_t* pevAttacker, int iGib ) override;

	CBaseEntity* CheckTraceHullAttack( float flDist, int iDamage, int iDmgType );

	CUSTOM_SCHEDULES;

	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pAttackSounds[];
	static const char *pPainSounds[];
	static const char *pAlertSounds[];

	EHANDLE m_pBeam[BABY_VOLTIGORE_MAX_BEAMS];
	int m_iBeams;

	float	m_flNextBeamAttackCheck;

	float m_flNextPainTime;

	// three hacky fields for speech stuff. These don't really need to be saved.
	float	m_flNextSpeakTime;
	float	m_flNextWordTime;
	int		m_iLastWord;

	int m_iBabyVoltigoreGibs;
	BOOL m_fDeathCharge;
	float m_flDeathStartTime;
};
LINK_ENTITY_TO_CLASS( monster_alien_babyvoltigore, COFBabyVoltigore );

TYPEDESCRIPTION	COFBabyVoltigore::m_SaveData[] = 
{
	DEFINE_ARRAY(COFBabyVoltigore, m_pBeam, FIELD_EHANDLE, BABY_VOLTIGORE_MAX_BEAMS),
	DEFINE_FIELD(COFBabyVoltigore, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD( COFBabyVoltigore, m_flNextBeamAttackCheck, FIELD_TIME ),
	DEFINE_FIELD( COFBabyVoltigore, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( COFBabyVoltigore, m_flNextSpeakTime, FIELD_TIME ),
	DEFINE_FIELD( COFBabyVoltigore, m_flNextWordTime, FIELD_TIME ),
	DEFINE_FIELD( COFBabyVoltigore, m_iLastWord, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( COFBabyVoltigore, CSquadMonster );

const char *COFBabyVoltigore::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *COFBabyVoltigore::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *COFBabyVoltigore::pAttackSounds[] =
{
	"voltigore/voltigore_attack_melee1.wav",
	"voltigore/voltigore_attack_melee2.wav",
};

const char *COFBabyVoltigore::pPainSounds[] =
{
	"voltigore/voltigore_pain1.wav",
	"voltigore/voltigore_pain2.wav",
	"voltigore/voltigore_pain3.wav",
	"voltigore/voltigore_pain4.wav",
};

const char *COFBabyVoltigore::pAlertSounds[] =
{
	"voltigore/voltigore_alert1.wav",
	"voltigore/voltigore_alert2.wav",
	"voltigore/voltigore_alert3.wav",
};

//=========================================================
// IRelationship - overridden because Human Grunts are 
// Alien Grunt's nemesis.
//=========================================================
int COFBabyVoltigore::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_human_grunt" ) )
	{
		return R_NM;
	}

	return CSquadMonster :: IRelationship( pTarget );
}

//=========================================================
// ISoundMask 
//=========================================================
int COFBabyVoltigore :: ISoundMask ()
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// TraceAttack
//=========================================================
void COFBabyVoltigore :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	//Ignore shock damage since we have a shock based attack
	//TODO: use a filter based on attacker to identify self harm
	if ( !( bitsDamageType & DMG_SHOCK ) )
	{
		if( ptr->iHitgroup == 10 && ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) ) )
		{
			// hit armor
			if( pev->dmgtime != gpGlobals->time || ( RANDOM_LONG( 0, 10 ) < 1 ) )
			{
				UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
				pev->dmgtime = gpGlobals->time;
			}

			if( RANDOM_LONG( 0, 1 ) == 0 )
			{
				Vector vecTracerDir = vecDir;

				vecTracerDir.x += RANDOM_FLOAT( -0.3, 0.3 );
				vecTracerDir.y += RANDOM_FLOAT( -0.3, 0.3 );
				vecTracerDir.z += RANDOM_FLOAT( -0.3, 0.3 );

				vecTracerDir = vecTracerDir * -512;

				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos );
				WRITE_BYTE( TE_TRACER );
				WRITE_COORD( ptr->vecEndPos.x );
				WRITE_COORD( ptr->vecEndPos.y );
				WRITE_COORD( ptr->vecEndPos.z );

				WRITE_COORD( vecTracerDir.x );
				WRITE_COORD( vecTracerDir.y );
				WRITE_COORD( vecTracerDir.z );
				MESSAGE_END();
			}

			flDamage -= 20;
			if( flDamage <= 0 )
				flDamage = 0.1;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
		}
		else
		{
			SpawnBlood( ptr->vecEndPos, BloodColor(), flDamage );// a little surface blood.
			TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		}

		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
}

//=========================================================
// StopTalking - won't speak again for 10-20 seconds.
//=========================================================
void COFBabyVoltigore::StopTalking()
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);
}

//=========================================================
// ShouldSpeak - Should this voltigore be talking?
//=========================================================
BOOL COFBabyVoltigore::ShouldSpeak()
{
	if ( m_flNextSpeakTime > gpGlobals->time )
	{
		// my time to talk is still in the future.
		return FALSE;
	}

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// if gagged, don't talk outside of combat.
			// if not going to talk because of this, put the talk time 
			// into the future a bit, so we don't talk immediately after 
			// going into combat
			m_flNextSpeakTime = gpGlobals->time + 3;
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
// AlertSound
//=========================================================
void COFBabyVoltigore :: AlertSound ()
{
	StopTalking();

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1)], 1.0, ATTN_NORM, 0, 180 );
}

//=========================================================
// AttackSound
//=========================================================
void COFBabyVoltigore::AttackSound()
{
	StopTalking();

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pAttackSounds[ RANDOM_LONG( 0, ARRAYSIZE( pAttackSounds ) - 1 ) ], 1.0, ATTN_NORM, 0, 180 );
}

//=========================================================
// PainSound
//=========================================================
void COFBabyVoltigore :: PainSound ()
{
	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	StopTalking();

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1)], 1.0, ATTN_NORM, 0, 180 );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	COFBabyVoltigore :: Classify ()
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COFBabyVoltigore :: SetYawSpeed ()
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 80;
		break;
	default:			ys = 70;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void COFBabyVoltigore :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	//This is in the original for some reason
	/*
	PRECACHE_SOUND( "voltigore/voltigore_footstep1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_footstep2.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_footstep3.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_run_grunt1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_run_grunt2.wav" );
	*/

	switch( pEvent->event )
	{
	case BABYVOLTIGORE_AE_LEFT_FOOT:
	case BABYVOLTIGORE_AE_RIGHT_FOOT:
		switch( RANDOM_LONG( 0, 2 ) )
		{
			// left foot
		case 0:	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "voltigore/voltigore_footstep1.wav", 1, ATTN_IDLE, 0, 130 );	break;
		case 1:	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "voltigore/voltigore_footstep2.wav", 1, ATTN_IDLE, 0, 130 );	break;
		case 2: EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "voltigore/voltigore_footstep3.wav", 1, ATTN_IDLE, 0, 130 );	break;
		}
		break;

	case BABYVOLTIGORE_AE_LEFT_PUNCH:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( BABYVOLTIGORE_MELEE_DIST, gSkillData.babyvoltigoreDmgPunch, DMG_CLUB );
			
			if ( pHurt )
			{
				pHurt->pev->punchangle.y = -25;
				pHurt->pev->punchangle.x = 8;

				// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
				if ( pHurt->IsPlayer() )
				{
					// this is a player. Knock him around.
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 250;
				}

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_IDLE, 0, 130 );

				Vector vecArmPos, vecArmAng;
				GetAttachment( 0, vecArmPos, vecArmAng );
				SpawnBlood(vecArmPos, pHurt->BloodColor(), 25);// a little surface blood.
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_IDLE, 0, 130 );
			}
		}
		break;

	case BABYVOLTIGORE_AE_RIGHT_PUNCH:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( BABYVOLTIGORE_MELEE_DIST, gSkillData.babyvoltigoreDmgPunch, DMG_CLUB );

			if ( pHurt )
			{
				pHurt->pev->punchangle.y = 25;
				pHurt->pev->punchangle.x = 8;

				// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
				if ( pHurt->IsPlayer() )
				{
					// this is a player. Knock him around.
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -250;
				}

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_IDLE, 0, 130 );

				Vector vecArmPos, vecArmAng;
				GetAttachment( 0, vecArmPos, vecArmAng );
				SpawnBlood(vecArmPos, pHurt->BloodColor(), 25);// a little surface blood.
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_IDLE, 0, 130 );
			}
		}
		break;

	case BABYVOLTIGORE_AE_RUN:
		switch( RANDOM_LONG( 0, 1 ) )
		{
			// left foot
		case 0:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "voltigore/voltigore_run_grunt1.wav", 1, ATTN_NORM, 0, 180 );	break;
		case 1:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "voltigore/voltigore_run_grunt2.wav", 1, ATTN_NORM, 0, 180 );	break;
		}
		break;

	default:
		CSquadMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void COFBabyVoltigore :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/baby_voltigore.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 32));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.babyvoltigoreHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= 0;
	m_afCapability		|= bits_CAP_SQUAD | bits_CAP_TURN_HEAD;

	m_HackedGunPos		= Vector( 24, 64, 48 );

	m_flNextSpeakTime	= m_flNextWordTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);

	m_flNextBeamAttackCheck = gpGlobals->time;
	m_fDeathCharge = false;
	m_flDeathStartTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COFBabyVoltigore :: Precache()
{
	int i;

	PRECACHE_MODEL("models/baby_voltigore.mdl");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	PRECACHE_SOUND_ARRAY( pAttackSounds );

	PRECACHE_SOUND( "voltigore/voltigore_attack_shock.wav" );

	PRECACHE_SOUND( "voltigore/voltigore_communicate1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_communicate2.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_communicate3.wav" );

	PRECACHE_SOUND( "voltigore/voltigore_die1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_die2.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_die3.wav" );

	PRECACHE_SOUND( "voltigore/voltigore_footstep1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_footstep2.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_footstep3.wav" );

	PRECACHE_SOUND( "voltigore/voltigore_run_grunt1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_run_grunt2.wav" );

	PRECACHE_SOUND( "hassault/hw_shoot1.wav" );

	PRECACHE_SOUND( "debris/beamstart2.wav" );

	iBabyVoltigoreMuzzleFlash = PRECACHE_MODEL( "sprites/muz4.spr" );

	UTIL_PrecacheOther( "charged_bolt" );

	m_iBabyVoltigoreGibs = PRECACHE_MODEL( "models/vgibs.mdl" );
}	
	
//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail Schedule
//=========================================================
Task_t	tlBabyVoltigoreFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slBabyVoltigoreFail[] =
{
	{
		tlBabyVoltigoreFail,
		ARRAYSIZE ( tlBabyVoltigoreFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"BabyVoltigore Fail"
	},
};

//=========================================================
// Combat Fail Schedule
//=========================================================
Task_t	tlBabyVoltigoreCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slBabyVoltigoreCombatFail[] =
{
	{
		tlBabyVoltigoreCombatFail,
		ARRAYSIZE ( tlBabyVoltigoreCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"BabyVoltigore Combat Fail"
	},
};

//=========================================================
// Standoff schedule. Used in combat when a monster is 
// hiding in cover or the enemy has moved out of sight. 
// Should we look around in this schedule?
//=========================================================
Task_t	tlBabyVoltigoreStandoff[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)2					},
};

Schedule_t slBabyVoltigoreStandoff[] = 
{
	{
		tlBabyVoltigoreStandoff,
		ARRAYSIZE ( tlBabyVoltigoreStandoff ),
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_MELEE_ATTACK1		|
		bits_COND_SEE_ENEMY				|
		bits_COND_NEW_ENEMY				|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"BabyVoltigore Standoff"
	}
};

//=========================================================
// primary range attacks
//=========================================================
Task_t	tlBabyVoltigoreRangeAttack1[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_RANGE_ATTACK1_NOTURN,	(float)0		},
	{ TASK_WAIT,					0.5 },
};

Schedule_t	slBabyVoltigoreRangeAttack1[] =
{
	{ 
		tlBabyVoltigoreRangeAttack1,
		ARRAYSIZE ( tlBabyVoltigoreRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_HEAVY_DAMAGE,
		
		0,
		"BabyVoltigore Range Attack1"
	},
};

//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t	tlBabyVoltigoreTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slBabyVoltigoreTakeCoverFromEnemy[] =
{
	{ 
		tlBabyVoltigoreTakeCoverFromEnemy,
		ARRAYSIZE ( tlBabyVoltigoreTakeCoverFromEnemy ), 
		bits_COND_NEW_ENEMY,
		0,
		"BabyVoltigoreTakeCoverFromEnemy"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlBabyVoltigoreVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_BABYVOLTIGORE_THREAT_DISPLAY	},
	{ TASK_WAIT,							(float)0.2					},
	{ TASK_BABYVOLTIGORE_GET_PATH_TO_ENEMY_CORPSE,	(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_CROUCH			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_STAND			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_THREAT_DISPLAY	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_CROUCH			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_STAND			},
};

Schedule_t	slBabyVoltigoreVictoryDance[] =
{
	{ 
		tlBabyVoltigoreVictoryDance,
		ARRAYSIZE ( tlBabyVoltigoreVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"BabyVoltigoreVictoryDance"
	},
};

//=========================================================
//=========================================================
Task_t	tlBabyVoltigoreThreatDisplay[] =
{
	{ TASK_STOP_MOVING,			(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_THREAT_DISPLAY	},
};

Schedule_t	slBabyVoltigoreThreatDisplay[] =
{
	{ 
		tlBabyVoltigoreThreatDisplay,
		ARRAYSIZE ( tlBabyVoltigoreThreatDisplay ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		
		bits_SOUND_PLAYER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_WORLD,
		"BabyVoltigoreThreatDisplay"
	},
};

DEFINE_CUSTOM_SCHEDULES( COFBabyVoltigore )
{
	slBabyVoltigoreFail,
	slBabyVoltigoreCombatFail,
	slBabyVoltigoreStandoff,
	slBabyVoltigoreRangeAttack1,
	slBabyVoltigoreTakeCoverFromEnemy,
	slBabyVoltigoreVictoryDance,
	slBabyVoltigoreThreatDisplay,
};

IMPLEMENT_CUSTOM_SCHEDULES( COFBabyVoltigore, CSquadMonster );

//=========================================================
// FCanCheckAttacks - this is overridden for alien grunts
// because they can use their smart weapons against unseen
// enemies. Base class doesn't attack anyone it can't see.
//=========================================================
BOOL COFBabyVoltigore :: FCanCheckAttacks ()
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// CheckMeleeAttack1 - alien grunts zap the crap out of 
// any enemy that gets too close. 
//=========================================================
BOOL COFBabyVoltigore :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( HasConditions ( bits_COND_SEE_ENEMY ) && flDist <= BABYVOLTIGORE_MELEE_DIST && flDot >= 0.6 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 
//
// !!!LATER - we may want to load balance this. Several
// tracelines are done, so we may not want to do this every
// server frame. Definitely not while firing. 
//=========================================================
BOOL COFBabyVoltigore :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
	{
		return false;
	}

	if ( flDist >= BABYVOLTIGORE_MELEE_DIST && flDist <= 1024 && flDot >= 0.5 && gpGlobals->time >= m_flNextBeamAttackCheck )
	{
		TraceResult	tr;
		Vector	vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		UTIL_MakeVectors( pev->angles );
		GetAttachment( 0, vecArmPos, vecArmDir );
//		UTIL_TraceLine( vecArmPos, vecArmPos + gpGlobals->v_forward * 256, ignore_monsters, ENT(pev), &tr);
		UTIL_TraceLine( vecArmPos, m_hEnemy->BodyTarget(vecArmPos), dont_ignore_monsters, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 || tr.pHit == m_hEnemy->edict() )
		{
			m_flNextBeamAttackCheck = gpGlobals->time + RANDOM_FLOAT( 20, 30 );
			return true;
		}

		m_flNextBeamAttackCheck = gpGlobals->time + 0.2;// don't check for half second if this check wasn't successful
	}
	
	return false;
}

//=========================================================
// StartTask
//=========================================================
void COFBabyVoltigore :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_BABYVOLTIGORE_GET_PATH_TO_ENEMY_CORPSE:
		{
			ClearBeams();

			UTIL_MakeVectors( pev->angles );
			if ( BuildRoute ( m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete();
			}
			else
			{
				ALERT ( at_aiconsole, "BabyVoltigoreGetPathToEnemyCorpse failed!!\n" );
				TaskFail();
			}
		}
		break;

	case TASK_RANGE_ATTACK1_NOTURN:
		{
			ClearBeams();

			UTIL_MakeVectors( pev->angles );

			const auto vecConverge = pev->origin + gpGlobals->v_forward * 50 + gpGlobals->v_up * 32;

			for( auto i = 0; i < 3; ++i )
			{
				CBeam* pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 50);;
				m_pBeam[ m_iBeams ] = pBeam;

				if( !pBeam)
					return;

				pBeam->PointEntInit( vecConverge, entindex() );
				pBeam->SetEndAttachment( i + 1 );
				pBeam->SetColor( 180, 16, 255 );
				pBeam->SetBrightness( 255 );
				pBeam->SetNoise( 20 );

				++m_iBeams;
			}

			UTIL_EmitAmbientSound( edict(), pev->origin, "debris/beamstart2.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
			CBaseMonster::StartTask( pTask );
		}
		break;

	default:
		ClearBeams();

		CSquadMonster :: StartTask ( pTask );
		break;
	}
}

void COFBabyVoltigore::RunTask( Task_t* pTask )
{
	switch( pTask->iTask )
	{
	case TASK_DIE:
		{
			if( m_fSequenceFinished )
			{
				if( pev->frame >= 255 )
				{
					pev->deadflag = DEAD_DEAD;

					pev->framerate = 0;

					//Flatten the bounding box so players can step on it
					if( BBoxFlat() )
					{
						const auto maxs = Vector( pev->maxs.x, pev->maxs.y, pev->mins.z + 1 );
						UTIL_SetSize( pev, pev->mins, maxs );
					}
					else
					{
						UTIL_SetSize( pev, { -4, -4, 0 }, { 4, 4, 1 } );
					}

					if( ShouldFadeOnDeath() )
					{
						SUB_StartFadeOut();
					}
					else
					{
						CSoundEnt::InsertSound( bits_SOUND_CARCASS, pev->origin, 384, 30.0 );
					}
				}
			}
		}
		break;

	default:
		CSquadMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *COFBabyVoltigore :: GetSchedule ()
{
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
		{
			// dangerous sound nearby!
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType( SCHED_WAKE_ANGRY );
			}

	// zap player!
			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				AttackSound();// this is a total hack. Should be parto f the schedule
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions ( bits_COND_HEAVY_DAMAGE ) )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}

	// can attack
			if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			if ( OccupySlot ( bits_SLOT_AGRUNT_CHASE ) )
			{
				return GetScheduleOfType ( SCHED_CHASE_ENEMY );
			}

			return GetScheduleOfType ( SCHED_STANDOFF );
		}
	}

	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* COFBabyVoltigore :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return &slBabyVoltigoreTakeCoverFromEnemy[ 0 ];
		break;
	
	case SCHED_RANGE_ATTACK1:
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			//normal attack
			return &slBabyVoltigoreRangeAttack1[ 0 ];
		}
		else
		{
			// attack an unseen enemy
			// return &slBabyVoltigoreHiddenRangeAttack[ 0 ];
			return &slBabyVoltigoreCombatFail[ 0 ];
		}
		break;

	case SCHED_CHASE_ENEMY_FAILED:
		return &slBabyVoltigoreRangeAttack1[ 0 ];

	case SCHED_BABYVOLTIGORE_THREAT_DISPLAY:
		return &slBabyVoltigoreThreatDisplay[ 0 ];
		break;

	case SCHED_STANDOFF:
		return &slBabyVoltigoreStandoff[ 0 ];
		break;

	case SCHED_VICTORY_DANCE:
		return &slBabyVoltigoreVictoryDance[ 0 ];
		break;

	case SCHED_FAIL:
		// no fail schedule specified, so pick a good generic one.
		{
			if ( m_hEnemy != NULL )
			{
				// I have an enemy
				// !!!LATER - what if this enemy is really far away and i'm chasing him?
				// this schedule will make me stop, face his last known position for 2 
				// seconds, and then try to move again
				return &slBabyVoltigoreCombatFail[ 0 ];
			}

			return &slBabyVoltigoreFail[ 0 ];
		}
		break;

	}

	return CSquadMonster :: GetScheduleOfType( Type );
}

void COFBabyVoltigore::ClearBeams()
{
	for( auto& pBeam : m_pBeam )
	{
		if( pBeam )
		{
			UTIL_Remove( pBeam );
			pBeam = nullptr;
		}
	}

	m_iBeams = 0;

	pev->skin = 0;
}

void COFBabyVoltigore::Killed( entvars_t* pevAttacker, int iGib )
{
	ClearBeams();

	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
//
// Used for many contact-range melee attacks. Bites, claws, etc.
//=========================================================
CBaseEntity* COFBabyVoltigore::CheckTraceHullAttack( float flDist, int iDamage, int iDmgType )
{
	TraceResult tr;

	if( IsPlayer() )
		UTIL_MakeVectors( pev->angles );
	else
		UTIL_MakeAimVectors( pev->angles );

	Vector vecStart = pev->origin;
	//Don't rescale the Z size for us since we're just a baby
	vecStart.z += pev->size.z;
	Vector vecEnd = vecStart + ( gpGlobals->v_forward * flDist );

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT( pev ), &tr );

	if( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}
