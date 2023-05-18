#include "include/common.hlsl"

//------------------------------------------------------------------------

// Temporal Reprojection Anti-Aliasing in INSIDE - Pedersen
float3 ClipAABB(float3 aabb_min, float3 aabb_max, float3 p, float3 q)
{
	float3 p_clip = 0.5*(aabb_max + aabb_min);
	float3 e_clip = 0.5*(aabb_max - aabb_min);

	float3 v_clip = q - p_clip;
	float3 v_unit = v_clip.xyz / e_clip;
	float3 a_unit = abs(v_unit);
	float ma_unit = max3(a_unit);

	if (ma_unit > 1.0)
	{
		return p_clip + v_clip / ma_unit;
	}
	else
	{
		return q;
	}
}

#define BORDER 1
#define SHARED_X (GROUP_X + 2*BORDER)
#define SHARED_Y (GROUP_Y + 2*BORDER)

groupshared float  shared_depth [SHARED_Y][SHARED_X];
groupshared float3 shared_color [SHARED_Y][SHARED_X];
groupshared float2 shared_motion[SHARED_Y][SHARED_X];

void Preload(uint2 shared_pos, uint2 global_pos)
{
	shared_depth [shared_pos.y][shared_pos.x] = img_depth		[global_pos].x;
	shared_color [shared_pos.y][shared_pos.x] = img_color       [global_pos].rgb;
	shared_motion[shared_pos.y][shared_pos.x] = img_motion      [global_pos].xy;
}

[numthreads(GROUP_X, GROUP_Y, 1)]
void TemporalAntiAliasingCS(COMPUTE_ARGS)
{
	//------------------------------------------------------------------------
	// Preload

	PRELOAD_INTO_SHARED

	//------------------------------------------------------------------------

	float2 dim = g_global_cb.render_dim;

	float3 neighborhood_min = LARGE_NUMBER;
	float3 neighborhood_max = 0;

	float2 motion = 0;

	float3 m1 = 0; // 1st raw moment
	float3 m2 = 0; // 2nd raw moment

	float3 curr_color = 0;

	float closest_depth = LARGE_NUMBER;
	for (int y = 0; y <= 2*BORDER; y++)
	for (int x = 0; x <= 2*BORDER; x++)
	{
		int2 shared_pos = thread_pos + int2(x, y);
		float  sample_depth = shared_depth[shared_pos.y][shared_pos.x];
		float3 sample_color = shared_color[shared_pos.y][shared_pos.x];

		if (and(x == BORDER, y == BORDER))
		{
			curr_color = sample_color;
		}

		neighborhood_min = min(neighborhood_min, sample_color);
		neighborhood_max = max(neighborhood_max, sample_color);

		if (and(abs(x - BORDER) <= 1, abs(y - BORDER) <= 1))
		{
			if (sample_depth < closest_depth)
			{
				motion        = shared_motion[shared_pos.y][shared_pos.x];
				closest_depth = sample_depth;
			}
		}

		m1 += sample_color;
		m2 += sample_color*sample_color;
	}

	motion *= dim;

	m1 *= 1.0 / ((2*BORDER + 1)*(2*BORDER + 1));
	m2 *= 1.0 / ((2*BORDER + 1)*(2*BORDER + 1));

	//------------------------------------------------------------------------
	// Sample reprojected pixel

	float2 prev_pos = pixel_pos + motion;
	float3 prev_color;
	
	if (tweak.taa_catmull_rom)
	{
		prev_color = SampleTextureCatmullRom(tex_taa_history, g_sampler_linear_clamp, (prev_pos + 0.5) / dim, dim).rgb;
		prev_color = max(0, prev_color);
	}
	else
	{
		prev_color = tex_taa_history.SampleLevel(g_sampler_linear_clamp, (prev_pos + 0.5) / dim, 0).rgb;
	}

	if (any(or(prev_pos <= 0, prev_pos >= dim - 1)))
	{
		prev_color = curr_color;
	}

	// Destroy NaNs
	if (any(prev_color != prev_color))
	{
		prev_color = curr_color;
	}

	//------------------------------------------------------------------------
	// Clip or clamp reprojected pixel to min/max of colors in the 
	// neighborhood of our center pixel to avoid pixels divering too much and 
	// causing ghosting.

	uint neighborhood_mode = tweak.taa_neighborhood_mode;

	switch (neighborhood_mode)
	{
		case TaaNeighborhoodMode_Clamp: 
		{
			prev_color = clamp(prev_color, neighborhood_min, neighborhood_max);
		} break;

		case TaaNeighborhoodMode_Clip: 
		{
			prev_color = ClipAABB(neighborhood_min, neighborhood_max, curr_color, prev_color);
		} break;

		case TaaNeighborhoodMode_VarianceClip:
		{
			float  gamma = tweak.taa_variance_clip_gamma;
			float3 sigma = max(0, sqrt(m2 - m1*m1));

			float3 color_min = m1 - gamma*sigma;
			float3 color_max = m1 + gamma*sigma;

#define CLAMP_AGAINST_OLD_AABB 0
#if CLAMP_AGAINST_OLD_AABB
			color_min = max(color_min, neighborhood_min);
			color_max = min(color_max, neighborhood_max);
#endif

			prev_color = ClipAABB(color_min, color_max, curr_color, prev_color);
		} break;
	}

	//------------------------------------------------------------------------
	// Blend

	float luma_weight = CalculateLumaWeight(curr_color, prev_color);
	float luma_weight_sqr = luma_weight*luma_weight;

	float feedback_min = tweak.taa_feedback_min;
	float feedback_max = tweak.taa_feedback_max;

	float feedback = lerp(feedback_min, feedback_max, luma_weight_sqr);

	float3 color = 0;

	if (tweak.taa_tonemapped_blend)
	{
		color = lerp(ReversibleTonemap(curr_color), ReversibleTonemap(prev_color), feedback);
		color = ReversibleTonemapInverse(color);
	}
	else
	{
		color = lerp(curr_color, prev_color, feedback);
	}

	img_taa_result[pixel_pos] = float4(color, 1);
}
