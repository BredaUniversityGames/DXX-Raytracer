#include "include/common.hlsl"

//------------------------------------------------------------------------

static float sharpen_kernel[3][3] =
{
	{ -0.0023, -0.0432, -0.0023 },
	{ -0.0432,  1.1820, -0.0432 },
	{ -0.0023, -0.0432, -0.0023 },
};

#define BORDER 1
#define SHARED_X (GROUP_X + 2*BORDER)
#define SHARED_Y (GROUP_Y + 2*BORDER)

groupshared float3 shared_color[SHARED_Y][SHARED_X];

void Preload(uint2 shared_pos, uint2 global_pos)
{
	shared_color[shared_pos.y][shared_pos.x] = img_postfx[global_pos].rgb;
}

[numthreads(GROUP_X, GROUP_Y, 1)]
void ResolveFinalColorCS(COMPUTE_ARGS)
{
	PRELOAD_INTO_SHARED

	float3 color = shared_color[thread_pos.y + BORDER][thread_pos.x + BORDER];

	if (tweak.sharpen_amount > 0.0)
	{
		float3 sharpened = 0;

		for (int y = 0; y < 3; y++)
		for (int x = 0; x < 3; x++)
		{
			int2 shared_pos = thread_pos + int2(x, y);
			float weight = sharpen_kernel[y][x];
			sharpened += weight*shared_color[shared_pos.y][shared_pos.x];
		}

		color = lerp(color, sharpened, tweak.sharpen_amount);
	}

	//------------------------------------------------------------------------
	// Brightness adjustments

	color = pow(color, exp2(tweak.gamma));
	color = (color + tweak.black_level) / (tweak.white_level + tweak.black_level);

	//------------------------------------------------------------------------

	float3 dither_noise = {
		RandomSample(pixel_pos, Random_DitherR),
		RandomSample(pixel_pos, Random_DitherG),
		RandomSample(pixel_pos, Random_DitherB),
	};

	// Cheap way to turn this into trianglular probability noise
	dither_noise = dither_noise.rgb - dither_noise.gbr;
	color += dither_noise / 255.0;

	img_resolve[pixel_pos] = float4(color, 1);
}
