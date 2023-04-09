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
#include "apache.h"

LINK_ENTITY_TO_CLASS(monster_apache, CApache);

BEGIN_DATAMAP(CApache)
DEFINE_FIELD(m_iRockets, FIELD_INTEGER),
	DEFINE_FIELD(m_flForce, FIELD_FLOAT),
	DEFINE_FIELD(m_flNextRocket, FIELD_TIME),
	DEFINE_FIELD(m_vecTarget, FIELD_VECTOR),
	DEFINE_FIELD(m_posTarget, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_vecDesired, FIELD_VECTOR),
	DEFINE_FIELD(m_posDesired, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_vecGoal, FIELD_VECTOR),
	DEFINE_FIELD(m_angGun, FIELD_VECTOR),
	DEFINE_FIELD(m_flLastSeen, FIELD_TIME),
	DEFINE_FIELD(m_flPrevSeen, FIELD_TIME),
	//	DEFINE_FIELD(m_iSoundState, FIELD_INTEGER),		// Don't save, precached
	//	DEFINE_FIELD(m_iSpriteTexture, FIELD_INTEGER),
	//	DEFINE_FIELD(m_iExplode, FIELD_INTEGER),
	//	DEFINE_FIELD(m_iBodyGibs, FIELD_INTEGER),
	DEFINE_FIELD(m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(m_flGoalSpeed, FIELD_FLOAT),
	DEFINE_FIELD(m_iDoSmokePuff, FIELD_INTEGER),
	DEFINE_FUNCTION(HuntThink),
	DEFINE_FUNCTION(FlyTouch),
	DEFINE_FUNCTION(CrashTouch),
	DEFINE_FUNCTION(DyingThink),
	DEFINE_FUNCTION(StartupUse),
	DEFINE_FUNCTION(NullThink),
	END_DATAMAP();

void CApache::OnCreate()
{
	CBaseMonster::OnCreate();

	pev->health = GetSkillFloat("apache_health"sv);
	pev->model = MAKE_STRING("models/apache.mdl");
}

void CApache::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SetModel(STRING(pev->model));
	SetSize(Vector(-32, -32, -64), Vector(32, 32, 0));
	SetOrigin(pev->origin);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->max_health = pev->health;

	m_flFieldOfView = -0.707; // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG(0, 0xFF);

	InitBoneControllers();

	if ((pev->spawnflags & SF_WAITFORTRIGGER) != 0)
	{
		SetThink(&CApache::NullThink);
		SetUse(&CApache::StartupUse);
	}
	else
	{
		SetThink(&CApache::HuntThink);
		SetTouch(&CApache::FlyTouch);
	}

	pev->nextthink = gpGlobals->time + 1.0;

	m_iRockets = 10;
}

void CApache::Precache()
{
	PrecacheModel(STRING(pev->model));

	PrecacheSound("apache/ap_rotor1.wav");
	PrecacheSound("apache/ap_rotor2.wav");
	PrecacheSound("apache/ap_rotor3.wav");
	PrecacheSound("apache/ap_whine1.wav");

	PrecacheSound("weapons/mortarhit.wav");

	m_iSpriteTexture = PrecacheModel("sprites/white.spr");

	PrecacheSound("turret/tu_fire1.wav");

	PrecacheModel("sprites/lgtning.spr");

	m_iExplode = PrecacheModel("sprites/fexplo.spr");
	m_iBodyGibs = PrecacheModel("models/metalplategibs_green.mdl");

	UTIL_PrecacheOther("hvr_rocket");
}

void CApache::NullThink()
{
	StudioFrameAdvance();
	FCheckAITrigger();
	pev->nextthink = gpGlobals->time + 0.5;
}

void CApache::StartupUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&CApache::HuntThink);
	SetTouch(&CApache::FlyTouch);
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse(nullptr);
}

void CApache::Killed(CBaseEntity* attacker, int iGib)
{
	ClearShockEffect();

	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;

	StopSound(CHAN_STATIC, "apache/ap_rotor2.wav");

	SetSize(Vector(-32, -32, -64), Vector(32, 32, 0));
	SetThink(&CApache::DyingThink);
	SetTouch(&CApache::CrashTouch);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DYING;

	if ((pev->spawnflags & SF_NOWRECKAGE) != 0)
	{
		m_flNextRocket = gpGlobals->time + 4.0;
	}
	else
	{
		m_flNextRocket = gpGlobals->time + 15.0;
	}
}

void CApache::DyingThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	pev->avelocity = pev->avelocity * 1.02;

	// still falling?
	if (m_flNextRocket > gpGlobals->time)
	{
		FCheckAITrigger();

		// random explosions
		// This just makes a dynamic light now
		Vector explosionOrigin = pev->origin;

		explosionOrigin.x += RANDOM_FLOAT(-150, 150);
		explosionOrigin.y += RANDOM_FLOAT(-150, 150);
		explosionOrigin.z += RANDOM_FLOAT(-150, -50);

		UTIL_ExplosionEffect(explosionOrigin, g_sModelIndexFireball,
			RANDOM_LONG(0, 29) + 30, 12, TE_EXPLFLAG_NONE,
			MSG_PVS, pev->origin);

		// lots of smoke
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-150, 150));
		WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-150, 150));
		WRITE_COORD(pev->origin.z + RANDOM_FLOAT(-150, -50));
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(100); // scale * 10
		WRITE_BYTE(10);	 // framerate
		MESSAGE_END();

		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpot);
		WRITE_BYTE(TE_BREAKMODEL);

		// position
		WRITE_COORD(vecSpot.x);
		WRITE_COORD(vecSpot.y);
		WRITE_COORD(vecSpot.z);

		// size
		WRITE_COORD(400);
		WRITE_COORD(400);
		WRITE_COORD(132);

		// velocity
		WRITE_COORD(pev->velocity.x);
		WRITE_COORD(pev->velocity.y);
		WRITE_COORD(pev->velocity.z);

		// randomization
		WRITE_BYTE(50);

		// Model
		WRITE_SHORT(m_iBodyGibs); // model id#

		// # of shards
		WRITE_BYTE(4); // let client decide

		// duration
		WRITE_BYTE(30); // 3.0 seconds

		// flags

		WRITE_BYTE(BREAK_METAL);
		MESSAGE_END();

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	else
	{
		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

		/*
		// This just makes a dynamic light now
		UTIL_ExplosionEffect(vecSpot + Vector(0, 0, 300), g_sModelIndexFireball,
			250, 10, TE_EXPLFLAG_NONE,
			MSG_BROADCAST);
		*/

		// fireball
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpot);
		WRITE_BYTE(TE_SPRITE);
		WRITE_COORD(vecSpot.x);
		WRITE_COORD(vecSpot.y);
		WRITE_COORD(vecSpot.z + 256);
		WRITE_SHORT(m_iExplode);
		WRITE_BYTE(120); // scale * 10
		WRITE_BYTE(255); // brightness
		MESSAGE_END();

		// big smoke
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpot);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(vecSpot.x);
		WRITE_COORD(vecSpot.y);
		WRITE_COORD(vecSpot.z + 512);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(250); // scale * 10
		WRITE_BYTE(5);	 // framerate
		MESSAGE_END();

		// blast circle
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_BEAMCYLINDER);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 2000); // reach damage radius over .2 seconds
		WRITE_SHORT(m_iSpriteTexture);
		WRITE_BYTE(0);	 // startframe
		WRITE_BYTE(0);	 // framerate
		WRITE_BYTE(4);	 // life
		WRITE_BYTE(32);	 // width
		WRITE_BYTE(0);	 // noise
		WRITE_BYTE(255); // r, g, b
		WRITE_BYTE(255); // r, g, b
		WRITE_BYTE(192); // r, g, b
		WRITE_BYTE(128); // brightness
		WRITE_BYTE(0);	 // speed
		MESSAGE_END();

		EmitSound(CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

		RadiusDamage(pev->origin, this, this, 300, CLASS_NONE, DMG_BLAST);

		if (/*(pev->spawnflags & SF_NOWRECKAGE) == 0 && */ (pev->flags & FL_ONGROUND) != 0)
		{
			CBaseEntity* pWreckage = Create("cycler_wreckage", pev->origin, pev->angles);
			// pWreckage->SetModel(STRING(pev->model));
			pWreckage->SetSize(Vector(-200, -200, -128), Vector(200, 200, -32));
			pWreckage->pev->frame = pev->frame;
			pWreckage->pev->sequence = pev->sequence;
			pWreckage->pev->framerate = 0;
			pWreckage->pev->dmgtime = gpGlobals->time + 5;
		}

		// gibs
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpot);
		WRITE_BYTE(TE_BREAKMODEL);

		// position
		WRITE_COORD(vecSpot.x);
		WRITE_COORD(vecSpot.y);
		WRITE_COORD(vecSpot.z + 64);

		// size
		WRITE_COORD(400);
		WRITE_COORD(400);
		WRITE_COORD(128);

		// velocity
		WRITE_COORD(0);
		WRITE_COORD(0);
		WRITE_COORD(200);

		// randomization
		WRITE_BYTE(30);

		// Model
		WRITE_SHORT(m_iBodyGibs); // model id#

		// # of shards
		WRITE_BYTE(200);

		// duration
		WRITE_BYTE(200); // 10.0 seconds

		// flags

		WRITE_BYTE(BREAK_METAL);
		MESSAGE_END();

		SetThink(&CApache::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CApache::FlyTouch(CBaseEntity* pOther)
{
	// bounce if we hit something solid
	if (pOther->pev->solid == SOLID_BSP)
	{
		TraceResult tr = UTIL_GetGlobalTrace();

		// UNDONE, do a real bounce
		pev->velocity = pev->velocity + tr.vecPlaneNormal * (pev->velocity.Length() + 200);
	}
}

void CApache::CrashTouch(CBaseEntity* pOther)
{
	// only crash if we hit something solid
	if (pOther->pev->solid == SOLID_BSP)
	{
		SetTouch(nullptr);
		m_flNextRocket = gpGlobals->time;
		pev->nextthink = gpGlobals->time;
	}
}

void CApache::GibMonster()
{
	// EmitSoundDyn(CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);
}

void CApache::HuntThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	UpdateShockEffect();

	ShowDamage();

	if (m_pGoalEnt == nullptr && !FStringNull(pev->target)) // this monster has a target
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));
		if (m_pGoalEnt)
		{
			m_posDesired = m_pGoalEnt->pev->origin;
			UTIL_MakeAimVectors(m_pGoalEnt->pev->angles);
			m_vecGoal = gpGlobals->v_forward;
		}
	}

	// if (m_hEnemy == nullptr)
	{
		Look(4092);
		m_hEnemy = BestVisibleEnemy();

		// If i have an enemy i'm in combat, otherwise i'm patrolling.
		if (m_hEnemy != nullptr)
		{
			m_MonsterState = MONSTERSTATE_COMBAT;
		}
		else
		{
			m_MonsterState = MONSTERSTATE_ALERT;
		}
	}

	Listen();

	// generic speed up
	if (m_flGoalSpeed < 800)
		m_flGoalSpeed += 5;

	if (m_hEnemy != nullptr)
	{
		// AILogger->debug("{}", STRING(m_hEnemy->pev->classname));
		if (FVisible(m_hEnemy))
		{
			if (m_flLastSeen < gpGlobals->time - 5)
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center();
		}
		else
		{
			m_hEnemy = nullptr;
		}
	}

	m_vecTarget = (m_posTarget - pev->origin).Normalize();

	float flLength = (pev->origin - m_posDesired).Length();

	if (m_pGoalEnt)
	{
		// AILogger->debug("{:.0f}", flLength);

		if (flLength < 128)
		{
			m_pGoalEnt = UTIL_FindEntityByTargetname(nullptr, STRING(m_pGoalEnt->pev->target));
			if (m_pGoalEnt)
			{
				m_posDesired = m_pGoalEnt->pev->origin;
				UTIL_MakeAimVectors(m_pGoalEnt->pev->angles);
				m_vecGoal = gpGlobals->v_forward;
				flLength = (pev->origin - m_posDesired).Length();
			}
		}
	}
	else
	{
		m_posDesired = pev->origin;
	}

	if (flLength > 250) // 500
	{
		// float flLength2 = (m_posTarget - pev->origin).Length() * (1.5 - DotProduct((m_posTarget - pev->origin).Normalize(), pev->velocity.Normalize() ));
		// if (flLength2 < flLength)
		if (m_flLastSeen + 90 > gpGlobals->time && DotProduct((m_posTarget - pev->origin).Normalize(), (m_posDesired - pev->origin).Normalize()) > 0.25)
		{
			m_vecDesired = (m_posTarget - pev->origin).Normalize();
		}
		else
		{
			m_vecDesired = (m_posDesired - pev->origin).Normalize();
		}
	}
	else
	{
		m_vecDesired = m_vecGoal;
	}

	Flight();

	// AILogger->debug("{:.0f} {:.0f} {:.0f}", gpGlobals->time, m_flLastSeen, m_flPrevSeen);
	if ((m_flLastSeen + 1 > gpGlobals->time) && (m_flPrevSeen + 2 < gpGlobals->time))
	{
		if (FireGun())
		{
			// slow down if we're fireing
			if (m_flGoalSpeed > 400)
				m_flGoalSpeed = 400;
		}

		// don't fire rockets and gun on easy mode
		if (g_Skill.GetSkillLevel() == SkillLevel::Easy)
			m_flNextRocket = gpGlobals->time + 10.0;
	}

	UTIL_MakeAimVectors(pev->angles);
	Vector vecEst = (gpGlobals->v_forward * 800 + pev->velocity).Normalize();
	// AILogger->debug("{} {} {} {:4.2f}", pev->angles.x < 0, DotProduct(pev->velocity, gpGlobals->v_forward) > -100, m_flNextRocket < gpGlobals->time, DotProduct(m_vecTarget, vecEst));

	if ((m_iRockets % 2) == 1)
	{
		FireRocket();
		m_flNextRocket = gpGlobals->time + 0.5;
		if (m_iRockets <= 0)
		{
			m_flNextRocket = gpGlobals->time + 10;
			m_iRockets = 10;
		}
	}
	else if (pev->angles.x < 0 && DotProduct(pev->velocity, gpGlobals->v_forward) > -100 && m_flNextRocket < gpGlobals->time)
	{
		if (m_flLastSeen + 60 > gpGlobals->time)
		{
			if (m_hEnemy != nullptr)
			{
				// make sure it's a good shot
				if (DotProduct(m_vecTarget, vecEst) > .965)
				{
					TraceResult tr;

					UTIL_TraceLine(pev->origin, pev->origin + vecEst * 4096, ignore_monsters, edict(), &tr);
					if ((tr.vecEndPos - m_posTarget).Length() < 512)
						FireRocket();
				}
			}
			else
			{
				TraceResult tr;

				UTIL_TraceLine(pev->origin, pev->origin + vecEst * 4096, dont_ignore_monsters, edict(), &tr);
				// just fire when close
				if ((tr.vecEndPos - m_posTarget).Length() < 512)
					FireRocket();
			}
		}
	}

	FCheckAITrigger();
}

void CApache::Flight()
{
	// tilt model 5 degrees
	Vector vecAdj = Vector(5.0, 0, 0);

	// estimate where I'll be facing in one seconds
	UTIL_MakeAimVectors(pev->angles + pev->avelocity * 2 + vecAdj);
	// Vector vecEst1 = pev->origin + pev->velocity + gpGlobals->v_up * m_flForce - Vector( 0, 0, 384 );
	// float flSide = DotProduct( m_posDesired - vecEst1, gpGlobals->v_right );

	float flSide = DotProduct(m_vecDesired, gpGlobals->v_right);

	if (flSide < 0)
	{
		if (pev->avelocity.y < 60)
		{
			pev->avelocity.y += 8; // 9 * (3.0/2.0);
		}
	}
	else
	{
		if (pev->avelocity.y > -60)
		{
			pev->avelocity.y -= 8; // 9 * (3.0/2.0);
		}
	}
	pev->avelocity.y *= 0.98;

	// estimate where I'll be in two seconds
	UTIL_MakeAimVectors(pev->angles + pev->avelocity * 1 + vecAdj);
	Vector vecEst = pev->origin + pev->velocity * 2.0 + gpGlobals->v_up * m_flForce * 20 - Vector(0, 0, 384 * 2);

	// add immediate force
	UTIL_MakeAimVectors(pev->angles + vecAdj);
	pev->velocity.x += gpGlobals->v_up.x * m_flForce;
	pev->velocity.y += gpGlobals->v_up.y * m_flForce;
	pev->velocity.z += gpGlobals->v_up.z * m_flForce;
	// add gravity
	pev->velocity.z -= 38.4; // 32ft/sec


	float flSpeed = pev->velocity.Length();
	float flDir = DotProduct(Vector(gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0), Vector(pev->velocity.x, pev->velocity.y, 0));
	if (flDir < 0)
		flSpeed = -flSpeed;

	float flDist = DotProduct(m_posDesired - vecEst, gpGlobals->v_forward);

	// float flSlip = DotProduct( pev->velocity, gpGlobals->v_right );
	float flSlip = -DotProduct(m_posDesired - vecEst, gpGlobals->v_right);

	// fly sideways
	if (flSlip > 0)
	{
		if (pev->angles.z > -30 && pev->avelocity.z > -15)
			pev->avelocity.z -= 4;
		else
			pev->avelocity.z += 2;
	}
	else
	{

		if (pev->angles.z < 30 && pev->avelocity.z < 15)
			pev->avelocity.z += 4;
		else
			pev->avelocity.z -= 2;
	}

	// sideways drag
	pev->velocity.x = pev->velocity.x * (1.0 - fabs(gpGlobals->v_right.x) * 0.05);
	pev->velocity.y = pev->velocity.y * (1.0 - fabs(gpGlobals->v_right.y) * 0.05);
	pev->velocity.z = pev->velocity.z * (1.0 - fabs(gpGlobals->v_right.z) * 0.05);

	// general drag
	pev->velocity = pev->velocity * 0.995;

	// apply power to stay correct height
	if (m_flForce < 80 && vecEst.z < m_posDesired.z)
	{
		m_flForce += 12;
	}
	else if (m_flForce > 30)
	{
		if (vecEst.z > m_posDesired.z)
			m_flForce -= 8;
	}

	// pitch forward or back to get to target
	if (flDist > 0 && flSpeed < m_flGoalSpeed /* && flSpeed < flDist */ && pev->angles.x + pev->avelocity.x > -40)
	{
		// AILogger->debug("F ");
		// lean forward
		pev->avelocity.x -= 12.0;
	}
	else if (flDist < 0 && flSpeed > -50 && pev->angles.x + pev->avelocity.x < 20)
	{
		// AILogger->debug("B ");
		// lean backward
		pev->avelocity.x += 12.0;
	}
	else if (pev->angles.x + pev->avelocity.x > 0)
	{
		// AILogger->debug("f ");
		pev->avelocity.x -= 4.0;
	}
	else if (pev->angles.x + pev->avelocity.x < 0)
	{
		// AILogger->debug("b ");
		pev->avelocity.x += 4.0;
	}

	// AILogger->debug("{:.0f} {:.0f} : {:.0f} {:.0f} : {:.0f} {:.0f} : {:.0f}", pev->origin.x, pev->velocity.x, flDist, flSpeed, pev->angles.x, pev->avelocity.x, m_flForce);
	// AILogger->debug("{:.0f} {:.0f} : {:.0f} {:0.f} : {:.0f}", pev->origin.z, pev->velocity.z, vecEst.z, m_posDesired.z, m_flForce);

	// make rotor, engine sounds
	if (m_iSoundState == 0)
	{
		EmitSoundDyn(CHAN_STATIC, "apache/ap_rotor2.wav", 1.0, 0.3, 0, 110);
		// EmitSoundDyn(CHAN_STATIC, "apache/ap_whine1.wav", 0.5, 0.2, 0, 110 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions
	}
	else
	{
		CBaseEntity* pPlayer = nullptr;

		pPlayer = UTIL_GetLocalPlayer();
		// UNDONE: this needs to send different sounds to every player for multiplayer.
		if (pPlayer)
		{

			float pitch = DotProduct(pev->velocity - pPlayer->pev->velocity, (pPlayer->pev->origin - pev->origin).Normalize());

			pitch = (int)(100 + pitch / 50.0);

			if (pitch > 250)
				pitch = 250;
			if (pitch < 50)
				pitch = 50;
			if (pitch == 100)
				pitch = 101;

			float flVol = (m_flForce / 100.0) + .1;
			if (flVol > 1.0)
				flVol = 1.0;

			EmitSoundDyn(CHAN_STATIC, "apache/ap_rotor2.wav", 1.0, 0.3, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
		}
		// EmitSoundDyn(CHAN_STATIC, "apache/ap_whine1.wav", flVol, 0.2, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);

		// AILogger->debug("{:.0f} {:.2f}", pitch, flVol);
	}
}

void CApache::FireRocket()
{
	static float side = 1.0;

	if (m_iRockets <= 0)
		return;

	UTIL_MakeAimVectors(pev->angles);
	Vector vecSrc = pev->origin + 1.5 * (gpGlobals->v_forward * 21 + gpGlobals->v_right * 70 * side + gpGlobals->v_up * -79);

	switch (m_iRockets % 5)
	{
	case 0:
		vecSrc = vecSrc + gpGlobals->v_right * 10;
		break;
	case 1:
		vecSrc = vecSrc - gpGlobals->v_right * 10;
		break;
	case 2:
		vecSrc = vecSrc + gpGlobals->v_up * 10;
		break;
	case 3:
		vecSrc = vecSrc - gpGlobals->v_up * 10;
		break;
	case 4:
		break;
	}

	CBaseEntity* pRocket = CBaseEntity::Create("hvr_rocket", vecSrc, pev->angles, this);
	if (pRocket)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(vecSrc.x);
		WRITE_COORD(vecSrc.y);
		WRITE_COORD(vecSrc.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(20); // scale * 10
		WRITE_BYTE(12); // framerate
		MESSAGE_END();

		pRocket->pev->velocity = pev->velocity + gpGlobals->v_forward * 100;

		m_iRockets--;

		side = -side;
	}
}

bool CApache::FireGun()
{
	UTIL_MakeAimVectors(pev->angles);

	Vector posGun, angGun;
	GetAttachment(1, posGun, angGun);

	Vector vecTarget = (m_posTarget - posGun).Normalize();

	Vector vecOut;

	vecOut.x = DotProduct(gpGlobals->v_forward, vecTarget);
	vecOut.y = -DotProduct(gpGlobals->v_right, vecTarget);
	vecOut.z = DotProduct(gpGlobals->v_up, vecTarget);

	Vector angles = UTIL_VecToAngles(vecOut);

	angles.x = -angles.x;
	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	if (angles.x > m_angGun.x)
		m_angGun.x = std::min(angles.x, m_angGun.x + 12);
	if (angles.x < m_angGun.x)
		m_angGun.x = std::max(angles.x, m_angGun.x - 12);
	if (angles.y > m_angGun.y)
		m_angGun.y = std::min(angles.y, m_angGun.y + 12);
	if (angles.y < m_angGun.y)
		m_angGun.y = std::max(angles.y, m_angGun.y - 12);

	m_angGun.y = SetBoneController(0, m_angGun.y);
	m_angGun.x = SetBoneController(1, m_angGun.x);

	Vector posBarrel, angBarrel;
	GetAttachment(0, posBarrel, angBarrel);
	Vector vecGun = (posBarrel - posGun).Normalize();

	if (DotProduct(vecGun, vecTarget) > 0.98)
	{
#if 1
		FireBullets(1, posGun, vecGun, VECTOR_CONE_4DEGREES, 8192, BULLET_MONSTER_12MM, 1);
		EmitSound(CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.3);
#else
		static float flNext;
		TraceResult tr;
		UTIL_TraceLine(posGun, posGun + vecGun * 8192, dont_ignore_monsters, edict(), &tr);

		if (!m_pBeam)
		{
			m_pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 80);
			m_pBeam->PointEntInit(pev->origin, entindex());
			m_pBeam->SetEndAttachment(1);
			m_pBeam->SetColor(255, 180, 96);
			m_pBeam->SetBrightness(192);
		}

		if (flNext < gpGlobals->time)
		{
			flNext = gpGlobals->time + 0.5;
			m_pBeam->SetStartPos(tr.vecEndPos);
		}
#endif
		return true;
	}
	else
	{
		if (m_pBeam)
		{
			UTIL_Remove(m_pBeam);
			m_pBeam = nullptr;
		}
	}
	return false;
}

void CApache::ShowDamage()
{
	if (m_iDoSmokePuff > 0 || RANDOM_LONG(0, 99) > pev->health)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z - 32);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(RANDOM_LONG(0, 9) + 20); // scale * 10
		WRITE_BYTE(12);						// framerate
		MESSAGE_END();
	}
	if (m_iDoSmokePuff > 0)
		m_iDoSmokePuff--;
}

bool CApache::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	if (inflictor->pev->owner == edict())
		return false;

	if ((bitsDamageType & DMG_BLAST) != 0)
	{
		flDamage *= 2;
	}

	/*
	if ( (bitsDamageType & DMG_BULLET) != 0 && flDamage > 50)
	{
		// clip bullet damage at 50
		flDamage = 50;
	}
	*/

	// AILogger->debug("{:.0f}", flDamage);
	const bool result = CBaseEntity::TakeDamage(inflictor, attacker, flDamage, bitsDamageType);

	// Are we damaged at all?
	if (pev->health < pev->max_health)
	{
		// Took some damage.
		SetConditions(bits_COND_LIGHT_DAMAGE);

		if (pev->health < (pev->max_health / 2))
		{
			// Seriously damaged now.
			SetConditions(bits_COND_HEAVY_DAMAGE);
		}
		else
		{
			// Maybe somebody healed us somehow (trigger_hurt with negative damage?), clear this.
			ClearConditions(bits_COND_HEAVY_DAMAGE);
		}
	}
	else
	{
		// Maybe somebody healed us somehow (trigger_hurt with negative damage?), clear this.
		ClearConditions(bits_COND_LIGHT_DAMAGE);
	}

	return result;
}

void CApache::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// AILogger->debug("{} {:.0f}", ptr->iHitgroup, flDamage);

	// ignore blades
	if (ptr->iHitgroup == 6 && (bitsDamageType & (DMG_ENERGYBEAM | DMG_BULLET | DMG_CLUB)) != 0)
		return;

	// hit hard, hits cockpit, hits engines
	if (flDamage > 50 || ptr->iHitgroup == 1 || ptr->iHitgroup == 2)
	{
		// AILogger->debug("{:.0f}", flDamage);
		AddMultiDamage(attacker, this, flDamage, bitsDamageType);
		m_iDoSmokePuff = 3 + (flDamage / 5.0);
	}
	else
	{
		// do half damage in the body
		// AddMultiDamage( attacker, this, flDamage / 2.0, bitsDamageType );
		UTIL_Ricochet(ptr->vecEndPos, 2.0);
	}
}

class CApacheHVR : public CGrenade
{
	DECLARE_CLASS(CApacheHVR, CGrenade);
	DECLARE_DATAMAP();

public:
	void Spawn() override;
	void Precache() override;
	void IgniteThink();
	void AccelerateThink();

	int m_iTrail;
	Vector m_vecForward;
};

LINK_ENTITY_TO_CLASS(hvr_rocket, CApacheHVR);

BEGIN_DATAMAP(CApacheHVR)
//	DEFINE_FIELD(m_iTrail, FIELD_INTEGER),	// Dont' save, precache
DEFINE_FIELD(m_vecForward, FIELD_VECTOR),
	DEFINE_FUNCTION(IgniteThink),
	DEFINE_FUNCTION(AccelerateThink),
	END_DATAMAP();

void CApacheHVR::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SetModel("models/HVR.mdl");
	SetSize(Vector(0, 0, 0), Vector(0, 0, 0));
	SetOrigin(pev->origin);

	SetThink(&CApacheHVR::IgniteThink);
	SetTouch(&CApacheHVR::ExplodeTouch);

	UTIL_MakeAimVectors(pev->angles);
	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.1;

	pev->dmg = 150;
}

void CApacheHVR::Precache()
{
	PrecacheModel("models/HVR.mdl");
	m_iTrail = PrecacheModel("sprites/smoke.spr");
	PrecacheSound("weapons/rocket1.wav");
}

void CApacheHVR::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	// pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;

	// make rocket sound
	EmitSound(CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5);

	// rocket trail
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex()); // entity
	WRITE_SHORT(m_iTrail);	 // model
	WRITE_BYTE(15);			 // life
	WRITE_BYTE(5);			 // width
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(224);		 // r, g, b
	WRITE_BYTE(255);		 // r, g, b
	WRITE_BYTE(255);		 // brightness

	MESSAGE_END(); // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	// set to accelerate
	SetThink(&CApacheHVR::AccelerateThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CApacheHVR::AccelerateThink()
{
	// check world boundaries
	if (pev->origin.x < -4096 || pev->origin.x > 4096 || pev->origin.y < -4096 || pev->origin.y > 4096 || pev->origin.z < -4096 || pev->origin.z > 4096)
	{
		UTIL_Remove(this);
		return;
	}

	// accelerate
	float flSpeed = pev->velocity.Length();
	if (flSpeed < 1800)
	{
		pev->velocity = pev->velocity + m_vecForward * 200;
	}

	// re-aim
	pev->angles = UTIL_VecToAngles(pev->velocity);

	pev->nextthink = gpGlobals->time + 0.1;
}
