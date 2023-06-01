#pragma once

#include "shared_defines.hlsl.h"

// ------------------------------------------------------------------
// This header holds common defines that can easily be shared between
// HLSL and C++, this doesn't include any types/defines that are part
// of the renderer's public interface.

// ------------------------------------------------------------------
// Defines, constants, etc

#define RT_PIXEL_DEBUG 0 // TODO(daniel): Make this be dependent on the build configuration and make it stand out in the GPU profiler so you can tell it apart from actual work
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
	// NOTE(Justin): The bitfields that existed here before broke the raytracing shaders (debugging them, as no PDB was found).
	// I do not know why, I wish I knew, and also why did it work with compute though?? Why did it not give any warnings or errors??
	// Only god knows.
struct Material
{
	uint   albedo_index;
	uint   normal_index;
	uint   metalness_index;
	uint   roughness_index;
	uint   emissive_index;
	uint   flags;
	float  metalness_factor;
	float  roughness_factor;
	uint   emissive_factor;
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

	// Color overlay for being shot at or picking up items.
	float4	 screen_color_overlay;

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
