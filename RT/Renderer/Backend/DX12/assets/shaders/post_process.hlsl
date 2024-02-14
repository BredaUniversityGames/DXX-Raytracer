#include "include/common.hlsl"

//------------------------------------------------------------------------

float LinearTosRGB(float x)
{
	return x <= 0.0031308 ? x*12.92 : pow(1.055*x, 1.0 / 2.4) - 0.055;
}

float3 LinearTosRGB(float3 x)
{
	return float3(LinearTosRGB(x.x), LinearTosRGB(x.y), LinearTosRGB(x.z));
}

float NaturalShoulder(float x)
{
    return 1.0 - exp(-x);
}

float NaturalShoulder(float x, float t)
{
    float v1 = x;
    float v2 = t + (1.0 - t) * NaturalShoulder((x - t) / (1.0 - t));
    return x <= t ? v1 : v2;
}

float3 NaturalShoulder(float3 x, float t)
{
    return float3(
        NaturalShoulder(x.x, t),
        NaturalShoulder(x.y, t),
        NaturalShoulder(x.z, t)
    );
}

float3 ApplyTonemappingCurve(float3 color)
{
    return NaturalShoulder(color.xyz, tweak.tonemap_linear_section) * rcp(NaturalShoulder(tweak.tonemap_whitepoint, tweak.tonemap_linear_section));
}

float3 ColorPreservingTonemap(float3 color)
{
    float3 per_channel = ApplyTonemappingCurve(color.xyz);

    float peak = max(color.x, max(color.y, color.z));
    color.xyz *= rcp(peak + 1e-6);
    color.xyz *= ApplyTonemappingCurve(peak);

    color.xyz = lerp(color.xyz, per_channel, tweak.tonemap_hue_shift);

    return color;
}

[numthreads(GROUP_X, GROUP_Y, 1)]
void PostProcessCS(COMPUTE_ARGS)
{
	int2 co = pixel_pos;
	float2 uv = (float2(co) + 0.5) / float2(g_global_cb.output_dim);

	int2 co_render = int2(float2(pixel_pos) * (float2(g_global_cb.render_dim) / float2(g_global_cb.output_dim)));

	float3 color = tex_taa_result[co].rgb;

	// Adding the color overlay for picking stuff up/damaged to the final color.

	if (tweak.motion_blur_quality > 0)
	{
		// TODO(daniel): Think about per-object motion blur extending out from the object silhouette

		float2 motion = img_motion[co_render].xy;
		float  jitter = RandomSample(co, Random_MotionBlur);

		uint sample_count = (1U << (tweak.motion_blur_quality + 1));
		for (uint i = 0; i < sample_count; i++)
		{
			float  t      = ((float(i) + 1.0 - tweak.motion_blur_jitter*jitter) / float(sample_count));
			float2 offset = t*motion;

			offset = lerp(offset, offset*abs(offset), tweak.motion_blur_curve);

			color += tex_taa_result.SampleLevel(g_sampler_linear_clamp, uv + offset, 0).rgb;
		}
		color *= rcp(1 + sample_count);
	}

	float3 bloom0 = tex_bloom0.SampleLevel(g_sampler_linear_clamp, uv, 0).rgb;
	float3 bloom1 = tex_bloom1.SampleLevel(g_sampler_linear_clamp, uv, 0).rgb;
	float3 bloom2 = tex_bloom2.SampleLevel(g_sampler_linear_clamp, uv, 0).rgb;
	float3 bloom3 = tex_bloom3.SampleLevel(g_sampler_linear_clamp, uv, 0).rgb;
	float3 bloom4 = tex_bloom4.SampleLevel(g_sampler_linear_clamp, uv, 0).rgb;
	float3 bloom5 = tex_bloom5.SampleLevel(g_sampler_linear_clamp, uv, 0).rgb;
	float3 bloom6 = tex_bloom6.SampleLevel(g_sampler_linear_clamp, uv, 0).rgb;
	float3 bloom7 = tex_bloom7.SampleLevel(g_sampler_linear_clamp, uv, 0).rgb;
	float3 bloom = (bloom0 + bloom1 + bloom2 + bloom3 + bloom4 + bloom5 + bloom6 + bloom7)*(1.0 / 8.0);

	float aspect = float(g_global_cb.render_dim.y) / float(g_global_cb.render_dim.x);

	float2 uv_sq = 2.0*uv - 1.0;
	uv_sq.y *= aspect;
	uv_sq *= tweak.vignette_scale;

	float vignette = dot(uv_sq, uv_sq);
	//vignette *= vignette;

	vignette = lerp(1, saturate(1.0 - vignette), tweak.vignette_strength);
	color *= vignette;

	switch (tweak.bloom_blend_mode)
	{
		case 0: // Lerp
		{
			color = lerp(color, bloom, tweak.bloom_amount);
		} break;

		case 1: // Additive
		{
			color += tweak.bloom_amount*bloom;
		} break;

		case 2: // Subtractive
		{
			color -= max(0, tweak.bloom_amount*(bloom - color));
		} break;
	}

	//------------------------------------------------------------------------
	// Tonemap and sRGB

	color *= exp2(tweak.exposure);

	color = ColorPreservingTonemap(color);
	color = LinearTosRGB(color);

	// NOTE(daniel): Moved up here from where it was before, because it was being applied
	// over top the debug display which isn't helpful. Adding it after tonemap/srgb conversion
	// is maybe questionable but it probably matches the game's original rendering better
	color += g_global_cb.screen_color_overlay.xyz;

	//------------------------------------------------------------------------
	// Debug texture view

	float3 debug_color = color;
	float debug_blend_factor = tweak.debug_render_blend_factor;

	switch (g_global_cb.debug_render_mode)
	{
		case RT_DebugRenderMode_Debug:
		{
			debug_color = img_debug[co_render].xyz;
			debug_blend_factor = img_debug[co_render].w;
		} break;

		case RT_DebugRenderMode_Normals:
		{
			float3 normal = DecodeNormalOctahedron(img_normal[co_render].xy);
			debug_color = 0.5*normal + 0.5;
		} break;

		case RT_DebugRenderMode_Depth:
		{
			float depth = img_depth[co_render].r;
			debug_color = depth / 1000.0f; // TODO: Specify depth scale
		} break;

		case RT_DebugRenderMode_Albedo:
		{
			debug_color = img_albedo[co_render].rgb;
		} break;

		case RT_DebugRenderMode_Emissive:
		{
			debug_color = img_emissive[co_render].rgb;
		} break;

		case RT_DebugRenderMode_Diffuse:
		{
			float3 direct = tweak.svgf_stabilize ? img_diff_stable[co_render].rgb : img_diff_denoise_ping[co_render].rgb;
			debug_color = direct.rgb;
		} break;

		case RT_DebugRenderMode_Specular:
		{
			float3 spec = tweak.svgf_stabilize ? img_spec_stable[co_render].rgb : img_spec_denoise_ping[co_render].rgb;
			debug_color = spec;
		} break;

		case RT_DebugRenderMode_Motion:
		{
			float4 motion = img_motion[co_render];
			debug_color = float3(abs(motion.xy) * 100.0, 0);
		} break;

		case RT_DebugRenderMode_MetallicRoughness:
		{
			float metallic = img_metallic[co_render].x;
			float roughness = img_roughness[co_render].x;
			debug_color = float3(metallic, roughness, 0);
		} break;

		case RT_DebugRenderMode_HistoryLength:
		{
			float2 history_length = img_history_length[co_render];

			float2 hist_max = 
			{
				tweak.svgf_max_hist_len_diff,
				tweak.svgf_max_hist_len_spec,
			};

			debug_color = float3(history_length / hist_max, 0);
		} break;

		case RT_DebugRenderMode_Materials:
		{
			static const float3 colors[] = {
				{ 1, 0, 0 },
				{ 0, 1, 0 },
				{ 0, 0, 1 },
				{ 1, 1, 0 },
				{ 1, 0, 1 },
				{ 0, 1, 1 },
			};

			uint  material_index = img_material[co_render].x;
			debug_color = colors[material_index % 6];

			if ((material_index / 6) % 2 == 1)
			{
				debug_color *= 0.5;
			}
		} break;

		case RT_DebugRenderMode_FirstMoment:
		{
			float4 moments = img_moments[co_render];
			debug_color = moments.x;
		} break;

		case RT_DebugRenderMode_SecondMoment:
		{
			float4 moments = img_moments[co_render];
			debug_color = moments.y;
		} break;

		case RT_DebugRenderMode_Variance:
		{
			float4 moments = img_moments[co_render];
			debug_color = sqrt(max(0.001, moments.y - square(moments.x)));
		} break;

		case RT_DebugRenderMode_Bloom0:
		{
			debug_color = bloom0;
		} break;

		case RT_DebugRenderMode_Bloom1:
		{
			debug_color = bloom1;
		} break;

		case RT_DebugRenderMode_Bloom2:
		{
			debug_color = bloom2;
		} break;

		case RT_DebugRenderMode_Bloom3:
		{
			debug_color = bloom3;
		} break;

		case RT_DebugRenderMode_Bloom4:
		{
			debug_color = bloom4;
		} break;

		case RT_DebugRenderMode_Bloom5:
		{
			debug_color = bloom5;
		} break;

		case RT_DebugRenderMode_Bloom6:
		{
			debug_color = bloom6;
		} break;

		case RT_DebugRenderMode_Bloom7:
		{
			debug_color = bloom7;
		} break;
	}

	float3 final_color = lerp(color, debug_color, debug_blend_factor);
	img_postfx[co] = float4(final_color, 1.0);
}