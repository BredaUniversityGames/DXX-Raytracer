#pragma once

#include "ApiTypes.h"
#include "Common.h"

// ------------------------------------------------------------------
// Simple math library that can be used in C and C++. Not optimized,
// just for getting simple stuff done without hassle.

#include <math.h>

// ------------------------------------------------------------------
// Scalar stuff

#define RT_PI32 3.14159265f

static inline float RT_DegreesFromRadians(float radians)
{
    return radians * (180.0f / RT_PI32);
}

static inline float RT_RadiansFromDegrees(float degrees)
{
    return degrees * (RT_PI32 / 180.0f);
}

static inline float RT_FloatAreEqual(float a, float b, float tolerance)
{
    return fabsf(a - b) <= tolerance;
}

static inline float RT_Lerp(float a, float b, float t)
{
    return (1.0f-t)*a + t*b;
}

static inline float RT_Smoothstep(float x)
{
    float result = x*x*(3.0f - 2.0f*x);
    return result;
}

static inline uint8_t RT_Uint8FromFloat(float x)
{
    x = RT_CLAMP(x, 0, 1);
    return (uint8_t)(255.0f*x);
}

static inline float RT_Absf(float x)
{
    if (x < 0.0f) 
    { 
        return x * -1.0f; 
    }
    return x;
}

// ------------------------------------------------------------------
// RT_Vec2i

static inline RT_Vec2i RT_Vec2iMake(int x, int y)
{
    RT_Vec2i result;
    result.x = x;
    result.y = y;
    return result;
}

// ------------------------------------------------------------------
// RT_Vec2

static inline RT_Vec2 RT_Vec2Make(float x, float y)
{
    RT_Vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

static inline RT_Vec2 RT_Vec2FromScalar(float s)
{
    RT_Vec2 result;
    result.x = s;
    result.y = s;
    return result;
}

static inline RT_Vec2 RT_Vec2Negate(RT_Vec2 a)
{
    RT_Vec2 result;
    result.x = -a.x;
    result.y = -a.y;
    return result;
}

static inline RT_Vec2 RT_Vec2Add(RT_Vec2 a, RT_Vec2 b)
{
    RT_Vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

static inline RT_Vec2 RT_Vec2Sub(RT_Vec2 a, RT_Vec2 b)
{
    RT_Vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

static inline RT_Vec2 RT_Vec2Mul(RT_Vec2 a, RT_Vec2 b)
{
    RT_Vec2 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

static inline RT_Vec2 RT_Vec2Muls(RT_Vec2 a, float b)
{
    RT_Vec2 result;
    result.x = a.x * b;
    result.y = a.y * b;
    return result;
}

static inline RT_Vec2 RT_Vec2Div(RT_Vec2 a, RT_Vec2 b)
{
    RT_Vec2 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

static inline RT_Vec2 RT_Vec2Divs(RT_Vec2 a, float b)
{
    RT_Vec2 result;
    result.x = a.x / b;
    result.y = a.y / b;
    return result;
}

static inline float RT_Vec2Dot(RT_Vec2 a, RT_Vec2 b)
{
    return a.x*b.x + a.y*b.y;
}

static inline float RT_Vec2LengthSq(RT_Vec2 a)
{
    return RT_Vec2Dot(a, a);
}

static inline float RT_Vec2Length(RT_Vec2 a)
{
    return sqrtf(RT_Vec2LengthSq(a));
}

static inline RT_Vec2 RT_Vec2Normalize(RT_Vec2 a)
{
    float length     = RT_Vec2Length(a);
    float rcp_length = 1.0f / length;
    return RT_Vec2Muls(a, rcp_length);
}

static inline RT_Vec2 RT_Vec2NormalizeOrZero(RT_Vec2 a)
{
    RT_Vec2 result = { 0 };

    float length = RT_Vec2Length(a);
    if (fabsf(length) > 0.0001f)
    {
        float rcp_length = 1.0f / length;
        result = RT_Vec2Muls(a, rcp_length);
    }

    return result;
}

static inline RT_Vec2 RT_Vec2Min(RT_Vec2 a, RT_Vec2 b)
{
    RT_Vec2 result;
    result.x = a.x < b.x ? a.x : b.x;
    result.y = a.y < b.y ? a.y : b.y;
    return result;
}

static inline RT_Vec2 RT_Vec2Max(RT_Vec2 a, RT_Vec2 b)
{
    RT_Vec2 result;
    result.x = a.x > b.x ? a.x : b.x;
    result.y = a.y > b.y ? a.y : b.y;
    return result;
}

// ------------------------------------------------------------------
// RT_Vec3

static inline RT_Vec3 RT_Vec3Make(float x, float y, float z)
{
    RT_Vec3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

static inline RT_Vec3 RT_Vec3FromScalar(float s)
{
    RT_Vec3 result;
    result.x = s;
    result.y = s;
    result.z = s;
    return result;
}

static inline RT_Vec3 RT_Vec3FromFloats(float v[3])
{
    RT_Vec3 result;
    result.x = v[0];
    result.y = v[1];
    result.z = v[2];
    return result;
}

static inline RT_Vec3 RT_Vec3Negate(RT_Vec3 a)
{
    RT_Vec3 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    return result;
}

static inline RT_Vec3 RT_Vec3Add(RT_Vec3 a, RT_Vec3 b)
{
    RT_Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

static inline RT_Vec3 RT_Vec3MulsAdd(RT_Vec3 a, RT_Vec3 b, float c)
{
    RT_Vec3 result;
    result.x = a.x + c*b.x;
    result.y = a.y + c*b.y;
    result.z = a.z + c*b.z;
    return result;
}

static inline RT_Vec3 RT_Vec3Sub(RT_Vec3 a, RT_Vec3 b)
{
    RT_Vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

static inline RT_Vec3 RT_Vec3Mul(RT_Vec3 a, RT_Vec3 b)
{
    RT_Vec3 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

static inline RT_Vec3 RT_Vec3Div(RT_Vec3 a, RT_Vec3 b)
{
    RT_Vec3 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

static inline RT_Vec3 RT_Vec3Muls(RT_Vec3 a, float b)
{
    RT_Vec3 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    return result;
}

static inline RT_Vec3 RT_Vec3Divs(RT_Vec3 a, float b)
{
    RT_Vec3 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    return result;
}

static inline RT_Vec3 RT_Vec3Add3(RT_Vec3 a, RT_Vec3 b, RT_Vec3 c)
{
    RT_Vec3 result;
    result.x = a.x + b.x + c.x;
    result.y = a.y + b.y + c.y;
    result.z = a.z + b.z + c.z;
    return result;
}

static inline float RT_Vec3Dot(RT_Vec3 a, RT_Vec3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static inline float RT_Vec3LengthSq(RT_Vec3 a)
{
    return RT_Vec3Dot(a, a);
}

static inline float RT_Vec3Length(RT_Vec3 a)
{
    return sqrtf(RT_Vec3LengthSq(a));
}

static inline RT_Vec3 RT_Vec3Normalize(RT_Vec3 a)
{
    float length     = RT_Vec3Length(a);
    float rcp_length = 1.0f / length;
    return RT_Vec3Muls(a, rcp_length);
}

static inline RT_Vec3 RT_Vec3NormalizeOrZero(RT_Vec3 a)
{
    RT_Vec3 result = { 0 };

    float length = RT_Vec3Length(a);
    if (fabsf(length) > 0.0001f)
    {
        float rcp_length = 1.0f / length;
        result = RT_Vec3Muls(a, rcp_length);
    }

    return result;
}

static inline RT_Vec3 RT_Vec3Cross(RT_Vec3 a, RT_Vec3 b)
{
    RT_Vec3 result;
    result.x = a.y*b.z - a.z*b.y;
    result.y = a.z*b.x - a.x*b.z;
    result.z = a.x*b.y - a.y*b.x;
    return result;
}

static inline bool RT_Vec3AreEqual(RT_Vec3 a, RT_Vec3 b, float tolerance)
{
    return (RT_FloatAreEqual(a.x, b.x, tolerance) &&    
            RT_FloatAreEqual(a.y, b.y, tolerance) &&    
            RT_FloatAreEqual(a.z, b.z, tolerance));
}

static inline RT_Vec3 RT_Lerp3(RT_Vec3 a, RT_Vec3 b, float t)
{
    return RT_Vec3Add(RT_Vec3Muls(a, 1.0f - t), RT_Vec3Muls(b, t));
}

static inline RT_Vec3 RT_Vec3Min(RT_Vec3 a, RT_Vec3 b)
{
    RT_Vec3 result;
    result.x = a.x < b.x ? a.x : b.x;
    result.y = a.y < b.y ? a.y : b.y;
    result.z = a.z < b.z ? a.z : b.z;
    return result;
}

static inline RT_Vec3 RT_Vec3Max(RT_Vec3 a, RT_Vec3 b)
{
    RT_Vec3 result;
    result.x = a.x > b.x ? a.x : b.x;
    result.y = a.y > b.y ? a.y : b.y;
    result.z = a.z > b.z ? a.z : b.z;
    return result;
}

// ------------------------------------------------------------------
// RT_Vec4

static inline RT_Vec4 RT_Vec4Make(float x, float y, float z, float w)
{
    RT_Vec4 result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

static inline RT_Vec4 RT_Vec4FromScalar(float s)
{
    RT_Vec4 result;
    result.x = s;
    result.y = s;
    result.z = s;
    result.w = s;
    return result;
}

static inline RT_Vec4 RT_Vec4FromFloats(float v[4])
{
    RT_Vec4 result;
    result.x = v[0];
    result.y = v[1];
    result.z = v[2];
    result.w = v[3];
    return result;
}

static inline RT_Vec4 RT_Vec4Negate(RT_Vec4 a)
{
    RT_Vec4 result;
    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = -a.w;
    return result;
}

static inline RT_Vec4 RT_Vec4Add(RT_Vec4 a, RT_Vec4 b)
{
    RT_Vec4 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;
    return result;
}

static inline RT_Vec4 RT_Vec4Sub(RT_Vec4 a, RT_Vec4 b)
{
    RT_Vec4 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.z = a.w - b.w;
    return result;
}

static inline RT_Vec4 RT_Vec4Mul(RT_Vec4 a, RT_Vec4 b)
{
    RT_Vec4 result;
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    result.w = a.w * b.w;
    return result;
}

static inline RT_Vec4 RT_Vec4Div(RT_Vec4 a, RT_Vec4 b)
{
    RT_Vec4 result;
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    result.w = a.w / b.w;
    return result;
}

static inline RT_Vec4 RT_Vec4Muls(RT_Vec4 a, float b)
{
    RT_Vec4 result;
    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;
    result.w = a.w * b;
    return result;
}

static inline RT_Vec4 RT_Vec4Divs(RT_Vec4 a, float b)
{
    RT_Vec4 result;
    result.x = a.x / b;
    result.y = a.y / b;
    result.z = a.z / b;
    result.w = a.w / b;
    return result;
}

static inline float RT_Vec4Dot(RT_Vec4 a, RT_Vec4 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

static inline float RT_Vec4LengthSq(RT_Vec4 a)
{
    return RT_Vec4Dot(a, a);
}

static inline float RT_Vec4Length(RT_Vec4 a)
{
    return sqrtf(RT_Vec4LengthSq(a));
}

static inline RT_Vec4 RT_Vec4Normalize(RT_Vec4 a)
{
    float length     = RT_Vec4Length(a);
    float rcp_length = 1.0f / length;
    return RT_Vec4Muls(a, rcp_length);
}

static inline RT_Vec4 RT_Vec4NormalizeOrZero(RT_Vec4 a)
{
    RT_Vec4 result = { 0 };

    float length = RT_Vec4Length(a);
    if (fabsf(length) > 0.0001f)
    {
        float rcp_length = 1.0f / length;
        result = RT_Vec4Muls(a, rcp_length);
    }

    return result;
}

// ------------------------------------------------------------------
// RGBA8 color helpers

static inline uint32_t RT_PackRGBA(RT_Vec4 color)
{
    uint8_t r = (uint8_t)RT_CLAMP(255.0f*sqrtf(color.x), 0, 255);
    uint8_t g = (uint8_t)RT_CLAMP(255.0f*sqrtf(color.y), 0, 255);
    uint8_t b = (uint8_t)RT_CLAMP(255.0f*sqrtf(color.z), 0, 255);
    uint8_t a = (uint8_t)RT_CLAMP(255.0f*color.w, 0, 255);
    uint32_t result = (r << 0)|(g << 8)|(b << 16)|(a << 24);
    return result;
}

static inline RT_Vec4 RT_UnpackRGBA(uint32_t rgba)
{
    float rcp_255 = 1.0f / 255.0f;

    RT_Vec4 result;
    result.x = rcp_255*(float)((rgba >>  0) & 0xFF);
    result.y = rcp_255*(float)((rgba >>  8) & 0xFF);
    result.z = rcp_255*(float)((rgba >> 16) & 0xFF);
    result.w = rcp_255*(float)((rgba >> 24) & 0xFF);
    result.xyz = RT_Vec3Mul(result.xyz, result.xyz);
    return result;
}

// ------------------------------------------------------------------
// RGBE8 color helpers

static inline uint32_t RT_PackRGBE(RT_Vec3 color)
{
    color.x = RT_MAX(0, color.x);
    color.y = RT_MAX(0, color.y);
    color.z = RT_MAX(0, color.z);

    float max_abs = RT_MAX(color.x, RT_MAX(color.y, color.z));

    if(max_abs == 0)
    {
        return 0;
    }

    float exponent = floorf(log2f(max_abs));

    float scale = powf(2, -exponent)*256.0f;
    uint32_t r = RT_MIN(511, (uint32_t)(roundf(color.x*scale)));
    uint32_t g = RT_MIN(511, (uint32_t)(roundf(color.y*scale)));
    uint32_t b = RT_MIN(511, (uint32_t)(roundf(color.z*scale)));

    uint32_t result;
    result  = (uint32_t)(RT_CLAMP(exponent + 20, 0, 31)) << 27;
    result |= r;
    result |= g << 9;
    result |= b << 18;

    return result;
}

static inline RT_Vec3 RT_UnpackRGBE(uint32_t rgbe)
{
    int exponent = (int)(rgbe >> 27) - 20;
    float scale = powf(2, (float)exponent) / 256.0f;

    RT_Vec3 result;
    result.x = (float)((rgbe >>  0) & 0x1ff)*scale;
    result.y = (float)((rgbe >>  9) & 0x1ff)*scale;
    result.z = (float)((rgbe >> 18) & 0x1ff)*scale;
    return result;
}

// ------------------------------------------------------------------
// RT_Mat34

// NOTE(daniel): For the time being, I haven't defined any useful
// functions for RT_Mat34, just use RT_Mat4 for all your actual
// calculations and convert to/from RT_Mat34 as required.

static inline RT_Mat34 RT_Mat34FromColumns(RT_Vec3 x, RT_Vec3 y, RT_Vec3 z, RT_Vec3 t)
{
    RT_Mat34 result;

    result.r0.x = x.x;
    result.r1.x = x.y;
    result.r2.x = x.z;

    result.r0.y = y.x;
    result.r1.y = y.y;
    result.r2.y = y.z;

    result.r0.z = z.x;
    result.r1.z = z.y;
    result.r2.z = z.z;

    result.r0.w = t.x;
    result.r1.w = t.y;
    result.r2.w = t.z;

    return result;
}

static inline RT_Mat34 RT_Mat34FromMat4(RT_Mat4 mat4)
{
    // Assert that the mat4 is 0, 0, 0, 1 in the 4th row
    RT_ASSERT(RT_FloatAreEqual(mat4.r3.x, 0, 0.001f));
    RT_ASSERT(RT_FloatAreEqual(mat4.r3.y, 0, 0.001f));
    RT_ASSERT(RT_FloatAreEqual(mat4.r3.z, 0, 0.001f));
    RT_ASSERT(RT_FloatAreEqual(mat4.r3.w, 1, 0.001f));
    return mat4.mat34;
}

static inline RT_Vec3 RT_TranslationFromMat34(RT_Mat34 mat)
{
    return RT_Vec3Make(mat.r0.w, mat.r1.w, mat.r2.w);
}

static inline RT_Vec3 RT_ScaleFromMat34(RT_Mat34 mat)
{
    return RT_Vec3Make(mat.r0.x, mat.r1.y, mat.r2.z);
}

// ------------------------------------------------------------------
// RT_Mat4

static inline RT_Mat4 RT_Mat4Identity(void)
{
    RT_Mat4 result = { 0 };
    result.e[0][0] = 1;
    result.e[1][1] = 1;
    result.e[2][2] = 1;
    result.e[3][3] = 1;
    return result;
}

static inline RT_Mat4 RT_MakeMat4(float m00, float m01, float m02, float m03,
                                  float m10, float m11, float m12, float m13,
                                  float m20, float m21, float m22, float m23,
                                  float m30, float m31, float m32, float m33)
{
    RT_Mat4 result;
    result.e[0][0] = m00; result.e[0][1] = m01; result.e[0][2] = m02; result.e[0][3] = m03;
    result.e[1][0] = m10; result.e[1][1] = m11; result.e[1][2] = m12; result.e[1][3] = m13;
    result.e[2][0] = m20; result.e[2][1] = m21; result.e[2][2] = m22; result.e[2][3] = m23;
    result.e[3][0] = m30; result.e[3][1] = m31; result.e[3][2] = m32; result.e[3][3] = m33;
    return result;
}

static inline RT_Mat4 RT_Mat4FromRows(RT_Vec4 r0, RT_Vec4 r1, RT_Vec4 r2, RT_Vec4 r3)
{
    RT_Mat4 result;
    result.r0 = r0;
    result.r1 = r1;
    result.r2 = r2;
    result.r3 = r3;
    return result;
}

static inline RT_Mat4 RT_Mat4FromBasisVectors(RT_Vec3 x, RT_Vec3 y, RT_Vec3 z)
{
    RT_Mat4 result = RT_Mat4Identity();

    result.r0.x = x.x;
    result.r1.x = x.y;
    result.r2.x = x.z;

    result.r0.y = y.x;
    result.r1.y = y.y;
    result.r2.y = y.z;

    result.r0.z = z.x;
    result.r1.z = z.y;
    result.r2.z = z.z;

    return result;
}

static inline RT_Mat4 RT_Mat4FromMat34(RT_Mat34 mat34)
{
    RT_Mat4 result;
    result.mat34 = mat34;
    result.r3.x = 0;
    result.r3.y = 0;
    result.r3.z = 0;
    result.r3.w = 1;
    return result;
}

static inline RT_Vec3 RT_TranslationFromMat4(RT_Mat4 mat)
{
    return RT_Vec3Make(mat.r0.w, mat.r1.w, mat.r2.w);
}

static inline RT_Mat4 RT_Mat4Mul(RT_Mat4 a, RT_Mat4 b)
{
    // NOTE(daniel): Not optimized, obviously.

    RT_Mat4 result = { 0 };

    for (int col = 0; col < 4; col++)
    {
        for (int row = 0; row < 4; row++)
        {
            for (int e = 0; e < 4; e++)
            {
                result.e[col][row] += a.e[col][e]*b.e[e][row];
            }
        }
    }

    return result;
}

static inline RT_Mat4 RT_Mat4Mul3(RT_Mat4 a, RT_Mat4 b, RT_Mat4 c)
{
    RT_Mat4 result = RT_Mat4Mul(a, b);
    result = RT_Mat4Mul(result, c);
    return result;
}


static inline RT_Mat4 RT_Mat4Transpose(RT_Mat4 m)
{
    for (int col = 0; col < 4; col++)
    {
        for (int row = 0; row < 4; row++)
        {
            RT_SWAP(float, m.e[col][row], m.e[row][col]);
        }
    }

    return m;
}

static inline RT_Vec4 RT_Mat4TransformVec4(RT_Mat4 m, RT_Vec4 v)
{
    RT_Vec4 result;
    result.x = RT_Vec4Dot(m.r0, v);
    result.y = RT_Vec4Dot(m.r1, v);
    result.z = RT_Vec4Dot(m.r2, v);
    result.w = RT_Vec4Dot(m.r3, v);
    return result;
}

static inline RT_Vec3 RT_Mat4TransformPoint(RT_Mat4 m, RT_Vec3 p)
{
    RT_Vec4 v = { p.x, p.y, p.z, 1.0f };
    RT_Vec4 result = RT_Mat4TransformVec4(m, v);
    return RT_Vec3Make(result.x, result.y, result.z);
}

static inline RT_Vec2 RT_Mat4Transform2DPoint(RT_Mat4 m, RT_Vec2 p)
{
    RT_Vec4 v = { p.x, p.y, 0.0f, 1.0f };
    RT_Vec4 result = RT_Mat4TransformVec4(m, v);
    return RT_Vec2Make(result.x, result.y);
}

static inline RT_Vec3 RT_Mat4TransformDirection(RT_Mat4 m, RT_Vec3 d)
{
    RT_Vec4 v = { d.x, d.y, d.z, 0.0f };
    RT_Vec4 result = RT_Mat4TransformVec4(m, v);
    return RT_Vec3Make(result.x, result.y, result.z);
}

static inline RT_Mat4 RT_Mat4FromTranslation(RT_Vec3 translation)
{
    RT_Mat4 result = RT_Mat4Identity();
    result.e[0][3] = translation.x;
    result.e[1][3] = translation.y;
    result.e[2][3] = translation.z;
    return result;
}

static inline RT_Mat4 RT_Mat4FromScale(RT_Vec3 scale)
{
    RT_Mat4 result = RT_MakeMat4(scale.x, 0,       0,       0,
                                 0,       scale.y, 0,       0,
                                 0,       0,       scale.z, 0,
                                 0,       0,       0,       1);
    return result;
}

static inline RT_Mat4 RT_Mat4FromXRotation(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    RT_Mat4 result = RT_MakeMat4( 
        1, 0, 0, 0,
        0, c,-s, 0,
        0, s, c, 0,
        0, 0, 0, 1
    );

    return result;
}

static inline RT_Mat4 RT_Mat4FromYRotation(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    RT_Mat4 result = RT_MakeMat4( 
        c, 0, s, 0,
        0, 1, 0, 0,
       -s, 0, c, 0,
        0, 0, 0, 1
    );

    return result;
}

static inline RT_Mat4 RT_Mat4FromZRotation(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    RT_Mat4 result = RT_MakeMat4( 
        c, s, 0, 0,
       -s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    return result;
}

static inline RT_Mat4 RT_Mat4Perspective(float fov, float aspect, float n, float f)
{
    float g = tanf(fov / 2);
    float k = f / (f - n);

    RT_Mat4 result = RT_MakeMat4(
        g / aspect, 0, 0, 0,
        0,          g, 0, 0,
        0,          0, k, -n * k,
        0,          0, -1, 0
    );

    return result;
}

static inline RT_Mat4 RT_Mat4Inverse(RT_Mat4 m)
{
    RT_Vec3 a = RT_Vec3Make(m.e[0][0], m.e[1][0], m.e[2][0]);
    RT_Vec3 b = RT_Vec3Make(m.e[0][1], m.e[1][1], m.e[2][1]);
    RT_Vec3 c = RT_Vec3Make(m.e[0][2], m.e[1][2], m.e[2][2]);
    RT_Vec3 d = RT_Vec3Make(m.e[0][3], m.e[1][3], m.e[2][3]);
    
    float x = m.e[3][0];
    float y = m.e[3][1];
    float z = m.e[3][2];
    float w = m.e[3][3];

    RT_Vec3 s = RT_Vec3Cross(a, b);
    RT_Vec3 t = RT_Vec3Cross(c, d);
    
    RT_Vec3 u = RT_Vec3Sub(RT_Vec3Muls(a, y), RT_Vec3Muls(b, x));
    RT_Vec3 v = RT_Vec3Sub(RT_Vec3Muls(c, w), RT_Vec3Muls(d, z));

    float inv_det = 1.0f / (RT_Vec3Dot(s, v) + RT_Vec3Dot(t, u));
    s = RT_Vec3Muls(s, inv_det);
    t = RT_Vec3Muls(t, inv_det);
    u = RT_Vec3Muls(u, inv_det);
    v = RT_Vec3Muls(v, inv_det);

    RT_Vec3 r0 = RT_Vec3Add(RT_Vec3Cross(b, v), RT_Vec3Muls(t, y));
    RT_Vec3 r1 = RT_Vec3Sub(RT_Vec3Cross(v, a), RT_Vec3Muls(t, x));
    RT_Vec3 r2 = RT_Vec3Add(RT_Vec3Cross(d, u), RT_Vec3Muls(s, w));
    RT_Vec3 r3 = RT_Vec3Sub(RT_Vec3Cross(u, c), RT_Vec3Muls(s, z));

    return RT_MakeMat4(
        r0.x, r0.y, r0.z, -RT_Vec3Dot(b, t),
        r1.x, r1.y, r1.z, RT_Vec3Dot(a, t),
        r2.x, r2.y, r2.z, -RT_Vec3Dot(d, s),
        r3.x, r3.y, r3.z, RT_Vec3Dot(c, s)
    );
}

// ------------------------------------------------------------------
// RT_Quat

static inline RT_Quat RT_QuatFromXYZW(float x, float y, float z, float w)
{
    RT_Quat result;
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
}

static inline RT_Quat RT_QuatFromFloats(float f[4])
{
    RT_Quat result;
    result.x = f[0];
    result.y = f[1];
    result.z = f[2];
    result.w = f[3];
    return result;
}

static inline RT_Quat RT_QuatFromVec3W(RT_Vec3 xyz, float w)
{
    RT_Quat result;
    result.xyz = xyz;
    result.w   = w;
    return result;
}

static inline RT_Quat RT_QuatIdentity(void)
{
    RT_Quat result;
    result.x = 0;
    result.y = 0;
    result.z = 0;
    result.w = 1;
    return result;
}

static inline RT_Quat RT_QuatFromAxisAngle(RT_Vec3 axis, float angle)
{
    axis = RT_Vec3Normalize(axis);

    float s = sinf(0.5f*angle);
    float c = cosf(0.5f*angle);

    RT_Quat result = RT_QuatFromVec3W(RT_Vec3Muls(axis, s), c);
    return result;
}

static inline RT_Quat RT_QuatNormalize(RT_Quat q)
{
    RT_Quat result;
    result.xyzw = RT_Vec4Normalize(q.xyzw);
    return result;
}

static inline RT_Quat RT_QuatMul(RT_Quat a, RT_Quat b)
{
    RT_Quat result;
    result.x = a.e[3]*b.e[0] + a.e[0]*b.e[3] + a.e[1]*b.e[2] - a.e[2]*b.e[1];
    result.y = a.e[3]*b.e[1] - a.e[0]*b.e[2] + a.e[1]*b.e[3] + a.e[2]*b.e[0];
    result.z = a.e[3]*b.e[2] + a.e[0]*b.e[1] - a.e[1]*b.e[0] + a.e[2]*b.e[3];
    result.w = a.e[3]*b.e[3] - a.e[0]*b.e[0] - a.e[1]*b.e[1] - a.e[2]*b.e[2];
    return result;
}

static inline RT_Quat RT_QuatConjugate(RT_Quat q)
{
    RT_Quat result;
    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w =  q.w;
    return result;
}

static inline RT_Vec3 RT_QuatRotateVec3(RT_Quat q, RT_Vec3 v)
{
    RT_Quat p = RT_QuatFromVec3W(v, 0.0f);

    RT_Quat r;
    r = RT_QuatMul(q, p);
    r = RT_QuatMul(r, RT_QuatConjugate(q));

    return r.xyz;
}

static inline RT_Mat4 RT_Mat4FromQuat(RT_Quat q)
{
    RT_Mat4 result;

    result.e[0][3] = 0.0f;
    result.e[1][3] = 0.0f;
    result.e[2][3] = 0.0f;

    result.e[3][0] = 0.0f;
    result.e[3][1] = 0.0f;
    result.e[3][2] = 0.0f;
    result.e[3][3] = 1.0f;

    float qx   = 2.0f*q.x*q.x;
    float qy   = 2.0f*q.y*q.y;
    float qz   = 2.0f*q.z*q.z;
    float qxqy = 2.0f*q.x*q.y;
    float qxqz = 2.0f*q.x*q.z;
    float qxqw = 2.0f*q.x*q.w;
    float qyqz = 2.0f*q.y*q.z;
    float qyqw = 2.0f*q.y*q.w;
    float qzqw = 2.0f*q.z*q.w;

    result.e[0][0] = 1.0f - qy - qz;
    result.e[1][1] = 1.0f - qx - qz;
    result.e[2][2] = 1.0f - qx - qy;

    result.e[1][0] = qxqy + qzqw;
    result.e[2][0] = qxqz - qyqw;

    result.e[0][1] = qxqy - qzqw;
    result.e[2][1] = qyqz + qxqw;

    result.e[0][2] = qxqz + qyqw;
    result.e[1][2] = qyqz - qxqw;

    return result;
}

static inline RT_Mat4 RT_Mat4FromTRS(RT_Vec3 translation, RT_Quat rotation, RT_Vec3 scale)
{
	RT_Mat4 transform = RT_Mat4Identity();
	transform = RT_Mat4Mul(transform, RT_Mat4FromTranslation(translation));
	transform = RT_Mat4Mul(transform, RT_Mat4FromQuat(rotation));
	transform = RT_Mat4Mul(transform, RT_Mat4FromScale(scale));
    return transform;
}

// ------------------------------------------------------------------
// Helpers for lights

static inline void RT_ConstructBasis(RT_Vec3 n, RT_Vec3 *t, RT_Vec3 *b)
{
    if (fabsf(n.x) > fabsf(n.y)) 
        *t = RT_Vec3Divs(RT_Vec3Make(n.z, 0, -n.x), sqrtf(n.x*n.x + n.z*n.z));
    else
        *t = RT_Vec3Divs(RT_Vec3Make(0, -n.z, n.y), sqrtf(n.y*n.y + n.z*n.z));
    *b = RT_Vec3Cross(n, *t);
}

static inline RT_Light RT_MakeRectangularLight(RT_Vec3 emission, RT_Vec3 position, RT_Quat rotation, RT_Vec2 scale)
{
    RT_Light result = {0};

    result.kind = RT_LightKind_Area_Rect;
    result.emission = RT_PackRGBE(emission);

    RT_Vec3 scale3 = RT_Vec3Make(scale.x, 1, scale.y);
    RT_Mat4 transform = RT_Mat4FromTRS(position, rotation, scale3);

    result.transform = transform.mat34;
    result.spot_angle    = 255;
    result.spot_softness = 0;

    return result;
}

static inline RT_Light RT_MakeSphericalLight(RT_Vec3 emission, RT_Vec3 position, float radius)
{
    RT_Light result = {0};

    result.kind = RT_LightKind_Area_Sphere;
    result.emission = RT_PackRGBE(emission);

    RT_Mat4 translate = RT_Mat4FromTranslation(position);
    RT_Mat4 scale     = RT_Mat4FromScale(RT_Vec3FromScalar(radius));
    RT_Mat4 transform = RT_Mat4Mul(translate, scale);

    result.transform = transform.mat34;
    result.spot_angle    = 255;
    result.spot_softness = 0;

    return result;
}

static inline RT_Light RT_MakeSphericalSpotlight(RT_Vec3 emission, RT_Vec3 position, RT_Vec3 direction, float radius, float angle, float softness, float vignette)
{
    RT_Light result = {0};

    result.kind = RT_LightKind_Area_Sphere;
    result.emission = RT_PackRGBE(emission);

    RT_Vec3 t, b;
    RT_ConstructBasis(direction, &t, &b);

    RT_Mat4 rotate = RT_Mat4FromBasisVectors(t, direction, b);

    RT_Mat4 translate = RT_Mat4FromTranslation(position);
    RT_Mat4 scale     = RT_Mat4FromScale(RT_Vec3FromScalar(radius));
    RT_Mat4 transform = RT_Mat4Mul3(translate, scale, rotate);

    result.transform = transform.mat34;
    result.spot_angle    = RT_Uint8FromFloat(angle);
    result.spot_softness = RT_Uint8FromFloat(softness);
    result.spot_vignette = RT_Uint8FromFloat(vignette);

    return result;
}

// ------------------------------------------------------------------
// C++ features for cool C++ people

#ifdef __cplusplus

// ------------------------------------------------------------------
// RT_Vec3 operator overloads

static inline RT_Vec3 operator - (const RT_Vec3 &a)
{
    return RT_Vec3Negate(a);
}

static inline RT_Vec3 operator - (const RT_Vec3 &a, const RT_Vec3 &b)
{
    return RT_Vec3Sub(a, b);
}

static inline RT_Vec3 operator + (const RT_Vec3 &a, const RT_Vec3 &b)
{
    return RT_Vec3Add(a, b);
}

static inline RT_Vec3 operator * (const RT_Vec3 &a, const RT_Vec3 &b)
{
    return RT_Vec3Mul(a, b);
}

static inline RT_Vec3 operator * (float a, const RT_Vec3 &b)
{
    return RT_Vec3Muls(b, a);
}

static inline RT_Vec3 operator * (const RT_Vec3 &a, float b)
{
    return RT_Vec3Muls(a, b);
}

static inline RT_Vec3 operator / (const RT_Vec3 &a, const RT_Vec3 &b)
{
    return RT_Vec3Div(a, b);
}

static inline RT_Vec3 operator / (const RT_Vec3 &a, float b)
{
    return RT_Vec3Divs(a, b);
}

static inline RT_Vec3 operator / (float a, const RT_Vec3 &b)
{
    return RT_Vec3Div(RT_Vec3FromScalar(a), b);
}

// ------------------------------------------------------------------
// RT_Mat4 operator overloads

static inline RT_Mat4 operator * (const RT_Mat4 &a, const RT_Mat4 &b)
{
    return RT_Mat4Mul(a, b);
}

static inline RT_Vec4 operator * (const RT_Mat4 &a, const RT_Vec4 &b)
{
    return RT_Mat4TransformVec4(a, b);
}

// ------------------------------------------------------------------
// RT_Quat operator overloads

static inline RT_Quat &operator *= (RT_Quat &a, const RT_Quat &b)
{
    a = RT_QuatMul(a, b);
    return a;
}

static inline RT_Quat operator * (const RT_Quat &a, const RT_Quat &b)
{
    RT_Quat result = a;
    result *= b;
    return result;
}

#endif
