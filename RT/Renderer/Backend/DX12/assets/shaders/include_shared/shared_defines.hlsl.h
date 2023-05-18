#pragma once

//------------------------------------------------------------------------	
// This file's purpose is to allow you to have shared headers between
// HLSL and C++, to avoid having to edit stuff in two places all the time

#if !defined(__HLSL_VERSION)
#define float2   RT_Vec2
#define float3   RT_Vec3
#define float4   RT_Vec4
#define int2     RT_Vec2i
#define int3     RT_Vec3i
#define int4     RT_Vec4i
#define float4x4 RT_Mat4
#define float3x4 RT_Mat34
#define uint     uint32_t
#endif