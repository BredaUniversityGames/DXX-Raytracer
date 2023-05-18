#include "include/common.hlsl"

// ------------------------------------------------------------------
// This file should do something like:
// 1) Sample history length
// 2) If history length is less than some amount, do some fast 
//    additional blur to avoid the main denoise pass from having
//    to eat overly noisy input.

[numthreads(GROUP_X, GROUP_Y, 1)]
void Denoise_HistoryFix(COMPUTE_ARGS)
{
	uint diff_hist_max = tweak.svgf_max_hist_len_diff;
	uint diff_hist_len = img_history_length[pixel_pos].x;

	uint hist_fix_max = min(tweak.svgf_history_fix_frames, diff_hist_max);
	uint hist_fix_len = min(diff_hist_len, hist_fix_max);

	float3 center_norm  = DecodeNormalOctahedron(img_normal[pixel_pos].xy);
	float3 center_world = ReconstructWorldPosFromGBuffer(pixel_pos);

	float3 diff   = img_diff_denoise_pong[pixel_pos].rgb;
	float  diff_w = 1;
	float3 spec   = img_spec_denoise_pong[pixel_pos].rgb;
	float  spec_w = 1;

	if (hist_fix_len < hist_fix_max)
	{
		float scale = tweak.svgf_history_fix_scale / (1.0 + hist_fix_len);

		for (int y = -2; y <= 2; y++)
		for (int x = -2; x <= 2; x++)
		{
			if (x == 0 || y == 0)
				continue;

			int2 pos = round(pixel_pos + scale*float2(x, y));

			float3 norm  = DecodeNormalOctahedron(img_normal[pos].xy);
			float3 world = ReconstructWorldPosFromGBuffer(pos);

			float plane_distance = abs(dot(center_norm, center_world) - dot(center_norm, world));
			float disocclusion_threshold = 1.0f;

			float3 d = img_diff_denoise_pong[pos].rgb;
			float3 s = img_spec_denoise_pong[pos].rgb;

			float w = 1;

			float w_norm = dot(center_norm, norm);
			w_norm *= w_norm;
			w_norm *= w_norm;
			w_norm *= w_norm;

			w *= w_norm;
			w *= plane_distance < disocclusion_threshold;

			diff   += w*d;
			diff_w += w;

			spec   += w*s;
			spec_w += w;
		}

		diff *= rcp(diff_w);
		spec *= rcp(spec_w);
	}

	img_diff_denoise_ping[pixel_pos].rgb = diff;
	img_spec_denoise_ping[pixel_pos].rgb = spec;
}
