/***
 *
 *	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// mathlib.h

#pragma once

#include <numbers>

#include "Platform.h"

typedef float vec_t;

#include "vector.h"

// up / down
#define PITCH 0
// left / right
#define YAW 1
// fall over
#define ROLL 2

typedef vec_t vec4_t[4]; // x,y,z,w
typedef vec_t vec5_t[5];

typedef short vec_s_t;
typedef vec_s_t vec3s_t[3];
typedef vec_s_t vec4s_t[4]; // x,y,z,w
typedef vec_s_t vec5s_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

constexpr double PI = std::numbers::pi;

constexpr Vector vec3_origin(0, 0, 0);
constexpr Vector g_vecZero(0, 0, 0);

constexpr Vector vec3_forward{1, 0, 0};
constexpr Vector vec3_right{0, -1, 0};
constexpr Vector vec3_up{0, 0, 1};

void VectorMA(const float* veca, float scale, const float* vecb, float* vecc);

float VectorNormalize(float* v); // returns vector length
void VectorInverse(float* v);
void VectorScale(const float* in, float scale, float* out);
int Q_log2(int val);

void AngleVectors(const Vector& angles, Vector* forward, Vector* right, Vector* up);

inline void AngleVectors(const Vector& angles, Vector& forward, Vector& right, Vector& up)
{
	AngleVectors(angles, &forward, &right, &up);
}

void AngleVectorsTranspose(const Vector& angles, Vector* forward, Vector* right, Vector* up);

void AngleMatrix(const float* angles, float matrix[3][4]);
void AngleIMatrix(const Vector& angles, float matrix[3][4]);
void VectorTransform(const float* in1, float in2[3][4], float* out);
void ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void MatrixCopy(float in[3][4], float out[3][4]);
void QuaternionMatrix(vec4_t quaternion, float (*matrix)[4]);
void QuaternionSlerp(vec4_t p, vec4_t q, float t, vec4_t qt);
void AngleQuaternion(float* angles, vec4_t quaternion);

void NormalizeAngles(float* angles);
void InterpolateAngles(float* start, float* end, float* output, float frac);
float AngleBetweenVectors(const Vector& v1, const Vector& v2);

void VectorMatrix(const Vector& forward, Vector& right, Vector& up);
void VectorAngles(const float* forward, float* angles);

float anglemod(float a);
