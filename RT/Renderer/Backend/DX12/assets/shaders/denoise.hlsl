#include "include/common.hlsl"

struct DenoiseCB
{
	uint iteration;
};

ConstantBuffer<DenoiseCB> g_denoise_cb : register(b0, space2);

void DoSvgfFilterStep(int2 center_pos,
					  RWTexture2D<float4> source_diff,
					  RWTexture2D<float4> source_spec,
					  RWTexture2D<float4> source_moments,
					  out float3 result_direct,
					  out float3 result_spec,
					  out float4 result_moments)
{
	float2 dim = g_global_cb.render_dim;
	float2 pix_size = 1.0 / dim;

	float3 center_diff         = source_diff     [center_pos].rgb;
	float3 center_spec         = source_spec     [center_pos].rgb;
	float3 center_normal = DecodeNormalOctahedron(img_normal[center_pos].xy);
	float  center_depth  = img_depth[center_pos].x;

	float center_metallic = img_metallic[center_pos].x;
	uint  center_material = img_material[center_pos].x;

	int step = g_denoise_cb.iteration;
	int step_size = 1L << step;

	float2 jitter = {
		tweak.svgf_jitter*(RandomSample(center_pos, Random_DenoiseJitterX + 3*step) - 0.5),
		tweak.svgf_jitter*(RandomSample(center_pos, Random_DenoiseJitterY + 3*step) - 0.5),
	};

	int radius = 1;

	float lum_diff_mean  = 0;
	float lum_diff_sigma = 0;

	float lum_spec_mean  = 0;
	float lum_spec_sigma = 0;

	float4 center_moments = source_moments[center_pos];

	float hist_len_diff = img_history_length[center_pos].x;
	float hist_len_spec = img_history_length[center_pos].y;

	float kernel[2][2] =
	{
		{ 1.0,       1.0 / 2.0, },
		{ 1.0 / 2.0, 1.0 / 4.0, },
	};
	
	int fix_frames = min(tweak.svgf_history_fix_frames, tweak.svgf_max_hist_len_diff);
	if (hist_len_diff >= fix_frames)
	{
		float lum_variance = 0;

		float2 moments = center_moments.xy;
		lum_diff_mean  = moments.x;
		lum_variance = moments.y - moments.x*moments.x;

		float hist_norm = fix_frames > 0 ? (hist_len_diff - fix_frames) / (tweak.svgf_max_hist_len_diff - fix_frames) : 1;
		lum_diff_sigma = tweak.svgf_luma_sigma_diff*hist_norm / max(0.0001, sqrt(lum_variance));
	}

	if (hist_len_spec >= fix_frames)
	{
		float lum_variance = 0;

		float2 moments = center_moments.zw;

		lum_spec_mean = moments.x;
		lum_variance  = moments.y - moments.x*moments.x;

		float hist_norm = fix_frames > 0 ? (hist_len_spec - fix_frames) / (tweak.svgf_max_hist_len_spec - fix_frames) : 1;
		lum_spec_sigma = tweak.svgf_luma_sigma_spec*hist_norm / max(0.0001, sqrt(lum_variance));
	}

	float  sum_diff_w = 1;
	float  sum_spec_w = 1;

	float3 sum_diff    = sum_diff_w*center_diff;
	float3 sum_spec    = sum_spec_w*center_spec;
	float4 sum_moments = float4(sum_diff_w.xx, sum_spec_w.xx)*center_moments;

	[unroll]
	for (int x = -radius; x <= radius; x++)
	{
		[unroll]
		for (int y = -radius; y <= radius; y++)
		{
			if (x == 0 && y == 0)
				continue;

			int2 sample_pos = round(float2(center_pos.x + step_size*x, center_pos.y + step_size*y) + step_size*jitter);

			float3 sample_diff    = source_diff   [sample_pos].rgb;
			float3 sample_spec    = source_spec   [sample_pos].rgb;
			float4 sample_moments = source_moments[sample_pos];
			float  sample_diff_lum  = Luminance(sample_diff);

			float3 sample_normal = DecodeNormalOctahedron(img_normal[sample_pos].xy);
			float  sample_depth  = img_depth[sample_pos].x;

			float sample_metallic = img_metallic[sample_pos].x;
			uint  sample_material = img_material[sample_pos].x;

			float w = kernel[abs(x)][abs(y)];
			float depth_dist    = abs(center_depth - sample_depth);
			float normal_dist   = max(0, dot(center_normal, sample_normal));
			float metallic_dist = abs(center_metallic - sample_metallic);

			w *= min(exp(-(depth_dist / square(tweak.svgf_depth_sigma))), 1.0);
			w *= pow(normal_dist, tweak.svgf_normal_sigma);
			w *= min(exp(-(metallic_dist / square(tweak.svgf_metallic_sigma))), 1.0);
			w *= (sample_material == center_material);

			float lum_diff_dist = abs(lum_diff_mean - sample_diff_lum);
			float w_lum_diff = exp(-square(lum_diff_dist)*lum_diff_sigma);
			float w_diff = w*w_lum_diff;

			float lum_spec_dist = abs(lum_spec_mean - Luminance(sample_spec));
			float w_lum_spec = exp(-lum_spec_dist*lum_spec_sigma);
			float w_spec = w*w_lum_spec;

			sum_diff       += w_diff*sample_diff;
			sum_spec       += w_spec*sample_spec;
			sum_moments.xy += w_diff*sample_moments.xy;
			sum_moments.zw += w_spec*sample_moments.zw;
			sum_diff_w     += w_diff;
			sum_spec_w     += w_spec;
		}
	}

	if (sum_diff_w > 0.001)
	{
		sum_diff       *= rcp(sum_diff_w);
		sum_moments.xy *= rcp(sum_diff_w);
	}

	if (sum_spec_w > 0.001)
	{
		sum_spec       *= rcp(sum_spec_w);
		sum_moments.zw *= rcp(sum_spec_w);
	}

	result_direct  = sum_diff;
	result_spec    = sum_spec;
	result_moments = sum_moments;
}

[numthreads(GROUP_X, GROUP_Y, 1)]
void DenoiseDirectCS(int2 pixel_pos : SV_DispatchThreadID)
{
	EARLY_OUT

	float3 filtered_direct, filtered_spec;
	float4 filtered_moments;
	switch (g_denoise_cb.iteration)
	{
		case 0: DoSvgfFilterStep(pixel_pos, img_diff_denoise_ping, img_spec_denoise_ping, img_moments,              filtered_direct, filtered_spec, filtered_moments); break;
		case 1: DoSvgfFilterStep(pixel_pos, img_diff_denoise_pong, img_spec_denoise_pong, img_moments_hist,         filtered_direct, filtered_spec, filtered_moments); break;
		case 2: DoSvgfFilterStep(pixel_pos, img_diff_hist,         img_spec_hist,         img_moments_denoise_pong, filtered_direct, filtered_spec, filtered_moments); break;
		case 3: DoSvgfFilterStep(pixel_pos, img_diff_denoise_ping, img_spec_denoise_ping, img_moments_denoise_ping, filtered_direct, filtered_spec, filtered_moments); break;
		case 4: DoSvgfFilterStep(pixel_pos, img_diff_denoise_pong, img_spec_denoise_pong, img_moments_denoise_pong, filtered_direct, filtered_spec, filtered_moments); break;
	}

	switch (g_denoise_cb.iteration)
	{
		case 0: 
		{
			img_diff_denoise_pong[pixel_pos].rgb = filtered_direct; 
			img_spec_denoise_pong[pixel_pos].rgb = filtered_spec;
			img_moments_hist[pixel_pos] = filtered_moments;
		} break;

		case 1: 
		{
			img_diff_hist[pixel_pos].rgb = filtered_direct; 
			img_spec_hist[pixel_pos].rgb = filtered_spec;
			img_moments_denoise_pong[pixel_pos] = filtered_moments;
		} break;

		case 2: 
		{
			img_diff_denoise_ping[pixel_pos].rgb = filtered_direct; 
			img_spec_denoise_ping[pixel_pos].rgb = filtered_spec;
			img_moments_denoise_ping[pixel_pos] = filtered_moments;
		} break;

		case 3: 
		{
			img_diff_denoise_pong[pixel_pos].rgb = filtered_direct; 
			img_spec_denoise_pong[pixel_pos].rgb = filtered_spec;
			img_moments_denoise_pong[pixel_pos] = filtered_moments;
		} break;

		case 4: 
		{
			img_diff_denoise_ping[pixel_pos].rgb = filtered_direct; 
			img_spec_denoise_ping[pixel_pos].rgb = filtered_spec;
			img_moments_denoise_ping[pixel_pos] = filtered_moments;
		} break;
	}
}

