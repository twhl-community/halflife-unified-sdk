// Client side entity management functions

#include <limits>

#include "hud.h"
#include "ClientLibrary.h"
#include "const.h"
#include "entity.h"
#include "entity_types.h"
#include "studio.h" // def. of mstudioevent_t
#include "r_efx.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pmtrace.h"
#include "pm_shared.h"
#include "r_studioint.h"
#include "Exports.h"

#include "particleman.h"
#include "view.h"

#include "networking/ClientUserMessages.h"

#include "sound/ClientSoundReplacementSystem.h"
#include "sound/ISoundSystem.h"

#include "utils/ConCommandSystem.h"

extern engine_studio_api_t IEngineStudio;

void Game_AddObjects();

bool g_iAlive = true;

static TEMPENTITY gTempEnts[MAX_TEMPENTS];
static TEMPENTITY* gpTempEntFree = nullptr;
static TEMPENTITY* gpTempEntActive = nullptr;

/*
========================
HUD_AddEntity
	Return 0 to filter entity from visible list for rendering
========================
*/
int DLLEXPORT HUD_AddEntity(int type, cl_entity_t* ent, const char* modelname)
{
	switch (type)
	{
	case ET_NORMAL:
		break;
	case ET_PLAYER:
	case ET_BEAM:
	case ET_TEMPENTITY:
	case ET_FRAGMENTED:
	default:
		break;
	}
	// each frame every entity passes this function, so the overview hooks it to filter the overview entities
	// in spectator mode:
	// each frame every entity passes this function, so the overview hooks
	// it to filter the overview entities

	if (0 != g_iUser1)
	{
		gHUD.m_Spectator.AddOverviewEntity(type, ent, modelname);

		if ((g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_pip->value == INSET_IN_EYE) &&
			ent->index == g_iUser2)
			return 0; // don't draw the player we are following in eye
	}

	return 1;
}

/*
=========================
HUD_TxferLocalOverrides

The server sends us our origin with extra precision as part of the clientdata structure, not during the normal
playerstate update in entity_state_t.  In order for these overrides to eventually get to the appropriate playerstate
structure, we need to copy them into the state structure at this point.
=========================
*/
void DLLEXPORT HUD_TxferLocalOverrides(entity_state_t* state, const clientdata_t* client)
{
	state->origin = client->origin;

	// Spectator
	state->iuser1 = client->iuser1;
	state->iuser2 = client->iuser2;

	// Duck prevention
	state->iuser3 = client->iuser3;

	// Fire prevention
	state->iuser4 = client->iuser4;
}

/*
=========================
HUD_ProcessPlayerState

We have received entity_state_t for this player over the network.  We need to copy appropriate fields to the
playerstate structure
=========================
*/
void DLLEXPORT HUD_ProcessPlayerState(entity_state_t* dst, const entity_state_t* src)
{
	// Copy in network data
	dst->origin = src->origin;
	dst->angles = src->angles;

	dst->velocity = src->velocity;

	dst->frame = src->frame;
	dst->modelindex = src->modelindex;
	dst->skin = src->skin;
	dst->effects = src->effects;
	dst->weaponmodel = src->weaponmodel;
	dst->movetype = src->movetype;
	dst->sequence = src->sequence;
	dst->animtime = src->animtime;

	dst->solid = src->solid;

	dst->rendermode = src->rendermode;
	dst->renderamt = src->renderamt;
	dst->rendercolor.r = src->rendercolor.r;
	dst->rendercolor.g = src->rendercolor.g;
	dst->rendercolor.b = src->rendercolor.b;
	dst->renderfx = src->renderfx;

	dst->framerate = src->framerate;
	dst->body = src->body;

	memcpy(&dst->controller[0], &src->controller[0], 4 * sizeof(byte));
	memcpy(&dst->blending[0], &src->blending[0], 2 * sizeof(byte));

	dst->basevelocity = src->basevelocity;

	dst->friction = src->friction;
	dst->gravity = src->gravity;
	dst->gaitsequence = src->gaitsequence;
	dst->spectator = src->spectator;
	dst->usehull = src->usehull;
	dst->playerclass = src->playerclass;
	dst->team = src->team;
	dst->colormap = src->colormap;


	// Save off some data so other areas of the Client DLL can get to it
	cl_entity_t* player = gEngfuncs.GetLocalPlayer(); // Get the local player's index
	if (dst->number == player->index)
	{
		g_iPlayerClass = dst->playerclass;
		g_iTeamNumber = dst->team;

		g_iUser1 = src->iuser1;
		g_iUser2 = src->iuser2;
		g_iUser3 = src->iuser3;
	}
}

/*
=========================
HUD_TxferPredictionData

Because we can predict an arbitrary number of frames before the server responds with an update, we need to be able to copy client side prediction data in
 from the state that the server ack'd receiving, which can be anywhere along the predicted frame path ( i.e., we could predict 20 frames into the future and the server ack's
 up through 10 of those frames, so we need to copy persistent client-side only state from the 10th predicted frame to the slot the server
 update is occupying.
=========================
*/
void DLLEXPORT HUD_TxferPredictionData(entity_state_t* ps, const entity_state_t* pps, clientdata_t* pcd, const clientdata_t* ppcd, weapon_data_t* wd, const weapon_data_t* pwd)
{
	ps->oldbuttons = pps->oldbuttons;
	ps->flFallVelocity = pps->flFallVelocity;
	ps->iStepLeft = pps->iStepLeft;
	ps->playerclass = pps->playerclass;

	pcd->viewmodel = ppcd->viewmodel;
	pcd->m_iId = ppcd->m_iId;
	pcd->ammo_shells = ppcd->ammo_shells;
	pcd->ammo_nails = ppcd->ammo_nails;
	pcd->ammo_cells = ppcd->ammo_cells;
	pcd->ammo_rockets = ppcd->ammo_rockets;
	pcd->m_flNextAttack = ppcd->m_flNextAttack;
	pcd->fov = ppcd->fov;
	pcd->weaponanim = ppcd->weaponanim;
	pcd->tfstate = ppcd->tfstate;
	pcd->maxspeed = ppcd->maxspeed;

	pcd->deadflag = ppcd->deadflag;

	// Spectating or not dead == get control over view angles.
	g_iAlive = 0 != ppcd->iuser1 || (pcd->deadflag == DEAD_NO);

	// Spectator
	pcd->iuser1 = ppcd->iuser1;
	pcd->iuser2 = ppcd->iuser2;

	// Duck prevention
	pcd->iuser3 = ppcd->iuser3;

	if (0 != gEngfuncs.IsSpectateOnly())
	{
		// in specator mode we tell the engine who we want to spectate and how
		// iuser3 is not used for duck prevention (since the spectator can't duck at all)
		pcd->iuser1 = g_iUser1; // observer mode
		pcd->iuser2 = g_iUser2; // first target
		pcd->iuser3 = g_iUser3; // second target
	}

	// Fire prevention
	pcd->iuser4 = ppcd->iuser4;

	pcd->fuser2 = ppcd->fuser2;
	pcd->fuser3 = ppcd->fuser3;

	pcd->vuser1 = ppcd->vuser1;
	pcd->vuser2 = ppcd->vuser2;
	pcd->vuser3 = ppcd->vuser3;
	pcd->vuser4 = ppcd->vuser4;

	memcpy(wd, pwd, MAX_WEAPONS * sizeof(weapon_data_t));
}

#if defined(BEAM_TEST)
// Note can't index beam[ 0 ] in Beam callback, so don't use that index
// Room for 1 beam ( 0 can't be used )
static cl_entity_t beams[2];

void BeamEndModel()
{
	cl_entity_t *player, *model;
	int modelindex;
	model_t* mod;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if (!player)
		return;

	mod = gEngfuncs.CL_LoadModel("models/sentry3.mdl", &modelindex);
	if (!mod)
		return;

	// Slot 1
	model = &beams[1];

	*model = *player;
	model->player = 0;
	model->model = mod;
	model->curstate.modelindex = modelindex;

	// Move it out a bit
	model->origin[0] = player->origin[0] - 100;
	model->origin[1] = player->origin[1];

	model->attachment[0] = model->origin;
	model->attachment[1] = model->origin;
	model->attachment[2] = model->origin;
	model->attachment[3] = model->origin;

	gEngfuncs.CL_CreateVisibleEntity(ET_NORMAL, model);
}

void Beams()
{
	static float lasttime;
	float curtime;
	model_t* mod;
	int index;

	BeamEndModel();

	curtime = gEngfuncs.GetClientTime();
	Vector end;

	if ((curtime - lasttime) < 10.0)
		return;

	mod = gEngfuncs.CL_LoadModel("sprites/laserbeam.spr", &index);
	if (!mod)
		return;

	lasttime = curtime;

	end[0] = v_origin.x + 100;
	end[1] = v_origin.y + 100;
	end[2] = v_origin.z;

	BEAM* p1;
	p1 = gEngfuncs.pEfxAPI->R_BeamEntPoint(-1, end, index,
		10.0, 2.0, 0.3, 1.0, 5.0, 0.0, 1.0, 1.0, 1.0, 1.0);
}
#endif

/*
=========================
HUD_CreateEntities

Gives us a chance to add additional entities to the render this frame
=========================
*/
void DLLEXPORT HUD_CreateEntities()
{
#if defined(BEAM_TEST)
	Beams();
#endif

	// Add in any game specific objects
	Game_AddObjects();

	GetClientVoiceMgr()->CreateEntities();
}


/*
=========================
HUD_StudioEvent

The entity's studio model description indicated an event was
fired during this frame, handle the event by it's tag ( e.g., muzzleflash, sound )
=========================
*/
void DLLEXPORT HUD_StudioEvent(const mstudioevent_t* event, const cl_entity_t* entity)
{
	bool iMuzzleFlash = true;


	switch (event->event)
	{
	case 5001:
		if (iMuzzleFlash)
			gEngfuncs.pEfxAPI->R_MuzzleFlash(entity->attachment[0], atoi(event->options));
		break;
	case 5011:
		if (iMuzzleFlash)
			gEngfuncs.pEfxAPI->R_MuzzleFlash(entity->attachment[1], atoi(event->options));
		break;
	case 5021:
		if (iMuzzleFlash)
			gEngfuncs.pEfxAPI->R_MuzzleFlash(entity->attachment[2], atoi(event->options));
		break;
	case 5031:
		if (iMuzzleFlash)
			gEngfuncs.pEfxAPI->R_MuzzleFlash(entity->attachment[3], atoi(event->options));
		break;
	case 5002:
		gEngfuncs.pEfxAPI->R_SparkEffect(entity->attachment[0], atoi(event->options), -100, 100);
		break;
		// Client side sound
	case 5004:
	{
		auto sample = sound::g_ClientSoundReplacement.Lookup(event->options);
		PlaySoundByNameAtLocation(sample, 1.0, entity->attachment[0]);
		break;
	}
	default:
		break;
	}
}

/**
 *	@brief Simulation and cleanup of temporary entities
 *	@param Callback_TempEntPlaySound Obsolete. Use CL_TempEntPlaySound instead.
 */
void DLLEXPORT HUD_TempEntUpdate(
	double frametime,			  // Simulation time
	double client_time,			  // Absolute time on client
	double cl_gravity,			  // True gravity on client
	TEMPENTITY** ppTempEntFree,	  // List of freed temporary ents
	TEMPENTITY** ppTempEntActive, // List
	int (*Callback_AddVisibleEntity)(cl_entity_t* pEntity),
	void (*Callback_TempEntPlaySound)(TEMPENTITY* pTemp, float damp))
{
	// Use our own temp ent list instead.
	ppTempEntFree = &gpTempEntFree;
	ppTempEntActive = &gpTempEntActive;

	static int gTempEntFrame = 0;
	int i;
	TEMPENTITY *pTemp, *pnext, *pprev;
	float freq, gravity, gravitySlow, life, fastFreq;

	Vector vAngles;

	gEngfuncs.GetViewAngles(vAngles);

	if (g_pParticleMan)
		g_pParticleMan->SetVariables(cl_gravity, vAngles);

	// Nothing to simulate
	if (!*ppTempEntActive)
		return;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL
	// tent, then set this bool to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);

	// !!!BUGBUG	-- This needs to be time based
	gTempEntFrame = (gTempEntFrame + 1) & 31;

	pTemp = *ppTempEntActive;

	// !!! Don't simulate while paused....  This is sort of a hack, revisit.
	if (frametime <= 0)
	{
		while (pTemp)
		{
			if ((pTemp->flags & FTENT_NOMODEL) == 0)
			{
				Callback_AddVisibleEntity(&pTemp->entity);
			}
			pTemp = pTemp->next;
		}
		goto finish;
	}

	pprev = nullptr;
	freq = client_time * 0.01;
	fastFreq = client_time * 5.5;
	gravity = -frametime * cl_gravity;
	gravitySlow = gravity * 0.5;

	while (pTemp)
	{
		bool active;

		active = true;

		life = pTemp->die - client_time;
		pnext = pTemp->next;
		if (life < 0)
		{
			if ((pTemp->flags & FTENT_FADEOUT) != 0)
			{
				if (pTemp->entity.curstate.rendermode == kRenderNormal)
					pTemp->entity.curstate.rendermode = kRenderTransTexture;
				pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt * (1 + life * pTemp->fadeSpeed);
				if (pTemp->entity.curstate.renderamt <= 0)
					active = false;
			}
			else
				active = false;
		}
		if (!active) // Kill it
		{
			pTemp->next = *ppTempEntFree;
			*ppTempEntFree = pTemp;
			if (!pprev) // Deleting at head of list
				*ppTempEntActive = pnext;
			else
				pprev->next = pnext;
		}
		else
		{
			pprev = pTemp;

			pTemp->entity.prevstate.origin = pTemp->entity.origin;

			if ((pTemp->flags & FTENT_SPARKSHOWER) != 0)
			{
				// Adjust speed if it's time
				// Scale is next think time
				if (client_time > pTemp->entity.baseline.scale)
				{
					// Show Sparks
					gEngfuncs.pEfxAPI->R_SparkEffect(pTemp->entity.origin, 8, -200, 200);

					// Reduce life
					pTemp->entity.baseline.framerate -= 0.1;

					if (pTemp->entity.baseline.framerate <= 0.0)
					{
						pTemp->die = client_time;
					}
					else
					{
						// So it will die no matter what
						pTemp->die = client_time + 0.5;

						// Next think
						pTemp->entity.baseline.scale = client_time + 0.1;
					}
				}
			}
			else if ((pTemp->flags & FTENT_PLYRATTACHMENT) != 0)
			{
				cl_entity_t* pClient;

				pClient = gEngfuncs.GetEntityByIndex(pTemp->clientIndex);

				pTemp->entity.origin = pClient->origin + pTemp->tentOffset;
			}
			else if ((pTemp->flags & FTENT_SINEWAVE) != 0)
			{
				pTemp->x += pTemp->entity.baseline.origin[0] * frametime;
				pTemp->y += pTemp->entity.baseline.origin[1] * frametime;

				pTemp->entity.origin[0] = pTemp->x + sin(pTemp->entity.baseline.origin[2] + client_time * pTemp->entity.prevstate.frame) * (10 * pTemp->entity.curstate.framerate);
				pTemp->entity.origin[1] = pTemp->y + sin(pTemp->entity.baseline.origin[2] + fastFreq + 0.7) * (8 * pTemp->entity.curstate.framerate);
				pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * frametime;
			}
			else if ((pTemp->flags & FTENT_SPIRAL) != 0)
			{
				float s, c;
				s = sin(pTemp->entity.baseline.origin[2] + fastFreq);
				c = cos(pTemp->entity.baseline.origin[2] + fastFreq);

				pTemp->entity.origin[0] += pTemp->entity.baseline.origin[0] * frametime + 8 * sin(client_time * 20 + (int)pTemp);
				pTemp->entity.origin[1] += pTemp->entity.baseline.origin[1] * frametime + 4 * sin(client_time * 30 + (int)pTemp);
				pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * frametime;
			}

			else
			{
				for (i = 0; i < 3; i++)
					pTemp->entity.origin[i] += pTemp->entity.baseline.origin[i] * frametime;
			}

			if ((pTemp->flags & FTENT_SPRANIMATE) != 0)
			{
				pTemp->entity.curstate.frame += frametime * pTemp->entity.curstate.framerate;
				if (pTemp->entity.curstate.frame >= pTemp->frameMax)
				{
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);

					if ((pTemp->flags & FTENT_SPRANIMATELOOP) == 0)
					{
						// this animating sprite isn't set to loop, so destroy it.
						pTemp->die = client_time;
						pTemp = pnext;
						continue;
					}
				}
			}
			else if ((pTemp->flags & FTENT_SPRCYCLE) != 0)
			{
				pTemp->entity.curstate.frame += frametime * 10;
				if (pTemp->entity.curstate.frame >= pTemp->frameMax)
				{
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);
				}
			}
			// Experiment
#if 0
			if (pTemp->flags & FTENT_SCALE)
				pTemp->entity.curstate.framerate += 20.0 * (frametime / pTemp->entity.curstate.framerate);
#endif

			if ((pTemp->flags & FTENT_ROTATE) != 0)
			{
				pTemp->entity.angles[0] += pTemp->entity.baseline.angles[0] * frametime;
				pTemp->entity.angles[1] += pTemp->entity.baseline.angles[1] * frametime;
				pTemp->entity.angles[2] += pTemp->entity.baseline.angles[2] * frametime;

				pTemp->entity.latched.prevangles = pTemp->entity.angles;
			}

			if ((pTemp->flags & (FTENT_COLLIDEALL | FTENT_COLLIDEWORLD)) != 0)
			{
				Vector traceNormal;
				float traceFraction = 1;

				if ((pTemp->flags & FTENT_COLLIDEALL) != 0)
				{
					pmtrace_t pmtrace;
					physent_t* pe;

					gEngfuncs.pEventAPI->EV_SetTraceHull(2);

					gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX, -1, &pmtrace);


					if (pmtrace.fraction != 1)
					{
						pe = gEngfuncs.pEventAPI->EV_GetPhysent(pmtrace.ent);

						if (0 == pmtrace.ent || (pe->info != pTemp->clientIndex))
						{
							traceFraction = pmtrace.fraction;
							traceNormal = pmtrace.plane.normal;

							if (pTemp->hitcallback)
							{
								(*pTemp->hitcallback)(pTemp, &pmtrace);
							}
						}
					}
				}
				else if ((pTemp->flags & FTENT_COLLIDEWORLD) != 0)
				{
					pmtrace_t pmtrace;

					gEngfuncs.pEventAPI->EV_SetTraceHull(2);

					gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &pmtrace);

					if (pmtrace.fraction != 1)
					{
						traceFraction = pmtrace.fraction;
						traceNormal = pmtrace.plane.normal;

						if ((pTemp->flags & FTENT_SPARKSHOWER) != 0)
						{
							// Chop spark speeds a bit more
							//
							pTemp->entity.baseline.origin = pTemp->entity.baseline.origin * 0.6;

							if (pTemp->entity.baseline.origin.Length() < 10)
							{
								pTemp->entity.baseline.framerate = 0.0;
							}
						}

						if (pTemp->hitcallback)
						{
							(*pTemp->hitcallback)(pTemp, &pmtrace);
						}
					}
				}

				if (traceFraction != 1) // Decent collision now, and damping works
				{
					float proj, damp;

					// Place at contact point
					pTemp->entity.origin = pTemp->entity.prevstate.origin + ((traceFraction * frametime) * pTemp->entity.baseline.origin);
					// Damp velocity
					damp = pTemp->bounceFactor;
					if ((pTemp->flags & (FTENT_GRAVITY | FTENT_SLOWGRAVITY)) != 0)
					{
						damp *= 0.5;
						if (traceNormal[2] > 0.9) // Hit floor?
						{
							if (pTemp->entity.baseline.origin[2] <= 0 && pTemp->entity.baseline.origin[2] >= gravity * 3)
							{
								damp = 0; // Stop
								pTemp->flags &= ~(FTENT_ROTATE | FTENT_GRAVITY | FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_SMOKETRAIL);
								pTemp->entity.angles[0] = 0;
								pTemp->entity.angles[2] = 0;
							}
						}
					}

					if (0 != pTemp->hitSound)
					{
						CL_TempEntPlaySound(pTemp, damp);
					}

					if ((pTemp->flags & FTENT_COLLIDEKILL) != 0)
					{
						// die on impact
						pTemp->flags &= ~FTENT_FADEOUT;
						pTemp->die = client_time;
					}
					else
					{
						// Reflect velocity
						if (damp != 0)
						{
							proj = DotProduct(pTemp->entity.baseline.origin, traceNormal);
							pTemp->entity.baseline.origin = pTemp->entity.baseline.origin + ((-proj * 2) * traceNormal);
							// Reflect rotation (fake)

							pTemp->entity.angles[1] = -pTemp->entity.angles[1];
						}

						if (damp != 1)
						{

							pTemp->entity.baseline.origin = pTemp->entity.baseline.origin * damp;
							pTemp->entity.angles = pTemp->entity.angles * 0.9;
						}
					}
				}
			}


			if ((pTemp->flags & FTENT_FLICKER) != 0 && gTempEntFrame == pTemp->entity.curstate.effects)
			{
				dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
				dl->origin = pTemp->entity.origin;
				dl->radius = 60;
				dl->color.r = 255;
				dl->color.g = 120;
				dl->color.b = 0;
				dl->die = client_time + 0.01;
			}

			if ((pTemp->flags & FTENT_SMOKETRAIL) != 0)
			{
				gEngfuncs.pEfxAPI->R_RocketTrail(pTemp->entity.prevstate.origin, pTemp->entity.origin, 1);
			}

			if ((pTemp->flags & FTENT_GRAVITY) != 0)
				pTemp->entity.baseline.origin[2] += gravity;
			else if ((pTemp->flags & FTENT_SLOWGRAVITY) != 0)
				pTemp->entity.baseline.origin[2] += gravitySlow;

			if ((pTemp->flags & FTENT_CLIENTCUSTOM) != 0)
			{
				if (pTemp->callback)
				{
					(*pTemp->callback)(pTemp, frametime, client_time);
				}
			}

			// Cull to PVS (not frustum cull, just PVS)
			if ((pTemp->flags & FTENT_NOMODEL) == 0)
			{
				if (0 == Callback_AddVisibleEntity(&pTemp->entity))
				{
					if ((pTemp->flags & FTENT_PERSIST) == 0)
					{
						pTemp->die = client_time;		// If we can't draw it this frame, just dump it.
						pTemp->flags &= ~FTENT_FADEOUT; // Don't fade out, just die
					}
				}
			}
		}
		pTemp = pnext;
	}

finish:
	// Restore state info
	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=================
HUD_GetUserEntity

If you specify negative numbers for beam start and end point entities, then
  the engine will call back into this function requesting a pointer to a cl_entity_t
  object that describes the entity to attach the beam onto.

Indices must start at 1, not zero.
=================
*/
cl_entity_t DLLEXPORT* HUD_GetUserEntity(int index)
{
#if defined(BEAM_TEST)
	// None by default, you would return a valic pointer if you create a client side
	//  beam and attach it to a client side entity.
	if (index > 0 && index <= 1)
	{
		return &beams[index];
	}
	else
	{
		return nullptr;
	}
#else
	return nullptr;
#endif
}

static void CL_ParseGunshot(BufferReader& reader)
{
	const Vector pos = reader.ReadCoordVector();
	gEngfuncs.pEfxAPI->R_RunParticleEffect(pos, vec3_origin, 0, 20);
	R_RicochetSound(pos);
}

static void CL_ParseExplosion(BufferReader& reader)
{
	const Vector pos = reader.ReadCoordVector();
	const int spriteIndex = reader.ReadShort();

	const float scale = reader.ReadByte() * 0.1f;
	const float framerate = reader.ReadByte();
	const int flags = reader.ReadByte();

	R_Explosion(pos, spriteIndex, scale, framerate, flags);
}

static void CL_ParseTarExplosion(BufferReader& reader)
{
	const Vector pos = reader.ReadCoordVector();
	gEngfuncs.pEfxAPI->R_BlobExplosion(pos);
	CL_StartSound(-1, CHAN_AUTO, "weapons/explode3.wav", pos, VOL_NORM, 1.0f, PITCH_NORM, 0);
}

static void CL_ParseExplosion2(BufferReader& reader)
{
	const Vector pos = reader.ReadCoordVector();
	const int colorStart = reader.ReadByte();
	const int colorLength = reader.ReadByte();

	gEngfuncs.pEfxAPI->R_ParticleExplosion2(pos, colorStart, colorLength);

	auto light = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
	light->origin = pos;
	light->radius = 350;
	light->die = gEngfuncs.GetClientTime() + 0.5;
	light->decay = 300;

	CL_StartSound(-1, CHAN_AUTO, "weapons/explode3.wav", pos, VOL_NORM, 0.6f, PITCH_NORM, 0);
}

static void CL_ParseGunshotDecal(BufferReader& reader)
{
	const Vector pos = reader.ReadCoordVector();
	const int entityIndex = reader.ReadShort();
	const int decalId = reader.ReadByte();

	gEngfuncs.pEfxAPI->R_BulletImpactParticles(pos);

	if (const int random = gEngfuncs.pfnRandomLong(0, std::numeric_limits<short>::max());
		random < (std::numeric_limits<short>::max() / 2))
	{
		R_RicochetSound(pos, random);
	}

	if (entityIndex < 0 || !gEngfuncs.GetEntityByIndex(entityIndex))
	{
		Con_DPrintf("Decal: entity = %d\n", entityIndex);
		return;
	}

	if (r_decals->value)
	{
		const int decalIndex = gEngfuncs.pEfxAPI->Draw_DecalIndex(decalId);
		gEngfuncs.pEfxAPI->R_DecalShoot(decalIndex, entityIndex, 0, pos, 0);
	}
}

static void CL_ParseArmorRicochet(BufferReader& reader)
{
	const Vector pos = reader.ReadCoordVector();
	const float life = reader.ReadByte() * 0.1f;

	const auto model = gEngfuncs.CL_LoadModel("sprites/richo1.spr", nullptr);

	gEngfuncs.pEfxAPI->R_RicochetSprite(pos, model, 0.1, life);

	R_RicochetSound(pos);
}

static void MsgFunc_TempEntity(const char* name, BufferReader& reader)
{
	const int type = reader.ReadByte();

	switch (type)
	{
	default:
		Con_DPrintf("Unsupported custom temp entity %d\n", type);
		return;

	case TE_GUNSHOT:
		CL_ParseGunshot(reader);
		break;

	case TE_EXPLOSION:
		CL_ParseExplosion(reader);
		break;

	case TE_TAREXPLOSION:
		CL_ParseTarExplosion(reader);
		break;

	case TE_EXPLOSION2:
		CL_ParseExplosion2(reader);
		break;

	case TE_GUNSHOTDECAL:
		CL_ParseGunshotDecal(reader);
		break;

	case TE_ARMOR_RICOCHET:
		CL_ParseArmorRicochet(reader);
		break;
	}
}

static BEAM* g_TargetLaser = nullptr;

static void MsgFunc_TargetLaser(const char* name, BufferReader& reader)
{
	if (g_TargetLaser)
	{
		// Die immediately
		g_TargetLaser->die = 0;
		g_TargetLaser->brightness = 0;
		g_TargetLaser = nullptr;
	}

	const bool enable = reader.ReadByte() != 0;

	if (enable)
	{
		const int entityIndex = reader.ReadShort();
		const int modelIndex = reader.ReadShort();
		const short width = reader.ReadByte();

		Vector color;

		color.x = reader.ReadByte();
		color.y = reader.ReadByte();
		color.z = reader.ReadByte();

		g_TargetLaser = gEngfuncs.pEfxAPI->R_BeamEntPoint(entityIndex, vec3_origin, modelIndex,
			1.f, width, 0, 255, 10, 0, 10, color.x, color.y, color.z);

		// Never dies on its own.
		g_TargetLaser->die = std::numeric_limits<float>::max();

		// Initialize laser end point for first frame.
		TempEntity_UpdateTargetLaser();
	}
}

void TempEntity_ResetTargetLaser()
{
	g_TargetLaser = nullptr;
}

physent_t* TempEntity_FindPhysent(cl_entity_t* entity)
{
	for (int i = 0; i < pmove->numphysent; ++i)
	{
		auto pe = &pmove->physents[i];

		if (pe->info == entity->index)
		{
			return pe;
		}
	}

	return nullptr;
}

int Physent_IndexOf(physent_t* pe)
{
	return pe - pmove->physents;
}

void TempEntity_UpdateTargetLaser()
{
	if (!g_TargetLaser)
	{
		return;
	}

	g_TargetLaser->brightness = 0;

	auto tankEntity = gEngfuncs.GetEntityByIndex(g_TargetLaser->startEntity);

	if (!tankEntity)
	{
		return;
	}

	// Find the physent that represents this entity so we can ignore it during the trace.
	// This can be nullptr if there are too many visible entities on the server side,
	// in which case we won't be able to hit it anyway.
	auto tankPhysent = TempEntity_FindPhysent(tankEntity);

	Vector forward;

	AngleVectors(tankEntity->curstate.angles, &forward, nullptr, nullptr);

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	auto localPlayer = gEngfuncs.GetLocalPlayer();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(localPlayer->index - 1);

	gEngfuncs.pEventAPI->EV_SetTraceHull(2);

	pmtrace_t tr;

	gEngfuncs.pEventAPI->EV_PlayerTrace(
		tankEntity->curstate.origin,
		tankEntity->curstate.origin + forward * 8192,
		PM_STUDIO_BOX, tankPhysent ? Physent_IndexOf(tankPhysent) : -1, &tr);

	gEngfuncs.pEventAPI->EV_PopPMStates();

	// Even if we didn't hit anything this is still the best we've got.
	g_TargetLaser->target = tr.endpos;

	g_TargetLaser->brightness = 255;
}

void CL_TempEntInit()
{
	memset(gTempEnts, 0, sizeof(gTempEnts));

	for (int i = 0; i < MAX_TEMPENTS - 1; ++i)
	{
		gTempEnts[i].next = &gTempEnts[i + 1];
	}

	gTempEnts[MAX_TEMPENTS - 1].next = nullptr;

	gpTempEntFree = gTempEnts;
	gpTempEntActive = nullptr;
}

void R_KillAttachedTents(int client)
{
	// TODO: off by one here?
	if (client < 0 || client > gEngfuncs.GetMaxClients())
	{
		Con_Printf("Bad client in R_KillAttachedTents()!\n");
		return;
	}

	const float time = gEngfuncs.GetClientTime();

	for (TEMPENTITY* i = gpTempEntActive; i; i = i->next)
	{
		if ((i->flags & FTENT_PLYRATTACHMENT) != 0 && i->clientIndex == client)
		{
			i->die = time;
		}
	}
}

void CL_TempEntPrepare(TEMPENTITY* pTemp, model_t* model)
{
	memset(&pTemp->entity, 0, sizeof(pTemp->entity));

	pTemp->flags = 0;
	pTemp->entity.curstate.colormap = 0;
	pTemp->die = gEngfuncs.GetClientTime() + 0.75f;
	pTemp->entity.model = model;
	pTemp->entity.curstate.rendermode = 0;
	pTemp->entity.curstate.renderfx = 0;
	pTemp->fadeSpeed = 0.5;
	pTemp->hitSound = 0;
	pTemp->clientIndex = -1;
	pTemp->bounceFactor = 1;
	pTemp->hitcallback = 0;
	pTemp->callback = 0;
}

static void WarnAboutTempEntOverflow()
{
	// Only print this once per frame to avoid frame drops.
	static float lastTempEntOverflowWarningTime = 0;

	const float time = gEngfuncs.GetClientTime();

	if (time != lastTempEntOverflowWarningTime)
	{
		lastTempEntOverflowWarningTime = time;
		Con_DPrintf("Overflow %d temporary ents!\n", MAX_TEMPENTS);
	}
}

TEMPENTITY* CL_TempEntAlloc(const float* org, model_t* model)
{
	if (!gpTempEntFree)
	{
		WarnAboutTempEntOverflow();
		return nullptr;
	}

	if (!model)
	{
		Con_DPrintf("efx.CL_TempEntAlloc: No model\n");
		return nullptr;
	}

	TEMPENTITY* ent = gpTempEntFree;
	gpTempEntFree = ent->next;

	CL_TempEntPrepare(ent, model);

	ent->priority = 0;

	ent->entity.origin.x = org[0];
	ent->entity.origin.y = org[1];
	ent->entity.origin.z = org[2];

	ent->next = gpTempEntActive;
	gpTempEntActive = ent;

	return ent;
}

TEMPENTITY* CL_TempEntAllocNoModel(const float* org)
{
	if (!gpTempEntFree)
	{
		WarnAboutTempEntOverflow();
		return nullptr;
	}

	TEMPENTITY* ent = gpTempEntFree;
	gpTempEntFree = ent->next;

	CL_TempEntPrepare(ent, nullptr);

	ent->priority = 0;

	ent->entity.origin.x = org[0];
	ent->entity.origin.y = org[1];
	ent->entity.origin.z = org[2];

	ent->next = gpTempEntActive;
	gpTempEntActive = ent;

	return ent;
}

TEMPENTITY* CL_TempEntAllocHigh(const float* org, model_t* model)
{
	if (!model)
	{
		Con_DPrintf("temporary ent model invalid\n");
		return nullptr;
	}

	TEMPENTITY* ent = gpTempEntFree;

	if (gpTempEntFree)
	{
		gpTempEntFree = gpTempEntFree->next;
		ent->next = gpTempEntActive;
		gpTempEntActive = ent;
	}
	else
	{
		ent = gpTempEntActive;

		while (ent && ent->priority != 0)
		{
			ent = ent->next;
		}

		if (!ent)
		{
			Con_DPrintf("Couldn't alloc a high priority TENT!\n");
			return nullptr;
		}
	}

	CL_TempEntPrepare(ent, model);

	ent->priority = 1;

	ent->entity.origin.x = org[0];
	ent->entity.origin.y = org[1];
	ent->entity.origin.z = org[2];

	return ent;
}

static efx_api_t g_EngineEFXAPI{};

void TempEntity_Initialize()
{
	// Override engine temp entity allocation to use our own list.
	auto efx = gEngfuncs.pEfxAPI;

	g_EngineEFXAPI = *efx;

	efx->R_KillAttachedTents = &R_KillAttachedTents;
	efx->CL_TempEntAlloc = &CL_TempEntAlloc;
	efx->CL_TempEntAllocNoModel = &CL_TempEntAllocNoModel;
	efx->CL_TempEntAllocHigh = &CL_TempEntAllocHigh;

	g_ClientUserMessages.RegisterHandler("TempEntity", &MsgFunc_TempEntity);
	g_ClientUserMessages.RegisterHandler("TgtLaser", &MsgFunc_TargetLaser);
}

void TempEntity_Shutdown()
{
	// Restore original API. Necessary in case somebody uses Change Game to load another mod.
	*gEngfuncs.pEfxAPI = g_EngineEFXAPI;
}
