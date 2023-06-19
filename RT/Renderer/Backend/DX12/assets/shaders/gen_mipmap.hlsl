#include "include_shared/shared_common.hlsl.h"

#define GEN_MIPMAP_WIDTH_HEIGHT_EVEN 0
#define GEN_MIPMAP_WIDTH_ODD_HEIGHT_EVEN 1
#define GEN_MIPMAP_WIDTH_EVEN_HEIGHT_ODD 2
#define GEN_MIPMAP_WIDTH_HEIGHT_ODD 3

ConstantBuffer<RT::GenMipMapSettings> g_mipmap_cb : register(b0);

Texture2D g_src_mip : register(t0);

RWTexture2D<float4> g_dst_mip0 : register(u0);
RWTexture2D<float4> g_dst_mip1 : register(u1);
RWTexture2D<float4> g_dst_mip2 : register(u2);
RWTexture2D<float4> g_dst_mip3 : register(u3);

SamplerState g_samp_linear_clamp : register(s0);

groupshared float g_R[64];
groupshared float g_G[64];
groupshared float g_B[64];
groupshared float g_A[64];

void StoreColorGroupShared(uint idx, float4 color)
{
	g_R[idx] = color.r;
	g_G[idx] = color.g;
	g_B[idx] = color.b;
	g_A[idx] = color.a;
}

float4 LoadColorGroupShared(uint idx)
{
	return float4(g_R[idx], g_G[idx], g_B[idx], g_A[idx]);
}

float3 ConvertToSRGB(float3 lin)
{
	return select(lin < 0.0031308, 12.92 * lin, 1.055 * pow(abs(lin), 1.0 / 2.4) - 0.055);
}

float4 PackColor(float4 color)
{
	if (g_mipmap_cb.is_srgb)
		return float4(ConvertToSRGB(color.rgb), color.a);
	else
		return color;
}

[numthreads(8, 8, 1)]
void GenMipMapCS(uint3 dispatch_idx : SV_DispatchThreadID, uint group_idx : SV_GroupIndex)
{
	float4 src0 = (float4)0;
	switch (g_mipmap_cb.src_dim)
	{
	case GEN_MIPMAP_WIDTH_HEIGHT_EVEN:
	{
		float2 uv = g_mipmap_cb.texel_size * (dispatch_idx.xy + 0.5);
		src0 = g_src_mip.SampleLevel(g_samp_linear_clamp, uv, g_mipmap_cb.src_mip);
	} break;
	case GEN_MIPMAP_WIDTH_ODD_HEIGHT_EVEN:
	{
		float2 uv = g_mipmap_cb.texel_size * (dispatch_idx.xy + float2(0.25f, 0.5f));
		float2 off = g_mipmap_cb.texel_size * float2(0.5f, 0.0f);

		src0 = 0.5f * (g_src_mip.SampleLevel(g_samp_linear_clamp, uv, g_mipmap_cb.src_mip) +
			g_src_mip.SampleLevel(g_samp_linear_clamp, uv + off, g_mipmap_cb.src_mip));
	}
	break;
	case GEN_MIPMAP_WIDTH_EVEN_HEIGHT_ODD:
	{
		float2 uv = g_mipmap_cb.texel_size * (dispatch_idx.xy + float2(0.5f, 0.25f));
		float2 off = g_mipmap_cb.texel_size * float2(0.0f, 0.5f);

		src0 = 0.5f * (g_src_mip.SampleLevel(g_samp_linear_clamp, uv, g_mipmap_cb.src_mip) +
			g_src_mip.SampleLevel(g_samp_linear_clamp, uv + off, g_mipmap_cb.src_mip));
	}
	break;
	case GEN_MIPMAP_WIDTH_HEIGHT_ODD:
	{
		float2 uv = g_mipmap_cb.texel_size * (dispatch_idx.xy + float2(0.25f, 0.25f));
		float2 off = g_mipmap_cb.texel_size * 0.5f;

		src0 = g_src_mip.SampleLevel(g_samp_linear_clamp, uv, g_mipmap_cb.src_mip);
		src0 += g_src_mip.SampleLevel(g_samp_linear_clamp, uv + float2(off.x, 0.0f), g_mipmap_cb.src_mip);
		src0 += g_src_mip.SampleLevel(g_samp_linear_clamp, uv + float2(0.0f, off.y), g_mipmap_cb.src_mip);
		src0 += g_src_mip.SampleLevel(g_samp_linear_clamp, uv + float2(off.x, off.y), g_mipmap_cb.src_mip);
		src0 *= 0.25f;
	}
	break;
	}

	g_dst_mip0[dispatch_idx.xy] = PackColor(src0);

	// Exit all threads if we are done
	if (g_mipmap_cb.num_mips == 1)
		return;

	StoreColorGroupShared(group_idx, src0);

	// Need to wait here until all threads have executed all instructions until this point
	GroupMemoryBarrierWithGroupSync();

	if ((group_idx & 0x9) == 0)
	{
		float4 src1 = LoadColorGroupShared(group_idx + 0x01);
		float4 src2 = LoadColorGroupShared(group_idx + 0x08);
		float4 src3 = LoadColorGroupShared(group_idx + 0x09);

		src0 = 0.25 * (src0 + src1 + src2 + src3);

		g_dst_mip1[dispatch_idx.xy / 2] = PackColor(src0);
		StoreColorGroupShared(group_idx, src0);
	}

	// Exit all threads if we are done
	if (g_mipmap_cb.num_mips == 2)
		return;

	GroupMemoryBarrierWithGroupSync();

	if ((group_idx & 0x1B) == 0)
	{
		float4 src1 = LoadColorGroupShared(group_idx + 0x02);
		float4 src2 = LoadColorGroupShared(group_idx + 0x10);
		float4 src3 = LoadColorGroupShared(group_idx + 0x12);

		src0 = 0.25 * (src0 + src1 + src2 + src3);

		g_dst_mip2[dispatch_idx.xy / 4] = PackColor(src0);
		StoreColorGroupShared(group_idx, src0);
	}

	// Exit all threads if we are done
	if (g_mipmap_cb.num_mips == 3)
		return;

	GroupMemoryBarrierWithGroupSync();

	if (group_idx == 0)
	{
		float4 src1 = LoadColorGroupShared(group_idx + 0x04);
		float4 src2 = LoadColorGroupShared(group_idx + 0x20);
		float4 src3 = LoadColorGroupShared(group_idx + 0x24);

		src0 = 0.25 * (src0 + src1 + src2 + src3);

		g_dst_mip3[dispatch_idx.xy / 8] = PackColor(src0);
	}
}
