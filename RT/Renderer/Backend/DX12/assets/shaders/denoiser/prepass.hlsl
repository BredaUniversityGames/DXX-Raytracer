#include "include/common.hlsl"

// ------------------------------------------------------------------
// This file preprocesses the raw output of the pathtracer to be
// more pleasant for subsequent passes.

#define BORDER 1
#define SHARED_X (GROUP_X + (2*BORDER))
#define SHARED_Y (GROUP_X + (2*BORDER))

groupshared float3 shared_diff      [SHARED_Y][SHARED_X];
groupshared float3 shared_spec      [SHARED_Y][SHARED_X];
groupshared float3 shared_norm[SHARED_Y][SHARED_X];
groupshared float3 shared_world_p   [SHARED_Y][SHARED_X];

void Preload(uint2 shared_pos, uint2 global_pos)
{
	shared_diff   [shared_pos.y][shared_pos.x] = img_diff[global_pos].rgb;
	shared_spec   [shared_pos.y][shared_pos.x] = img_spec[global_pos].rgb;
	shared_norm	  [shared_pos.y][shared_pos.x] = DecodeNormalOctahedron(img_normal[global_pos].rg);
	shared_world_p[shared_pos.y][shared_pos.x] = ReconstructWorldPosFromGBuffer(global_pos);
}

[numthreads(GROUP_X, GROUP_Y, 1)]
void Denoise_Prepass(COMPUTE_ARGS)
{
	PRELOAD_INTO_SHARED

	// ------------------------------------------------------------------

	int2 shared_pos = thread_pos + BORDER;

	float3 curr_norm  = shared_norm	   [shared_pos.y][shared_pos.x].xyz;
	float3 curr_world = shared_world_p [shared_pos.y][shared_pos.x].xyz;

	float3 diff   = shared_diff[shared_pos.y][shared_pos.x];
	float3 spec   = shared_spec[shared_pos.y][shared_pos.x];
	float  diff_w = 1; // g_gaussian_3x3[1][1];
	float  spec_w = 1; // g_gaussian_3x3[1][1];

	float diff_center_luma = Luminance(diff);
	float diff_min_luma =  LARGE_NUMBER;
	float diff_max_luma = -LARGE_NUMBER;

	int2 diff_min_pos = shared_pos;
	int2 diff_max_pos = shared_pos;

	float spec_center_luma = Luminance(spec);
	float spec_min_luma =  LARGE_NUMBER;
	float spec_max_luma = -LARGE_NUMBER;

	int2 spec_min_pos = shared_pos;
	int2 spec_max_pos = shared_pos;

	// if (tweak.svgf_prepass)
	{
		for (int y = 0; y < 2*BORDER + 1; y++)
		for (int x = 0; x < 2*BORDER + 1; x++)
		{
			if (x == BORDER && y == BORDER)
				continue;

			int2 pos = thread_pos + int2(x, y);

			float3 d = shared_diff[pos.y][pos.x];
			float  d_luma = Luminance(d);

			if (diff_max_luma < diff_max_luma)
			{
				diff_max_luma = d_luma;
				diff_max_pos  = pos;
			}

			if (diff_min_luma > diff_min_luma)
			{
				diff_min_luma = d_luma;
				diff_min_pos  = pos;
			}

			float3 s = shared_diff[pos.y][pos.x];
			float  s_luma = Luminance(s);

			if (spec_max_luma < spec_max_luma)
			{
				spec_max_luma = s_luma;
				spec_max_pos  = pos;
			}

			if (spec_min_luma > spec_min_luma)
			{
				spec_min_luma = s_luma;
				spec_min_pos  = pos;
			}
		}
	}

	int2 diff_pos = shared_pos;
	if (diff_center_luma > diff_max_luma)
	{
		diff_pos = diff_max_pos;
	}
	if (diff_center_luma < diff_min_luma)
	{
		diff_pos = diff_min_pos;
	}

	int2 spec_pos = shared_pos;
	if (spec_center_luma > spec_max_luma)
	{
		spec_pos = spec_max_pos;
	}
	if (spec_center_luma < spec_min_luma)
	{
		spec_pos = spec_min_pos;
	}

	img_diff_denoise_ping[pixel_pos].rgb = shared_diff[diff_pos.y][diff_pos.x];
	img_spec_denoise_ping[pixel_pos].rgb = shared_spec[spec_pos.y][spec_pos.x];
}
