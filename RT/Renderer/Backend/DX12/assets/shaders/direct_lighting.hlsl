#ifndef DIRECT_LIGHTING_HLSL
#define DIRECT_LIGHTING_HLSL

#include "primary_ray.hlsli"
#include "direct_lighting.hlsli"

[shader("raygeneration")]
void DirectLightingRaygen()
{
	// -----------------------------------------------------------
	// Compute direct lighting 
	uint2 dispatch_idx = DispatchRaysIndex().xy;

	// Get values from g-buffers and store them into the geometry ray output struct
	HitGeometry geo = (HitGeometry)0;
	geo.material_index = img_material[dispatch_idx].x;
	geo.albedo         = img_albedo[dispatch_idx].xyz;
	geo.emissive       = img_emissive[dispatch_idx].xyz;
	geo.normal		  = img_normal[dispatch_idx].xy;
	geo.depth		  = img_depth[dispatch_idx].r;
	geo.view_dir       = img_view_dir[dispatch_idx].xyz;
	geo.metallic       = img_metallic[dispatch_idx].x;
	geo.roughness      = img_roughness[dispatch_idx].x;
	geo.vis_prim       = img_visibility_prim[dispatch_idx].xy;
	geo.vis_bary       = img_visibility_bary[dispatch_idx].xy;
	geo.world_p        = ReconstructWorldPosFromGBuffer(dispatch_idx);

	geo.instance_data  = g_instance_data_buffer[geo.vis_prim.x];
	geo.hit_triangle   = GetHitTriangle(geo.instance_data.triangle_buffer_idx, geo.vis_prim.y);

	// Set up direct lighting output
	DirectLightingOutput direct_lighting_output = (DirectLightingOutput)0;
	CalculateDirectLightingAtSurface(geo, direct_lighting_output, dispatch_idx, false);

	if (tweak.svgf_enabled)
	{
		direct_lighting_output.direct_specular *= rcp(lerp(0.04, 0.01 + direct_lighting_output.albedo, geo.metallic));
	}

	img_spec[dispatch_idx] = float4(direct_lighting_output.direct_specular, 0);
	img_diff[dispatch_idx] = float4(direct_lighting_output.direct_lighting, 0);
	img_emissive[dispatch_idx] = float4(direct_lighting_output.emissive_lighting, 0);
}

#endif /* DIRECT_LIGHTING_HLSL */
