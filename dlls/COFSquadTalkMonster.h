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
// CSquadMonster - for any monster that forms squads.
//=========================================================
class COFSquadTalkMonster : public COFAllyMonster
{
public:
	// squad leader info
	EHANDLE	m_hSquadLeader;		// who is my leader
	EHANDLE	m_hSquadMember[ MAX_SQUAD_MEMBERS - 1 ];	// valid only for leader
	int		m_afSquadSlots;
	float	m_flLastEnemySightTime; // last time anyone in the squad saw the enemy
	BOOL	m_fEnemyEluded;

	EHANDLE m_hWaitMedic;
	float m_flMedicWaitTime;
	float m_flLastHitByPlayer;
	int m_iPlayerHits;

	// squad member info
	int		m_iMySlot;// this is the behaviour slot that the monster currently holds in the squad. 

	int  CheckEnemy( CBaseEntity *pEnemy );
	void StartMonster( void );
	void VacateSlot( void );
	void ScheduleChange( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	BOOL OccupySlot( int iDesiredSlot );
	BOOL NoFriendlyFire( void );

	// squad functions still left in base class
	COFSquadTalkMonster *MySquadLeader()
	{
		COFSquadTalkMonster *pSquadLeader = ( COFSquadTalkMonster * ) ( ( CBaseEntity * ) m_hSquadLeader );
		if( pSquadLeader != NULL )
			return pSquadLeader;
		return this;
	}
	COFSquadTalkMonster *MySquadMember( int i )
	{
		if( i >= MAX_SQUAD_MEMBERS - 1 )
			return this;
		else
			return ( COFSquadTalkMonster * ) ( ( CBaseEntity * ) m_hSquadMember[ i ] );
	}
	int	InSquad( void ) { return m_hSquadLeader != NULL; }
	int IsLeader( void ) { return m_hSquadLeader == this; }
	int SquadJoin( int searchRadius );
	int SquadRecruit( int searchRadius, int maxMembers );
	int	SquadCount( void );
	void SquadRemove( COFSquadTalkMonster *pRemove );
	void SquadUnlink( void );
	BOOL SquadAdd( COFSquadTalkMonster *pAdd );
	void SquadDisband( void );
	void SquadAddConditions( int iConditions );
	void SquadMakeEnemy( CBaseEntity *pEnemy );
	void SquadPasteEnemyInfo( void );
	void SquadCopyEnemyInfo( void );
	BOOL SquadEnemySplit( void );
	BOOL SquadMemberInRange( const Vector &vecLocation, float flDist );

	virtual COFSquadTalkMonster *MySquadTalkMonsterPointer( void ) { return this; }

	static TYPEDESCRIPTION m_SaveData[];

	int	Save( CSave &save );
	int Restore( CRestore &restore );

	BOOL FValidateCover( const Vector &vecCoverLocation );

	MONSTERSTATE GetIdealState( void );
	Schedule_t	*GetScheduleOfType( int iType );

	void EXPORT FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	COFSquadTalkMonster* MySquadMedic();

	COFSquadTalkMonster* FindSquadMedic( int searchRadius );

	BOOL HealMe( COFSquadTalkMonster* pTarget );
};

