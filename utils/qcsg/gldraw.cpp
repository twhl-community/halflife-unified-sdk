/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
 *
 *	This product contains software technology licensed from Id
 *	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
 *	All Rights Reserved.
 *
 ****/


#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <SDL2/SDL.h>

#include "csg.h"

// can't use the glvertex3fv functions, because the vec3_t fields
// could be either floats or doubles, depending on DOUBLEVEC_T

qboolean drawflag;
vec3_t draw_mins, draw_maxs;


#define WIN_SIZE 512

SDL_Window* g_pWindow = NULL;

SDL_GLContext g_GLContext = NULL;

void InitWindow(void)
{
	if (SDL_Init(SDL_INIT_VIDEO))
	{
		printf("Couldn't initialize SDL2\n");
		exit(EXIT_FAILURE);
	}

	g_pWindow = SDL_CreateWindow("qcsg", 0, 0, WIN_SIZE, WIN_SIZE, SDL_WINDOW_OPENGL);

	if (!g_pWindow)
	{
		printf("Failed to create SDL Window\n");
		exit(EXIT_FAILURE);
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, false);

	g_GLContext = SDL_GL_CreateContext(g_pWindow);

	SDL_GL_MakeCurrent(g_pWindow, g_GLContext);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearIndex(0);
	glLoadIdentity();
	glDrawBuffer(GL_FRONT);
}

void Draw_ClearWindow(void)
{
	static int init;
	int w, h, g;
	vec_t mx, my;

	if (!drawflag)
		return;

	if (!init)
	{
		init = true;
		InitWindow();
	}

	glClearColor(1, 0.8, 0.8, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	w = (draw_maxs[0] - draw_mins[0]);
	h = (draw_maxs[1] - draw_mins[1]);

	mx = draw_mins[0] + w / 2;
	my = draw_mins[1] + h / 2;

	g = w > h ? w : h;

	glLoadIdentity();
	gluPerspective(90, 1, 2, 16384);
	gluLookAt(mx, my, draw_maxs[2] + g / 2, mx, my, draw_maxs[2], 0, 1, 0);

	glColor3f(0, 0, 0);
	//	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if 0
	glColor4f (1,0,0,0.5);
	glBegin (GL_POLYGON);

	glVertex3f (0, 500, 0);
	glVertex3f (0, 900, 0);
	glVertex3f (0, 900, 100);
	glVertex3f (0, 500, 100);

	glEnd ();
#endif

	glFlush();
}

void Draw_SetRed(void)
{
	if (!drawflag)
		return;

	glColor3f(1, 0, 0);
}

void Draw_SetGrey(void)
{
	if (!drawflag)
		return;

	glColor3f(0.5, 0.5, 0.5);
}

void Draw_SetBlack(void)
{
	if (!drawflag)
		return;

	glColor3f(0, 0, 0);
}

void DrawWinding(winding_t* w)
{
	int i;

	if (!drawflag)
		return;

	glColor4f(0, 0, 0, 0.5);
	glBegin(GL_LINE_LOOP);
	for (i = 0; i < w->numpoints; i++)
		glVertex3f(w->points[i][0], w->points[i][1], w->points[i][2]);
	glEnd();

	glColor4f(0, 1, 0, 0.3);
	glBegin(GL_POLYGON);
	for (i = 0; i < w->numpoints; i++)
		glVertex3f(w->points[i][0], w->points[i][1], w->points[i][2]);
	glEnd();

	glFlush();
}

void DrawAuxWinding(winding_t* w)
{
	int i;

	if (!drawflag)
		return;

	glColor4f(0, 0, 0, 0.5);
	glBegin(GL_LINE_LOOP);
	for (i = 0; i < w->numpoints; i++)
		glVertex3f(w->points[i][0], w->points[i][1], w->points[i][2]);
	glEnd();

	glColor4f(1, 0, 0, 0.3);
	glBegin(GL_POLYGON);
	for (i = 0; i < w->numpoints; i++)
		glVertex3f(w->points[i][0], w->points[i][1], w->points[i][2]);
	glEnd();

	glFlush();
}
