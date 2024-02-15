#ifndef DIRECT_LIGHTING_HLSLI
#define DIRECT_LIGHTING_HLSLI

#include "primary_ray.hlsli"
#include "occlusion.hlsl"
#include "brdf.hlsl"

struct Reservoir
{
	uint y;
	uint M;
	float w_y;
	float w_sum;
};

void Update(inout Reservoir r, uint x_i, float w_i, uint2 pixel_pos)
{
	r.w_sum += w_i;
	r.M     += 1;
	if (RandomSample(pixel_pos, r.M) < (w_i / r.w_sum))
	{
		r.y   = x_i;
		r.w_y = w_i;
	}
}

struct LightSample
{
	float3 L;
	float  L_dist;
	float3 e;
};

void EvaluateLight(in RT_Light l, float3 world_p, float2 r, out LightSample result)
{
	float angle    = FloatFromUint8(l.spot_angle);
	float softness = FloatFromUint8(l.spot_softness);

	float angle_min = angle - 0.5f*softness;
	float angle_max = angle + 0.5f*softness;

	float vignette_strength = FloatFromUint8(l.spot_vignette);

	float min_distance    = 0.1f;
	float min_distance_sq = min_distance*min_distance;

	float3 L         = 0;
	float  L_dist    = 0;
	float  L_dist_sq = 0;
	float3 e         = 0;

	float3 emission = RT_LIGHT_SCALE*UnpackRGBE(l.emission);

	float3 x, y, z, t;
	DecomposeColumns(l.transform, x, y, z, t);
	
	switch (l.kind)
	{
		case RT_LightKind_Area_Sphere:
		{

			float3 sphere_sample       = SampleUnitSphere(r);
			float3 sphere_sample_local = (x*sphere_sample.x +
										  y*sphere_sample.y +
										  z*sphere_sample.z);
			float3 sphere_sample_world = sphere_sample_local + t;
			float3 sphere_normal = normalize(sphere_sample_local);

			float3 to_light = sphere_sample_world - world_p;

			L_dist_sq = max(min_distance_sq, dot(to_light, to_light));
			L_dist    = sqrt(L_dist_sq);
			L         = to_light*rcp(L_dist);

			float n_dot_v = dot(L, -sphere_normal);
			float attenuation = abs(n_dot_v); // TODO: abs is a hack.

			float v_dot_y = dot(L, normalize(y));
			float spot_distance = 0.5 + 0.5*v_dot_y; // spherical spotlights can "wrap around".
			attenuation *= 1.0 - smoothstep(angle_min, angle_max, spot_distance);

			float spot_vignette = max(0, v_dot_y);
			spot_vignette *= spot_vignette;
			spot_vignette *= spot_vignette;
			spot_vignette = lerp(1.0, spot_vignette, vignette_strength);

			attenuation *= spot_vignette;

			e = attenuation*emission;
		} break;

		case RT_LightKind_Area_Rect:
		{
			float3 rect_sample = x*(r.x - 0.5) + z*(r.y - 0.5) + t;
			float3 rect_normal = y;

			float3 to_light = rect_sample - world_p;
	
			L_dist_sq = max(min_distance_sq, dot(to_light, to_light));
			L_dist    = sqrt(L_dist_sq);
			L         = to_light*rcp(L_dist);

			float attenuation = max(0, dot(L, -rect_normal));

			float spot_vignette = attenuation;
			spot_vignette *= spot_vignette;
			spot_vignette *= spot_vignette;
			spot_vignette = lerp(1.0, spot_vignette, vignette_strength);

			attenuation *= spot_vignette;

			float3 x_n = normalize(x);
			float3 z_n = normalize(z);

			float3 light_d_rej_x = normalize(L - x_n*dot(L, x_n));
			float3 light_d_rej_z = normalize(L - z_n*dot(L, z_n));

			attenuation *= 1.0 - smoothstep(angle_min, angle_max, 1.0 - dot(light_d_rej_z, -rect_normal));
			attenuation *= 1.0 - smoothstep(angle_min, angle_max, 1.0 - dot(light_d_rej_x, -rect_normal));

			e = attenuation*emission;
		} break;
	}

	e *= rcp(L_dist_sq);
	
	result.L      = L;
	result.L_dist = L_dist;
	result.e      = e;
}

struct DirectLightingOutput
{
	float3 albedo;
	float3 direct_lighting;
	float3 direct_specular;
	float3 emissive_lighting;
};

void CalculateDirectLightingAtSurface(in HitGeometry IN, inout DirectLightingOutput OUT, uint2 pixel_pos, bool is_indirect)
{
	MaterialDesc material_desc = (MaterialDesc)0;
	material_desc.ior = 0.04;
	material_desc.albedo = IN.albedo;
	material_desc.metallic = IN.metallic;
	material_desc.roughness = IN.roughness;

	OUT.emissive_lighting = IN.emissive;

	if (HasHitGeometry(IN.vis_prim))
	{
		float3 unpacked_normal = DecodeNormalOctahedron(IN.normal);
		float3 geo_world_p = IN.world_p; // ReconstructWorldPosFromGBuffer(pixel_pos);

		if (!(all(material_desc.albedo == 0) && material_desc.metallic == 1))
		{
			uint lights_count = g_global_cb.lights_count;

			Reservoir r = (Reservoir)0;

			float W = 0;

			float2 rand;
			if (is_indirect)
			{
				rand = IntegerHash(uint3(pixel_pos, g_global_cb.frame_index)).xy;
			}
			else
			{
				rand.x = RandomSample(pixel_pos, Random_DirectJitterX);
				rand.y = RandomSample(pixel_pos, Random_DirectJitterY);
			}

			if (tweak.ris && (!is_indirect || tweak.ris_indirect))
			{
				if (tweak.ris == 2)
				{
					for (uint light_index = 0; light_index < lights_count; light_index++)
					{
						RT_Light l = g_lights[light_index];

						LightSample s;
						EvaluateLight(l, geo_world_p, rand, s);

						float3 brdf_diffuse, brdf_specular;
						EvaluateBRDF(-IN.view_dir, unpacked_normal, s.L, material_desc, brdf_diffuse, brdf_specular);

						float NoL = max(0, dot(unpacked_normal, s.L));

						float3 c = s.e*NoL;
						float3 e = c*brdf_diffuse + c*brdf_specular;

						Update(r, light_index, Luminance(e), pixel_pos);
					}

					W = r.w_y > 0.001 ? rcp(r.w_y)*r.w_sum : 0.0;
				}
				else if (tweak.ris == 1)
				{
					for (uint sample_index = 0; sample_index < min(tweak.ris_spp, lights_count); sample_index++)
					{
						uint light_index = min((uint)(lights_count*RandomSample(pixel_pos, Random_PickLight + 3*sample_index)), lights_count - 1);

						RT_Light l = g_lights[light_index];

						LightSample s;
						EvaluateLight(l, geo_world_p, rand, s);

						float3 brdf_diffuse, brdf_specular;
						EvaluateBRDF(-IN.view_dir, unpacked_normal, s.L, material_desc, brdf_diffuse, brdf_specular);

						float NoL = max(0, dot(unpacked_normal, s.L));

						float3 c = s.e*NoL;
						float3 e = c*brdf_diffuse + c*brdf_specular;

						Update(r, light_index, Luminance(e), pixel_pos);
					}

					W = r.w_y > 0.001 ? lights_count*rcp(r.w_y)*rcp(r.M)*r.w_sum : 0.0;
				}
			}
			else
			{
				r.y = min((uint)(lights_count*RandomSample(pixel_pos, Random_PickLight)), lights_count - 1);
				W = lights_count;
			}

			LightSample s;
			EvaluateLight(g_lights[r.y], geo_world_p, rand, s);
			
			float ndotl = max(0, dot(unpacked_normal, s.L));

			if (any(s.e > 0.001) && ndotl > 0.001)
			{
				RayDesc occlusion_ray;
				occlusion_ray.Origin = geo_world_p + 0.01 * unpacked_normal;
				occlusion_ray.Direction = s.L;
				occlusion_ray.TMin = RT_RAY_T_MIN;
				occlusion_ray.TMax = s.L_dist;

				OcclusionRayPayload occlusion_payload;
				occlusion_payload.visible = false;

				TraceOcclusionRay(occlusion_ray, occlusion_payload, pixel_pos);

				float3 c = occlusion_payload.visible * ndotl * s.e * W;

				float3 brdf_diffuse, brdf_specular;
				EvaluateBRDF(-IN.view_dir, unpacked_normal, s.L, material_desc, brdf_diffuse, brdf_specular);

				float direct_light_specular_weight = 1.0f;
				if (!is_indirect)
				{
					direct_light_specular_weight = smoothstep(tweak.direct_specular_threshold - 0.1,
															  tweak.direct_specular_threshold + 0.1,
															  material_desc.roughness);
				}

				OUT.direct_lighting += c * brdf_diffuse;
				OUT.direct_specular += c * brdf_specular * direct_light_specular_weight;
			}
		}
	}
	else
	{
		// Sky color! We have not hit any valid geometry
		float3 sky_color_top = g_global_cb.sky_color_top;//float3(0.51f, 0.80f, 0.92f);
		float3 sky_color_bot = g_global_cb.sky_color_bottom;//float3(0.15f, 0.35f, 0.65f);
		float3 sky_color = lerp(sky_color_top, sky_color_bot, abs(IN.view_dir.y));

		if (is_indirect)
		{
			OUT.emissive_lighting = sky_color;
			OUT.direct_lighting   = sky_color;
		}
	}

	OUT.albedo = IN.albedo;
}

#endif /* DIRECT_LIGHTING_HLSLI */
