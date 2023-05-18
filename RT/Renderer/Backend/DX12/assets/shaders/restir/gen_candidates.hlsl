#include "include/common.hlsl"
#include "direct_lighting.hlsli"

// groupshared Reservoir s_reservoirs[GROUP_Y][GROUP_X];
// groupshared uint      s_diff      [GROUP_Y][GROUP_X];
// groupshared uint      s_spec      [GROUP_Y][GROUP_X];

[numthreads(GROUP_X, GROUP_Y, 1)]
void ReSTIR_GenerateCandidates(COMPUTE_ARGS)
{
	if (any(pixel_pos >= g_global_cb.render_dim))
	{
		return;
	}

	{
		float3 I =  img_world_p     [pixel_pos].xyz;
		float3 N = DecodeNormalOctahedron(img_normal_depth[pixel_pos].xy);
		float3 V = -img_view_dir    [pixel_pos].xyz;

		uint material_index = img_material[pixel_pos];
		Material material = g_materials[material_index];

		MaterialDesc mat_desc;
		mat_desc.ior       = 0.04;
		mat_desc.albedo    = img_albedo   [pixel_pos].rgb;
		mat_desc.metallic  = img_metallic [pixel_pos].r;
		mat_desc.roughness = img_roughness[pixel_pos].r;

		uint light_count = g_global_cb.lights_count;
		uint x = RandomIndex(pixel_pos, light_count, Random_PickLight);

		RT_Light light = g_lights[x];

		float2 r_light = 
		{ 
			RandomSample(pixel_pos, Random_DirectJitterX), 
			RandomSample(pixel_pos, Random_DirectJitterY), 
		};

		LightSample s;
		EvaluateLight(light, I, r_light, s);

		float  NoL     = max(0, dot(N, s.L));
		float3 contrib = light_count*s.e*NoL;

		float3 diff = 0;
		float3 spec = 0;

		[branch]
		if (any(contrib > 0))
		{
			RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH|
				     RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES|
					 RAY_FLAG_CULL_BACK_FACING_TRIANGLES> q;

			RayDesc shadow_ray;
			shadow_ray.Origin    = I + 0.01*N;
			shadow_ray.Direction = s.L;
			shadow_ray.TMin      = RT_RAY_T_MIN;
			shadow_ray.TMax      = s.L_dist;

			q.TraceRayInline(g_scene, 0, 1, shadow_ray);
			q.Proceed();

			if (q.CommittedStatus() == COMMITTED_NOTHING)
			{
				float3 brdf_diffuse, brdf_specular;
				EvaluateBRDF(V, N, s.L, mat_desc, brdf_diffuse, brdf_specular);

				diff = contrib*brdf_diffuse;
				spec = contrib*brdf_specular;

				[branch]
				if (!tweak.reference_mode && tweak.svgf_enabled)
				{
					float3 reflectance = lerp(0.04, 0.1 + mat_desc.albedo, mat_desc.metallic);
					spec *= rcp(reflectance);
				}
			}
		}

		img_diff[pixel_pos].rgb = diff;
		img_spec[pixel_pos].rgb = spec;
	}
}
