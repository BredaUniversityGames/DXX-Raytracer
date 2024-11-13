#ifndef COMMON_HLSL
#define COMMON_HLSL

#include "include_shared/shared_common.hlsl.h"
#include "include_shared/shared_rendertargets.hlsl.h"

using namespace RT;

#ifdef DO_EARLY_OUT_IN_COMPUTE_SHADERS
#define EARLY_OUT if (pixel_pos.x >= g_global_cb.render_dim.x || pixel_pos.y >= g_global_cb.render_dim.y) return;
#else
#define EARLY_OUT
#endif

// ------------------------------------------------------------------
// Preload stuff

#define COMPUTE_ARGS int2 pixel_pos : SV_DispatchThreadID, int2 thread_pos : SV_GroupThreadID, uint thread_index : SV_GroupIndex

// GROUP_X and GROUP_Y defined in GlobalDX.h as COMPUTE_GROUP_SIZE_X
// and COMPUTE_GROUP_SIZE_Y
// SHARED_X/Y should be defined as GROUP_X/Y + 2*BORDER where BORDER
// is whatever size border you need (1 for 3x3 filter, 2 for 5x5, 3
// for 7x7, etc)

// This looks awkward, if you work it out on paper you will find that
// it works. Basically, if you want to use group shared memory to
// optimize a filter, let's say a 3x3 filter, it's not enough to load
// values only for the pixels inside the group, you also need to 
// account for the border around it the size of the filter radius.
// The code below will make some threads in the group load two pixels
// because we have to fill more values than there are threads.
// If there is no border, then this code still works, and with a bit
// of luck the compiler will see that the for loop will always only
// run once and optimize appropriately...
#define PRELOAD_INTO_SHARED                                                         \
	int2 group_base = pixel_pos - thread_pos - BORDER;                              \
	for (int virtual_index = thread_index;                                          \
		 virtual_index < SHARED_X*SHARED_Y;                                         \
		 virtual_index += GROUP_X*GROUP_Y)                                          \
	{                                                                               \
		int2 shared_pos = int2(virtual_index % SHARED_X, virtual_index / SHARED_X); \
		Preload(shared_pos, group_base + shared_pos);                               \
	}                                                                               \
	GroupMemoryBarrierWithGroupSync();

// ------------------------------------------------------------------
// Common definitions that are part of the public API of the renderer

// Volatile: Must match Renderer.h
#define RT_DebugRenderMode_None              (0)
#define RT_DebugRenderMode_Debug			 (1)
#define RT_DebugRenderMode_Normals           (2)
#define RT_DebugRenderMode_Depth             (3)
#define RT_DebugRenderMode_Albedo            (4)
#define RT_DebugRenderMode_Emissive          (5)
#define RT_DebugRenderMode_Diffuse           (6)
#define RT_DebugRenderMode_Specular          (7)
#define RT_DebugRenderMode_Motion            (8)
#define RT_DebugRenderMode_MetallicRoughness (9)
#define RT_DebugRenderMode_HistoryLength     (10)
#define RT_DebugRenderMode_Materials         (11)
#define RT_DebugRenderMode_FirstMoment       (12)
#define RT_DebugRenderMode_SecondMoment      (13)
#define RT_DebugRenderMode_Variance          (14)
#define RT_DebugRenderMode_Bloom0            (15)
#define RT_DebugRenderMode_Bloom1            (16)
#define RT_DebugRenderMode_Bloom2            (17)
#define RT_DebugRenderMode_Bloom3            (18)
#define RT_DebugRenderMode_Bloom4            (19)
#define RT_DebugRenderMode_Bloom5            (20)
#define RT_DebugRenderMode_Bloom6            (21)
#define RT_DebugRenderMode_Bloom7            (22)
#define RT_DebugRenderMode_Fsr2Reactive		 (23)
#define RT_DebugRenderMode_COUNT             (24)

static const float PI = 3.14159265359;
#define LARGE_NUMBER 1000000000

// @Volatile: Must match Renderer.h
#define RT_TRIANGLE_HOLDS_MATERIAL_EDGE        (1u << 31)
#define RT_TRIANGLE_HOLDS_MATERIAL_INDEX       (1u << 30)
#define RT_TRIANGLE_MATERIAL_INSTANCE_OVERRIDE (0xFFFF)

#define RT_RAY_T_MIN 0.001
#define RT_RAY_T_MAX 10000

// @Volatile: Must match RT_Triangle in Renderer.h
struct RT_Triangle
{
	float3 pos0;
	float3 pos1;
	float3 pos2;

	float3 normal0;
	float3 normal1;
	float3 normal2;

	float4 tangent0;
	float4 tangent1;
	float4 tangent2;

	float2 uv0;
	float2 uv1;
	float2 uv2;

	uint color;
	uint material_edge_index;
};

// @Volatile: Must match RT_MaterialFlags in Renderer.h
#define RT_MaterialFlag_BlackbodyRadiator (0x1) // things like lava, basically just treats the albedo as an emissive map and skips all shading
#define RT_MaterialFlag_NoCastingShadow   (0x2) // some materials/meshes we do not want to cast shadows (like the 3D cockpit)
#define RT_MaterialFlag_Light             (0x4)
#define RT_MaterialFlag_Fsr2ReactiveMask  (0x8)

// @Volatile: Must match RT_LightKind in ApiTypes.h
#define RT_LightKind_Area_Sphere (0)
#define RT_LightKind_Area_Rect   (1)

// @Volatile: Must match RT_LIGHT_SCALE in ApiTypes.h
#define RT_LIGHT_SCALE (1000.0)

// @Volatile: Must match RT_Light in ApiTypes.h
struct RT_Light
{
	uint     kind          : 8;
	uint     spot_angle    : 8;
	uint     spot_softness : 8;
	uint     spot_vignette : 8;
	uint     emission;
	float3x4 transform;
};

// ------------------------------------------------------------------
// Render Targets

#define RT_RENDER_TARGET_DECLARE_UAVS(name, reg, scale_x, scale_y, output_dir, type, fmt) \
	RWTexture2D<type> img_##name : register(u##reg, space999);
RT_RENDER_TARGETS(RT_RENDER_TARGET_DECLARE_UAVS)

#define RT_RENDER_TARGET_DECLARE_SRVS(name, reg, scale_x, scale_y, output_dir, type, fmt) \
	Texture2D<type> tex_##name : register(t##reg, space999);
RT_RENDER_TARGETS(RT_RENDER_TARGET_DECLARE_SRVS)

// ------------------------------------------------------------------
// UAVs

RWStructuredBuffer<PixelDebugData> g_pixel_debug : register(u0);

// ------------------------------------------------------------------
// SRVs

RaytracingAccelerationStructure g_scene                            : register(t0);
StructuredBuffer<RT_Light>      g_lights                           : register(t1);
StructuredBuffer<Material>      g_materials                        : register(t2);
StructuredBuffer<InstanceData>  g_instance_data_buffer             : register(t3);
ByteAddressBuffer               g_material_edges                   : register(t4);
ByteAddressBuffer               g_material_indices                 : register(t5);
Texture2D                       g_blue_noise[BLUE_NOISE_TEX_COUNT] : register(t6);
Texture2D						g_bindless_srvs[]				   : register(t0, space3);
StructuredBuffer<RT_Triangle>	g_bindless_triangle_buffers[]	   : register(t0, space4);

// ------------------------------------------------------------------
// CBVs

ConstantBuffer<GlobalConstantBuffer> g_global_cb : register(b0);
ConstantBuffer<TweakVars>            tweak       : register(b1);

// ------------------------------------------------------------------
// Samplers

SamplerState g_sampler_linear_wrap  : register(s0);
SamplerState g_sampler_point_wrap   : register(s1);
SamplerState g_sampler_linear_clamp : register(s2);

// ------------------------------------------------------------------
// Useful functions

float3 GetColumn(float3x4 transform, int column)
{
	return float3(transform[0][column],
				  transform[1][column],
				  transform[2][column]);
}

void DecomposeColumns(float3x4 transform, out float3 x, out float3 y, out float3 z, out float3 t)
{
	x = float3(transform[0][0],
			   transform[1][0],
			   transform[2][0]);

	y = float3(transform[0][1],
			   transform[1][1],
			   transform[2][1]);

	z = float3(transform[0][2],
			   transform[1][2],
			   transform[2][2]);

	t = float3(transform[0][3],
			   transform[1][3],
			   transform[2][3]);
}

float max3(float3 x)
{
	return max(x.x, max(x.y, x.z));
}

float square(float x)
{
	return x*x;
}

float3 ReversibleTonemap(float3 color)
{
	return color*rcp(1 + max3(color));
}

float3 ReversibleTonemapInverse(float3 color)
{
	return color*rcp(1 - max3(color));
}

uint PackMetallicRoughnessMaterial(float metallic, float roughness, uint material)
{
	uint result = 0;
	result |= (uint(255.0*metallic)  & 0xFF) << 0;
	result |= (uint(255.0*roughness) & 0xFF) << 8;
	result |= material << 16;
	return result;
}

void UnpackMetallicRoughnessMaterial(uint packed, out float metallic, out float roughness, out uint material)
{
	metallic  = rcp(255.0)*float((packed >> 0) & 0xFF);
	roughness = rcp(255.0)*float((packed >> 8) & 0xFF);
	material  = packed >> 16;
}

float FloatFromUint8(uint u8)
{
	return float(u8) / 255.0f;
}

float4 UnpackRGBA(uint rgba)
{
    float rcp_255 = 1.0f / 255.0f;

    float4 result;
    result.x = rcp_255*(float)((rgba >>  0) & 0xFF);
    result.y = rcp_255*(float)((rgba >>  8) & 0xFF);
    result.z = rcp_255*(float)((rgba >> 16) & 0xFF);
    result.w = rcp_255*(float)((rgba >> 24) & 0xFF);

	// Coarse sRGB to linear transform
	result.rgb *= result.rgb;

    return result;
}

uint PackRGBE(float3 color)
{
	color = max(0, color);
    float max_abs = max(color.x, max(color.y, color.z));

    if(max_abs == 0)
    {
        return 0;
    }

    float exponent = floor(log2(max_abs));

    float scale = pow(2, -exponent)*256.0f;
	uint3 rgb = min(511, uint3(round(color*scale)));

    uint result;
    result  = uint(clamp(exponent + 20, 0, 31)) << 27;
    result |= rgb.r;
    result |= rgb.g << 9;
    result |= rgb.b << 18;

    return result;
}

float3 UnpackRGBE(uint rgbe)
{
    int exponent = int(rgbe >> 27) - 20;
    float scale = pow(2, float(exponent)) / 256.0f;

    float3 result;
    result.x = float((rgbe >>  0) & 0x1ff)*scale;
    result.y = float((rgbe >>  9) & 0x1ff)*scale;
    result.z = float((rgbe >> 18) & 0x1ff)*scale;
    return result;
}

float3 IntegerHash(uint3 x)
{
	// source: https://www.shadertoy.com/view/XlXcW4

	const uint k = 1103515245U;  // GLIB C
	x = ((x >> 8U) ^ x.yzx)*k;
	x = ((x >> 8U) ^ x.yzx)*k;
	x = ((x >> 8U) ^ x.yzx)*k;
    
	return float3(x)*(1.0 / float(0xffffffffU));
}

float Luminance(float3 color)
{
	return dot(color, float3(0.25, 0.5, 0.25));
}

static float g_gaussian_3x3[3][3] = 
{
	{ 0.0023, 0.0432, 0.0023, },
	{ 0.0432, 0.8180, 0.0432, },
	{ 0.0023, 0.0432, 0.0023, },
};

static float g_gaussian_5[5] =
{
	0.1573f, 0.0013f, 0.6827f, 0.1573f, 0.0013f,
};

static float g_gaussian_7[7] =
{
	0.0108,	0.0749,	0.2387,	0.3511,	0.2387,	0.0749,	0.0108,
};

#define Random_TaaJitterX       (0)
#define Random_TaaJitterY       (1)
#define Random_DirectJitterX    (2)
#define Random_DirectJitterY    (3)
#define Random_IndirectJitterX  (4)
#define Random_IndirectJitterY  (5)
#define Random_DenoiseJitterX   (6)
#define Random_DenoiseJitterY   (7)
#define Random_DitherR          (9)
#define Random_DitherG          (10)
#define Random_DitherB          (11)
#define Random_PickLight        (12)
#define Random_IndirectSpecular (13)
#define Random_MotionBlur       (14)

float RandomSample(uint2 xy, uint id)
{
	float result = 0;

	uint cycle = g_global_cb.frame_index / BLUE_NOISE_TEX_COUNT;

	uint texture_index  = ((g_global_cb.frame_index + id) / 4) % BLUE_NOISE_TEX_COUNT;
	uint texture_offset = ((g_global_cb.frame_index + id) % 4);

	float4 s = g_blue_noise[NonUniformResourceIndex(texture_index)].Load(uint3((xy + 13*cycle) % 64, 0));
	result = s[texture_offset];

	return result;
}

uint RandomIndex(uint2 co, uint count, uint id)
{
	float r = RandomSample(co, id);
	return uint(r*(count - 1));
}

float2 GetTAAJitter(uint2 co)
{
	float2 jitter = 0;
	if (tweak.taa_per_pixel_jitter)
	{
		jitter = float2(RandomSample(co, Random_TaaJitterX) - 0.5,
						RandomSample(co, Random_TaaJitterY) - 0.5);
	}
	else
	{
		jitter = g_global_cb.taa_jitter;
	}
	jitter /= (float2)g_global_cb.render_dim;
	return jitter;
}

float CalculateLumaWeight(float3 curr, float3 prev)
{
	// Feedback weight from unbianced luminance diff (courtesy of Timothy Lottes, but I haven't found the original source, I saw it in the Playdead TAA, as well as in denoising reprojection code from a Unity workshop)
	float lum_curr = Luminance(curr);
	float lum_prev = Luminance(prev);
	// This divide with the max of curr or prev luma corrects for the fact that for example a diff between
	// 0.1 and 0.2 would be 0.1, but between 0.5 and 1.0 would be 0.5, even though both represent a doubling in luminance
	float lum_diff = abs(lum_curr - lum_prev) / max(lum_curr, max(lum_prev, 0.2));
	float lum_weight = 1.0 - lum_diff;
	return lum_weight;
}

float2 Project(float4x4 proj, float3 view)
{
    float4 clip_pos   = mul(proj, float4(view, 1));
    float3 normalized = clip_pos.xyz / clip_pos.w;
    float2 result = 0.5*normalized.xy + 0.5;
    return result;
}

float3 Unproject(float4x4 proj_inv, float2 screen, float z)
{
    float4 clip_pos = float4(2.0*screen - 1.0, 1, 1);
    float3  view_d  = normalize(mul(proj_inv, clip_pos).xyz);
    return view_d*z;
}

struct BilinearSamples
{
	int2  positions[4];
	float weights  [4];
};

void GetBilinearSamples(in float2 pos, out BilinearSamples samples)
{
	int2   posi = pos;
	float2 posf = frac(pos);

	samples.positions[0] = posi + int2(0, 0);
	samples.positions[1] = posi + int2(1, 0);
	samples.positions[2] = posi + int2(0, 1);
	samples.positions[3] = posi + int2(1, 1);

	samples.weights[0] = (1.0 - posf.x)*(1.0 - posf.y);
	samples.weights[1] =        posf.x *(1.0 - posf.y);
	samples.weights[2] = (1.0 - posf.x)*       posf.y;
	samples.weights[3] =        posf.x *       posf.y;
}

float4 SampleBilinear(RWTexture2D<float4> tex, float2 pos)
{
	// pos is in pixel coordinates

	int2   posi = pos;
	float2 posf = frac(pos);

	int2 sample_positions[] = {
		posi + int2(0, 0), posi + int2(1, 0),
		posi + int2(0, 1), posi + int2(1, 1),
	};

	float sample_weights[] = {
		(1.0 - posf.x)*(1.0 - posf.y), posf.x*(1.0 - posf.y),
		(1.0 - posf.x)*       posf.y,  posf.x*       posf.y,
	};

	float4 color = 0.0;
	float  sum_w = 0.0;

	for (int i = 0; i < 4; i++)
	{
		int2  sample_pos = sample_positions[i];
		float w          = sample_weights  [i];

		float4 sample_color = tex[sample_pos];
		color += w*sample_color;
		sum_w += w;
	}
	color *= rcp(sum_w);

	return color;
}

// The following code is licensed under the MIT license: https://gist.github.com/TheRealMJP/bc503b0b87b643d3505d41eab8b332ae
// Original gist: https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1

// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
float4 SampleTextureCatmullRom(in Texture2D<float4> tex, in SamplerState linearSampler, in float2 uv, in float2 texSize)
{
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 samplePos = uv * texSize;
    float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 w12 = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 texPos0 = texPos1 - 1;
    float2 texPos3 = texPos1 + 2;
    float2 texPos12 = texPos1 + offset12;

    texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;

    float4 result = 0.0f;
    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

    return result;
}

// TODO(daniel): Remove the corner samples of the above Catmull-Rom sampling function
// to reduce it to 5 samples instead of 9, as an optimization as described in:
// https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
// Or don't, as it probably doesn't matter _that_ much

// ------------------------------------------------------------------
// Materials

RT_Triangle GetHitTriangle(uint triangle_buffer_idx, uint primitive_idx)
{
	return g_bindless_triangle_buffers[NonUniformResourceIndex(triangle_buffer_idx)][primitive_idx];
}

float2 GetHitAttribute(float2 vertex_attrib[3], float2 bary)
{
	return vertex_attrib[0] +
		bary.x * (vertex_attrib[1] - vertex_attrib[0]) +
		bary.y * (vertex_attrib[2] - vertex_attrib[0]);
}

float3 GetHitAttribute(float3 vertex_attrib[3], float2 bary)
{
	return vertex_attrib[0] +
		bary.x * (vertex_attrib[1] - vertex_attrib[0]) +
		bary.y * (vertex_attrib[2] - vertex_attrib[0]);
}

Texture2D GetTextureFromIndex(uint index)
{
	return g_bindless_srvs[NonUniformResourceIndex(index)];
}

uint GetMaterialIndex(uint material_edge)
{
	uint load = g_material_indices.Load((2 * material_edge) & ~3);

	uint result;
	if (material_edge & 1)
	{
		result = load >> 16;
	}
	else
	{
		result = load & 0xFFFF;
	}

	return result;
}

void GetMaterialEdges(uint material_edge_index, out uint mat1, out uint mat2, out uint orient)
{
	uint byte_offset = 4 * material_edge_index; // 4 bytes per RT_MaterialEdge
	uint material_edge = g_material_edges.Load(byte_offset);

	mat1 = (material_edge >> 0) & 0xFFFF;
	mat2 = (material_edge >> 16) & 0x3FFF;
	orient = (material_edge >> 30) & 3;
}

void GetMaterialIndicesAndOrient(uint material_edge_index, out uint material_index, out uint material_index2, out uint orient, uint material_override)
{
	//if (material_edge_index == RT_TRIANGLE_MATERIAL_INSTANCE_OVERRIDE)
	if (material_override)
	{
		material_index = material_override;
	}
	else
	{
		if (material_edge_index & RT_TRIANGLE_HOLDS_MATERIAL_INDEX)
		{
			material_index = material_edge_index & ~RT_TRIANGLE_HOLDS_MATERIAL_INDEX;
		}
		else if (material_edge_index & RT_TRIANGLE_HOLDS_MATERIAL_EDGE)
		{
			material_index = GetMaterialIndex(material_edge_index & ~RT_TRIANGLE_HOLDS_MATERIAL_EDGE);
		}
		else
		{
			uint mat1, mat2;
			GetMaterialEdges(material_edge_index, mat1, mat2, orient);

			material_index = GetMaterialIndex(mat1);
			if (mat2 != 0)
			{
				material_index2 = GetMaterialIndex(mat2);
			}
		}
	}
}

// Rotation with angle (in radians) and axis (https://gist.github.com/keijiro/ee439d5e7388f3aafc5296005c8c3f33)
float3x3 AngleAxis3x3(float angle, float3 axis)
{
    float c, s;
    sincos(angle, s, c);

    float t = 1 - c;
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    return float3x3(
        t * x * x + c,      t * x * y - s * z,  t * x * z + s * y,
        t * x * y + s * z,  t * y * y + c,      t * y * z - s * x,
        t * x * z - s * y,  t * y * z + s * x,  t * z * z + c
    );
}

float3 GetRotatedTangent(uint orient, float3 axis, float3 tangent)
{
	float3x3 rotator = AngleAxis3x3(orient * -1.570796,axis);
	return mul(rotator, tangent);
	
}

float2 GetRotatedUVs(uint orient, float2 uv)
{
	switch (orient)
	{
	case 1:
	{
		return float2(1.0 - uv.y, uv.x);
	} break;

	case 2:
	{
		return float2(1.0 - uv.x, 1.0 - uv.y);
	} break;

	case 3:
	{
		return float2(uv.y, 1.0 - uv.x);
	} break;

	default:
	{
		return uv;
	} break;
	}
}

void GetHitMaterialAndUVs(InstanceData instance_data, RT_Triangle hit_triangle, float2 barycentrics, inout uint material_index, inout float2 uv, inout float3 normal, inout float3 tangent)
{
	// Figure out the materials and apply potential debug material override
	uint material_edge_index = hit_triangle.material_edge_index;

	uint orient = 0;
	uint material_index2 = 0xFFFFFFFF;
	GetMaterialIndicesAndOrient(material_edge_index, material_index, material_index2, orient, instance_data.material_override);

	// -------------------------------------------------------------------------------------
	// Calculate UV coordinates

	float2 uvs[] = {
		hit_triangle.uv0,
		hit_triangle.uv1,
		hit_triangle.uv2,
	};

	uv = GetHitAttribute(uvs, barycentrics);
	float2 uv_rotated = GetRotatedUVs(orient, uv);
	
	// -------------------------------------------------------------------------------------
	// Calculate Normal
	
	float3 normals[] =
	{
		hit_triangle.normal0,
		hit_triangle.normal1,
		hit_triangle.normal2,
	};
	normal = GetHitAttribute(normals, barycentrics);
	
	// -------------------------------------------------------------------------------------
	// Calculate Tangent

	float3 tangents[] = {
		hit_triangle.tangent0.xyz,
		hit_triangle.tangent1.xyz,
		hit_triangle.tangent2.xyz,
	};

	tangent = GetHitAttribute(tangents, barycentrics);
	
	// TODO(daniel): Clean this messy silly code up!
	if (material_index2 != 0xFFFFFFFF)
	{
		Material material2 = g_materials[material_index2];
		Texture2D tex_albedo_material2 = GetTextureFromIndex(material2.albedo_index);
		float4 albedo2 = tex_albedo_material2.SampleLevel(g_sampler_point_wrap, uv_rotated, 0);

		if (albedo2.a > 0.0)
		{
			material_index = material_index2;
			if (orient != 0)
			{
				uv = uv_rotated;

				// Calculate UV direction vectors
				float2 uvEdge1 = hit_triangle.uv1 - hit_triangle.uv0;
				float2 uvEdge2 = hit_triangle.uv2 - hit_triangle.uv0;

				// Compute the determinant
				float det = uvEdge1.x * uvEdge2.y - uvEdge1.y * uvEdge2.x;

				if (det < 0.0 && orient % 2 == 1)
				{
					// if the uv coords are mirrored and the orientation is 1 or 3 (90 and 270 degress) add 180 degress
					// this is because 90 and 270 need to be swapped when uvs are mirrored and its easier to just add 180 degress.
					orient += 2;
				}

				tangent = GetRotatedTangent(orient, normal, tangent);
			}
		}
	}
}

bool IsHitTransparent(uint instance_idx, uint primitive_idx, float2 barycentrics, int2 pixel_pos, inout Material material)
{
	InstanceData instance_data = g_instance_data_buffer[instance_idx];
	RT_Triangle hit_triangle = GetHitTriangle(instance_data.triangle_buffer_idx, primitive_idx);

	uint material_edge_index = hit_triangle.material_edge_index;

	uint orient = 0;
	uint material_index = 0;
	uint material_index2 = 0xFFFFFFFF;

	if (instance_data.material_override)
	{
		material_index = instance_data.material_override;
	}
	else
	{
		if (material_edge_index & RT_TRIANGLE_HOLDS_MATERIAL_INDEX)
		{
			material_index = material_edge_index & ~RT_TRIANGLE_HOLDS_MATERIAL_INDEX;
		}
		else if (material_edge_index & RT_TRIANGLE_HOLDS_MATERIAL_EDGE)
		{
			material_index = GetMaterialIndex(material_edge_index & ~RT_TRIANGLE_HOLDS_MATERIAL_EDGE);
		}
		else
		{
			uint mat1, mat2;
			GetMaterialEdges(material_edge_index, mat1, mat2, orient);

			material_index = GetMaterialIndex(mat1);
			if (mat2 != 0)
			{
				material_index2 = GetMaterialIndex(mat2);
			}
		}
	}

	float2 uvs[] = {
		hit_triangle.uv0,
		hit_triangle.uv1,
		hit_triangle.uv2,
	};
	float2 uv = GetHitAttribute(uvs, barycentrics);
	float2 uv_rotated = GetRotatedUVs(orient, uv);
	float4 color = UnpackRGBA(instance_data.material_color);
	float4 tri_color = UnpackRGBA(hit_triangle.color);
	float base_alpha = color.a * tri_color.a;
	float  dither = RandomSample(pixel_pos, instance_idx) * 0.99;  // NOTE: dither is multiplied by .99 because dither should never be 1.0 (You end up with single pixel holes in geometry)

	// TODO(daniel): Clean this messy silly code up!
	if (material_index2 != 0xFFFFFFFF)
	{
		Material material2 = g_materials[material_index2];
		Texture2D tex_albedo_material2 = GetTextureFromIndex(material2.albedo_index);
		float4 albedo2 = tex_albedo_material2.SampleLevel(g_sampler_point_wrap, uv_rotated, 0);

		// Check for supertransparency pixel
		// The reason the xyz check is 0.95 and not 1.0 like original game is to handle compressed textures slightly shifting colors 
		if (all(albedo2.xyz >= 0.95) && albedo2.a == 0.0)
		{
			return true;
		}

		if ( dither < albedo2.a )
		{
			material = g_materials[material_index2];
			return dither >= base_alpha;
		}
	}

	material = g_materials[material_index];
	Texture2D tex_albedo = GetTextureFromIndex(material.albedo_index);
	float4 albedo = tex_albedo.SampleLevel(g_sampler_point_wrap, uv, 0);

	return dither >= albedo.a * base_alpha;
}

// -----------------------------------------------------------
// Sampling

float3x3 ConstructOrthonormalBasis(float3 N)
{
	float3 T, B;

	if (abs(N.x) > abs(N.y))
		T = float3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
	else
		T = float3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
	B = cross(N, T);

	return transpose(float3x3(T, N, B));
}

float3 SampleRect(float3 position, float3 normal, float2 size)
{
	float3 tangent = cross(normal, float3(0, 0, 1));
	float3 bitangent = cross(normal, tangent);

	float rx = RandomSample(DispatchRaysIndex().xy, Random_DirectJitterX);
	float ry = RandomSample(DispatchRaysIndex().xy, Random_DirectJitterY);
	return position + size.x * tangent * rx + size.y * bitangent * ry;
}

// Generate a random point on the surface of the unit sphere, centered at origin
float3 SampleUnitSphere(float2 r)
{
	// Generate random spherical coordinates (theta, phi)
	float theta = r.x * 2.0 * PI;
	float phi = r.y * PI;

	// Convert spherical coordinates to Cartesian coordinates
	float x = sin(phi) * cos(theta);
	float y = sin(phi) * sin(theta);
	float z = cos(phi);

	return float3(x, y, z);
}

float3 UniformSampleHemisphere(float r1, float r2)
{
	// cos(theta) = r1 = y
	// cos^2(theta) + sin^2(theta) = 1 -> sin(theta) = srtf(1 - cos^2(theta))
	float sinTheta = sqrt(1 - r1 * r1);
	float phi = 2 * PI * r2;
	float x = sinTheta * cos(phi);
	float z = sinTheta * sin(phi);
	return float3(x, r1, z);
}

float3 CosineSampleHemisphere(float r1, float r2)
{
	float sinTheta = sqrt(1 - r1);
	float phi = 2 * PI * r2;
	float x = sinTheta * cos(phi);
	float z = sinTheta * sin(phi);
	return float3(x, sqrt(r1), z);
}

bool HasHitGeometry(uint2 instance_idx_prim_idx)
{
	return !(instance_idx_prim_idx.x == 0xFFFFFFFF || instance_idx_prim_idx.y == 0xFFFFFFFF);
}

void WritePixelDebug(in PixelDebugData data)
{
	uint2 co = DispatchRaysIndex().xy;
	uint2 dim = DispatchRaysDimensions().xy;

	g_pixel_debug[co.y * dim.x + co.x] = data;
}

float2 OctWrap(float2 value)
{
	return (1.0 - abs(value.yx)) * select(value.xy >= 0.0, 1.0, -1.0);
}

float2 EncodeNormalOctahedron(float3 normal)
{
	normal /= (abs(normal.x) + abs(normal.y) + abs(normal.z));
	normal.xy = select(normal.z >= 0.0, normal.xy, OctWrap(normal.xy));
	normal.xy = normal.xy * 0.5 + 0.5;

	return normal.xy;
}

float3 DecodeNormalOctahedron(float2 packed)
{
	packed = packed * 2.0 - 1.0;
	float3 normal = float3(packed.x, packed.y, 1.0 - abs(packed.x) - abs(packed.y));
	float t = saturate(-normal.z);
	normal.xy += select(normal.xy >= 0.0, -t, t);

	return normalize(normal);
}

float3 ReconstructWorldPosition(float4x4 view_matrix, float3 view_dir, float depth)
{
	float3 view_pos = mul(view_matrix, float4(0, 0, 0, 1)).xyz;
	return view_pos + view_dir * depth;
}

float3 ReconstructWorldPosFromGBuffer(uint2 dispatch_idx)
{
	// Note(Justin): This is a prime example for "branches on GPUs can be slow". Branches on GPUs are not slow by default,
	// but in this case they are, because the branch is not predictable in advance (e.g. if you used a constant for it that is the same
	// across the entire dispatch, it would not be slow/costly at all). This if statement cost me 0.3ms on 1080p on my RTX 3080 Ti, which was simply
	// replaced with a multiplier of 1 or 0 to save that precious time.
	/*float3 world_p = float3(0, 0, 0);
	if (HasHitGeometry(img_visibility_prim[dispatch_idx].xy))
	{
		world_p = ReconstructWorldPosition(g_global_cb.view_inv, img_view_dir[dispatch_idx].xyz, img_depth[dispatch_idx]);
	}
	return world_p;*/

	return ReconstructWorldPosition(g_global_cb.view_inv, img_view_dir[dispatch_idx].xyz, img_depth[dispatch_idx]) * img_view_dir[dispatch_idx].w;
}

float3 ReconstructPrevWorldPosFromGBuffer(uint2 dispatch_idx)
{
	/*float3 world_p = float3(0, 0, 0);
	if (HasHitGeometry(img_visibility_prim[dispatch_idx].xy))
	{
		world_p = ReconstructWorldPosition(g_global_cb.prev_view_inv, img_view_dir_prev[dispatch_idx].xyz, img_depth_prev[dispatch_idx]);
	}
	return world_p;*/

	return ReconstructWorldPosition(g_global_cb.prev_view_inv, img_view_dir_prev[dispatch_idx].xyz, img_depth_prev[dispatch_idx]) * img_view_dir_prev[dispatch_idx].w;
}

RayDesc GetRayDesc(uint2 dispatch_idx, uint2 dispatch_dim)
{
	// Calculate UV
	float2 dispatch_uv = (dispatch_idx + 0.5) / dispatch_dim;
	dispatch_uv.y = 1.0f - dispatch_uv.y;
	dispatch_uv.y -= g_global_cb.viewport_offset_y;

	// Apply TAA jitter if TAA is enabled
	if (tweak.upscaling_aa_mode != 0)
	{
		dispatch_uv += GetTAAJitter(dispatch_idx);
	}

	float3 curr_view_d = Unproject(g_global_cb.proj_inv, dispatch_uv, 1);
	float3 curr_world_d = mul(g_global_cb.view_inv, float4(curr_view_d, 0)).xyz;
	float3 curr_world_p = mul(g_global_cb.view_inv, float4(0, 0, 0, 1)).xyz;

	// Set up geometry input for primary ray trace
	RayDesc ray_desc = (RayDesc)0;
	ray_desc.Origin = curr_world_p;
	ray_desc.Direction = curr_world_d;
	ray_desc.TMin = RT_RAY_T_MIN;
	ray_desc.TMax = RT_RAY_T_MAX;

	return ray_desc;
}

// Paper: https://jcgt.org/published/0010/01/01/
// ray_dir is the ray direction in world space
// ray_cone_radius is the cone radius on the intersection plane with the triangle
// hit_bary are the barycentric coordinates at the intersection point
// hit_position is the world space position of the intersection point
// hit_normal is the interpolated normal at the intersection point
// triangle_pos are the three triangle vertex positions in world space
// triangle_uv are the three triangle vertex uv coordinates
// tex_gradient1 and tex_gradient2 are the two texture gradients in the ray cone ellipse axes, which can be used in SampleGrad for anisotropic filtering
void ComputeTextureGradientRayCone(float3 ray_dir, float ray_cone_radius, float2 hit_bary, float3 hit_position, float3 hit_normal,
	float3 triangle_pos[3], float2 triangle_uv[3], out float2 tex_gradient1, out float2 tex_gradient2)
{
	float2 intersect_uv = GetHitAttribute(triangle_uv, hit_bary);

	// Calculate both ellipse axes
	float3 a1 = ray_dir - dot(hit_normal, ray_dir) * hit_normal;
	float3 p1 = a1 - dot(ray_dir, a1) * ray_dir;
	a1 *= ray_cone_radius / max(tweak.angle_cutoff, length(p1));

	float3 a2 = cross(hit_normal, a1);
	float3 p2 = a2 - dot(ray_dir, a2) * ray_dir;
	a2 *= ray_cone_radius / max(tweak.angle_cutoff, length(p2));

	// Compute texture coordinate gradients
	float3 eP, delta = hit_position - triangle_pos[0];
	float3 e1 = triangle_pos[1] - triangle_pos[0];
	float3 e2 = triangle_pos[2] - triangle_pos[0];
	float one_over_area_triangle = 1.0 / dot(hit_normal, cross(e1, e2));
	
	eP = delta + a1;
	float u1 = dot(hit_normal, cross(eP, e2)) * one_over_area_triangle;
	float v1 = dot(hit_normal, cross(e1, eP)) * one_over_area_triangle;
	tex_gradient1 = (1.0 - u1 - v1) * triangle_uv[0] + u1 * triangle_uv[1] + v1 * triangle_uv[2] - intersect_uv;
	
	eP = delta + a2;
	float u2 = dot(hit_normal, cross(eP, e2)) * one_over_area_triangle;
	float v2 = dot(hit_normal, cross(e1, eP)) * one_over_area_triangle;
	tex_gradient2 = (1.0 - u2 - v2) * triangle_uv[0] + u2 * triangle_uv[1] + v2 * triangle_uv[2] - intersect_uv;
}

// This function will sample a texture anisotropically, using ray cones to determine which mips will be sampled
float4 SampleTextureAnisotropic(Texture2D tex, SamplerState samp, float2 tex_gradient1, float2 tex_gradient2, float2 uv)
{
	if (tweak.mip_bias_u != 0.0)
	{
		tex_gradient1 *= pow(2.0, tweak.mip_bias_u);
	}
	if (tweak.mip_bias_v != 0.0)
	{
		tex_gradient2 *= pow(2.0, tweak.mip_bias_v);
	}

	return tex.SampleGrad(samp, uv, tex_gradient1, tex_gradient2);
}

#endif /* COMMON_HLSL */