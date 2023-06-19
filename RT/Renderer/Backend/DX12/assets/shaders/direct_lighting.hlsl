#ifndef DIRECT_LIGHTING_HLSL
#define DIRECT_LIGHTING_HLSL

#include "primary_ray.hlsli"
#include "direct_lighting.hlsli"

/*

	The direct light raygen shader will determine the direct lighting for each pixel in the g-buffers.
	This includes shooting occlusion rays and importance sampling light sources.

*/

[shader("raygeneration")]
void DirectLightingRaygen()
{
	// -----------------------------------------------------------
	// Compute direct lighting 
	uint2 dispatch_idx = DispatchRaysIndex().xy;

	// Get values from g-buffers and store them into the geometry ray output struct
	GeometryRayOutput geo_ray_output = (GeometryRayOutput)0;
	geo_ray_output.material_index = img_material[dispatch_idx].x;
	geo_ray_output.albedo         = img_albedo[dispatch_idx].xyz;
	geo_ray_output.emissive       = img_emissive[dispatch_idx].xyz;
	geo_ray_output.normal		  = img_normal[dispatch_idx].xy;
	geo_ray_output.depth		  = img_depth[dispatch_idx].r;
	geo_ray_output.view_dir       = img_view_dir[dispatch_idx].xyz;
	geo_ray_output.metallic       = img_metallic[dispatch_idx].x;
	geo_ray_output.roughness      = img_roughness[dispatch_idx].x;
	geo_ray_output.vis_prim       = img_visibility_prim[dispatch_idx].xy;
	geo_ray_output.vis_bary       = img_visibility_bary[dispatch_idx].xy;
	geo_ray_output.world_p        = ReconstructWorldPosFromGBuffer(dispatch_idx);

	geo_ray_output.instance_data  = g_instance_data_buffer[geo_ray_output.vis_prim.x];
	geo_ray_output.hit_triangle   = GetHitTriangle(geo_ray_output.instance_data.triangle_buffer_idx, geo_ray_output.vis_prim.y);

	// Set up direct lighting output
	DirectLightingOutput direct_lighting_output = (DirectLightingOutput)0;
	CalculateDirectLightingAtSurface(geo_ray_output, direct_lighting_output, false);

	if (!tweak.reference_mode && tweak.svgf_enabled)
	{
		direct_lighting_output.direct_specular *= rcp(lerp(0.04, 0.01 + direct_lighting_output.albedo, geo_ray_output.metallic));
	}

	img_spec[dispatch_idx] = float4(direct_lighting_output.direct_specular, 0);
	img_diff[dispatch_idx] = float4(direct_lighting_output.direct_lighting, 0);
	img_emissive[dispatch_idx] = float4(direct_lighting_output.emissive_lighting, 0);
}

#endif /* DIRECT_LIGHTING_HLSL */
