#include "include/common.hlsl"

// ------------------------------------------------------------------
// This is a post temporal accumulation pass which stabilizes the 
// result and does antilag detection.

#define BORDER 1
#define SHARED_X (GROUP_X + (2*BORDER))
#define SHARED_Y (GROUP_X + (2*BORDER))

groupshared float3 shared_diff      [SHARED_Y][SHARED_X];
groupshared float3 shared_spec      [SHARED_Y][SHARED_X];

void Preload(uint2 shared_pos, uint2 global_pos)
{
	shared_diff[shared_pos.y][shared_pos.x] = img_diff_denoise_ping[global_pos].rgb;
	shared_spec[shared_pos.y][shared_pos.x] = img_spec_denoise_ping[global_pos].rgb;
}

[numthreads(GROUP_X, GROUP_Y, 1)]
void Denoise_PostResample(COMPUTE_ARGS)
{
	PRELOAD_INTO_SHARED

	// ------------------------------------------------------------------

	int2 shared_pos = thread_pos + BORDER;

	float3 diff_curr = shared_diff[shared_pos.y][shared_pos.x];
	float3 diff_m1 = diff_curr;
	float3 diff_m2 = diff_curr*diff_curr;

	float3 spec_curr = shared_spec[shared_pos.y][shared_pos.x];
	float3 spec_m1 = spec_curr;
	float3 spec_m2 = spec_curr*spec_curr;

	for (int y = 0; y <= 2*BORDER; y++)
	for (int x = 0; x <= 2*BORDER; x++)
	{
		if (x == BORDER && y == BORDER)
			continue;

		int2 pos = thread_pos + int2(x, y);

		float3 d = shared_diff[pos.y][pos.x];
		diff_m1 += d;
		diff_m2 += d*d;

		float3 s = shared_spec[pos.y][pos.x];
		spec_m1 += s;
		spec_m2 += s*s;
	}

	diff_m1 /= (2*BORDER + 1)*(2*BORDER + 1);
	diff_m2 /= (2*BORDER + 1)*(2*BORDER + 1);

	spec_m1 /= (2*BORDER + 1)*(2*BORDER + 1);
	spec_m2 /= (2*BORDER + 1)*(2*BORDER + 1);

	float2 motion  = img_motion[pixel_pos].xy;
	float2 uv      = (float2(pixel_pos) + 0.5) / float2(g_global_cb.render_dim);
	float2 uv_prev = uv + motion;

	float3 diff_prev = diff_curr;
	float3 spec_prev = spec_curr;
	if (all(and(uv_prev >= 0, uv_prev < 1)))
	{
		if (tweak.diff_stabilize_sharp)
		{
			diff_prev = max(0, SampleTextureCatmullRom(tex_diff_stable_hist, g_sampler_linear_clamp, uv_prev, g_global_cb.render_dim).rgb);
		}
		else
		{
			diff_prev = tex_diff_stable_hist.SampleLevel(g_sampler_linear_clamp, uv_prev, 0).rgb;
		}

		if (tweak.spec_stabilize_sharp)
		{
			spec_prev = max(0, SampleTextureCatmullRom(tex_spec_stable_hist, g_sampler_linear_clamp, uv_prev, g_global_cb.render_dim).rgb);
		}
		else
		{
			spec_prev = tex_spec_stable_hist.SampleLevel(g_sampler_linear_clamp, uv_prev, 0).rgb;
		}
	}

	float3 diff_sigma = max(0, sqrt(diff_m2 - diff_m1*diff_m1));
	float3 diff_min = min(diff_curr, diff_m1 - tweak.diff_stabilize_gamma*diff_sigma);
	float3 diff_max = max(diff_curr, diff_m1 + tweak.diff_stabilize_gamma*diff_sigma);

	diff_prev = clamp(diff_prev, diff_min, diff_max);

	float3 spec_sigma = max(0, sqrt(spec_m2 - spec_m1*spec_m1));
	float3 spec_min = min(spec_curr, spec_m1 - tweak.spec_stabilize_gamma*spec_sigma);
	float3 spec_max = max(spec_curr, spec_m1 + tweak.spec_stabilize_gamma*spec_sigma);

	spec_prev = clamp(spec_prev, spec_min, spec_max);

	float2 t = 
	{
		tweak.diff_stabilize_alpha,
		tweak.spec_stabilize_alpha,
	};

	float3 diff = lerp(diff_prev, diff_curr, t.x);
	float3 spec = lerp(spec_prev, spec_curr, t.y);

	img_diff_stable[pixel_pos].rgb = diff;
	img_spec_stable[pixel_pos].rgb = spec;

	//------------------------------------------------------------------------
	// Anti-Lag

	if (tweak.svgf_antilag)
	{
		float2 hist_len = img_history_length[pixel_pos];

		float2 w =
		{
			CalculateLumaWeight(diff_m1, diff_prev),
			CalculateLumaWeight(spec_m1, spec_prev),
		};
		w *= w;
		w *= w;
		w *= w;

		float2 hist_max =
		{
			tweak.svgf_max_hist_len_diff,
			tweak.svgf_max_hist_len_spec,
		};

		float2 hist_min =
		{
			3,
			3,
		};

		float2 antilag_hist_max = lerp(hist_min, hist_max, w);
		img_history_length[pixel_pos] = min(hist_len, antilag_hist_max);
	}
}
