#include "include/common.hlsl"

// three stages:
// 1) prepass + downsample
// 2) gaussian blur -> downsample
// 3) combine in post process shader

struct BloomCB
{
	uint iteration;
};

ConstantBuffer<BloomCB> g_bloom_cb : register(b0, space2);

[numthreads(GROUP_X, GROUP_Y, 1)]
void Bloom_Prepass(COMPUTE_ARGS)
{
	float3 color = 0;
	color += img_taa_result[2*pixel_pos].rgb;
	color += img_taa_result[2*pixel_pos + int2(1, 0)].rgb;
	color += img_taa_result[2*pixel_pos + int2(0, 1)].rgb;
	color += img_taa_result[2*pixel_pos + int2(1, 1)].rgb;
	color *= 0.25f;

	float threshold = tweak.bloom_threshold;
	color = max(0, color - threshold);

	img_bloom_prepass[pixel_pos].rgb = color;
}

float3 GaussianBlur(RWTexture2D<float4> source, int2 pixel_pos, int2 direction)
{
	int scale = 1L << (g_bloom_cb.iteration + 1);
	int h = (g_global_cb.output_dim.y + scale - 1) / scale;

	int radius = 8;
	float s = 26.5;

	float3 sum   = 0;
	float  sum_w = 0;

	for (int i = -radius; i <= radius; i++)
	{
		int2 pos = pixel_pos*(1 + direction) + i*direction;

		if (pos.y < h)
		{
			float w = exp(-i*i / s);
			sum   += w*source[pos].rgb;
			sum_w += w;
		}
	}

	sum /= sum_w;
	return sum;
}

[numthreads(GROUP_X, GROUP_Y, 1)]
void Bloom_BlurHorz(COMPUTE_ARGS)
{
#ifdef DO_EARLY_OUT_IN_COMPUTE_SHADERS
	int scale = 1L << (g_bloom_cb.iteration + 1);
	int w = (g_global_cb.output_dim.x + scale - 1) / scale;
	int h = (g_global_cb.output_dim.y + scale - 1) / scale;
	if (pixel_pos.x >= w || pixel_pos.y >= h)
	{
		return;
	}
#endif
	switch (g_bloom_cb.iteration)
	{
		case 0: img_bloom_pong[pixel_pos].rgb = GaussianBlur(img_bloom_prepass, pixel_pos, int2(1, 0)); break;
		case 1: img_bloom_pong[pixel_pos].rgb = GaussianBlur(img_bloom0, pixel_pos, int2(1, 0)); break;
		case 2: img_bloom_pong[pixel_pos].rgb = GaussianBlur(img_bloom1, pixel_pos, int2(1, 0)); break;
		case 3: img_bloom_pong[pixel_pos].rgb = GaussianBlur(img_bloom2, pixel_pos, int2(1, 0)); break;
		case 4: img_bloom_pong[pixel_pos].rgb = GaussianBlur(img_bloom3, pixel_pos, int2(1, 0)); break;
		case 5: img_bloom_pong[pixel_pos].rgb = GaussianBlur(img_bloom4, pixel_pos, int2(1, 0)); break;
		case 6: img_bloom_pong[pixel_pos].rgb = GaussianBlur(img_bloom5, pixel_pos, int2(1, 0)); break;
		case 7: img_bloom_pong[pixel_pos].rgb = GaussianBlur(img_bloom6, pixel_pos, int2(1, 0)); break;
	}
}

[numthreads(GROUP_X, GROUP_Y, 1)]
void Bloom_BlurVert(COMPUTE_ARGS)
{
#ifdef DO_EARLY_OUT_IN_COMPUTE_SHADERS
	int scale = 1L << (g_bloom_cb.iteration + 1);
	int w = (g_global_cb.output_dim.x + scale - 1) / scale;
	int h = (g_global_cb.output_dim.y + scale - 1) / scale;
	if (pixel_pos.x >= w || pixel_pos.y >= h)
	{
		return;
	}
#endif
	switch (g_bloom_cb.iteration)
	{
		case 0: img_bloom0[pixel_pos].rgb = GaussianBlur(img_bloom_pong, pixel_pos, int2(0, 1)); break;
		case 1: img_bloom1[pixel_pos].rgb = GaussianBlur(img_bloom_pong, pixel_pos, int2(0, 1)); break;
		case 2: img_bloom2[pixel_pos].rgb = GaussianBlur(img_bloom_pong, pixel_pos, int2(0, 1)); break;
		case 3: img_bloom3[pixel_pos].rgb = GaussianBlur(img_bloom_pong, pixel_pos, int2(0, 1)); break;
		case 4: img_bloom4[pixel_pos].rgb = GaussianBlur(img_bloom_pong, pixel_pos, int2(0, 1)); break;
		case 5: img_bloom5[pixel_pos].rgb = GaussianBlur(img_bloom_pong, pixel_pos, int2(0, 1)); break;
		case 6: img_bloom6[pixel_pos].rgb = GaussianBlur(img_bloom_pong, pixel_pos, int2(0, 1)); break;
		case 7: img_bloom7[pixel_pos].rgb = GaussianBlur(img_bloom_pong, pixel_pos, int2(0, 1)); break;
	}
}
