#include "include/common.hlsl"

// ------------------------------------------------------------------
// This file should resample the _blurred_ result, it will combine it
// with the (maybe prefiltered) input of the current frame, producing 
// the input for the subsequent blur pass.

#define BORDER 1
#define SHARED_X (GROUP_X + (2*BORDER))
#define SHARED_Y (GROUP_X + (2*BORDER))

groupshared float4 shared_diff   [SHARED_Y][SHARED_X];
groupshared float4 shared_spec   [SHARED_Y][SHARED_X];
groupshared float3 shared_norm   [SHARED_Y][SHARED_X];
groupshared float3 shared_world_p[SHARED_Y][SHARED_X];

void Preload(uint2 shared_pos, uint2 global_pos)
{
	float3 hf       = img_diff[global_pos].rgb;
	float  hf_lum   = Luminance(hf);
	float3 spec     = img_spec[global_pos].rgb;
	float  spec_lum = Luminance(spec);
	shared_diff   [shared_pos.y][shared_pos.x] = float4(hf, hf_lum);
	shared_spec   [shared_pos.y][shared_pos.x] = float4(spec, spec_lum);
	shared_norm	  [shared_pos.y][shared_pos.x] = DecodeNormalOctahedron(img_normal[global_pos].rg);
	shared_world_p[shared_pos.y][shared_pos.x] = ReconstructWorldPosFromGBuffer(global_pos);
}

[numthreads(GROUP_X, GROUP_Y, 1)]
void Denoise_Resample(COMPUTE_ARGS)
{
	PRELOAD_INTO_SHARED

	// ------------------------------------------------------------------

	int2 shared_pos = thread_pos + BORDER;

	float2 motion = g_global_cb.render_dim*img_motion[pixel_pos].xy;

	float3 curr_norm  = shared_norm   [shared_pos.y][shared_pos.x].xyz;
	float3 curr_world = shared_world_p[shared_pos.y][shared_pos.x].xyz;

	// ------------------------------------------------------------------
	// Gather moments

	float  center_hf_lum   = shared_diff[shared_pos.y][shared_pos.x].w;
	float  center_spec_lum = shared_spec[shared_pos.y][shared_pos.x].w;
	float  curr_moments_w  = 1; // g_gaussian_3x3[1][1];
	float4 curr_moments    = curr_moments_w*float4(center_hf_lum, center_hf_lum*center_hf_lum,
												   center_spec_lum, center_spec_lum*center_spec_lum);

	for (int y = 0; y < 2*BORDER + 1; y++)
	for (int x = 0; x < 2*BORDER + 1; x++)
	{
		if (and(x == BORDER, y == BORDER))
			continue;

		int2  pos = thread_pos + int2(x, y);
		float w   = 1; //g_gaussian_3x3[y][x];

		float3 norm  = shared_norm   [pos.y][pos.x].xyz;
		float3 world = shared_world_p[pos.y][pos.x].xyz;

		float plane_distance = abs(dot(curr_norm, curr_world) - dot(curr_norm, world));
		float disocclusion_threshold = 1.0f;

		float w_norm = dot(curr_norm, norm);
		w_norm *= w_norm;
		w_norm *= w_norm;
		w_norm *= w_norm;

		w *= w_norm;
		w *= plane_distance < disocclusion_threshold;

		float  lum_hf   = shared_diff[pos.y][pos.x].w;
		float  lum_spec = shared_spec[pos.y][pos.x].w;
		float4 moments = { lum_hf, lum_hf*lum_hf, lum_spec, lum_spec*lum_spec };

		curr_moments   += w*moments;
		curr_moments_w += w;
	}

	curr_moments *= rcp(curr_moments_w);

	// ------------------------------------------------------------------
	// Resample

	float2 reprojected_pos = float2(pixel_pos) + motion;

	BilinearSamples samples;
	GetBilinearSamples(reprojected_pos, samples);

	float3 hist_diff = 0;
	float3 hist_spec    = 0;
	float4 hist_moments = 0;
	float2 hist_len     = 0;
	float  hist_w       = 0;

	for (int i = 0; i < 4; i++)
	{
		int2  pos = samples.positions[i];
		float w   = samples.weights  [i];

		if (any(or(pos < 0, pos >= g_global_cb.render_dim)))
			continue;

		int2 shared_pos = pos - thread_pos;

		float3 norm  = DecodeNormalOctahedron(img_normal_prev[pos].xy);
		float3 world = ReconstructPrevWorldPosFromGBuffer(pos);
		uint2  len   = img_history_length[pos];

		float plane_distance = abs(dot(curr_norm, curr_world) - dot(curr_norm, world));
		float disocclusion_threshold = 1.0f;

		w *= pow(dot(curr_norm, norm), 8);
		w *= plane_distance < disocclusion_threshold;

		float3 diff    = img_diff_hist   [pos].rgb;
		float3 spec    = img_spec_hist   [pos].rgb;
		float4 moments = img_moments_hist[pos];

		hist_diff    += w*diff;
		hist_spec    += w*spec;
		hist_moments += w*moments;
		hist_len     += w*len;
		hist_w       += w;
	}

	float3 curr_diff = shared_diff[shared_pos.y][shared_pos.x].rgb;
	float3 curr_spec = shared_spec[shared_pos.y][shared_pos.x].rgb;

	float3 result_diff    = curr_diff;
	float3 result_spec    = curr_spec;
	float4 result_moments = curr_moments;

	if (hist_w > 0.001)
	{
		float rcp_hist_w = rcp(hist_w);
		hist_diff *= rcp_hist_w;
		hist_spec    *= rcp_hist_w;
		hist_moments *= rcp_hist_w;
		hist_len     *= rcp_hist_w;

		uint2 hist_max = 
		{
			tweak.svgf_max_hist_len_diff,
			tweak.svgf_max_hist_len_spec,
		};

		hist_len = min(hist_len + 1, hist_max);
		float2 alpha = 1.0 / float2(hist_len);

		result_diff       = lerp(hist_diff, curr_diff, alpha.x);
		result_spec       = lerp(hist_spec, curr_spec, alpha.y);
		result_moments.xy = lerp(hist_moments.xy, curr_moments.xy, alpha.x);
		result_moments.zw = lerp(hist_moments.zw, curr_moments.zw, alpha.y);
	}
	else
	{
		hist_len = 0;
	}

	img_diff_denoise_ping[pixel_pos].rgb = result_diff;
	img_spec_denoise_ping[pixel_pos].rgb = result_spec;
	img_moments          [pixel_pos]     = result_moments;
	img_history_length   [pixel_pos]     = hist_len;
}
