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
 *	functions dealing with damage infliction & death
 */

#include <EASTL/fixed_vector.h>

#include "cbase.h"
#include "func_break.h"
#include "UserMessages.h"

BEGIN_DATAMAP(CGib)
DEFINE_FUNCTION(BounceGibTouch),
	DEFINE_FUNCTION(StickyGibTouch),
	DEFINE_FUNCTION(WaitTillLand),
	END_DATAMAP();

LINK_ENTITY_TO_CLASS(gib, CGib);

// HACKHACK -- The gib velocity equations don't work
void CGib::LimitVelocity()
{
	float length = pev->velocity.Length();

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if (length > 1500.0)
		pev->velocity = pev->velocity.Normalize() * 1500; // This should really be sv_maxvelocity * 0.75 or something
}

void CGib::SpawnStickyGibs(CBaseEntity* victim, Vector vecOrigin, int cGibs)
{
	int i;

	if (g_Language == LANGUAGE_GERMAN)
	{
		// no sticky gibs in germany right now!
		return;
	}

	for (i = 0; i < cGibs; i++)
	{
		CGib* pGib = g_EntityDictionary->Create<CGib>("gib");

		pGib->Spawn("models/stickygib.mdl");
		pGib->pev->body = RANDOM_LONG(0, 2);

		if (victim)
		{
			pGib->pev->origin.x = vecOrigin.x + RANDOM_FLOAT(-3, 3);
			pGib->pev->origin.y = vecOrigin.y + RANDOM_FLOAT(-3, 3);
			pGib->pev->origin.z = vecOrigin.z + RANDOM_FLOAT(-3, 3);

			/*
			pGib->pev->origin.x = victim->pev->absmin.x + victim->pev->size.x * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.y = victim->pev->absmin.y + victim->pev->size.y * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.z = victim->pev->absmin.z + victim->pev->size.z * (RANDOM_FLOAT ( 0 , 1 ) );
			*/

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT(-0.15, 0.15);
			pGib->pev->velocity.y += RANDOM_FLOAT(-0.15, 0.15);
			pGib->pev->velocity.z += RANDOM_FLOAT(-0.15, 0.15);

			pGib->pev->velocity = pGib->pev->velocity * 900;

			pGib->pev->avelocity.x = RANDOM_FLOAT(250, 400);
			pGib->pev->avelocity.y = RANDOM_FLOAT(250, 400);

			// copy owner's blood color
			pGib->m_bloodColor = victim->BloodColor();

			if (victim->pev->health > -50)
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			}
			else if (victim->pev->health > -200)
			{
				pGib->pev->velocity = pGib->pev->velocity * 2;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4;
			}


			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			pGib->SetSize(Vector(0, 0, 0), Vector(0, 0, 0));
			pGib->SetTouch(&CGib::StickyGibTouch);
			pGib->SetThink(nullptr);
		}
		pGib->LimitVelocity();
	}
}

void CGib::SpawnHeadGib(CBaseEntity* victim)
{
	CGib* pGib = g_EntityDictionary->Create<CGib>("gib");

	if (g_Language == LANGUAGE_GERMAN)
	{
		pGib->Spawn("models/germangibs.mdl"); // throw one head
		pGib->pev->body = 0;
	}
	else
	{
		pGib->Spawn("models/hgibs.mdl"); // throw one head
		pGib->pev->body = 0;
	}

	if (victim)
	{
		pGib->pev->origin = victim->pev->origin + victim->pev->view_ofs;

		CBasePlayer* player = UTIL_FindClientInPVS(pGib);

		if (RANDOM_LONG(0, 100) <= 5 && player)
		{
			// 5% chance head will be thrown at player's face.
			pGib->pev->velocity = ((player->pev->origin + player->pev->view_ofs) - pGib->pev->origin).Normalize() * 300;
			pGib->pev->velocity.z += 100;
		}
		else
		{
			pGib->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
		}


		pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
		pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);

		// copy owner's blood color
		pGib->m_bloodColor = victim->BloodColor();

		if (victim->pev->health > -50)
		{
			pGib->pev->velocity = pGib->pev->velocity * 0.7;
		}
		else if (victim->pev->health > -200)
		{
			pGib->pev->velocity = pGib->pev->velocity * 2;
		}
		else
		{
			pGib->pev->velocity = pGib->pev->velocity * 4;
		}
	}
	pGib->LimitVelocity();
}

void CGib::SpawnRandomGibs(CBaseEntity* victim, int cGibs, const GibData& gibData)
{
	// Track the number of uses of a particular submodel so we can avoid spawning too many of the same submodel
	eastl::fixed_vector<int, 64> limitTracking;

	if (gibData.Limits)
	{
		limitTracking.resize(gibData.SubModelCount);
	}

	int currentBody = 0;

	for (int cSplat = 0; cSplat < cGibs; cSplat++)
	{
		CGib* pGib = g_EntityDictionary->Create<CGib>("gib");

		if (g_Language == LANGUAGE_GERMAN)
		{
			pGib->Spawn("models/germangibs.mdl");
			pGib->pev->body = RANDOM_LONG(0, GERMAN_GIB_COUNT - 1);
		}
		else
		{
			pGib->Spawn(gibData.ModelName);

			if (gibData.Limits)
			{
				if (limitTracking[currentBody] >= gibData.Limits[currentBody].MaxGibs)
				{
					++currentBody;

					// We've hit the limit, stop spawning gibs.
					// This should only happen if calling code told us to spawn more gibs than the provided submodel limits allow for.
					if (currentBody >= gibData.SubModelCount)
					{
						Logger->warn("Too many gibs spawned; gib submodel limit reached (code configuration issue)");
						UTIL_Remove(pGib);
						return;
					}
				}

				pGib->pev->body = currentBody;

				++limitTracking[currentBody];
			}
			else
			{
				pGib->pev->body = RANDOM_LONG(gibData.FirstSubModel, gibData.SubModelCount - 1);
			}
		}

		if (victim)
		{
			// spawn the gib somewhere in the monster's bounding volume
			pGib->pev->origin.x = victim->pev->absmin.x + victim->pev->size.x * (RANDOM_FLOAT(0, 1));
			pGib->pev->origin.y = victim->pev->absmin.y + victim->pev->size.y * (RANDOM_FLOAT(0, 1));
			pGib->pev->origin.z = victim->pev->absmin.z + victim->pev->size.z * (RANDOM_FLOAT(0, 1)) + 1; // absmin.z is in the floor because the engine subtracts 1 to enlarge the box

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT(-0.25, 0.25);
			pGib->pev->velocity.y += RANDOM_FLOAT(-0.25, 0.25);
			pGib->pev->velocity.z += RANDOM_FLOAT(-0.25, 0.25);

			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT(300, 400);

			pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
			pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);

			// copy owner's blood color
			pGib->m_bloodColor = victim->BloodColor();

			if (victim->pev->health > -50)
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			}
			else if (victim->pev->health > -200)
			{
				pGib->pev->velocity = pGib->pev->velocity * 2;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4;
			}

			pGib->pev->solid = SOLID_BBOX;
			pGib->SetSize(Vector(0, 0, 0), Vector(0, 0, 0));
		}
		pGib->LimitVelocity();
	}
}

void CGib::SpawnRandomGibs(CBaseEntity* victim, int cGibs, bool human)
{
	SpawnRandomGibs(victim, cGibs, human ? HumanGibs : AlienGibs);
}

static void SpawnClientGibsCore(CBaseEntity* victim, const GibType type, int cGibs, bool playSound, bool spawnHead)
{
	if (!victim)
	{
		return;
	}

	const Vector origin = victim->Center();
	const int bloodColor = victim->BloodColor();

	// If you bleed, you stink!
	if (bloodColor != DONT_BLEED)
	{
		// ok, start stinkin!
		CSoundEnt::InsertSound(bits_SOUND_MEAT, origin, 384, 25);
	}

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgClientGibs);
	WRITE_SHORT(victim->entindex());
	WRITE_BYTE(bloodColor);
	WRITE_BYTE(cGibs);
	WRITE_COORD_VECTOR(origin); // Spawn in center since we can't get bounding box on the client side.
	WRITE_COORD_VECTOR(g_vecAttackDir);

	const auto multiplier = [=]()
	{
		if (victim->pev->health > -50)
		{
			return GibVelocityMultiplier::Fraction;
		}
		else if (victim->pev->health > -200)
		{
			return GibVelocityMultiplier::Double;
		}

		return GibVelocityMultiplier::Quadruple;
	}();

	WRITE_BYTE(int(multiplier));

	int flags = int(type);

	if (playSound)
	{
		flags |= GibFlag_GibSound;
	}

	if (spawnHead)
	{
		flags |= GibFlag_SpawnHead;
	}

	WRITE_BYTE(flags);

	MESSAGE_END();
}

void CGib::SpawnClientGibs(CBaseEntity* victim, const GibType type, bool playSound, bool spawnHead)
{
	SpawnClientGibsCore(victim, type, 0, playSound, spawnHead);
}

void CGib::SpawnClientGibs(CBaseEntity* victim, const GibType type, int cGibs, bool playSound, bool spawnHead)
{
	SpawnClientGibsCore(victim, type, std::clamp(cGibs, 0, 255), playSound, spawnHead);
}

void CBaseMonster::FadeMonster()
{
	StopAnimation();
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->avelocity = g_vecZero;
	pev->animtime = gpGlobals->time;
	pev->effects |= EF_NOINTERP;
	SUB_StartFadeOut();
}

void CBaseMonster::GibMonster()
{
	// Most of this is client side now to save on server side entities, network overhead and possible crashes.
	bool gibbed = false;

	//EmitSound(CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);

	GibType type = GibType::None;
	bool spawnHead = false;

	// only humans throw skulls !!!UNDONE - eventually monsters will have their own sets of gibs
	if (HasHumanGibs())
	{
		/*
		if (CVAR_GET_FLOAT("violence_hgibs") != 0) // Only the player will ever get here
		{
			CGib::SpawnHeadGib(this);
			CGib::SpawnRandomGibs(this, 4, true); // throw some human gibs.
		}
		*/

		type = GibType::Human;
		spawnHead = true;
		gibbed = true;
	}
	else if (HasAlienGibs())
	{
		/*
		if (CVAR_GET_FLOAT("violence_agibs") != 0) // Should never get here, but someone might call it directly
		{
			CGib::SpawnRandomGibs(this, 4, false); // Throw alien gibs
		}
		*/

		type = GibType::Alien;
		gibbed = true;
	}

	CGib::SpawnClientGibs(this, type, true, spawnHead);

	if (!IsPlayer())
	{
		if (gibbed)
		{
			// don't remove players!
			SetThink(&CBaseMonster::SUB_Remove);
			pev->nextthink = gpGlobals->time;
		}
		else
		{
			FadeMonster();
		}
	}
}

Activity CBaseMonster::GetDeathActivity()
{
	Activity deathActivity;
	bool fTriedDirection;
	float flDot;
	TraceResult tr;
	Vector vecSrc;

	if (pev->deadflag != DEAD_NO)
	{
		// don't run this while dying.
		return m_IdealActivity;
	}

	vecSrc = Center();

	fTriedDirection = false;
	deathActivity = ACT_DIESIMPLE; // in case we can't find any special deaths to do.

	UTIL_MakeVectors(pev->angles);
	flDot = DotProduct(gpGlobals->v_forward, g_vecAttackDir * -1);

	switch (m_LastHitGroup)
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;

	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;

	case HITGROUP_GENERIC:
		// try to pick a death based on attack direction
		fTriedDirection = true;

		if (flDot > 0.3)
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if (flDot <= -0.3)
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;

	default:
		// try to pick a death based on attack direction
		fTriedDirection = true;

		if (flDot > 0.3)
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if (flDot <= -0.3)
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	}


	// can we perform the prescribed death?
	if (LookupActivity(deathActivity) == ACTIVITY_NOT_AVAILABLE)
	{
		// no! did we fail to perform a directional death?
		if (fTriedDirection)
		{
			// if yes, we're out of options. Go simple.
			deathActivity = ACT_DIESIMPLE;
		}
		else
		{
			// cannot perform the ideal region-specific death, so try a direction.
			if (flDot > 0.3)
			{
				deathActivity = ACT_DIEFORWARD;
			}
			else if (flDot <= -0.3)
			{
				deathActivity = ACT_DIEBACKWARD;
			}
		}
	}

	if (LookupActivity(deathActivity) == ACTIVITY_NOT_AVAILABLE)
	{
		// if we're still invalid, simple is our only option.
		deathActivity = ACT_DIESIMPLE;
	}

	if (deathActivity == ACT_DIEFORWARD)
	{
		// make sure there's room to fall forward
		UTIL_TraceHull(vecSrc, vecSrc + gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);

		if (tr.flFraction != 1.0)
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	if (deathActivity == ACT_DIEBACKWARD)
	{
		// make sure there's room to fall backward
		UTIL_TraceHull(vecSrc, vecSrc - gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);

		if (tr.flFraction != 1.0)
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	return deathActivity;
}

Activity CBaseMonster::GetSmallFlinchActivity()
{
	Activity flinchActivity;
	bool fTriedDirection;
	float flDot;

	fTriedDirection = false;
	UTIL_MakeVectors(pev->angles);
	flDot = DotProduct(gpGlobals->v_forward, g_vecAttackDir * -1);

	switch (m_LastHitGroup)
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchActivity = ACT_FLINCH_HEAD;
		break;
	case HITGROUP_STOMACH:
		flinchActivity = ACT_FLINCH_STOMACH;
		break;
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}


	// do we have a sequence for the ideal activity?
	if (LookupActivity(flinchActivity) == ACTIVITY_NOT_AVAILABLE)
	{
		flinchActivity = ACT_SMALL_FLINCH;
	}

	return flinchActivity;
}

void CBaseMonster::BecomeDead()
{
	pev->takedamage = DAMAGE_YES; // don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health.
	pev->health = pev->max_health / 2;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.

	// make the corpse fly away from the attack vector
	pev->movetype = MOVETYPE_TOSS;
	// pev->flags &= ~FL_ONGROUND;
	// pev->origin.z += 2;
	// pev->velocity = g_vecAttackDir * -1;
	// pev->velocity = pev->velocity * RANDOM_FLOAT( 300, 400 );
}

bool CBaseMonster::ShouldGibMonster(int iGib)
{
	if ((iGib == GIB_NORMAL && pev->health < GIB_HEALTH_VALUE) || (iGib == GIB_ALWAYS))
		return true;

	return false;
}

void CBaseMonster::CallGibMonster()
{
	bool fade = false;

	if (HasHumanGibs())
	{
		if (CVAR_GET_FLOAT("violence_hgibs") == 0)
			fade = true;
	}
	else if (HasAlienGibs())
	{
		if (CVAR_GET_FLOAT("violence_agibs") == 0)
			fade = true;
	}

	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT; // do something with the body. while monster blows up

	if (fade)
	{
		FadeMonster();
	}
	else
	{
		pev->effects = EF_NODRAW; // make the model invisible.
		GibMonster();
	}

	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	// don't let the status bar glitch for players.with <0 health.
	if (pev->health < -99)
	{
		pev->health = 0;
	}

	if (ShouldFadeOnDeath() && !fade)
		UTIL_Remove(this);
}

void CBaseMonster::Killed(CBaseEntity* attacker, int iGib)
{
	// If this NPC is using the follower use function, remove it to prevent players from using it.
	if (m_pfnUse == &CBaseMonster::FollowerUse)
	{
		SetUse(nullptr);
	}

	if (HasMemory(bits_MEMORY_KILLED))
	{
		if (ShouldGibMonster(iGib))
			CallGibMonster();
		return;
	}

	Remember(bits_MEMORY_KILLED);

	// clear the deceased's sound channels.(may have been firing or reloading when killed)
	EmitSound(CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM);
	m_IdealMonsterState = MONSTERSTATE_DEAD;
	// Make sure this condition is fired too (TakeDamage breaks out before this happens on death)
	SetConditions(bits_COND_LIGHT_DAMAGE);

	// tell owner ( if any ) that we're dead.This is mostly for MonsterMaker functionality.
	MaybeNotifyOwnerOfDeath();

	if (ShouldGibMonster(iGib))
	{
		CallGibMonster();
		return;
	}
	else if ((pev->flags & FL_MONSTER) != 0)
	{
		SetTouch(nullptr);
		BecomeDead();
	}

	// don't let the status bar glitch for players.with <0 health.
	if (pev->health < -99)
	{
		pev->health = 0;
	}

	// pev->enemy = ENT(attacker);//why? (sjb)

	m_IdealMonsterState = MONSTERSTATE_DEAD;

	ClearShockEffect();
}

void CBaseEntity::SUB_StartFadeOut()
{
	if (pev->rendermode == kRenderNormal)
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CBaseEntity::SUB_FadeOut);
}

void CBaseEntity::SUB_FadeOut()
{
	if (pev->renderamt > 7)
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2;
		SetThink(&CBaseEntity::SUB_Remove);
	}
}

void CGib::WaitTillLand()
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->velocity == g_vecZero)
	{
		SetThink(&CGib::SUB_StartFadeOut);
		pev->nextthink = gpGlobals->time + m_lifeTime;

		// If you bleed, you stink!
		if (m_bloodColor != DONT_BLEED)
		{
			// ok, start stinkin!
			CSoundEnt::InsertSound(bits_SOUND_MEAT, pev->origin, 384, 25);
		}
	}
	else
	{
		// wait and check again in another half second.
		pev->nextthink = gpGlobals->time + 0.5;
	}
}

void CGib::BounceGibTouch(CBaseEntity* pOther)
{
	Vector vecSpot;
	TraceResult tr;

	// if ( RANDOM_LONG(0,1) )
	//	return;// don't bleed everytime

	if ((pev->flags & FL_ONGROUND) != 0)
	{
		pev->velocity = pev->velocity * 0.9;
		pev->angles.x = 0;
		pev->angles.z = 0;
		pev->avelocity.x = 0;
		pev->avelocity.z = 0;
	}
	else
	{
		if (g_Language != LANGUAGE_GERMAN && m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED)
		{
			vecSpot = pev->origin + Vector(0, 0, 8); // move up a bit, and trace down.
			UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -24), ignore_monsters, edict(), &tr);

			UTIL_BloodDecalTrace(&tr, m_bloodColor);

			m_cBloodDecals--;
		}

		if (m_material != matNone && RANDOM_LONG(0, 2) == 0)
		{
			float volume;
			float zvel = fabs(pev->velocity.z);

			volume = 0.8 * std::min(1.0, ((float)zvel) / 450.0);

			CBreakable::MaterialSoundRandom(this, (Materials)m_material, volume);
		}
	}
}

void CGib::StickyGibTouch(CBaseEntity* pOther)
{
	TraceResult tr;

	SetThink(&CGib::SUB_Remove);
	pev->nextthink = gpGlobals->time + 10;

	if (!pOther->ClassnameIs("worldspawn"))
	{
		pev->nextthink = gpGlobals->time;
		return;
	}

	UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 32, ignore_monsters, edict(), &tr);

	UTIL_BloodDecalTrace(&tr, m_bloodColor);

	pev->velocity = tr.vecPlaneNormal * -1;
	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
}

void CGib::Spawn(const char* szGibModel)
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55; // deading the bounce a bit

	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_SLIDEBOX; /// hopefully this will fix the VELOCITY TOO LOW crap

	SetModel(szGibModel);
	SetSize(Vector(0, 0, 0), Vector(0, 0, 0));

	pev->nextthink = gpGlobals->time + 4;
	m_lifeTime = 25;
	SetThink(&CGib::WaitTillLand);
	SetTouch(&CGib::BounceGibTouch);

	m_material = matNone;
	m_cBloodDecals = 5; // how many blood decals this gib can place (1 per bounce until none remain).
}

bool CBaseMonster::GiveHealth(float flHealth, int bitsDamageType)
{
	if (0 == pev->takedamage)
		return false;

	// clear out any damage types we healed.
	// UNDONE: generic health should not heal any
	// UNDONE: time-based damage

	m_bitsDamageType &= ~(bitsDamageType & ~DMG_TIMEBASED);

	return CBaseEntity::GiveHealth(flHealth, bitsDamageType);
}

bool CBaseMonster::TakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	float flTake;
	Vector vecDir;

	if (0 == pev->takedamage)
		return false;

	if (!IsAlive())
	{
		return DeadTakeDamage(inflictor, attacker, flDamage, bitsDamageType);
	}

	if (pev->deadflag == DEAD_NO)
	{
		// no pain sound during death animation.
		PainSound(); // "Ouch!"
	}

	//!!!LATER - make armor consideration here!
	flTake = flDamage;

	// set damage type sustained
	m_bitsDamageType |= bitsDamageType;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector(0, 0, 0);
	if (!FNullEnt(inflictor))
	{
		if (inflictor)
		{
			vecDir = (inflictor->Center() - Vector(0, 0, 10) - Center()).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if (IsPlayer())
	{
		if (inflictor)
			pev->dmg_inflictor = inflictor->edict();

		pev->dmg_take += flTake;

		// check for godmode or invincibility
		if ((pev->flags & FL_GODMODE) != 0)
		{
			return false;
		}
	}

	// if this is a player, move him around!
	if ((!FNullEnt(inflictor)) && (pev->movetype == MOVETYPE_WALK) && (!attacker || attacker->pev->solid != SOLID_TRIGGER))
	{
		pev->velocity = pev->velocity + vecDir * -DamageForce(flDamage);
	}

	// do the damage
	pev->health -= flTake;

	if (IsUnkillable() && pev->health < 1)
	{
		// Adjust damage done to leave character barely alive.
		pev->health = 1;
	}

	// HACKHACK Don't kill monsters in a script.  Let them break their scripts first
	if (m_MonsterState == MONSTERSTATE_SCRIPT)
	{
		SetConditions(bits_COND_LIGHT_DAMAGE);
		return false;
	}

	if (pev->health <= 0)
	{
		g_pevLastInflictor = inflictor;

		if ((bitsDamageType & DMG_ALWAYSGIB) != 0)
		{
			Killed(attacker, GIB_ALWAYS);
		}
		else if ((bitsDamageType & DMG_NEVERGIB) != 0)
		{
			Killed(attacker, GIB_NEVER);
		}
		else
		{
			Killed(attacker, GIB_NORMAL);
		}

		g_pevLastInflictor = nullptr;

		return false;
	}

	// react to the damage (get mad)
	if ((pev->flags & FL_MONSTER) != 0 && !FNullEnt(attacker))
	{
		if ((attacker->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
		{ // only if the attack was a monster or client!

			// enemy's last known position is somewhere down the vector that the attack came from.
			if (inflictor)
			{
				if (m_hEnemy == nullptr || inflictor == m_hEnemy || !HasConditions(bits_COND_SEE_ENEMY))
				{
					m_vecEnemyLKP = inflictor->pev->origin;
				}
			}
			else
			{
				m_vecEnemyLKP = pev->origin + (g_vecAttackDir * 64);
			}

			MakeIdealYaw(m_vecEnemyLKP);

			// add pain to the conditions
			// !!!HACKHACK - fudged for now. Do we want to have a virtual function to determine what is light and
			// heavy damage per monster class?
			if (flDamage > 0)
			{
				SetConditions(bits_COND_LIGHT_DAMAGE);
			}

			if (flDamage >= 20)
			{
				SetConditions(bits_COND_HEAVY_DAMAGE);
			}
		}
	}

	return true;
}

bool CBaseMonster::DeadTakeDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType)
{
	Vector vecDir;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector(0, 0, 0);
	if (!FNullEnt(inflictor))
	{
		if (inflictor)
		{
			vecDir = (inflictor->Center() - Vector(0, 0, 10) - Center()).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

#if 0 // turn this back on when the bounding box issues are resolved.

	pev->flags &= ~FL_ONGROUND;
	pev->origin.z += 1;

	// let the damage scoot the corpse around a bit.
	if (!FNullEnt(inflictor) && (attacker->pev->solid != SOLID_TRIGGER))
	{
		pev->velocity = pev->velocity + vecDir * -DamageForce(flDamage);
	}

#endif

	// kill the corpse if enough damage was done to destroy the corpse and the damage is of a type that is allowed to destroy the corpse.
	if ((bitsDamageType & DMG_GIB_CORPSE) != 0)
	{
		if (pev->health <= flDamage)
		{
			pev->health = -50;
			Killed(attacker, GIB_ALWAYS);
			return false;
		}
		// Accumulate corpse gibbing damage, so you can gib with multiple hits
		pev->health -= flDamage * 0.1;
	}

	return true;
}

float CBaseMonster::DamageForce(float damage)
{
	float force = damage * ((32 * 32 * 72.0) / (pev->size.x * pev->size.y * pev->size.z)) * 5;

	if (force > 1000.0)
	{
		force = 1000.0;
	}

	return force;
}

void RadiusDamage(Vector vecSrc, CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, float flRadius, int bitsDamageType, EntityClassification iClassIgnore)
{
	CBaseEntity* pEntity = nullptr;
	TraceResult tr;
	float flAdjustedDamage, falloff;
	Vector vecSpot;

	if (0 != flRadius)
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	const bool bInWater = (UTIL_PointContents(vecSrc) == CONTENTS_WATER);

	vecSrc.z += 1; // in case grenade is lying on the ground

	if (!attacker)
		attacker = inflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != nullptr)
	{
		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			// UNDONE: this should check a damage mask, not an ignore
			if (iClassIgnore != ENTCLASS_NONE && pEntity->Classify() == iClassIgnore)
			{ // houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			// blast's don't tavel into or out of water
			if (bInWater && pEntity->pev->waterlevel == WaterLevel::Dry)
				continue;
			if (!bInWater && pEntity->pev->waterlevel == WaterLevel::Head)
				continue;

			vecSpot = pEntity->BodyTarget(vecSrc);

			UTIL_TraceLine(vecSrc, vecSpot, dont_ignore_monsters, inflictor->edict(), &tr);

			if (tr.flFraction == 1.0 || tr.pHit == pEntity->edict())
			{ // the explosion can 'see' this entity, so hurt them!
				if (0 != tr.fStartSolid)
				{
					// if we're stuck inside them, fixup the position and distance
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0;
				}

				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = (vecSrc - tr.vecEndPos).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;

				if (flAdjustedDamage < 0)
				{
					flAdjustedDamage = 0;
				}

				// AILogger->debug("hit {}", STRING(pEntity->pev->classname));
				if (tr.flFraction != 1.0)
				{
					ClearMultiDamage();
					pEntity->TraceAttack(inflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize(), &tr, bitsDamageType);
					ApplyMultiDamage(inflictor, attacker);
				}
				else
				{
					pEntity->TakeDamage(inflictor, attacker, flAdjustedDamage, bitsDamageType);
				}
			}
		}
	}
}

void CBaseMonster::RadiusDamage(CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType, EntityClassification iClassIgnore)
{
	::RadiusDamage(pev->origin, inflictor, attacker, flDamage, flDamage * 2.5, bitsDamageType, iClassIgnore);
}

void CBaseMonster::RadiusDamage(Vector vecSrc, CBaseEntity* inflictor, CBaseEntity* attacker, float flDamage, int bitsDamageType, EntityClassification iClassIgnore)
{
	::RadiusDamage(vecSrc, inflictor, attacker, flDamage, flDamage * 2.5, bitsDamageType, iClassIgnore);
}

CBaseEntity* CBaseMonster::CheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	TraceResult tr;

	if (IsPlayer())
		UTIL_MakeVectors(pev->angles);
	else
		UTIL_MakeAimVectors(pev->angles);

	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, edict(), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (iDamage > 0)
		{
			pEntity->TakeDamage(this, this, iDamage, iDmgType);
		}

		return pEntity;
	}

	return nullptr;
}

bool CBaseMonster::FInViewCone(CBaseEntity* pEntity)
{
	Vector2D vec2LOS;
	float flDot;

	UTIL_MakeVectors(pev->angles);

	vec2LOS = (pEntity->pev->origin - pev->origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	if (flDot > m_flFieldOfView)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CBaseMonster::FInViewCone(Vector* pOrigin)
{
	Vector2D vec2LOS;
	float flDot;

	UTIL_MakeVectors(pev->angles);

	vec2LOS = (*pOrigin - pev->origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	if (flDot > m_flFieldOfView)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CBaseEntity::FVisible(CBaseEntity* pEntity)
{
	TraceResult tr;
	Vector vecLookerOrigin;
	Vector vecTargetOrigin;

	if (FBitSet(pEntity->pev->flags, FL_NOTARGET))
		return false;

	// don't look through water
	if ((pev->waterlevel != WaterLevel::Head && pEntity->pev->waterlevel == WaterLevel::Head) || (pev->waterlevel == WaterLevel::Head && pEntity->pev->waterlevel == WaterLevel::Dry))
		return false;

	vecLookerOrigin = pev->origin + pev->view_ofs; // look through the caller's 'eyes'
	vecTargetOrigin = pEntity->EyePosition();

	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, edict() /*pentIgnore*/, &tr);

	if (tr.flFraction != 1.0)
	{
		return false; // Line of sight is not established
	}
	else
	{
		return true; // line of sight is valid.
	}
}

bool CBaseEntity::FVisible(const Vector& vecOrigin)
{
	TraceResult tr;
	Vector vecLookerOrigin;

	vecLookerOrigin = EyePosition(); // look through the caller's 'eyes'

	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, edict() /*pentIgnore*/, &tr);

	if (tr.flFraction != 1.0)
	{
		return false; // Line of sight is not established
	}
	else
	{
		return true; // line of sight is valid.
	}
}

void CBaseEntity::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;

	if (0 != pev->takedamage)
	{
		AddMultiDamage(attacker, this, flDamage, bitsDamageType);

		int blood = BloodColor();

		if (blood != DONT_BLEED)
		{
			SpawnBlood(vecOrigin, blood, flDamage); // a little surface blood.
			TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		}
	}
}

void CBaseMonster::TraceAttack(CBaseEntity* attacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (0 != pev->takedamage)
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch (ptr->iHitgroup)
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= GetSkillFloat("monster_head"sv);
			break;
		case HITGROUP_CHEST:
			flDamage *= GetSkillFloat("monster_chest"sv);
			break;
		case HITGROUP_STOMACH:
			flDamage *= GetSkillFloat("monster_stomach"sv);
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= GetSkillFloat("monster_arm"sv);
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= GetSkillFloat("monster_leg"sv);
			break;
		default:
			break;
		}

		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage); // a little surface blood.
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
		AddMultiDamage(attacker, this, flDamage, bitsDamageType);
	}
}

void CBaseEntity::FireBullets(unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread,
	float flDistance, int iBulletType,
	int iTracerFreq, int iDamage, CBaseEntity* attacker)
{
	static int tracerCount;
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	if (attacker == nullptr)
		attacker = this; // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for (unsigned int iShot = 1; iShot <= cShots; iShot++)
	{
		// get circular gaussian spread
		float x, y, z;
		do
		{
			x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			z = x * x + y * y;
		} while (z > 1);

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict() /*pentIgnore*/, &tr);

		if (iTracerFreq != 0 && (tracerCount++ % iTracerFreq) == 0)
		{
			Vector vecTracerSrc;

			if (IsPlayer())
			{ // adjust tracer position for player
				vecTracerSrc = vecSrc + Vector(0, 0, -4) + gpGlobals->v_right * 2 + gpGlobals->v_forward * 16;
			}
			else
			{
				vecTracerSrc = vecSrc;
			}

			switch (iBulletType)
			{
			case BULLET_MONSTER_MP5:
			case BULLET_MONSTER_9MM:
			case BULLET_MONSTER_12MM:
			default:
				MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecTracerSrc);
				WRITE_BYTE(TE_TRACER);
				WRITE_COORD(vecTracerSrc.x);
				WRITE_COORD(vecTracerSrc.y);
				WRITE_COORD(vecTracerSrc.z);
				WRITE_COORD(tr.vecEndPos.x);
				WRITE_COORD(tr.vecEndPos.y);
				WRITE_COORD(tr.vecEndPos.z);
				MESSAGE_END();
				break;
			}
		}
		// do damage, paint decals
		if (tr.flFraction != 1.0)
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

			if (0 != iDamage)
			{
				pEntity->TraceAttack(attacker, iDamage, vecDir, &tr, DMG_BULLET | ((iDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB));

				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot(&tr, iBulletType);
			}
			else
				switch (iBulletType)
				{
				case BULLET_PLAYER_MP5:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_9mmAR_bullet"sv), vecDir, &tr, DMG_BULLET);
					break;

				case BULLET_PLAYER_357:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_357_bullet"sv), vecDir, &tr, DMG_BULLET);
					break;

				case BULLET_PLAYER_BUCKSHOT:
					// make distance based!
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_buckshot"sv), vecDir, &tr, DMG_BULLET);

					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);
					break;

				default:
				case BULLET_MONSTER_9MM:
					pEntity->TraceAttack(attacker, GetSkillFloat("bullet_9mm"sv), vecDir, &tr, DMG_BULLET);

					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);

					break;

				case BULLET_MONSTER_MP5:
					pEntity->TraceAttack(attacker, GetSkillFloat("bullet_9mmAR"sv), vecDir, &tr, DMG_BULLET);

					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);

					break;

				case BULLET_MONSTER_12MM:
					pEntity->TraceAttack(attacker, GetSkillFloat("bullet_12mm"sv), vecDir, &tr, DMG_BULLET);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);
					break;

				case BULLET_PLAYER_556:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_556_bullet"sv), vecDir, &tr, DMG_BULLET);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);
					break;

				case BULLET_PLAYER_762:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_762_bullet"sv), vecDir, &tr, DMG_BULLET);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType);
					break;

				case BULLET_PLAYER_EAGLE:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_eagle"sv), vecDir, &tr, DMG_BULLET);
					break;

				case BULLET_NONE: // FIX
					pEntity->TraceAttack(attacker, 50, vecDir, &tr, DMG_CLUB);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					// only decal glass
					if (!FNullEnt(tr.pHit) && VARS(tr.pHit)->rendermode != 0)
					{
						UTIL_DecalTrace(&tr, DECAL_GLASSBREAK1 + RANDOM_LONG(0, 2));
					}

					break;
				}
		}
		// make bullet trails
		UTIL_BubbleTrail(vecSrc, tr.vecEndPos, (flDistance * tr.flFraction) / 64.0);
	}
	ApplyMultiDamage(this, attacker);
}

Vector CBaseEntity::FireBulletsPlayer(unsigned int cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread,
	float flDistance, int iBulletType,
	int iTracerFreq, int iDamage, CBaseEntity* attacker, int shared_rand)
{
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	float x = 0, y = 0, z;

	if (attacker == nullptr)
		attacker = this; // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for (unsigned int iShot = 1; iShot <= cShots; iShot++)
	{
		// Use player's random seed.
		//  get circular gaussian spread
		x = UTIL_SharedRandomFloat(shared_rand + iShot, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (1 + iShot), -0.5, 0.5);
		y = UTIL_SharedRandomFloat(shared_rand + (2 + iShot), -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + (3 + iShot), -0.5, 0.5);
		z = x * x + y * y;

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict() /*pentIgnore*/, &tr);

		// do damage, paint decals
		if (tr.flFraction != 1.0)
		{
			CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

			if (0 != iDamage)
			{
				pEntity->TraceAttack(attacker, iDamage, vecDir, &tr, DMG_BULLET | ((iDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB));

				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot(&tr, iBulletType);
			}
			else
				switch (iBulletType)
				{
				default:
				case BULLET_PLAYER_9MM:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_9mm_bullet"sv), vecDir, &tr, DMG_BULLET);
					break;

				case BULLET_PLAYER_MP5:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_9mmAR_bullet"sv), vecDir, &tr, DMG_BULLET);
					break;

				case BULLET_PLAYER_BUCKSHOT:
					// make distance based!
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_buckshot"sv), vecDir, &tr, DMG_BULLET);
					break;

				case BULLET_PLAYER_357:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_357_bullet"sv), vecDir, &tr, DMG_BULLET);
					break;

				case BULLET_PLAYER_556:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_556_bullet"sv), vecDir, &tr, DMG_BULLET);
					break;

				case BULLET_PLAYER_762:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_762_bullet"sv), vecDir, &tr, DMG_BULLET);

					if (tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO)
					{
						auto pHitEntity = Instance(tr.pHit);

						pHitEntity->EmitSound(CHAN_BODY, "weapons/xbow_hitbod2.wav", VOL_NORM, ATTN_NORM);

						if (pHitEntity->BloodColor() != DONT_BLEED)
						{
							MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, EyePosition());
							WRITE_BYTE(TE_BLOODSTREAM);

							WRITE_COORD(tr.vecEndPos.x);
							WRITE_COORD(tr.vecEndPos.y);
							WRITE_COORD(tr.vecEndPos.z);

							const auto direction = vecSrc - tr.vecEndPos;

							WRITE_COORD(direction.x);
							WRITE_COORD(direction.y);
							WRITE_COORD(direction.z);

							if (pHitEntity->BloodColor() == BLOOD_COLOR_RED)
							{
								WRITE_BYTE(70);
							}
							else
							{
								WRITE_BYTE(pHitEntity->BloodColor());
							}

							WRITE_BYTE(150);
							MESSAGE_END();
						}
					}
					break;

				case BULLET_PLAYER_EAGLE:
					pEntity->TraceAttack(attacker, GetSkillFloat("plr_eagle"sv), vecDir, &tr, DMG_BULLET);
					break;

				case BULLET_NONE: // FIX
					pEntity->TraceAttack(attacker, 50, vecDir, &tr, DMG_CLUB);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					// only decal glass
					if (!FNullEnt(tr.pHit) && VARS(tr.pHit)->rendermode != 0)
					{
						UTIL_DecalTrace(&tr, DECAL_GLASSBREAK1 + RANDOM_LONG(0, 2));
					}

					break;
				}
		}
		// make bullet trails
		UTIL_BubbleTrail(vecSrc, tr.vecEndPos, (flDistance * tr.flFraction) / 64.0);
	}
	ApplyMultiDamage(this, attacker);

	return Vector(x * vecSpread.x, y * vecSpread.y, 0.0);
}

void CBaseEntity::TraceBleed(float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (BloodColor() == DONT_BLEED)
		return;

	if (flDamage == 0)
		return;

	if ((bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_MORTAR)) == 0)
		return;

	// make blood decal on the wall!
	TraceResult Bloodtr;
	Vector vecTraceDir;
	float flNoise;
	int cCount;
	int i;

	/*
		if ( !IsAlive() )
		{
			// dealing with a dead monster.
			if ( pev->max_health <= 0 )
			{
				// no blood decal for a monster that has already decalled its limit.
				return;
			}
			else
			{
				pev->max_health--;
			}
		}
	*/

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for (i = 0; i < cCount; i++)
	{
		vecTraceDir = vecDir * -1; // trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.y += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.z += RANDOM_FLOAT(-flNoise, flNoise);

		UTIL_TraceLine(ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172, ignore_monsters, edict(), &Bloodtr);

		if (Bloodtr.flFraction != 1.0)
		{
			UTIL_BloodDecalTrace(&Bloodtr, BloodColor());
		}
	}
}

void CBaseMonster::MakeDamageBloodDecal(int cCount, float flNoise, TraceResult* ptr, const Vector& vecDir)
{
	// make blood decal on the wall!
	TraceResult Bloodtr;
	Vector vecTraceDir;
	int i;

	if (!IsAlive())
	{
		// dealing with a dead monster.
		if (pev->max_health <= 0)
		{
			// no blood decal for a monster that has already decalled its limit.
			return;
		}
		else
		{
			pev->max_health--;
		}
	}

	for (i = 0; i < cCount; i++)
	{
		vecTraceDir = vecDir;

		vecTraceDir.x += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.y += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.z += RANDOM_FLOAT(-flNoise, flNoise);

		UTIL_TraceLine(ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * 172, ignore_monsters, edict(), &Bloodtr);

		/*
				MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
					WRITE_BYTE( TE_SHOWLINE);
					WRITE_COORD( ptr->vecEndPos.x );
					WRITE_COORD( ptr->vecEndPos.y );
					WRITE_COORD( ptr->vecEndPos.z );

					WRITE_COORD( Bloodtr.vecEndPos.x );
					WRITE_COORD( Bloodtr.vecEndPos.y );
					WRITE_COORD( Bloodtr.vecEndPos.z );
				MESSAGE_END();
		*/

		if (Bloodtr.flFraction != 1.0)
		{
			UTIL_BloodDecalTrace(&Bloodtr, BloodColor());
		}
	}
}
