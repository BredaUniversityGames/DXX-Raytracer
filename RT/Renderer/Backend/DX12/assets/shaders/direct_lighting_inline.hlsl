#ifndef DIRECT_LIGHTING_INLINE_HLSL
#define DIRECT_LIGHTING_INLINE_HLSL

#include "include/common.hlsl"
#include "primary_ray.hlsli"
#include "direct_lighting.hlsli"

[numthreads(GROUP_X, GROUP_Y, 1)]
void DirectLightingInline(COMPUTE_ARGS)
{
	// ----------------------------------------------------------------------------
	// Fetch geometry data from G-buffers

	HitGeometry geo = (HitGeometry)0;
	geo.material_index = img_material[pixel_pos].x;
	geo.albedo         = img_albedo[pixel_pos].xyz;
	geo.emissive       = img_emissive[pixel_pos].xyz;
	geo.normal		  = img_normal[pixel_pos].xy;
	geo.depth		  = img_depth[pixel_pos].r;
	geo.view_dir       = img_view_dir[pixel_pos].xyz;
	geo.metallic       = img_metallic[pixel_pos].x;
	geo.roughness      = img_roughness[pixel_pos].x;
	geo.vis_prim       = img_visibility_prim[pixel_pos].xy;
	geo.vis_bary       = img_visibility_bary[pixel_pos].xy;
	geo.world_p        = ReconstructWorldPosFromGBuffer(pixel_pos);
	
	// ----------------------------------------------------------------------------
	// Get the instance and hit triangle from the instance and primitive indices

	geo.instance_data  = g_instance_data_buffer[geo.vis_prim.x];
	geo.hit_triangle   = GetHitTriangle(geo.instance_data.triangle_buffer_idx, geo.vis_prim.y);
	
	// ----------------------------------------------------------------------------
	// Calculate the direct lighting at hit surface

	DirectLightingOutput direct_lighting_output = (DirectLightingOutput)0;
	CalculateDirectLightingAtSurface(geo, direct_lighting_output, pixel_pos, false);

	if (!tweak.reference_mode && tweak.svgf_enabled)
	{
		direct_lighting_output.direct_specular *= rcp(lerp(0.04, 0.01 + direct_lighting_output.albedo, geo.metallic));
	}
	
	// ----------------------------------------------------------------------------
	// Write the diffuse, specular, and emissive lighting to their respective buffers

	img_spec[pixel_pos] = float4(direct_lighting_output.direct_specular, 0);
	img_diff[pixel_pos] = float4(direct_lighting_output.direct_lighting, 0);
	img_emissive[pixel_pos] = float4(direct_lighting_output.emissive_lighting, 0);
}

#endif /* DIRECT_LIGHTING_INLINE_HLSL */
