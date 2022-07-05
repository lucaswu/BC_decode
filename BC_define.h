#pragma once

#ifndef __x86_64__
#define DAMON_ANDROID 1
#endif

#include <math.h>
#if DAMON_ANDROID
#include <arm_neon.h>
#endif
struct vector4
{
    union {
        float       vector4_f32[4];
        uint32_t    vector4_u32[4];
    };
};

#if DAMON_ANDROID
typedef float32x4_t VECTOR;
#else
typedef vector4 VECTOR;
#endif

struct VECTORF32
{
    union
    {
        float f[4];
        VECTOR v;
    };

    inline operator VECTOR() const { return v; }
    inline operator const float* () const { return f; }
};

struct  VECTORU32
{
    union
    {
        uint32_t u[4];
        VECTOR v;
    };
    inline operator VECTOR() const { return v; }
};

struct VECTORI32
{
    union
    {
        int32_t i[4];
        VECTOR v;
    };

    inline operator VECTOR() const { return v; }
};

#include "BC_MathVector.inl"