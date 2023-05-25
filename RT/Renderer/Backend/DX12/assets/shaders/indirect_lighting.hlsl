#ifndef INDIRECT_LIGHTING_HLSL
#define INDIRECT_LIGHTING_HLSL

#include "direct_lighting.hlsl"

[shader("raygeneration")]
void IndirectLightingRaygen()
{
	// -----------------------------------------------------------
	// Compute indirect lighting

	uint2 dispatch_idx = DispatchRaysIndex().xy;

	// Get G-buffer values
	float3 gbuf_albedo = img_albedo[dispatch_idx].xyz;
	float3 gbuf_normal = DecodeNormalOctahedron(img_normal[dispatch_idx].xy);
	float3 gbuf_view_dir = img_view_dir[dispatch_idx].xyz;
	float gbuf_depth = img_depth[dispatch_idx];
	float gbuf_metallic = img_metallic[dispatch_idx].x;
	float gbuf_roughness = img_roughness[dispatch_idx].x;
	uint gbuf_material = img_material[dispatch_idx].x;
	uint2 gbuf_instance_idx_prim_idx = img_visibility_prim[dispatch_idx].xy;

	if (HasHitGeometry(gbuf_instance_idx_prim_idx))
	{
		// Get instance data and hit triangle/material
		InstanceData instance_data = g_instance_data_buffer[gbuf_instance_idx_prim_idx.x];
		RT_Triangle hit_triangle = GetHitTriangle(instance_data.triangle_buffer_idx, gbuf_instance_idx_prim_idx.y);
		Material mat = g_materials[gbuf_material];

		float4 color_mod = UnpackRGBA(instance_data.material_color) * UnpackRGBA(hit_triangle.color);

		// If material is a black body radiator, we do not want to calculate any indirect lighting for that surface
		if (!(mat.flags & RT_MaterialFlag_BlackbodyRadiator))
		{
			// G-buffer albedo is stored as sampled from the texture, so we need to apply the color mod here
			gbuf_albedo *= color_mod.rgb;

			MaterialDesc material_desc = (MaterialDesc)0;
			material_desc.ior = 0.04;
			material_desc.albedo = gbuf_albedo;
			material_desc.metallic = gbuf_metallic;
			material_desc.roughness = gbuf_roughness;

			float3x3 basis = ConstructOrthonormalBasis(gbuf_normal);
			float3x3 basis_inv = transpose(basis);

			// Note(Justin): Do we still need the recursion depth here? Dont think we do
			float r1 = RandomSample(dispatch_idx, Random_IndirectJitterX + 3 * 0/*payload.recursion*/);
			float r2 = RandomSample(dispatch_idx, Random_IndirectJitterY + 3 * 0/*payload.recursion*/);

			float3 bounce_direction = float3(0, 1, 0);
			float3 specular_throughput = 0;
			float3 diffuse_throughput = 0;

			float3 N = gbuf_normal;
			float3 V = -gbuf_view_dir;

			if (tweak.enable_pbr && tweak.importance_sample_brdf)
			{
				float specular_probability = lerp(0.5, 1.0, material_desc.metallic);
				// Note(Justin): Do we still need the recursion depth here? Dont think we do
				//if (/*payload.recursion*/0 > 0)
				//{
				//	specular_probability = 0;
				//}

				// Note(Justin): Do we still need the recursion depth here? Dont think we do
				float r_specular = RandomSample(dispatch_idx, Random_IndirectSpecular + 3 * 0/*payload.recursion*/);

				bool specular_bounce = r_specular <= specular_probability;
				// Note(Justin): Do we still need the recursion depth here? Dont think we do
				//if (/*payload.recursion*/0 > 0)
				//{
				//	specular_bounce = false;
				//}

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

			float3 gbuf_world_p = ReconstructWorldPosFromGBuffer(dispatch_idx);

			// Set up geometry input for primary ray trace
			PrimaryRayPayload ray_payload = (PrimaryRayPayload)0;
			RayDesc ray_desc = (RayDesc)0;
			ray_desc.Origin = gbuf_world_p + 0.01f * gbuf_normal;
			ray_desc.Direction = bounce_direction;
			ray_desc.TMin = RT_RAY_T_MIN;
			ray_desc.TMax = RT_RAY_T_MAX;

			// Trace the primary ray
			TracePrimaryRay(ray_desc, ray_payload);

			// Set up geometry output from primary ray trace and set non-zero defaults where necessary
			GeometryRayOutput geo_ray_output = (GeometryRayOutput)0;

			// Get geometry data from primary ray trace
			GetGeometryDataFromPrimaryRay(ray_desc, ray_payload, 1, geo_ray_output);

			// Set up direct lighting output
			DirectLightingOutput direct_lighting_output = (DirectLightingOutput)0;

			CalculateDirectLightingAtSurface(geo_ray_output, direct_lighting_output, true);

			float3 indirect_color;
			if (HasHitGeometry(geo_ray_output.vis_prim))
			{
				indirect_color = direct_lighting_output.albedo * direct_lighting_output.direct_lighting +
					direct_lighting_output.emissive_lighting + direct_lighting_output.direct_specular;
			}
			else
			{
				// Sky color is stored in direct_lighting
				indirect_color = direct_lighting_output.direct_lighting;
			}

			float3 indirect_specular = specular_throughput * indirect_color;
			float3 indirect_diffuse = diffuse_throughput * indirect_color;

			if (!tweak.reference_mode && tweak.svgf_enabled)
			{
				indirect_specular *= rcp(lerp(0.04, 0.01 + gbuf_albedo, gbuf_metallic));
			}

			img_spec[dispatch_idx].rgb += indirect_specular;
			img_diff[dispatch_idx].rgb += indirect_diffuse;
		}
	}
}

#endif /* INDIRECT_LIGHTING_HLSL */
