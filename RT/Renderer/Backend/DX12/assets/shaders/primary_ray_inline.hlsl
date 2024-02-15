#ifndef PRIMARY_RAY_INLINE_HLSL
#define PRIMARY_RAY_INLINE_HLSL

#include "include/common.hlsl"
#include "primary_ray.hlsli"

[numthreads(GROUP_X, GROUP_Y, 1)]
void PrimaryRayInline(COMPUTE_ARGS)
{
	// ---------------------------------------------------------------------------------------------------------------
	// Set up the primary ray and its payload, then trace the ray

	RayDesc ray = GetRayDesc(pixel_pos, g_global_cb.render_dim);
	PrimaryRayPayload payload = (PrimaryRayPayload)0;
	TracePrimaryRay(ray, payload, pixel_pos);
	
	// ---------------------------------------------------------------------------------------------------------------
	// Get the geometry data from the primary ray hit

	HitGeometry geo = (HitGeometry)0;
	geo.depth = RT_RAY_T_MAX;

	GetHitGeometryFromRay(ray,
		payload.instance_idx, payload.primitive_idx, payload.barycentrics, payload.hit_distance,
		0, pixel_pos, g_global_cb.render_dim, geo
	);

	// ---------------------------------------------------------------------------------------------------------------
	// Evaluate G-buffer motion vectors

    float3 geo_world_p = ReconstructWorldPosition(g_global_cb.view_inv, ray.Direction, payload.hit_distance);
	float3x4 world_to_object = float3x4(geo.instance_data.world_to_object[0],
										geo.instance_data.world_to_object[1],
										geo.instance_data.world_to_object[2]);

	float3 object_p     = mul(world_to_object, float4(geo_world_p, 1)).xyz;
	float3 prev_world_p = mul(geo.instance_data.object_to_world_prev, float4(object_p, 1)).xyz;

	if (!tweak.object_motion_vectors || tweak.freezeframe)
    {
		prev_world_p = geo_world_p;
    }

	float3 view_p      = mul(g_global_cb.view, float4(geo_world_p, 1)).xyz;
	float3 prev_view_p = mul(g_global_cb.prev_view, float4(prev_world_p, 1)).xyz;

	float2 screen_p      = Project(g_global_cb.proj, view_p);
	float2 prev_screen_p = Project(g_global_cb.prev_proj, prev_view_p);

	float2 screen_motion = prev_screen_p - screen_p;
	screen_motion.y = -screen_motion.y;

	geo.motion = screen_motion;

	// -------------------------------------------------------------------------------------
    // Write to G-buffers

    // G-buffer values are stored as follows:
    // img_albedo - albedo color as sampled from the texture (no color mod)
    // img_emissive - emissive color as sampled from the texture (no color mod)
    // img_normal - normal is stored as an octahedral encoding, world-space with normal mapping already applied
    // img_depth - linear, unnormalized depth (RAY_T)
    // img_motion - screen-space motion vectors
    // img_view_dir - view direction of pixel (ray direction), and a multiplier used to reconstruct world position with, 0 = no geometry hit, 1 = geometry hit
    // img_metallic - metallic, sampled from texture and modified with material metalness factor, metalness overrides
    // img_roughness - roughness, sampled from texture and modified with material roughness factor, roughness overrides, and path space regulation
    // img_material - material index of the global material buffer
    // img_visibility_prim - InstanceIndex() and PrimitiveIndex() of surface at hit location
    //      is used to get the InstanceData and RT_Triangle of the surface
    // img_visibility_bary - barycentric coordinates of surface at hit location

	img_albedo[pixel_pos] = float4(geo.albedo, 1.0);
	img_emissive[pixel_pos] = float4(geo.emissive, 0.0);
	img_normal[pixel_pos] = geo.normal;
	img_depth[pixel_pos] = geo.depth;
	img_motion[pixel_pos] = float4(geo.motion, 0, 0);
	img_view_dir[pixel_pos] = float4(geo.view_dir, select(geo.depth == RT_RAY_T_MAX, 0.0, 1.0));
	img_metallic[pixel_pos] = geo.metallic;
	img_roughness[pixel_pos] = geo.roughness;
	img_material[pixel_pos] = geo.material_index;
	img_visibility_prim[pixel_pos] = geo.vis_prim;
	img_visibility_bary[pixel_pos] = geo.vis_bary;

	if (tweak.upscaling_aa_mode == UPSCALING_AA_MODE_AMD_FSR_2_2)
	{
		// Determine if the pixel should write to the reactive mask for FSR2
        uint reactive_mask_value = g_materials[geo.material_index].flags & RT_MaterialFlag_Fsr2ReactiveMask || g_materials[geo.material_index].flags & RT_MaterialFlag_Light;
	    img_fsr2_reactive_mask[pixel_pos] = float(reactive_mask_value) * tweak.amd_fsr2_reactive_scale;
	}

#if RT_PIXEL_DEBUG
    // Write pixel debug data
    PixelDebugData debug_data = (PixelDebugData)0;

    debug_data.primitive_id = geo.vis_prim.y;
    debug_data.metallic_roughness = float2(geo.metallic, geo.roughness);
    debug_data.uv_barycentrics = float4(float2(0, 0), geo.vis_bary);
    debug_data.material_edge_index = geo.hit_triangle.material_edge_index;
    debug_data.material_index1 = geo.material_index;

    WritePixelDebug(debug_data);
#endif
}

#endif /* PRIMARY_RAY_INLINE_HLSL */
