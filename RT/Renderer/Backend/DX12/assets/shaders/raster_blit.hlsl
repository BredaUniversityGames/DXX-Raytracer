// Since DirectX 12 does not have a vkCmdBlitImage equivalent,
// we have to make do with a quad-based blit technique
// ------------------------------------------------------
// Vertex shader

struct Vertex
{
	float2 pos;
	float2 uv;
};

static Vertex g_quad_verts[6] = {
	{ float2(-1.0f, 1.0f), float2(0.0f, 0.0f) },
	{ float2(1.0f, 1.0f), float2(1.0f, 0.0f) },
	{ float2(1.0f, -1.0f), float2(1.0f, 1.0f) },
	{ float2(-1.0f, 1.0f), float2(0.0f, 0.0f) },
	{ float2(1.0f, -1.0f), float2(1.0f, 1.0f) },
	{ float2(-1.0f, -1.0f), float2(0.0f, 1.0f) }
};

struct Constants
{
	uint blit_x;
	uint blit_y;
	uint blit_width;
	uint blit_height;
	uint blit_blend;
};

ConstantBuffer<Constants> g_constants : register(b0);

struct VertexShaderOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEX_COORDS;
};

VertexShaderOutput VertexShaderEntry(uint vertex_id : SV_VertexID)
{
	VertexShaderOutput OUT;
	OUT.position = float4(g_quad_verts[vertex_id].pos, 0.0f, 1.0f);
	OUT.uv = g_quad_verts[vertex_id].uv;
	OUT.position.x *= (float)g_constants.blit_width / (float)g_constants.blit_height;
	return OUT;
}

// ------------------------------------------------------
// Pixel shader

#define BLIT_BLEND_INNER 0.5
#define BLIT_BLEND_OUTER 1.0
#define BLIT_BLEND_STRENGTH 1.0
// Shape will make the blend form more circular with higher values (vignette)
#define BLIT_BLEND_SHAPE 0.5

Texture2D g_source_texture : register(t0);
SamplerState g_sampler : register(s0);

float4 PixelShaderEntry(VertexShaderOutput IN) : SV_Target
{
	float blend_factor = 0.0;
	if (g_constants.blit_blend)
	{
		float2 uv = (IN.position.xy - float2(g_constants.blit_x, g_constants.blit_y)) / float2(g_constants.blit_width, g_constants.blit_height);
		float2 blend_curve = pow(abs(uv * 2.0 - 1.0), float2(1.0 / BLIT_BLEND_SHAPE, 1.0 / BLIT_BLEND_SHAPE));
		float blend_edge = pow(length(blend_curve), BLIT_BLEND_SHAPE);
		blend_factor = 1.0 - BLIT_BLEND_STRENGTH * smoothstep(BLIT_BLEND_INNER, BLIT_BLEND_OUTER, blend_edge);
	}
	
	float4 sampled_color = g_source_texture.SampleLevel(g_sampler, IN.uv, 0);
	return float4(sampled_color.xyz, blend_factor);
}
