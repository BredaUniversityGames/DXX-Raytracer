#ifndef PRIMARY_RAY_HLSL
#define PRIMARY_RAY_HLSL

#include "primary_ray.hlsli"

// -----------------------------------------------------------
// Misc.


float2 BrownConradyDistortion(float2 uv, float amount, float width_over_height)
{
    uv.y /= width_over_height;
    float2 d1 = 0.1 * amount;
    float2 d2 = -0.025 * amount;

    float r2 = dot(uv, uv);
    uv *= 1.0 + r2 * d1 + r2 * r2 * d2;

    uv.y *= width_over_height;
    return uv;
}

float2 ApplyLensDistortion(float2 uv, float amount, float width_over_height)
{
    float2 min_uv_scale = BrownConradyDistortion(float2(0, 0), amount, width_over_height);
    float2 max_uv_scale = BrownConradyDistortion(float2(1, 1), amount, width_over_height);

    uv = BrownConradyDistortion(uv, amount, width_over_height);
    uv = (uv - min_uv_scale) / (min_uv_scale + max_uv_scale);

    return uv;
}

[shader("raygeneration")]
void PrimaryRaygen()
{
    // Get dispatch ray index and dispatch dimensions
    uint2 dispatch_idx = DispatchRaysIndex().xy;
    uint2 dispatch_dim = DispatchRaysDimensions().xy;

    // Trace the primary ray
    RayDesc ray = GetRayDesc(dispatch_idx, dispatch_dim);
    PrimaryRayPayload ray_payload = (PrimaryRayPayload)0;
    TracePrimaryRay(ray, ray_payload, dispatch_idx);

    // Set up geometry output from primary ray trace and set non-zero defaults where necessary
    HitGeometry geo = (HitGeometry)0;
    geo.depth = RT_RAY_T_MAX;

    // Get geometry data from primary ray trace
    GetHitGeometryFromRay(ray,
		ray_payload.instance_idx, ray_payload.primitive_idx, ray_payload.barycentrics, ray_payload.hit_distance,
		0, dispatch_idx, dispatch_dim, geo
	);
    float3 geo_world_p = ReconstructWorldPosition(g_global_cb.view_inv, ray.Direction, ray_payload.hit_distance);

	// -------------------------------------------------------------------------------------
	// Determine gbuffer motion value

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
    // We store the values as follows
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

    img_albedo[dispatch_idx] = float4(geo.albedo, 1);
    img_emissive[dispatch_idx] = float4(geo.emissive, 0);
    img_normal[dispatch_idx].xy = geo.normal;
    img_depth[dispatch_idx] = geo.depth;
    img_motion[dispatch_idx] = float4(geo.motion, 0, 0);
    img_view_dir[dispatch_idx] = float4(geo.view_dir, select(geo.depth == RT_RAY_T_MAX, 0, 1));
    img_metallic[dispatch_idx] = geo.metallic;
    img_roughness[dispatch_idx] = geo.roughness;
    img_material[dispatch_idx] = geo.material_index;
    img_visibility_prim[dispatch_idx] = geo.vis_prim;
    img_visibility_bary[dispatch_idx] = geo.vis_bary;
    
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

[shader("closesthit")]
void PrimaryClosesthit(inout PrimaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.instance_idx = InstanceIndex();
    payload.primitive_idx = PrimitiveIndex();
    payload.barycentrics = attr.barycentrics;
    payload.hit_distance = RayTCurrent();
}

[shader("anyhit")]
void PrimaryAnyhit(inout PrimaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    Material hit_material;
    if (IsHitTransparent(InstanceIndex(), PrimitiveIndex(), attr.barycentrics, DispatchRaysIndex().xy, hit_material))
    {
        IgnoreHit();
    }
}

[shader("miss")]
void PrimaryMiss(inout PrimaryRayPayload payload)
{
    payload.instance_idx = ~0;
    payload.primitive_idx = ~0;
    payload.barycentrics = float2(0, 0);
    payload.hit_distance = RT_RAY_T_MAX;
}

#endif /* PRIMARY_RAY_HLSL */
