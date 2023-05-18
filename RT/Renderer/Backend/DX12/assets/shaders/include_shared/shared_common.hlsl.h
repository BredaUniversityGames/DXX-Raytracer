#pragma once

#include "shared_defines.hlsl.h"

// ------------------------------------------------------------------
// This header holds common defines that can easily be shared between
// HLSL and C++, this doesn't include any types/defines that are part
// of the renderer's public interface.

// ------------------------------------------------------------------
// Defines, constants, etc

#define RT_PIXEL_DEBUG 0
#define GROUP_X 16
#define GROUP_Y 16
#define BLUE_NOISE_TEX_COUNT 16

// GlobalDebugFlags
#define GlobalDebugFlags_HideDirectLighting           (0x1)
#define GlobalDebugFlags_HideIndirectDiffuseLighting  (0x2)
#define GlobalDebugFlags_HideSpecularLighting         (0x4)

// TaaNeighborhoodMode
#define TaaNeighborhoodMode_Off           (0)
#define TaaNeighborhoodMode_Clamp         (1)
#define TaaNeighborhoodMode_Clip          (2)
#define TaaNeighborhoodMode_VarianceClip  (3)

// MaterialFlag
#define RT_MaterialFlag_BlackbodyRadiator (0x1) // things like lava, basically just treats the albedo as an emissive map and skips all shading

// ------------------------------------------------------------------
// Common structs

namespace RT
{

struct Material
{
	uint   albedo_index     : 16; // 2
	uint   normal_index     : 16; // 4
	uint   metalness_index  : 16; // 6
	uint   roughness_index  : 16; // 8
	uint   emissive_index   : 16; // 10
	uint   flags            : 16; // 12
	float  metalness_factor;      // 16
	float  roughness_factor;      // 20
	uint   emissive_factor;       // 24 (RGBE)
};

struct InstanceData
{
	float4x4 object_to_world;
	float4x4 world_to_object;
    float4x4 object_to_world_prev;
	uint     material_override;
	uint     material_color;
	uint	 triangle_buffer_idx;
};

struct GlobalConstantBuffer
{
	float4x4 view;
	float4x4 view_inv;
	float4x4 proj;
	float4x4 proj_inv;

	float4x4 prev_view;
	float4x4 prev_view_inv;
	float4x4 prev_proj;
	float4x4 prev_proj_inv;

	float2   taa_jitter;

	int2     render_dim;
	uint     frame_index;
	uint     debug_flags;
	uint     debug_render_mode;
	uint     lights_count;

    // Viewport offset, effectively offsets the center of the viewport after projecting
    float    viewport_offset_y;
    float3   sky_color_top;
    float3   sky_color_bottom;
};

struct PixelDebugData
{
	float4 uv_barycentrics;
	float2 metallic_roughness;
	uint   primitive_id;
	uint   material_edge_index;
	uint   material_index1;
	uint   material_index2;
};

// ------------------------------------------------------------------
// Tweak-vars: Struct generation

#define TWEAK_CATEGORY_BEGIN(name)
#define TWEAK_CATEGORY_END()
#define TWEAK_BOOL(name, var, value) int var;
#define TWEAK_INT(name, var, value, min, max) int var;
#define TWEAK_FLOAT(name, var, value, min, max) float var;
#define TWEAK_COLOR(name, var, value) float4 var;
#define TWEAK_OPTIONS(name, var, value, ...) int var;

struct TweakVars
{
	#include "shared_tweakvars.hlsl.h"
};

#undef TWEAK_CATEGORY_BEGIN
#undef TWEAK_CATEGORY_END
#undef TWEAK_BOOL
#undef TWEAK_INT
#undef TWEAK_FLOAT
#undef TWEAK_COLOR
#undef TWEAK_OPTIONS

}
