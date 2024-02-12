#ifndef INDIRECT_LIGHTING_INLINE_HLSL
#define INDIRECT_LIGHTING_INLINE_HLSL

#include "include/common.hlsl"
#include "primary_ray.hlsli"
#include "direct_lighting.hlsli"

[numthreads(GROUP_X, GROUP_Y, 1)]
void IndirectLightingInline(COMPUTE_ARGS)
{
	// ----------------------------------------------------------------------------
	// Fetch geometry data from G-buffers

	float3 gbuf_albedo                = img_albedo[pixel_pos].xyz;
	float3 gbuf_normal                = DecodeNormalOctahedron(img_normal[pixel_pos].xy);
	float3 gbuf_view_dir              = img_view_dir[pixel_pos].xyz;
	float  gbuf_depth                 = img_depth[pixel_pos];
	float  gbuf_metallic              = img_metallic[pixel_pos].x;
	float  gbuf_roughness             = img_roughness[pixel_pos].x;
	uint   gbuf_material              = img_material[pixel_pos].x;
	uint2  gbuf_instance_idx_prim_idx = img_visibility_prim[pixel_pos].xy;
	
	// ----------------------------------------------------------------------------
	// If we have hit valid geometry on this pixel, we evaluate the indirect lighting

	if (HasHitGeometry(gbuf_instance_idx_prim_idx))
	{
		// Get instance data and hit triangle/material
		InstanceData instance_data = g_instance_data_buffer[gbuf_instance_idx_prim_idx.x];
		RT_Triangle  hit_triangle  = GetHitTriangle(instance_data.triangle_buffer_idx, gbuf_instance_idx_prim_idx.y);
		Material     mat           = g_materials[gbuf_material];

		float4 color_mod = UnpackRGBA(instance_data.material_color)*UnpackRGBA(hit_triangle.color);

		// If material is a black body radiator, we do not want to calculate any indirect lighting for that surface
		if (!(mat.flags & RT_MaterialFlag_BlackbodyRadiator))
		{
			// G-buffer albedo is stored as sampled from the texture, so we need to apply the color mod here
			gbuf_albedo *= color_mod.rgb;

			MaterialDesc material_desc = (MaterialDesc)0;
			material_desc.ior       = 0.04;
			material_desc.albedo    = gbuf_albedo;
			material_desc.metallic  = gbuf_metallic;
			material_desc.roughness = gbuf_roughness;

			float3x3 basis     = ConstructOrthonormalBasis(gbuf_normal);
			float3x3 basis_inv = transpose(basis);

			float r1 = RandomSample(pixel_pos, Random_IndirectJitterX);
			float r2 = RandomSample(pixel_pos, Random_IndirectJitterY);

			float3 bounce_direction    = float3(0, 1, 0);
			float3 specular_throughput = 0;
			float3 diffuse_throughput  = 0;

			float3 N =  gbuf_normal;
			float3 V = -gbuf_view_dir;

			bool specular_bounce = false;

			if (tweak.enable_pbr && tweak.importance_sample_brdf)
			{
				float r_specular = RandomSample(pixel_pos, Random_IndirectSpecular);

				float specular_probability = lerp(0.5, 1.0, material_desc.metallic);
				specular_bounce = r_specular <= specular_probability;

				if (specular_bounce)
				{
					float3 N = mul(basis, SampleGGXVNDF(mul(basis_inv, V), material_desc.roughness, r1, r2));
					float3 L = reflect(-V, N);
					float3 H = normalize(V + L);

					float NoV = max(0, dot(N, V));
					float NoH = max(0, dot(N, H));
					float NoL = max(0, dot(N, L));
					float HoV = max(0, dot(H, V));

					if (NoL > 0 && NoV > 0)
					{
						float G1_NoL = G1_Smith(NoL, material_desc.roughness);

						float3 F0 = lerp(material_desc.ior, material_desc.albedo, material_desc.metallic);
						float3 F = FresnelSchlick(HoV, F0);

						specular_throughput = (G1_NoL * F) / specular_probability;

						bounce_direction = L;
					}
				}
				else
				{
					float3 L = mul(basis, CosineSampleHemisphere(r1, r2));
					float3 H = normalize(V + L);

					float HoV = max(0, dot(V, H));

					float3 F0 = lerp(material_desc.ior, material_desc.albedo, material_desc.metallic);
					float3 F = FresnelSchlick(HoV, F0);

					float NoL = max(0, dot(N, L));
					float NoV = max(0, dot(N, V));
					float LoH = max(0, dot(L, H));
					float NoH = max(0, dot(N, H));

					if (tweak.use_oren_nayar_brdf)
					{
						BRDFDataON data;
						data.roughness = material_desc.roughness;
						data.v = V;
						data.n = N;
						data.h = H;
						data.l = L;
						diffuse_throughput = (EvaluateOrenNayar(data) * (1.0 - material_desc.metallic)) / (1.0 - specular_probability);
					}
					else
					{
						diffuse_throughput = EvaluateBRDFDiffuse(F, material_desc) / (1.0 - specular_probability);
					}

					bounce_direction = L;
				}
			}
			else
			{
				float3 L = mul(basis, CosineSampleHemisphere(r1, r2));

				float3 H = normalize(V + L);

				float NoL = max(0, dot(N, L));
				float NoV = max(0, dot(N, V));
				float LoH = max(0, dot(L, H));
				float NoH = max(0, dot(N, H));
				float HoV = max(0, dot(H, V));

				float3 F0 = lerp(material_desc.ior, material_desc.albedo, material_desc.metallic);
				float3 F = FresnelSchlick(HoV, F0);

				float3 specular = EvaluateBRDFSpecular(NoV, NoH, NoL, F, material_desc);
				float3 diffuse;

				if (tweak.use_oren_nayar_brdf)
				{
					BRDFDataON data;
					data.roughness = material_desc.roughness;
					data.v = V;
					data.n = N;
					data.h = H;
					data.l = L;
					diffuse = (EvaluateOrenNayar(data) * (1.0 - material_desc.metallic));
				}
				else
				{
					diffuse = EvaluateBRDFDiffuse(F, material_desc);
				}

				specular_throughput = specular;
				diffuse_throughput = diffuse; // division by PI is cancelled by the multiplication by PI that would happen to compensate for the consine hemisphere sampling BRDF
				bounce_direction = L;
			}

			float3 gbuf_world_p = ReconstructWorldPosFromGBuffer(pixel_pos);

			// Set up geometry input for primary ray trace
			PrimaryRayPayload ray_payload = (PrimaryRayPayload)0;
			RayDesc ray   = (RayDesc)0;
			ray.Origin    = gbuf_world_p + 0.01f * gbuf_normal;
			ray.Direction = bounce_direction;
			ray.TMin      = RT_RAY_T_MIN;
			ray.TMax      = RT_RAY_T_MAX;

			// Trace the primary ray
			TracePrimaryRay(ray, ray_payload, pixel_pos);

			// Set up geometry output from primary ray trace and set non-zero defaults where necessary
			HitGeometry geo = (HitGeometry)0;

			// Get geometry data from primary ray trace
			GetHitGeometryFromRay(ray,
				ray_payload.instance_idx, ray_payload.primitive_idx, ray_payload.barycentrics, ray_payload.hit_distance,
				0, pixel_pos, g_global_cb.render_dim, geo
			);
			geo.world_p = gbuf_world_p + ray_payload.hit_distance*bounce_direction;

			// Set up direct lighting output
			DirectLightingOutput direct_lighting_output = (DirectLightingOutput)0;

			CalculateDirectLightingAtSurface(geo, direct_lighting_output, pixel_pos, true);

			// TODO(daniel): Fetching the material again? Should be optimized away by the compiler
			Material material = g_materials[geo.material_index];

			float3 indirect_color;
			if (HasHitGeometry(geo.vis_prim))
			{
				float direct_light_specular_weight = 1.0f;

				if (material.flags & RT_MaterialFlag_Light)
				{
					direct_light_specular_weight = saturate(1.0 - smoothstep(tweak.direct_specular_threshold - 0.1,
																			 tweak.direct_specular_threshold,
																			 geo.roughness));
				}

				indirect_color = (direct_lighting_output.albedo*direct_lighting_output.direct_lighting + 
								  direct_lighting_output.direct_specular);

				if (specular_bounce)
				{
					indirect_color += direct_light_specular_weight*geo.emissive;
				}
				else // if(!(material.flags & RT_MaterialFlag_Light)) // This would be more correct stopping the double counting of lights. But in practice I think it just looks darker but not better, so meh.
				{
					indirect_color += geo.emissive;
				}
			}
			else
			{
				// Sky color is stored in direct_lighting
				indirect_color = direct_lighting_output.direct_lighting;
			}

			float3 indirect_specular = indirect_color*specular_throughput;
			float3 indirect_diffuse  = indirect_color*diffuse_throughput;

			if (!tweak.reference_mode && tweak.svgf_enabled)
			{
				indirect_specular *= rcp(lerp(0.04, 0.01 + gbuf_albedo, gbuf_metallic));
			}

			img_spec[pixel_pos].rgb += indirect_specular;
			img_diff[pixel_pos].rgb += indirect_diffuse;
		}
	}
}

#endif /* INDIRECT_LIGHTING_INLINE_HLSL */
