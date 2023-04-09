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

#include "cbase.h"

#define BARNACLE_BODY_HEIGHT 44 //!< how 'tall' the barnacle's model is.
#define BARNACLE_PULL_SPEED 8
#define BARNACLE_KILL_VICTIM_DELAY 5 //!< how many seconds after pulling prey in to gib them.

#define BARNACLE_AE_PUKEGIB 2

/**
 *	@brief stationary ceiling mounted 'fishing' monster
 */
class CBarnacle : public CBaseMonster
{
	DECLARE_CLASS(CBarnacle, CBaseMonster);
	DECLARE_DATAMAP();

public:
	void OnCreate() override;
	void Spawn() override;
	void Precache() override;

	/**
	 *	@brief does a trace along the barnacle's tongue to see if any entity is touching it.
	 *	Also stores the length of the trace in the float reference provided.
	 */
	CBaseEntity* TongueTouchEnt(float& flLength);
	int Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void BarnacleThink();
	void WaitTillDead();
	void Killed(CBaseEntity* attacker, int iGib) override;
	bool TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType) override;

	float m_flAltitude;
	float m_flKillVictimTime;
	int m_cGibs; // barnacle loads up on gibs each time it kills something.
	bool m_fTongueExtended;
	bool m_fLiftingPrey;
	float m_flTongueAdj;
};

LINK_ENTITY_TO_CLASS(monster_barnacle, CBarnacle);

BEGIN_DATAMAP(CBarnacle)
DEFINE_FIELD(m_flAltitude, FIELD_FLOAT),
	DEFINE_FIELD(m_flKillVictimTime, FIELD_TIME),
	DEFINE_FIELD(m_cGibs, FIELD_INTEGER), // barnacle loads up on gibs each time it kills something.
	DEFINE_FIELD(m_fTongueExtended, FIELD_BOOLEAN),
	DEFINE_FIELD(m_fLiftingPrey, FIELD_BOOLEAN),
	DEFINE_FIELD(m_flTongueAdj, FIELD_FLOAT),
	DEFINE_FUNCTION(BarnacleThink),
	DEFINE_FUNCTION(WaitTillDead),
	END_DATAMAP();

void CBarnacle::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = 25;
	pev->model = MAKE_STRING("models/barnacle.mdl");
}

int CBarnacle::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

void CBarnacle::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BARNACLE_AE_PUKEGIB:
		CGib::SpawnRandomGibs(this, 1, true);
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CBarnacle::Spawn()
{
	Precache();

	SetModel(STRING(pev->model));
	SetSize(Vector(-16, -16, -32), Vector(16, 16, 0));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_AIM;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = EF_INVLIGHT; // take light from the ceiling
	m_flFieldOfView = 0.5;		// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flKillVictimTime = 0;
	m_cGibs = 0;
	m_fLiftingPrey = false;
	m_flTongueAdj = -100;

	InitBoneControllers();

	SetActivity(ACT_IDLE);

	SetThink(&CBarnacle::BarnacleThink);
	pev->nextthink = gpGlobals->time + 0.5;

	SetOrigin(pev->origin);
}

bool CBarnacle::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if ((bitsDamageType & DMG_CLUB) != 0)
	{
		flDamage = pev->health;
	}

	return CBaseMonster::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);
}

void CBarnacle::BarnacleThink()
{
	CBaseEntity* pTouchEnt;
	CBaseMonster* pVictim;
	float flLength;

	pev->nextthink = gpGlobals->time + 0.1;

	UpdateShockEffect();

	if (m_hEnemy != nullptr)
	{
		// barnacle has prey.

		if (!m_hEnemy->IsAlive())
		{
			// someone (maybe even the barnacle) killed the prey. Reset barnacle.
			m_fLiftingPrey = false; // indicate that we're not lifting prey.
			m_hEnemy = nullptr;
			return;
		}

		if (m_fLiftingPrey)
		{
			if (m_hEnemy != nullptr && m_hEnemy->pev->deadflag != DEAD_NO)
			{
				// crap, someone killed the prey on the way up.
				m_hEnemy = nullptr;
				m_fLiftingPrey = false;
				return;
			}

			// still pulling prey.
			Vector vecNewEnemyOrigin = m_hEnemy->pev->origin;
			vecNewEnemyOrigin.x = pev->origin.x;
			vecNewEnemyOrigin.y = pev->origin.y;

			// guess as to where their neck is
			vecNewEnemyOrigin.x -= 6 * cos(m_hEnemy->pev->angles.y * PI / 180.0);
			vecNewEnemyOrigin.y -= 6 * sin(m_hEnemy->pev->angles.y * PI / 180.0);

			m_flAltitude -= BARNACLE_PULL_SPEED;
			vecNewEnemyOrigin.z += BARNACLE_PULL_SPEED;

			if (fabs(pev->origin.z - (vecNewEnemyOrigin.z + m_hEnemy->pev->view_ofs.z - 8)) < BARNACLE_BODY_HEIGHT)
			{
				// prey has just been lifted into position ( if the victim origin + eye height + 8 is higher than the bottom of the barnacle, it is assumed that the head is within barnacle's body )
				m_fLiftingPrey = false;

				EmitSound(CHAN_WEAPON, "barnacle/bcl_bite3.wav", 1, ATTN_NORM);

				pVictim = m_hEnemy->MyMonsterPointer();

				m_flKillVictimTime = gpGlobals->time + 10; // now that the victim is in place, the killing bite will be administered in 10 seconds.

				if (pVictim)
				{
					pVictim->BarnacleVictimBitten(this);
					SetActivity(ACT_EAT);
				}
			}

			m_hEnemy->SetOrigin(vecNewEnemyOrigin);
		}
		else
		{
			// prey is lifted fully into feeding position and is dangling there.

			pVictim = m_hEnemy->MyMonsterPointer();

			if (m_flKillVictimTime != -1 && gpGlobals->time > m_flKillVictimTime)
			{
				// kill!
				if (pVictim)
				{
					pVictim->TakeDamage(this, this, pVictim->pev->health, DMG_SLASH | DMG_ALWAYSGIB);
					m_cGibs = 3;
				}

				return;
			}

			// bite prey every once in a while
			if (pVictim && (RANDOM_LONG(0, 49) == 0))
			{
				switch (RANDOM_LONG(0, 2))
				{
				case 0:
					EmitSound(CHAN_WEAPON, "barnacle/bcl_chew1.wav", 1, ATTN_NORM);
					break;
				case 1:
					EmitSound(CHAN_WEAPON, "barnacle/bcl_chew2.wav", 1, ATTN_NORM);
					break;
				case 2:
					EmitSound(CHAN_WEAPON, "barnacle/bcl_chew3.wav", 1, ATTN_NORM);
					break;
				}

				pVictim->BarnacleVictimBitten(this);
			}
		}
	}
	else
	{
		// barnacle has no prey right now, so just idle and check to see if anything is touching the tongue.

		// If idle and no nearby client, don't think so often
		if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT(1, 1.5); // Stagger a bit to keep barnacles from thinking on the same frame

		if (m_fSequenceFinished)
		{ // this is done so barnacle will fidget.
			SetActivity(ACT_IDLE);
			m_flTongueAdj = -100;
		}

		if (0 != m_cGibs && RANDOM_LONG(0, 99) == 1)
		{
			// cough up a gib.
			CGib::SpawnRandomGibs(this, 1, true);
			m_cGibs--;

			switch (RANDOM_LONG(0, 2))
			{
			case 0:
				EmitSound(CHAN_WEAPON, "barnacle/bcl_chew1.wav", 1, ATTN_NORM);
				break;
			case 1:
				EmitSound(CHAN_WEAPON, "barnacle/bcl_chew2.wav", 1, ATTN_NORM);
				break;
			case 2:
				EmitSound(CHAN_WEAPON, "barnacle/bcl_chew3.wav", 1, ATTN_NORM);
				break;
			}
		}

		pTouchEnt = TongueTouchEnt(flLength);

		if (pTouchEnt != nullptr && m_fTongueExtended)
		{
			// tongue is fully extended, and is touching someone.
			if (pTouchEnt->FBecomeProne())
			{
				EmitSound(CHAN_WEAPON, "barnacle/bcl_alert2.wav", 1, ATTN_NORM);

				SetSequenceByName("attack1");
				m_flTongueAdj = -20;

				m_hEnemy = pTouchEnt;

				pTouchEnt->pev->movetype = MOVETYPE_FLY;
				pTouchEnt->pev->velocity = g_vecZero;
				pTouchEnt->pev->basevelocity = g_vecZero;
				pTouchEnt->pev->origin.x = pev->origin.x;
				pTouchEnt->pev->origin.y = pev->origin.y;

				m_fLiftingPrey = true;	 // indicate that we should be lifting prey.
				m_flKillVictimTime = -1; // set this to a bogus time while the victim is lifted.

				m_flAltitude = (pev->origin.z - pTouchEnt->EyePosition().z);
			}
		}
		else
		{
			// calculate a new length for the tongue to be clear of anything else that moves under it.
			if (m_flAltitude < flLength)
			{
				// if tongue is higher than is should be, lower it kind of slowly.
				m_flAltitude += BARNACLE_PULL_SPEED;
				m_fTongueExtended = false;
			}
			else
			{
				m_flAltitude = flLength;
				m_fTongueExtended = true;
			}
		}
	}

	// AILogger->debug("tounge {}", m_flAltitude + m_flTongueAdj);
	SetBoneController(0, -(m_flAltitude + m_flTongueAdj));
	StudioFrameAdvance(0.1);
}

void CBarnacle::Killed(CBaseEntity* attacker, int iGib)
{
	CBaseMonster* pVictim;

	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	// Added for Op4
	pev->deadflag = DEAD_DYING;

	ClearShockEffect();

	if (m_hEnemy != nullptr)
	{
		pVictim = m_hEnemy->MyMonsterPointer();

		if (pVictim)
		{
			pVictim->BarnacleVictimReleased();
		}
	}

	//	CGib::SpawnRandomGibs( pev, 4, 1 );

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EmitSound(CHAN_WEAPON, "barnacle/bcl_die1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EmitSound(CHAN_WEAPON, "barnacle/bcl_die3.wav", 1, ATTN_NORM);
		break;
	}

	SetActivity(ACT_DIESIMPLE);
	SetBoneController(0, 0);

	StudioFrameAdvance(0.1);

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CBarnacle::WaitTillDead);
}

void CBarnacle::WaitTillDead()
{
	pev->nextthink = gpGlobals->time + 0.1;

	float flInterval = StudioFrameAdvance(0.1);
	DispatchAnimEvents(flInterval);

	if (m_fSequenceFinished)
	{
		// death anim finished.
		StopAnimation();
		SetThink(nullptr);
	}
}

void CBarnacle::Precache()
{
	PrecacheModel(STRING(pev->model));

	PrecacheSound("barnacle/bcl_alert2.wav"); // happy, lifting food up
	PrecacheSound("barnacle/bcl_bite3.wav");  // just got food to mouth
	PrecacheSound("barnacle/bcl_chew1.wav");
	PrecacheSound("barnacle/bcl_chew2.wav");
	PrecacheSound("barnacle/bcl_chew3.wav");
	PrecacheSound("barnacle/bcl_die1.wav");
	PrecacheSound("barnacle/bcl_die3.wav");
}

#define BARNACLE_CHECK_SPACING 8
CBaseEntity* CBarnacle::TongueTouchEnt(float& flLength)
{
	TraceResult tr;

	// trace once to hit architecture and see if the tongue needs to change position.
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 2048), ignore_monsters, edict(), &tr);
	const float length = fabs(pev->origin.z - tr.vecEndPos.z);
	flLength = length;

	Vector delta = Vector(BARNACLE_CHECK_SPACING, BARNACLE_CHECK_SPACING, 0);
	Vector mins = pev->origin - delta;
	Vector maxs = pev->origin + delta;
	maxs.z = pev->origin.z;
	mins.z -= length;

	CBaseEntity* pList[10];
	int count = UTIL_EntitiesInBox(pList, 10, mins, maxs, (FL_CLIENT | FL_MONSTER));
	if (0 != count)
	{
		for (int i = 0; i < count; i++)
		{
			// only clients and monsters
			if (pList[i] != this && IRelationship(pList[i]) > R_NO && pList[i]->pev->deadflag == DEAD_NO) // this ent is one of our enemies. Barnacle tries to eat it.
			{
				return pList[i];
			}
		}
	}

	return nullptr;
}
